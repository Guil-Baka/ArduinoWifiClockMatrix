#include <ArduinoJson.h>
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>
#include <TextAnimation.h>
#include <RTC.h>
#include <NTPClient.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

#include "animation.h"

ArduinoLEDMatrix matrix;

int httpPort = 80;
String method = "GET";
char hostName[] = "api.weatherapi.com";
String pathName = "/v1/current.json?key=b603cb58a97a476489312046251102&q=Rio%20de%20Janeiro&aqi=no";
WiFiClient client;
WiFiUDP Udp;
NTPClient timeClient(Udp);

int status = WL_IDLE_STATUS;

typedef struct NetworkStruct
{
  String SSID;
  String PASS;
} NetworkStruct;

NetworkStruct redes[2] = {
    {"Geralt of Rivia", "unoluh08"},
    {"Redmi Note 13 Pro+ 5G", "unoluh08"}};

float temperature;
String time0;
int isDay;
const char *location_localtime;
const char *current_condition_text;
RTCTime currentTime;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  matrix.loadSequence(animation);
  matrix.begin();
  Serial.println("Startup...");
  matrix.play(true);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }
  connectToNetworks(redes, 2);
  updateValues();
  setRTCTime();
  matrix.play(false);
  matrix.clear();
  Serial.println("...End of startup.");
}

void loop()
{
  // put your main code here, to run repeatedly:
  // Serial.println(WL_CONNECTED);
  delay(500);
  checkConn();// Verifica se to conectado
  // printWifiData();
  // while (status != WL_CONNECTED)
  // {
  //   byte mac[6];
  //   // scan for existing networks:
  //   Serial.println("Scanning available networks...");
  //   listNetworks();
  //   WiFi.macAddress(mac);
  //   Serial.println();
  //   Serial.print("Your MAC Address is: ");
  //   printMacAddress(mac);
  //   Serial.println();
  //   checkConn();
  //   delay(5000);
  // }
  textScroll(String(current_condition_text));
  delay(500);
  textScroll(String(temperature));
  delay(500);
  printRTCTime();
  // Serial.println(location_localtime);
  // setRTCTime();
  checkInfoUpdated();//de 15 em 15 minutos
}

void bootAnim(){
  matrix.loadSequence(animation);
  matrix.begin();
  matrix.play(true);
  matrix.play(false);
  matrix.clear();
}

void checkInfoUpdated(){
  RTC.getTime(currentTime);
  if((currentTime.getMinutes() == 15) || (currentTime.getMinutes() == 30) || (currentTime.getMinutes() == 45) || (currentTime.getMinutes() == 0))
  {
    textScroll("updating");
    updateValues();
  }
  else{
    
  }
}

void setRTCTime(){
  RTC.begin();
  Serial.println("\nStarting connection to time server...");
  timeClient.begin();
  timeClient.update();

  auto timeZoneOffsetHours = -3;
  auto unixTime = timeClient.getEpochTime() + (timeZoneOffsetHours * 3600);
  Serial.print("Unix Time = ");
  Serial.println(unixTime);
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);
  RTC.getTime(currentTime);
  Serial.println("RTC set to: " + String(currentTime));
}

void printRTCTime(){
  RTC.getTime(currentTime);
  String time;
  String hour;
  String mins;
  int hours = currentTime.getHour();
  int minutes = currentTime.getMinutes();
  if (hours < 10){
    hour.reserve(4);
    hour+="0";
    hour+=String(hours);
  }
  else{
    hour=String(hours);
  }
  if (minutes < 10){
    mins.reserve(4);
    mins+="0";
    mins+=String(minutes);
  }
  else{
    mins=String(minutes);
  }
  time.reserve(16);
  time+= " ";
  time+= hour;
  time+= ":";
  time+= mins;
  Serial.println(time);
  textScroll(time);
}

void textScroll(String data){
  // Make it scroll!
  matrix.beginDraw();

  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(100);

  // add the text
  // const char text[] = "    Scrolling text!    ";
  matrix.textFont(Font_5x7);
  matrix.beginText(12, 1, 0xFFFFFF);
  matrix.println(data);
  matrix.endText(SCROLL_LEFT);

  matrix.endDraw();
  
}

void updateValues(){
  
  if (client.connect(hostName, httpPort))
  {
    Serial.println("Connected to server");
    client.println(method + " " + pathName + " HTTP/1.0");
    client.println("Host: " + String(hostName));
    client.println("Connection: Close");
    client.println();
    while (client.connected())
    {
      if (client.available())
      {
        // read an incoming byte from the server and print it to serial monitor:

        char endOfHeaders[] = "\r\n\r\n";
        if (!client.find(endOfHeaders))
        {
          Serial.println("Invalid response");
          return;
        }
        else
        {
          String response = client.readString();
          // Serial.print(response.c_str());
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, response);

          if (error)
          {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
          }

          JsonObject location = doc["location"];
          location_localtime = location["localtime"];

          JsonObject current = doc["current"];
          temperature = current["temp_c"];
          isDay = current["is_day"];

          JsonObject curent_condition = current["condition"];
          current_condition_text = curent_condition["text"];
        }
      }
    }

    if (!client.connected())
    {
      Serial.println("Disconnected");
      client.stop();
    }
  }
  else
  {
    Serial.println("Connection to server failed");
  }
  // textScroll(String(temperature));
}


void connectToNetworks(NetworkStruct networks[], int networkCount)
{
  for (int i = 0; i < networkCount; i++)
  {
    Serial.println("Connecting to: ");
    Serial.print(networks[i].SSID.c_str());
    Serial.println("");
    status = WiFi.begin(networks[i].SSID.c_str(), networks[i].PASS.c_str());
    delay(5000);
    if (status == WL_CONNECTED)
    {
      Serial.println("Connected to network: " + networks[i].SSID);
      break;
    }
    else
    {
      Serial.println("Failed to connect to network: " + networks[i].SSID);
    }
  }
}

void checkConn()
{
  status = WiFi.status();
  if (status == 3)
  {
    Serial.println("Connected");
    Serial.print(status);
    Serial.println("");
  }
  else
  {
    Serial.println("Not Connected");
    Serial.print(status);
    Serial.println("");
    bootAnim();
    connectToNetworks(redes, 2);
  }
}

void printWifiData()
{
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");

  Serial.println(ip);

  // print your MAC address:
  // byte mac[6];
  // WiFi.macAddress(mac);
  // Serial.print("MAC address: ");
  // printMacAddress(mac);
}

void listNetworks()
{
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    Serial.println("Couldn't get a WiFi connection");
    while (true)
      ;
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++)
  {
    Serial.println("");
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print(" Signal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print(" Encryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
  }
}

void printEncryptionType(int thisType)
{
  // read the encryption type and print out the name:
  switch (thisType)
  {
  case ENC_TYPE_WEP:
    Serial.print("WEP");
    break;
  case ENC_TYPE_WPA:
    Serial.print("WPA");
    break;
  case ENC_TYPE_WPA2:
    Serial.print("WPA2");
    break;
  case ENC_TYPE_WPA3:
    Serial.print("WPA3");
    break;
  case ENC_TYPE_NONE:
    Serial.print("None");
    break;
  case ENC_TYPE_AUTO:
    Serial.print("Auto");
    break;
  case ENC_TYPE_UNKNOWN:
  default:
    Serial.print("Unknown");
    break;
  }
}

void printMacAddress(byte mac[])
{
  for (int i = 0; i < 6; i++)
  {
    if (i > 0)
    {
      Serial.print(":");
    }
    if (mac[i] < 16)
    {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}
