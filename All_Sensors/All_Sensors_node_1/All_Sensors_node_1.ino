

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
#include <TinyGPS.h>


//Constants
#define DHTPIN D4    // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define node_RX D5
#define node_TX D6
#define ADDR_I2C 0x04
#define LED D0
#define gps_RX D7
#define gps_TX D3
TinyGPS gps;
SoftwareSerial ss(gps_TX, gps_RX);
SDS011 my_sds;
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino
unsigned long Channel_ID = 864148;
char API_KEY[] = "UKQQPFB6U0R8V4US";
//char ssid[] = "JioFi_20E7D6C";
//char psswd[] = "2wufpbqmx0";
//char ssid[] = "esw-m19@iiith";
//char psswd[] = "e5W-eMai@3!20hOct";
//char ssid[] = "Nitro 5";
//char psswd[] = "wvZu69eF";
char ssid[] = "MGG5";
char psswd[] = "drowssapp";
WiFiClient  client;

//Variables
int chk, error;
float hum, temp, p10, p25, c, flat, flon;
String data;
unsigned long age;

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

static void smartdelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec, int md) {
  char *temp;
  if (val == invalid) {
    while (len-- > 1){
      Serial.print('*');
      data = data + '*';
    }
    Serial.print(' ');
  }
  else{
    char temp[50];
    sprintf(temp, "%.6f", val);
    Serial.print(val, prec);
    data = data + String(temp);
    if(md == 0) ThingSpeak.setLatitude(String(temp));
    if(md == 1) ThingSpeak.setLongitude(String(temp));
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  data = data + ',';
  smartdelay(0);
}

static void print_date(TinyGPS &gps) {
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE) {
    Serial.print("********** ******** ");
    data = data + "********** ******** ";
  }
  else {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
        month, day, year, hour, minute, second);
    Serial.print(sz);
    data = data + sz;
  }
  smartdelay(0);
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
  ss.begin(9600);
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


    gps.f_get_position(&flat, &flon, &age);
    print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6, 0);
    print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6, 1);
    Serial.println();
    print_date(gps);
    smartdelay(1000);
    data = data + ",OAP4_1";
    
    update(post("Team6_Outdoor_air_pollution_mobile/", "node_1", data));

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
