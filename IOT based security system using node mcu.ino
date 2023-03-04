//IOT based Advanced Industrial Security System

//Include the library files
#include <Blynk.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


WiFiClient client;

//Blynk Template and authorization Token Declaration
#define WIFI_SSID ""   //Enter your WIFI name
#define WIFI_PASS ""  //Enter your WIFI password

//Copy your Blynk Template
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""
#define IFTTT_Key ""            //Enter your IFTTT Key
#define IFTTT_Event ""         // IFTTT Event name

//Wifi Credintial
char auth[] = "";       //Enter your Blynk Auth token
char ssid[] = "";      //Enter your WIFI name
char pass[] = "";     //Enter your WIFI password

//Firebase Things:
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// Insert Firebase project API Key
#define API_KEY ""

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL ""
#define USER_PASSWORD ""

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL  ""

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth autho;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String timePath = "/timestamp";
// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
int timestamp;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 18000;

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

DHT dht(D3, DHT11); //(sensor pin,sensor type)
BlynkTimer timer;
bool pirbutton = 0;

// Define component pins
#define Buzzer D0
#define MQ2 A0
#define trig D4
#define echo D5
#define PIR D6
#define relay1 D7
#define relay2 D8

//Get buttons values
BLYNK_WRITE(V0) {
  pirbutton = param.asInt();
}

float h,t;
int value;
void setup() {
  Serial.begin(9600);
  pinMode(Buzzer, OUTPUT);
  pinMode(PIR, INPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  dht.begin();
  timeClient.begin();
  Serial.println("System Booted");

 //wifi setup
 WiFi.begin(WIFI_SSID, WIFI_PASS); 
  // Connecting to WiFi...
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
 
  // Connected to WiFi 
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  autho.user.email = USER_EMAIL;
  autho.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &autho);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((autho.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = autho.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

//Call the functions
  timer.setInterval(100L, gassensor);
  timer.setInterval(100L, DHT11sensor);
  timer.setInterval(100L, pirsensor);
  timer.setInterval(100L, ultrasonic);
}
#define IFTTT_Value1 String(t);
#define IFTTT_Value2 String(h);
#define IFTTT_Value3 String(value);
void send_webhook(){
  // construct the JSON payload
  String jsonString = "";
  jsonString += "{\"value1\":\"";
  jsonString += IFTTT_Value1;
  jsonString += "\",\"value2\":\"";
  jsonString += IFTTT_Value2;
  jsonString += "\",\"value3\":\"";
  jsonString += IFTTT_Value3;
  jsonString += "\"}";
  int jsonLength = jsonString.length();  
  String lenString = String(jsonLength);
  // connect to the Maker event server
  client.connect("maker.ifttt.com", 80);
  // construct the POST request
  String postString = "";
  postString += "POST /trigger/";
  postString += IFTTT_Event;
  postString += "/with/key/";
  postString += IFTTT_Key;
  postString += " HTTP/1.1\r\n";
  postString += "Host: maker.ifttt.com\r\n";
  postString += "Content-Type: application/json\r\n";
  postString += "Content-Length: ";
  postString += lenString + "\r\n";
  postString += "\r\n";
  postString += jsonString; // combine post request and JSON
  
  client.print(postString);
  delay(500);
  client.stop();
}
//Get the MQ2 sensor values
void gassensor() {
  value = analogRead(MQ2);
  value = map(value, 0, 1024, 0, 100);
  if (value <= 55) {
    digitalWrite(Buzzer, LOW);
  } else if (value > 55) {
    Blynk.logEvent("gas_leakage");
    send_webhook();
    digitalWrite(Buzzer, HIGH);
  }
  Blynk.virtualWrite(V1, value);
  Serial.print("Gas Value : ");
  Serial.print(value);
  Serial.print("  ");

}

//Get the DHT11 sensor values
void DHT11sensor() {
   h = dht.readHumidity();
   t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Blynk.virtualWrite(V2, t);
  Blynk.virtualWrite(V3, h);
  
  Serial.print("Temp : ");
  Serial.print(t);
  Serial.print("  ");

  Serial.print("Humidity : ");
  Serial.print(h);
  Serial.println("  ");

}

//Get the PIR sensor values
void pirsensor() {
  int val=0;
  val = digitalRead(PIR); // IR Sensor output pin connected to D6 
  //Serial.println(val);  // see the value in serial m0nitor in Arduino IDE  
  delay(100);      // for timer  
  if (pirbutton == 1) 
  {
    if(val == 0 )  
    { 
    Blynk.logEvent("security"); 
    send_webhook(); 
    digitalWrite(Buzzer, HIGH);
    digitalWrite(relay1,LOW);
    Blynk.virtualWrite(V5, 1);
    Blynk.virtualWrite(V7, 1);
    Serial.print("  ");
    Serial.print("Motion Detected");
    Serial.print("  ");
    }  
    else  
    {  
    digitalWrite(Buzzer, LOW);
    Blynk.virtualWrite(V7, 0);
    }
  }
}

//Get the ultrasonic sensor values
void ultrasonic() {
  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long t = pulseIn(echo, HIGH);
  long cm = t / 29 / 2;
  Blynk.virtualWrite(V4, cm);

    if (cm <=10)
    {
     Blynk.logEvent("reserver_level");
     send_webhook();
     Serial.print("Reserver OverFlow!!  ");
     Serial.print(cm);
     Serial.print(" cm  ");
    }

}

//Get buttons values
BLYNK_WRITE(V5) {
 bool RelayOne = param.asInt();
  if (RelayOne == 1) {
    digitalWrite(relay1, LOW);
  } else {
    digitalWrite(relay1, HIGH);
  }
}

//Get buttons values
BLYNK_WRITE(V6) {
 bool RelayTwo = param.asInt();
  if (RelayTwo == 1) {
    digitalWrite(relay2, LOW);
  } else {
    digitalWrite(relay2, HIGH);
  }
}

void loop() {
  Blynk.run();//Run the Blynk library
  timer.run();//Run the Blynk timer

  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
     timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(), String(t));
    json.set(humPath.c_str(), String(h));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}