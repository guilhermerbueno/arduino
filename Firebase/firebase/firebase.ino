#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <Ticker.h>
#include "DHT.h"
#include "credentials.h"

// Set these to run example.
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;
const char host[] = FIREBASE_HOST;
const char auth[] = FIREBASE_AUTH;

#define LAMP_PIN 5
#define PRESENCE_PIN 0
#define DHT_PIN 4
#define DHTTYPE DHT11
// Publique a cada 1 min
#define PUBLISH_INTERVAL 1000*60*1

DHT dht(DHT_PIN, DHTTYPE);
Ticker ticker;
bool publishNewState = true;
int chk;
float humidity;    //Stores humidity value
float temperature; //Stores temperature value

void publish() {
  publishNewState = true;
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
  Firebase.begin(host, auth);
  Firebase.setBool("lamp", false);
  Firebase.setBool("presence", false);
}

void setup() {
  Serial.begin(9600);

  setupPins();
  setupWifi();
  setupFirebase();

  // Registra o ticker para publicar de tempos em tempos
  ticker.attach_ms(PUBLISH_INTERVAL, publish);
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
    // Manda para o firebase
    Firebase.pushFloat("temperature", temperature);
    Firebase.pushFloat("humidity", humidity);
    //Firebase.setFloat("temperature", temperature);
    //Firebase.setFloat("humidity", humidity);
    publishNewState = false;
  } else {
    Serial.println("Error Publishing");
  }
}

void readPresence() {
  // Verifica o valor do sensor de presença
  // LOW sem movimento
  // HIGH com movimento
  int presence = digitalRead(PRESENCE_PIN);
  Serial.print("presence is: ");
  Serial.println(presence);
  Firebase.setBool("presence", presence == HIGH);
}

void readLamp() {
  // Verifica o valor da lampada no firebase
  bool lampValue = Firebase.getBool("lamp");
  Serial.print("Lamp is: ");
  Serial.println(lampValue);
  digitalWrite(LAMP_PIN, lampValue == 0 ? HIGH : LOW);
}

void loop() {

  // Apenas publique quando passar o tempo determinado
  if (publishNewState) {
    Serial.println("Publish new State");
    // Obtem os dados do sensor DHT
    readDHT();
  }

  readPresence();
  readLamp();

  delay(200);
}
