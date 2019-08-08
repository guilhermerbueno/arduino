/**************************************************************
 * IoT Motion Detector with Blynk
 * Blynk library is licensed under MIT license
 * This example code is in public domain.
 * 
 * Developed by Marcelo Rovai - 30 November 2016
 **************************************************************/
#include <ESP8266WiFi.h>

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <BlynkSimpleEsp8266.h>

//ESP Web Server Library to host a web page
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

char auth[] = "553ddce9a0eb45ff8e6466d578b14b2f";

/* WiFi credentials */
char ssid[] = "AEG";
char pass[] = "a1b2c3d4";
const char* mqttServer = "mqtt.demo.konkerlabs.net";
const int mqttPort = 1883;
const char* mqttUser = "2sn6ghtmaohe";
const char* mqttPassword = "RUAw6Qec92WW";

/* HC-SR501 Motion Detector */
#define ledPin 5 //D1
#define pirPin 13 //D7 Input for HC-S501
#define LED 15 //D8
int pirValue; // Place to store read PIR Value
int lastMovement = 0;

//Declare a global object variable from the ESP8266WebServer class.
ESP8266WebServer server(80); //Server on port 80

WiFiClient espClient;
PubSubClient client(espClient);

//---------------------------------------------------------------
//Our HTML webpage contents in program memory
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<center>
<h1>WiFi LED on off demo: 1</h1><br>
Ciclk to turn <a href="ledOn" target="myIframe">LED ON</a><br>
Ciclk to turn <a href="ledOff" target="myIframe">LED OFF</a><br>
LED State:<iframe name="myIframe" width="100" height="25" frameBorder="0"><br>
</center>

</body>
</html>MAIN_page
)=====";
//---------------------------------------------------------------
//On board LED Connected to GPIO2


//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
 Serial.println("You called root page");
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

void handleLEDon() { 
 Serial.println("LED on page");
 digitalWrite(LED,HIGH); //LED is connected in reverse
 server.send(200, "text/html", "ON"); //Send ADC value only to client ajax request
}

void handleLEDoff() { 
 Serial.println("LED off page");
 digitalWrite(LED,LOW); //LED off
 server.send(200, "text/html", "OFF"); //Send ADC value only to client ajax request
}

void setup()
{
  Serial.begin(115200);
  delay(10);
  WiFi.begin(ssid, pass);
  Blynk.begin(auth, ssid, pass);
  pinMode(ledPin, OUTPUT);
  pinMode(pirPin, INPUT);
  digitalWrite(ledPin, LOW);
  
  //Onboard LED port Direction output
  pinMode(LED,OUTPUT); 
  //Power on LED state off
  digitalWrite(LED,LOW);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
 
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/ledOn", handleLEDon); //as Per  <a href="ledOn">, Subroutine to be called
  server.on("/ledOff", handleLEDoff);

  server.begin();                  //Start server

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected");
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
  
  Serial.println("HTTP server started");
}

void loop()
{
  getPirValue();
  server.handleClient();          //Handle client requests
  client.loop();
  Blynk.run();
}

/***************************************************
 * Get PIR data
 **************************************************/
void getPirValue(void)
{
  pirValue = digitalRead(pirPin);
  if(pirValue != lastMovement){
    if (pirValue == HIGH)
    { 
      lastMovement = pirValue;
      //Serial.println("==> Motion detected");
      Blynk.notify("Movimento detectado no escritório");
      client.publish("data/2sn6ghtmaohe/pub/detection", "Movimento detectado no escritório");
    } else {
      lastMovement = pirValue;
      //Serial.println("==> Motion not detected");
      Blynk.notify("Escritório parece estar vazio");
      client.publish("data/2sn6ghtmaohe/pub/detection", "Escritório parece estar vazio");
    }
  }
  digitalWrite(ledPin, pirValue);
}

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}
