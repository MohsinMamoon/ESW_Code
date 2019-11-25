
//Libraries
#include <ThingSpeak.h>
#include "ESP8266WiFi.h"
#include "DHT.h"
#include <Wire.h>
#include "MutichannelGasSensor.h"
#include <SDS011.h>
#include <ESP8266HTTPClient.h>
#include <stdlib.h>
#include <SoftwareSerial.h>


//Constants
#define DHTPIN D3    // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define node_RX D5
#define node_TX D6
#define ADDR_I2C 0x04
#define LED D0
SDS011 my_sds;
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino
 unsigned long Channel_ID=886431;
 char API_KEY[]="Y1GPJGG8VEFVTC0V";
//char ssid[] = "JioFi_20E7D6C";
//char psswd[] = "2wufpbqmx0";
//char ssid[] = "esw-m19@iiith";
//char psswd[] = "e5W-eMai@3!20t";
//char ssid[] = "Nitro 5";
//char psswd[] = "wvZu69eF";
char ssid[] = "MGG5";
char psswd[] = "drowssapp";
WiFiClient  client;

//Variables
int chk, error;
float hum, temp, p10, p25, c;
String data;

//ONEM2M
int post(String ae, String container, String data) {
  String server = "http://onem2m.iiit.ac.in:80";
  String cse = "/~/in-cse/in-name/";
  char m2m[200];
  String Data;
  Data = "{\"m2m:cin\": {"
    "\"con\":\"" + data + "\""
    "}}";
  HTTPClient http;
  Serial.println("\nConnecting to : " + server+cse+ae+container);
  if(http.begin(server+cse+ae+container)) {
    http.addHeader("X-M2M-Origin", "admin:admin");
    http.addHeader("Content-type", "application/json;ty=4");
    Serial.println("Uploading data: " + data);
    int response = http.POST(Data);
    http.end();
    return response;
  }
  else {
    return 1;
  }
}

void update(int val) {
  if(val == 201) Serial.println("Data updated to OneM2M successfully\n");
  else Serial.println("There was an error while uploading Data to OneM2M. Error Code: " + String(val));
}

void setup(){
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  my_sds.begin(node_RX, node_TX);
  gas.begin(ADDR_I2C);
  gas.powerOn();  
  Serial.println("Begin.");
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  ThingSpeak.begin(client);
  digitalWrite(LED, HIGH);
  delay(10000);
}
int led = 0;
void loop() {
     if(WiFi.status() != WL_CONNECTED){
      digitalWrite(LED, HIGH);
      Serial.print("Attempting to connect to SSID: ");
      Serial.print(ssid); 
      Serial.println(" with password: " + String(psswd));
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, psswd);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        Serial.print(".");
        led = (led == 0) ? 1 : 0;
        digitalWrite(LED, led);
        delay(5000);     
      } 
      Serial.println("\nConnected.");
      digitalWrite(LED, LOW);
    }

    
    hum = dht.readHumidity();
    temp= dht.readTemperature();

//    Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");
    ThingSpeak.setField(1, hum);
    ThingSpeak.setField(2, temp);
    data = String(temp) + "," + String(hum) + ',';

    
    error = my_sds.read(&p25, &p10);
    if (!error) {
      Serial.println("P2.5: " + String(p25) + "\t" + "P10:  " + String(p10));
      ThingSpeak.setField(6, p25);
      ThingSpeak.setField(7, p10);
      data = data + String(p25) + "," + String(p10) + ',';
      
    }
    else {
      data = data + "-1,-1,";
    }
//

    c = gas.measure_CO();
    Serial.print("The concentration of CO is ");
    if(c>=0) Serial.print(c);
    else Serial.print("invalid");
    Serial.println(" ppm");
    ThingSpeak.setField(3, c);
    data = data + String(c) + ",";
    
    c = gas.measure_NO2();
    Serial.print("The concentration of NO2 is ");
    if(c>=0) Serial.print(c);
    else Serial.print("invalid");
    Serial.println(" ppm");
    ThingSpeak.setField(4, c);
    data = data + String(c) + ",";

    c = gas.measure_NH3();
    Serial.print("The concentration of NH3 is ");
    if(c>=0) Serial.print(c);
    else Serial.print("invalid");
    Serial.println(" ppm");
    ThingSpeak.setField(5, c);
    data = data + String(c) + ',';


    
    data = data + ",OAP4_2";
    
    update(post("Team6_Outdoor_air_pollution_mobile/", "node_2", data));

    int response;
    if((response = ThingSpeak.writeFields(Channel_ID, API_KEY)) == 200) {
      Serial.println("Data Updated to ThingSpeak successfully!");
    }
    else {
      Serial.println("Problem Updating data for ThingSpeak!\nError code: " + String(response)); 
    }
    delay(15000); //Delay 15 sec.
    Serial.println("...\n\n\n\n\n\n\n");
}
