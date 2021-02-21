from flask import Flask
from flask import request
import json, random

app = Flask(__name__)


total = 0

@app.route("/solar_api/v1/GetInterverRealtimeData.cgi", methods=['GET', 'POST'])
def indx():
    global total
    curr = random.randrange(1000)
    total += curr
    return {"Body": {
        "Data" : {
            "DAY_ENERGY": {
                "Unit": "Wh",
                "Values": {
                    "1": total
                }
            },
            "PAC" : {
                "Unit": "W",
                "Values": {
                    "1": curr
                }
            }
        }
    }}

if __name__ == "__main__":
    app.run(host='0.0.0.0', port='8080')