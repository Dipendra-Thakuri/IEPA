#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <string.h>

// WiFi credentials
const char* ssid = "V2030";
const char* password = "vaishu987";

// Sensor pins
const int soil_sensor_pin = 32;
const int rain_sensor_pin = 34;
const int pir_sensor_pin = 13;
const int relay = 26;

String motor = "Off";

String temp1;
String humi1;
String soil1;
String rain1;

String Web_App_URL = "https://script.google.com/macros/s/AKfycbwyT855n1dG-FY3LPPsBd2mIllfC04ERefqjA5iTwDuK50PyRGqAicyRNUHkdy2F-njsQ/exec";

#define DHTPIN 27
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd (0x27, 16, 2);
AsyncWebServer server(80);

String readPIRSensor() {
  int sensor_output;
  pinMode(pir_sensor_pin, INPUT);
  sensor_output = digitalRead(pir_sensor_pin);
  if (sensor_output == LOW) {
    Serial.println("No Motion...");
    return "NoMotion...";
  } else if (sensor_output == HIGH) {
    Serial.println("Motion Detected!!!");
    return "MotionDetected!!!";
  }
}

String readDHTTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  } else {
    Serial.print("Temperature : ");
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  } else {
    Serial.print("Humidity : ");
    Serial.println(h);
    return String(h);
  }
}

String readSoilMoisture() {
  int sensor_analog = analogRead(soil_sensor_pin);
  int _moisture = (100 - ((sensor_analog / 4095.00) * 100));
  Serial.print("Moisture = ");
  Serial.print(_moisture);  
  Serial.println("%");

  pinMode(relay, OUTPUT);
  if (_moisture < 20) {
    digitalWrite(relay, LOW);
    motor = "ON";
  } else if (_moisture >= 40) {
    digitalWrite(relay, HIGH);
    motor = "OFF";
  }
  return String(_moisture);
}

String readRainFallSensor() {
  pinMode(rain_sensor_pin, INPUT);
  int rainDigitalVal = digitalRead(rain_sensor_pin);
  if (rainDigitalVal == HIGH) {
    Serial.println("The rain is NOT detected");
    return "NotRaining";
  } else if (rainDigitalVal == LOW) {
    Serial.println("The rain is detected");
    return "Raining";
  }
}

void display(String temp, String humi, String soil, String rain) {
  lcd.setCursor(0, 0);
  lcd.print("H=" + humi + " T=" + temp);
  lcd.setCursor(0, 1);
  lcd.print("S=" + soil + " R=" + rain);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css" integrity="sha512-DTOQO9RWCH3ppGqcWaEA1BIZOC6xxalwEsw9c2QQeAIftl+Vegovlnee1c9QX4TctnWMn13TZye+giMm8e2LwA==" crossorigin="anonymous" referrerpolicy="no-referrer" />
  <style>
    * {
      background-color: rgb(227, 227, 227);
      border: 0px;
      margin: 0px;
    }

    p {
      background-color: white;
      padding-top: 4rem;
      font-size: 2rem;
    }

    #humidity-text, #temperature-text, #soilmoisture-text, #rainfall-text, #pir-text, #waterpump-text {
      background-color: white;
    }

    .humidity-main, .temperature-main, .soilmoisture-main, .rainfall-main, .pir-main, .waterpump-main {
      display: inline-block;
      background-color: white;
      height: 15.1rem;
      width: 25.1rem;
      text-align: center;
      margin: 1rem 0rem 0rem 1rem;
      font-size: 1.1rem;
      font-weight: 900;
      font-family: 'Times New Roman', Times, serif;
      padding-top: 0.7rem;
      border-radius: 0.3rem;
    }

    #humidity, #temperature, #soilmoisture, #rainfall, #pir, #waterpump {
      background-color: white;
    }

    #humi-symbol, #temp-symbol, #soil-symbol, #rain-symbol, #pir-symbol, #water-symbol {
      background-color: white;
    }

    #humi-op, #temp-op, #soil-op {
      background-color: white;
    }

    #water-button {
      height: 2.5rem;
      width: 9.5rem;
      font-size: 1.9rem;
      background-color: white;
      border: 0.1rem solid black;
    }
  </style>
</head>
<body>
  <div class="humidity-main">
    <span id="humidity-text">Humidity</span>
    <p>
      <i class="fas fa-tint" style="color:#00add6;" id="humi-symbol"></i> 
      <span id="humidity">%HUMIDITY%</span>
      <sup id="humi-op">&percnt;</sup>
    </p>
  </div>
  <div class="temperature-main">
    <span id="temperature-text">Temperature</span>
    <p>
      <i class="fas fa-thermometer-half" style="color:#059e8a;" id="temp-symbol"></i> 
      <span id="temperature">%TEMPERATURE%</span>
      <sup id="temp-op">&deg;C</sup>
    </p>
  </div>
  <div class="soilmoisture-main">
    <span id="soilmoisture-text">SoilMoisture</span>
    <p>
      <i class="fas fa-tint" style="color:#00add6;" id="soil-symbol"></i> 
      <span id="soilmoisture">%SOILMOISTURE%</span>
      <sup id="soil-op">&percnt;</sup>
    </p>
  </div>
  <div class="rainfall-main">
    <span id="rainfall-text">Rainfall</span>
    <p>
      <i class="fa-solid fa-cloud-showers-heavy" style="color:#00add6;" id="rain-symbol"></i> 
      <span id="rainfall">%RAINFALL%</span>
    </p>
  </div>
  <div class="pir-main">
    <span id="pir-text">PIR</span>
    <p>
      <i class="fa-solid fa-person-running" style="color:#ff0000;" id="pir-symbol"></i> 
      <span id="pir">%MOTION%</span>
    </p>
  </div>
  <div class="waterpump-main">
    <span id="waterpump-text">WaterPump</span>
    <p> 
      <button id="water-button" onclick="toggleWaterPump()">OFF <i class="fa-solid fa-faucet-drip" style="color: steelblue;" id="water-symbol"></i></button>
    </p>
  </div>
</body>
<script>
setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("pir").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/pir", true);
  xhttp.send();
}, 5000);

setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 5000);

setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 5000);

setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("soilmoisture").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/soilmoisture", true);
  xhttp.send();
}, 5000);

setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("rainfall").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/rainfall", true);
  xhttp.send();
}, 5000);

var waterPumpState = false;

function toggleWaterPump() {
  waterPumpState = !waterPumpState; // Toggle the state

  var xhttp = new XMLHttpRequest();
  var url = "/waterpump?state=" + (waterPumpState ? "on" : "off");
  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("water-button").innerText = waterPumpState ? "ON" : "OFF"; // Update button text
      document.getElementById("water-button").style.backgroundColor = waterPumpState ? "green" : "white"; // Update symbol color
    }
  };
  xhttp.open("GET", url, true);
  xhttp.send();
}
</script>
</html>)rawliteral";

String processor(const String& var){
  if(var == "MOTION"){
    return readPIRSensor();
  }
  else if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
  else if(var == "SOILMOISTURE"){
    return readSoilMoisture();
  }
  else if(var == "RAINFALL"){
    return readRainFallSensor();
  }
  return String();
}

void setup(){
  Serial.begin(115200);

  //----------------------------------------Set Wifi to STA mode
  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  WiFi.mode(WIFI_STA);
  Serial.println("-------------");
  //---------------------------------------- 

  dht.begin();

  // Initialize the LCD connected 
  lcd.init();
  
  // Turn on the backlight on LCD. 
  lcd.backlight();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/pir", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readPIRSensor().c_str());
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });
  server.on("/soilmoisture", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSoilMoisture().c_str());
  });
  server.on("/rainfall", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readRainFallSensor().c_str());
  });
  server.on("/waterpump", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
        String state = request->getParam("state")->value();
        if (state == "on") {
            digitalWrite(relay, LOW); // Turn on the relay
            motor = "ON";
        } else if (state == "off") {
            digitalWrite(relay, HIGH); // Turn off the relay
            motor = "OFF";
        }
        request->send(200, "text/plain", state); // Send back the current state
    } else {
        request->send(400); // Bad request if state parameter is missing
    }
  });

  server.begin();
}

void loop() {
  // Read sensor values
  temp1 = readDHTTemperature();
  humi1 = readDHTHumidity();
  soil1 = readSoilMoisture();
  rain1 = readRainFallSensor();

  String soilMoisture = readSoilMoisture();
  String rainfall = readRainFallSensor();
  String pir = readPIRSensor();

  // Update LCD display
  display(temp1, humi1, soil1, rain1);

  if (WiFi.status() == WL_CONNECTED) {
    // Create a URL for sending or writing data to Google Sheets
    String Send_Data_URL = Web_App_URL + "?sts=write";
    Send_Data_URL += "&temp=" + temp1;
    Send_Data_URL += "&humd=" + humi1;
    Send_Data_URL += "&somo=" + soilMoisture;
    Send_Data_URL += "&rain=" + rainfall;
    Send_Data_URL += "&pir=" + pir;
    Send_Data_URL += "&moto=" + motor;

    Serial.println();
    Serial.println("-------------");
    Serial.println("Send data to Google Spreadsheet...");
    Serial.print("URL : ");
    Serial.println(Send_Data_URL);

    // Sending data to Google Sheets
    HTTPClient http;
    http.begin(Send_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    // Getting response from Google Sheets
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
      Serial.println("Payload : " + payload);    
    }
    
    http.end();
    Serial.println("-------------");
  }

  // Delay to prevent rapid looping
  delay(30000); // 30 seconds delay
}
