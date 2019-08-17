//Libraries
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <FirebaseArduino.h>
#include <Ticker.h>

//Constants
#define DHTPIN 4     //GPIO number
#define DHTTYPE DHT11   // DHT 11  (AM2302)
#define FIREBASE_HOST "smartclassroom-d8200.firebaseio.com"
#define FIREBASE_AUTH "xC5GfkbL3oyYDgkFzzw2HvUOu2bnjiFSfP7M66PN"
#define WIFI_SSID "bueno"
#define WIFI_PASSWORD "12345678"
#define PUBLISH_INTERVAL 1000*60*1

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
Ticker ticker;
bool publishNewState = true;

//Variables
int chk;
float humidity;    //Stores humidity value
float temperature; //Stores temperature value

void publish(){
  publishNewState = true;
}

void setupWifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
}

void setupFirebase(){
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.setBool("temperature", 0);
  Firebase.setBool("humidity", 0);
}

void setup()
{
    Serial.begin(115200);
    dht.begin();
 
    setupWifi();
    setupFirebase();
         
    // Registra o ticker para publicar de tempos em tempos
    ticker.attach_ms(PUBLISH_INTERVAL, publish);
}

void readDHT(){
    //Read data and store it to variables hum and temp
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temp: ");
    Serial.print(temperature);
    Serial.println(" Celsius");
    
    if(!isnan(humidity) && !isnan(temperature)){
      // Manda para o firebase
      Firebase.pushFloat("temperature", temperature);
      Firebase.pushFloat("humidity", humidity);
      //Firebase.setFloat("temperature", temperature);
      //Firebase.setFloat("humidity", humidity);    
      publishNewState = false;
    }else{
      Serial.println("Error Publishing");
    }
}

void loop()
{
    readDHT();
    delay(30000); //Delay 2 sec.
}
