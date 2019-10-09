#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h"
#include "credentials.h"

#define DHT_PIN 4
#define DHTTYPE DHT11

DHT dht(DHT_PIN, DHTTYPE);
WiFiClientSecure client;

const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;
const char* host = "script.google.com";
const int httpsPort = 443;
float humidity;    //Stores humidity value
float temperature; //Stores temperature value
//String GAS_ID = "AKfycbyHs_okYL0StbajOmsfZBy_tD4cfhL1ADqUP8aIgIDEJiuIsDmL";  // Replace by your GAS service id
//String GAS_ID = "AKfycbzFOhR8juAX9jjZ0YWvmqDeaAieArslz4t6_STk5iW1nmqCr4Tp";
const String google_script_id = GAS_ID;

void setupWifi() {
  WiFi.begin(ssid, password);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(9600);
  client.setInsecure();
  dht.begin();
  setupWifi();
}

void loop()
{
  //readDHT();
  sendData(-22.855513,-47.0482262);

  delay(20000);
}


void readDHT() {
  //Read data and store it to variables hum and temp
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  //Print temp and humidity values to serial monitor
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Temp: ");
  Serial.print(temperature);
  Serial.println(" Celsius");

  if (!isnan(humidity) && !isnan(temperature)) {
    // Manda para o google drive
    sendData(temperature, humidity);    
  } else {
    Serial.println("Error Publishing");
  }
}

void sendData(float temp, float humi)
{
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  String string_temp     =  String(temp, DEC);
  String string_humi     =  String(humi, DEC);
  String url = "/macros/s/" + google_script_id + "/exec?coordx=" + string_temp + "&coordy=" + string_humi;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
  String line = client.readStringUntil('\n');
  if (line == "\r") {
    Serial.println("headers received");
    break;
  }
  }
  String line = client.readStringUntil('\a');
  if (line.startsWith("{\"state\":\"success\"")) {
  Serial.println("esp8266/Arduino CI successfull!");
  } else {
  Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
}
