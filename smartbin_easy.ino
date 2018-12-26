#include <SPI.h>
#include <Wire.h>
#include <WiFi101.h>
#include <PubSubClient.h>

#include <math.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HX711_ADC.h>
#include "Adafruit_VL53L0X.h"

#include "config.h"

/***************** SENSORS PINS ****************/

byte mac_mkr[6];
uint32_t mac_int; 
char jsonChar[230];
long height[PRECISION_SAMPLES];

/***************** FIELDS ****************/
long real_height;
long real_weight;
int sequenceNumber = 0;

typedef struct t  {
    unsigned long tStart;
    unsigned long tTimeout;
};


//Task and its schedule.
t t_read = {START_TIME, TOTAL_TIME*1000}; //Run at beginning (60 sec)


WiFiClient net;
PubSubClient client(net);
StaticJsonBuffer<300> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();
Adafruit_VL53L0X tof = Adafruit_VL53L0X();
HX711_ADC LoadCell(D5, D4);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("***** SMART BIN by Edward Halmstad v2.0 *****");
 
  client.setServer(MQTT_BROKER, 1883);
  client.setCallback(on_message);
  
  ensure_connections();
  mac_int = storeMacAddress();

  check_sensors();
   Serial.println("Start tare");
  //Tare everything at startup
  tare();
}

/********** START LOOP ************/

void loop() {
  client.loop();
  
  
  if (!client.connected()) {
    ensure_connections();
  }


    if (tCheck(&t_read)) {
      send_data();
      
      print_time(millis());
      tRun(&t_read);
    
    }
   
}
/********** END LOOP ************/

/********** SEND DATA ************/
void send_data(){
  int wrong_loops = 0;
  real_height = readValues();

  while (!readValues()) {
    Serial.print(".");
    delay(500);
    wrong_loops++;
    
    if (wrong_loops == WRONG_LOOPS) {
      Serial.println("BIN ERROR");
      //too much errors on reading, the bins is completely fucked up
      //(or the threshold is wrong)
      while(1);  
    }
  }
  real_weight = getWeightAIO();
  createJson();
  //convert in json
  root.printTo(jsonChar, sizeof(jsonChar)); 
  
  if (client.publish(MQTT_TOPIC_STATUS, jsonChar) == true) {
     Serial.println("Success sending message");
  } else {
     Serial.println("Error sending message");
  }
}


/********** RECEIVE DATA ************/
void on_message(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived: ");
  Serial.println(topic);
  if(strcmp(topic, MQTT_TOPIC_CTRL)== 0){
    Serial.print("Read & send");
    send_data();
  }
  Serial.println();
}

/********** FUNCTIONS ************/

int storeMacAddress(){
  int mac;
  WiFi.macAddress(mac_mkr);
  mac = *(uint32_t*)mac_mkr;
  Serial.print("Hi, i'm ");
  Serial.println(mac);

  return mac;
}


long getWeightAIO() {
  LoadCell.powerUp();
  LoadCell.begin();
  long t = millis();
  int count = 0;

  while (1 == 1) {
    LoadCell.update();
    //get smoothed value from data set + current calibration factor
    if (millis() > t + STABILISING_TIME) {
      long i = (long) round(LoadCell.getData());
      count = count + 1;
      //Serial.print("W: ");
      //Serial.println(i);
      t = millis();

      if (count > 0) {
        count = 0;
        //Serial.println("Return");
        LoadCell.powerDown();
        return i;
      }
    }
  }
}

bool readValues(){
    long max_val = 0;
    long min_val = 0;
    long sum = 0;
    float mean = 0;
    
    VL53L0X_RangingMeasurementData_t measure;
    
    for(int i=0; i<PRECISION_SAMPLES; i++){
      tof.rangingTest(&measure, false); 
      height[i] = measure.RangeMilliMeter;
      if (max_val < height[i]) {
        max_val = height[i];
      } else if (min_val > height[i]) {
        min_val = height[i];
      }
      sum += height[i];
    }

    mean = sum/PRECISION_SAMPLES;

    if(calculateSD(mean, height) < STD_SENSIBILITY)
    {
      real_height = round(mean);
      return true;
      }
    else{
      return false;
    }
    
}

float calculateSD(long my_mean, long data[])
{
    float standardDeviation = 0.0;

    for(int i=0; i<PRECISION_SAMPLES; ++i){
        standardDeviation += pow(data[i] - my_mean, 2);
        
    }
    return sqrt(standardDeviation/PRECISION_SAMPLES);
}




void createJson(){
  root["SN"] = sequenceNumber;
  root["height"] = real_height;
  root["username"] = mac_int;
  root["weight"] = real_weight;
  sequenceNumber++;
  root.prettyPrintTo(Serial);
}

void ensure_connections(){
  if (wifi_connect()){
    if(!mqtt_connect()){
      Serial.println("very big MQTT problem, restart");
      while(1); //block the program
    }
  }
  else {
    Serial.println("very big Wi-Fi problem, restart");
    while(1); //block the program
  }
}

boolean wifi_connect() {
  Serial.print("Connecting to ");
  Serial.println(SSID_WIFI);
  
  Serial.print("checking wifi..");
  int i = 0;

  if(WiFi.status() != WL_CONNECTED){
      WiFi.begin(SSID_WIFI, PASS_WIFI);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    i++;
    if (i == 20) {
      Serial.println("WI-FI Connection Error!!!!");
      return 0;                     //too much time to connect, given up
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  return 1;  
}

boolean mqtt_connect() {
  int i = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("my_mkr")) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC_CTRL);
      return 1;
    }
    else {
      Serial.print("failed mqtt");
      if (i > 1) {
        Serial.println("impossible to connect to MQTT");
        return 0;
      }
      Serial.println(" try again in 5 seconds");
      delay(3000);
    }
    i++;
  }
}

bool tCheck (struct t *t ) {
   if (millis() > t->tStart + t->tTimeout) {
    return true;  
  }
  else {
    return false;
    } 
}

void tRun (struct t *t) {
    t->tStart = millis();
}

void print_time(unsigned long time_millis){
    Serial.print("Time: ");
    Serial.print(time_millis/1000);
    Serial.println("s");
}

/*
   Do a tare only if this is the first boot
*/
void tare() {
  //Initialize load cell
  LoadCell.powerUp();
  LoadCell.begin();
  
  Serial.println("Storing bin weight");
  LoadCell.start(STABILISING_TIME);
  long offset = LoadCell.getTareOffset();
  
  //Finish set-up loadcell calibration
  LoadCell.setCalFactor(SCALE_FACTOR);
  LoadCell.powerDown();
}

void check_sensors(){
  Serial.print("Height sensor: ");
  if (!tof.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    Serial.println(F("Restart the MKR"));
    while(1);
  }
  Serial.println("RUNNING");
}
