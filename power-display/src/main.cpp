
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <Wire.h>

//TODO: configurable host

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
HTTPClient http;

void initalizeWiFiManager() {
    WiFiManager wifiManager;

    wifiManager.autoConnect("AutoConnectAP");

    delay(500);
}

bool readValues(long& current, long& total) {
    http.begin("192.168.20.201", 8080,
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

    display.println("Initializing wifi...");
    display.display();

    initalizeWiFiManager();

    display.println("Wifi initialized...");
    display.display();

    MDNS.begin("fronius-disp");

    httpUpdater.setup(&server);

    server.begin();
}

void displayTotalAligned(double total) {
        display.setCursor(0, 0);

        char buf[10];
        dtostrf(total, 2, 2, buf);
        int length = strlen(buf);

        display.setCursor(128-12*length, 0);

        display.println(buf);
}

void loop() {
    long current, total;
    
    if(readValues(current, total)) {
        display.clearDisplay();
        display.setTextSize(2);
        displayTotalAligned((double)total/1000);

        display.setTextSize(5);
        double formattedCurrent = (double)current/1000;
        display.print(formattedCurrent, 2);

        display.display();
    } else {
        // if something goes wrong - just don't refresh
    }

    delay(1000);
}
