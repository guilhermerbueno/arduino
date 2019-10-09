#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <Ticker.h>
#include "DHT.h"
#include <WiFiClientSecure.h>
#include "credentials.h"
#include <PubSubClient.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoJson.h> // lib para criar objetos json

#define LAMP_PIN 5
#define PRESENCE_PIN 0
#define DHT_PIN 4
#define DHTTYPE DHT11
#define PUBLISH_INTERVAL 1000*30*1 // Publique a cada 0.5 min

// Set these to run example.
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWORD;
const char firebaseHost[] = FIREBASE_HOST;
const char firebaseAuth[] = FIREBASE_AUTH;
const String google_script_id = GAS_ID;
const char* googleHost = "script.google.com";
const int httpsPort = 443;
char blinkAuth[] = "553ddce9a0eb45ff8e6466d578b14b2f";

/*MQTT credentials*/
const char* mqttServer = "mqtt.demo.konkerlabs.net";
const char* mqttPort = "1883";
const char* USER_Temp = "5u94a87gldlr";
const char* PWD_Temp = "0qdFfiRAECyk";
const char* PUB_Temp = "data/5u94a87gldlr/pub/temperatura";
const char* SUB_Temp = "data/5u94a87gldlr/sub/temperatura";

const char* USER_Room = "2sn6ghtmaohe";
const char* PWD_Room = "RUAw6Qec92WW";
const char* PUB_Room = "data/2sn6ghtmaohe/pub/";
const char* SUB_Room = "data/2sn6ghtmaohe/sub/";
const char* CHANNEL_Temp = "temperatura";
const char* CHANNEL_Mov = "movimento";

DHT dht(DHT_PIN, DHTTYPE);
Ticker ticker;
WiFiClientSecure client;
PubSubClient pubSubClient(client);
bool publishNewState = true;
int chk;
float humidity;    //Stores humidity value
float temperature; //Stores temperature value
char bufferJ[256];
char *mensagem;

void publish() {
  publishNewState = true;
}

/*** SETUPS ***/
void setup() {
  Serial.begin(9600);

  setupPins();
  setupWifi();
  setupFirebase();
  //setupMqtt();
  client.setInsecure();

  // Registra o ticker para publicar de tempos em tempos
  ticker.attach_ms(PUBLISH_INTERVAL, publish);
}

void setupPins() {
  pinMode(LAMP_PIN, OUTPUT);
  digitalWrite(LAMP_PIN, LOW);
  pinMode(PRESENCE_PIN, INPUT);
  dht.begin();
}

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

void setupFirebase() {
  Firebase.begin(firebaseHost, firebaseAuth);
  sendToFirebase("lamp",false);
  sendToFirebase("presence", false);
}

void setupMqtt(){
  pubSubClient.setServer(mqttServer, 1883);
  pubSubClient.setCallback(callback);
}

void reconnect(){
  // Entra no Loop ate estar conectado
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Usando um ID unico (Nota: IDs iguais causam desconexao no Mosquito)
    // Tentando conectar
    if (pubSubClient.connect(USER_Room, USER_Room, PWD_Room)) {
      Serial.println("connected");
      // Subscrevendo no topico esperado
      pubSubClient.subscribe(SUB_Temp);
    } else {
      Serial.print("Falhou! Codigo rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" Tentando novamente em 5 segundos");
      // Esperando 5 segundos para tentar novamente
      delay(5000);
    }
  }
}
/*** SETUPS ***/


/*** READING VALUES ***/
void readDHT() {
  //Read data and store it to variables hum and temp
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  //Print temp and humidity values to serial monitor
  printMonitorSerial("Humidity: " +  String(humidity) + " %, Temp: " +  String(temperature) + " Celsius");

  if (!isnan(humidity) && !isnan(temperature)) {
    // Manda para o firebase
    sendToFirebase("temperature", temperature);
    sendToFirebase("humidity", humidity);
    sendToGoogleDrive(temperature, humidity);
    
    publishNewState = false;
  } else {
    printMonitorSerial("Error Publishing");
  }
}

void readPresence() {
  // Verifica o valor do sensor de presença
  // LOW sem movimento
  // HIGH com movimento
  int presence = digitalRead(PRESENCE_PIN);
  printMonitorSerial("presence is: " + String(presence));

  sendToFirebase("presence", presence == HIGH);
}

void readLamp() {
  // Verifica o valor da lampada no firebase
  bool lampValue = readFromFirebase("lamp");
  printMonitorSerial("Lamp is: " + String(lampValue));
  digitalWrite(LAMP_PIN, lampValue == 0 ? HIGH : LOW);
}

bool readFromFirebase(String key){
  return Firebase.getBool(key);
}
/*** READING VALUES ***/


/*** SENDING VALUES ***/
void sendToFirebase(String key, bool value){
  Firebase.setBool(key, value);
}

void sendToFirebase(String key, float value){
  Firebase.pushFloat(key, value);
}

void sendTemperatureByMqtt(float temperature){
  //Enviando via MQTT o resultado calculado da temperatura
  mensagem = jsonMQTTmsgDATA("My_favorite_thermometer", "Celsius", temperature);
  //pubSubClient.publish(PUB_Room.concat(CHANNEL_Temp), mensagem);
  pubSubClient.loop(); // executa mqtt
}

void sendToGoogleDrive(float temp, float humi)
{
  Serial.print("connecting to ");
  Serial.println(googleHost);
  if (!client.connect(googleHost, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  String string_temp     =  String(temp);
  String string_humi     =  String(humi);
  String url = "/macros/s/" + google_script_id + "/exec?temperature=" + string_temp + "&humidity=" + string_humi;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "host: " + googleHost + "\r\n" +
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
/*** SENDING VALUES ***/

void printMonitorSerial(String value){
  Serial.println(value);
}

//Funcao que formato os dados do termometro que serao enviados via mqtt
char *jsonMQTTmsgDATA(const char *device_id, const char *metric, float value) {
  /*const int capacity = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<capacity> jsonMSG;
  jsonMSG["deviceId"] = device_id;
  jsonMSG["metric"] = metric;
  jsonMSG["value"] = value;
  serializeJson(jsonMSG, bufferJ);*/
  return bufferJ;
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

void loop() {
  // Apenas publique quando passar o tempo determinado
  if (publishNewState) {
    printMonitorSerial("Publish new State");
    readDHT();
  }

  readPresence();
  readLamp();

  //se nao estiver conectado no Broker MQTT, se conecte!
  //if (!pubSubClient.connected()) {
  //  reconnect();
  //}


  delay(200);
}
