#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <FirebaseArduino.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include "credentials.h"

#define PUBLISH_INTERVAL 1000*60*10 // Publish new states in 10 min

/*** WIFI ***/
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWORD;
/*** WIFI ***/

/*** FIREBASE ***/
const char* firebaseHost = FIREBASE_HOST;
const char firebaseAuth[] = FIREBASE_AUTH;
const int httpsPort = 443;
const String google_script_id = GAS_ID;
/*** FIREBASE ***/

/*** KONKER ***/
const char mqttServer[] = MQTT_SERVER;
const int mqttPort = 1883;
const char* mqttUser = MQTT_USER;
const char* mqttPassword = MQTT_PASS;
const char* PUB_Room = MQTT_PUB_Room;
const char* SUB_Room = MQTT_SUB_Room;
const char* CHANNEL_Temp = "temperature";
const char* CHANNEL_DHT_Temp = "dhttemperature";
const char* CHANNEL_Humidity = "dhthumidity";
/*** KONKER ***/

/*** DEFAULT ***/
int delay_in_seconds = 30; //time to new read
bool publishNewState = true;
int chk;
/*** DEFAULT ***/

/*Dados do termistor*/
// Resistencia nominal a 25C (Estamos utilizando um MF52 com resistencia nominal de 1kOhm)
#define TERMISTORNOMINAL 1000
// Temperatura na qual eh feita a medida nominal (25C)
#define TEMPERATURANOMINAL 25
//Quantas amostras usaremos para calcular a tensao media (um numero entre 4 e 10 eh apropriado)
#define AMOSTRAS 4
// Coeficiente Beta (da equacao de Steinhart-Hart) do termistor (segundo o datasheet eh 3100)
#define BETA 3100
// Valor da resistencia utilizada no divisor de tensao (para temperatura ambiente, qualquer resistencia entre 470 e 2k2 pode ser usada)
#define RESISTOR 470
/*Dados do termistor*/

/*** DHT ***/
#define DHTPIN 5     //GPIO number
#define DHTTYPE DHT11   // DHT 11  (AM2302)
DHT dht(DHTPIN, DHTTYPE);
/*** DHT ***/

//Variaveis do termometro
float tensao;
float resistencia_termistor;
int i = 0;

WiFiClient espClient;
PubSubClient pubSubClient(espClient);
Ticker ticker;

void publish() {
  publishNewState = true;
}

/*** SETUPS ***/
void setupWifi() {
  WiFi.begin(ssid, password);
  serialMonitor("connecting to wifi");
  while (WiFi.status() != WL_CONNECTED) {
    serialMonitor(".");
    delay(500);
  }
  serialMonitor("");
  serialMonitor("connected: ");
  serialMonitor(WiFi.localIP().toString());
}

void setupMqtt() {
  pubSubClient.setServer(mqttServer, mqttPort);
  pubSubClient.setCallback(callback);
  pubSubClient.subscribe(SUB_Room);
}

void reconnectMqtt() {
  while (!pubSubClient.connected()) {
    serialMonitor("Connecting to MQTT...");
    if (pubSubClient.connect(mqttUser, mqttUser, mqttPassword)) {
      serialMonitor(String(pubSubClient.connected()));
    } else {
      serialMonitor("failed with state ");
      serialMonitor(String(pubSubClient.state()));
      delay(2000);
    }
  }
}

void setupFirebase() {
  Firebase.begin(firebaseHost, firebaseAuth);
  sendToFirebase("lamp", false);
  sendToFirebase("presence", false);
}

void setupConfigurations(){
  int configuredTime = readIntFromFirebase("publish_interval");
  int publishInterval = PUBLISH_INTERVAL;
  serialMonitor("Valor lido de publish interval do firebase foi: ");
  serialMonitor(String(configuredTime));
  if(configuredTime > 0){
    publishInterval = configuredTime*1000*60; //converting to minutes
  }
  // timer to new publishes
  ticker.attach_ms(publishInterval, publish);
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  setupWifi();
  setupMqtt();
  setupFirebase();
  setupConfigurations();
}
/*** SETUPS ***/

/*** READING VALUES ***/
void readDHT() {
  //Read data and store it to variables hum and temp
  float humidity = dht.readHumidity();
  float temperature_dht = dht.readTemperature();

  //Print temp and humidity values to serial monitor
  serialMonitor("Humidity: " +  String(humidity) + " %, Temp: " +  String(temperature_dht) + " Celsius");
  sendData(humidity, temperature_dht, CHANNEL_DHT_Temp, "dht");
}

void readTemperature() {
  tensao = 0;

  //Tirando a media do valor lido no ADC
  for (i = 0; i < AMOSTRAS; i++) {
    tensao += analogRead(0) / AMOSTRAS;
    delay(10);
  }
  //Calculando a resistencia do Termistor
  resistencia_termistor = RESISTOR * tensao / (1023 - tensao);
  //Equacao de Steinhart-Hart
  float temperature = (1 / (log(resistencia_termistor / TERMISTORNOMINAL) * 1 / BETA + 1 / (TEMPERATURANOMINAL + 273.15))) - 273.15;
  //Vamos imprimir via Serial o resultado para ajudar na verificacao
  serialMonitor("Resistencia do Termistor: ");
  serialMonitor(String(resistencia_termistor));
  serialMonitor("Temperatura: ");
  serialMonitor(String(temperature));

  // Enviando via MQTT o resultado calculado da temperatura
  sendData(0, temperature, CHANNEL_Temp, "termometro");
}

bool readBoolFromFirebase(String key){
  return Firebase.getBool(key);
}

int readIntFromFirebase(String key){
  return Firebase.getInt(key);
}
/*** READING VALUES ***/

/*** SENDING VALUES ***/
void sendData(float humidity, float temperature, String channelName, String deviceId) {
  serialMonitor(String(humidity));
  if (!isnan(humidity) && humidity != float(0)) {
    // send data to firebase
    sendToFirebase("humidity", humidity);
    sendHumidityMqtt(humidity, deviceId);
  } else {
    serialMonitor("Error Publishing humidity");
  }
  if (!isnan(temperature)) {
    sendToFirebase(String(channelName), temperature);
    sendTemperatureMqtt(temperature, channelName, deviceId);
  } else {
    serialMonitor("Error Publishing temperature");
  }
}

void sendToFirebase(String key, float value) {
  Firebase.pushFloat(key, value);
}

void sendHumidityMqtt(float humidity, String deviceId) {
  char buf[100];
  strcpy(buf, PUB_Room);
  strcat(buf, CHANNEL_Humidity);
  
  char* mensagem = jsonMQTTmsgDATA(deviceId, "Percentage", humidity);
  Serial.println("Retorno publish");
  Serial.println(pubSubClient.publish(buf, mensagem));
  Serial.println(mensagem);
}

void sendTemperatureMqtt(float temperature, String channelName, String deviceId) {
  char buf[100];
  strcpy(buf, PUB_Room);
  if(channelName == "temperature"){
    strcat(buf, CHANNEL_Temp); 
  }else {
    strcat(buf, CHANNEL_DHT_Temp); 
  }

  char* mensagem = jsonMQTTmsgDATA(deviceId, "Celsius", temperature);
  Serial.println("Retorno publish: ");
  Serial.println(pubSubClient.publish(buf, mensagem));
  Serial.println(mensagem);
}

//Format data to send by mqtt
char * jsonMQTTmsgDATA(String device_id, const char *metric, float value) {
  static char bufferJ[256];
  const int capacity = JSON_OBJECT_SIZE(200);
  StaticJsonBuffer<capacity> jsonMSG;
  JsonObject& object = jsonMSG.createObject();
  object["deviceId"] = device_id;
  object["metric"] = metric;
  object["value"] = value;
  object.printTo(bufferJ);
  return bufferJ;
}
/*** SENDING VALUES ***/

bool checkIsActive(){
  bool configureActive = readBoolFromFirebase("active");
  if(configureActive != NULL){
    return configureActive;
  }

  return true;
}

// callback for mqqt calls
void callback(char* topic, byte* payload, unsigned int length) {

  serialMonitor("Message arrived in topic: ");
  serialMonitor(topic);

  serialMonitor("Message:");
  for (int i = 0; i < length; i++) {
    serialMonitor(String((char)payload[i]));
  }

  serialMonitor("");
  serialMonitor("-----------------------");
}

void serialMonitor(String value) {
  Serial.println(value);
}

void loop() {
  if(checkIsActive()){ // remote shutdown the publishes
    //if not connect to mqtt broker, reconnect
    if (!pubSubClient.connected()) {
      reconnectMqtt();
    }

    // publish after timer
    if (publishNewState) {
      setupConfigurations();
      serialMonitor("Publish new State");
      readDHT();
      readTemperature();
      publishNewState = false;
      pubSubClient.loop(); // executa mqtt
    }
  }

  delay(2000);
}
