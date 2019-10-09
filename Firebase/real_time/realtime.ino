#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>


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

void setupFirebase() {
  Firebase.begin(firebaseHost, firebaseAuth);
  sendToFirebase("lamp", false);
  sendToFirebase("presence", false);
}

void setup(){
  Serial.begin(115200);
  setupWifi();
  setupFirebase();
}

void serialMonitor(String value) {
  Serial.println(value);
}

getObject(){
    FirebaseObject reservas = Firebase.get("Reserva");
    if(Firebase.success()){
        serialMonitor("SUCESSOOO!");
    }
    serialMonitor(reservas.getString("Reserva1"));
}

void loop(){
  delay(3000);
  getObject();
}