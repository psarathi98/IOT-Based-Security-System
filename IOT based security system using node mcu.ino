
//IOT based Advanced Industrial Security System

//Include the library files
#include <Blynk.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <WiFiClientSecure.h>


//Blynk Template and authorization Token Declaration

#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

//Wifi Credintial
char auth[] = "";//Enter your Auth token
char ssid[] = "";//Enter your WIFI name
char pass[] = "";//Enter your WIFI password

const char* host ="script.google.com";
const int httpsPort = 443;


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

  Serial.println("System Booted");

//Call the functions
  timer.setInterval(100L, gassensor);
  timer.setInterval(100L, DHT11sensor);
  timer.setInterval(100L, pirsensor);
  timer.setInterval(100L, ultrasonic);
}

//Get the MQ2 sensor values
void gassensor() {
  int value = analogRead(MQ2);
  value = map(value, 0, 1024, 0, 100);
  if (value <= 55) {
    digitalWrite(Buzzer, LOW);
  } else if (value > 55) {
    Blynk.logEvent("gas_leakage");
    digitalWrite(Buzzer, HIGH);
  }
  Blynk.virtualWrite(V1, value);
  Serial.print("Gas Value : ");
  Serial.print(value);
  Serial.print("  ");

}

//Get the DHT11 sensor values
void DHT11sensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

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

 
}
