#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>

#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
HTTPClient http;

const char CONFIG_LOCATION[] = "/config.json";
char hostname[30];

bool shouldSaveConfig;

void saveConfigCallback() { shouldSaveConfig = true; }

void loadStoredData() {
    Serial.println("Opening file");
    if (LittleFS.exists(CONFIG_LOCATION)) {
        File configFile = LittleFS.open(CONFIG_LOCATION, "r");
        Serial.println("File opened");
        if (configFile) {
            DynamicJsonDocument jsonBuffer(1024);
            Serial.println("Content loaded");
            if (deserializeJson(jsonBuffer, configFile) ==
                DeserializationError::Ok) {
                JsonObject json = jsonBuffer.as<JsonObject>();
                strcpy(hostname, json["hostname"]);
            } else {
                Serial.println("failed to load json config");
            }
            configFile.close();
        }
    }
}

void initalizeWiFiManager() {
    WiFi.hostname("fronius-disp");
    WiFiManagerParameter custom_hostname("hostname", "Hostname", hostname, 30);

    WiFiManager wifiManager;

    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.addParameter(&custom_hostname);

    wifiManager.autoConnect("AutoConnectAP");

    delay(500);

    MDNS.begin("fronius-disp");

    strcpy(hostname, custom_hostname.getValue());

    if (shouldSaveConfig) {
        Serial.println("saving config");
        DynamicJsonDocument jsonBuffer(1024);
        JsonObject json = jsonBuffer.to<JsonObject>();
        json["hostname"] = hostname;

        File configFile = LittleFS.open(CONFIG_LOCATION, "w");
        if (!configFile) {
            Serial.println("failed to open config file for writing");
        }
        serializeJson(jsonBuffer, configFile);
        configFile.close();
    }
}

void cleanData() {
    Serial.println("Clearing settings!");
    LittleFS.remove(CONFIG_LOCATION);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
}

bool readValues(long& current, long& total) {
    http.begin(hostname, 8080,
               "/solar_api/v1/GetInterverRealtimeData.cgi?Scope=System");
    int httpCode = http.GET();  // Send the request

    if (httpCode > 0) {                     // Check the returning code
        String payload = http.getString();  // Get the request response payload
        http.end();

        // The filter: it contains "true" for each value we want to keep
        StaticJsonDocument<200> filter;
        filter["Body"]["Data"]["DAY_ENERGY"]["Values"]["1"] = true;
        filter["Body"]["Data"]["PAC"]["Values"]["1"] = true;

        // Deserialize the document
        StaticJsonDocument<400> doc;
        deserializeJson(doc, payload, DeserializationOption::Filter(filter));

        total = doc["Body"]["Data"]["DAY_ENERGY"]["Values"]["1"];
        current = doc["Body"]["Data"]["PAC"]["Values"]["1"];
        return true;

    } else {
        http.end();
        return false;
    }
}

void setup() {
    Serial.begin(9600);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    display.clearDisplay();
    display.setTextColor(WHITE);

    LittleFS.begin();

    display.print("Press flash to reset settings");
    display.display();
    for(int i=0; i<3; i++){
        display.print(".");
        display.display();
        delay(200);
    }
    display.println();

    if (digitalRead(0) == 0) {
        display.println("Clearing!");
        display.display();
        cleanData();
    }

    display.println("Initializing wifi...");
    display.display();

    loadStoredData();

    initalizeWiFiManager();

    display.println("Wifi initialized...");
    display.display();


    httpUpdater.setup(&server);

    server.begin();

    IPAddress ip;
    WiFi.hostByName(hostname,ip);

    display.printf("Host %s resolved to %s\n", hostname, ip.toString().c_str());
    display.display();
}

void displayTotalAligned(double total) {
    display.setCursor(0, 0);

    char buf[10];
    dtostrf(total, 2, 2, buf);
    int length = strlen(buf);

    display.setCursor(128 - 12 * length, 0);

    display.println(buf);
}

void loop() {
    long current, total;

    if (readValues(current, total)) {
        display.clearDisplay();
        display.setTextSize(2);
        displayTotalAligned((double)total / 1000);

        display.setTextSize(5);
        double formattedCurrent = (double)current / 1000;
        display.print(formattedCurrent, 2);

        display.display();
    } else {
        // if something goes wrong - just don't refresh
    }

    delay(1000);
}
