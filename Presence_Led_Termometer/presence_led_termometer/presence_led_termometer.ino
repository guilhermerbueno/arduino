#include <ESP8266WiFi.h> // lib para conectar ao wifi
#include <BlynkSimpleEsp8266.h> // lib para conectar ao app blynk
#include <ESP8266WebServer.h> // lib para publicar servidor web na node mcu
#include <PubSubClient.h> // lib de mqtt
#include <ArduinoJson.h> // lib para criar objetos json

/*blynk credentials*/
char auth[] = "553ddce9a0eb45ff8e6466d578b14b2f";

/* WiFi credentials */
char ssid[] = "bueno";
char pass[] = "12345678";

/*MQTT credentials*/
const char* mqtt_server = "mqtt.demo.konkerlabs.net";
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

/* HC-SR501 Motion Detector */
#define ledPin 5 //D1 - lampada
#define pirPin 13 //D7 Input for HC-S501
#define LED 15 //D8 - lampada do sensor de movimento
int pirValue; // Salvar valor lido do sensor
int lastMovement = 0;

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

//Variaveis globais
char bufferJ[256];
char *mensagem;

//Variaveis do termometro
float temperature;
float tensao;
float resistencia_termistor;
int i = 0;


ESP8266WebServer server(80); //Server on port 80
WiFiClient espClient;
PubSubClient client(espClient);

//codigo html da pagina que sera carregada no servidor web do dispositivo
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

// metodo executado ao abrir a pagina web do servidor web do dispositivo
void handleRoot() {
 Serial.println("You called root page");
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

// metodo chamado ao clicar no link para acender o led pela pagina web
void handleLEDon() { 
 Serial.println("LED on page");
 digitalWrite(LED,HIGH); //LED is connected in reverse
 server.send(200, "text/html", "ON"); //Send ADC value only to client ajax request
}

// metodo chamado ao clicar no link para apagar o led pela pagina web
void handleLEDoff() { 
 Serial.println("LED off page");
 digitalWrite(LED,LOW); //LED off
 server.send(200, "text/html", "OFF"); //Send ADC value only to client ajax request
}

//Funcao que formato os dados do termometro que serao enviados via mqtt
char *jsonMQTTmsgDATA(const char *device_id, const char *metric, float value) {
  const int capacity = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<capacity> jsonMSG;
  jsonMSG["deviceId"] = device_id;
  jsonMSG["metric"] = metric;
  jsonMSG["value"] = value;
  serializeJson(jsonMSG, bufferJ);
  return bufferJ;
}

void setup_wifi()
{
  WiFi.begin(ssid, pass);
  
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
}

void setup_blynk()
{
  Blynk.begin(auth, ssid, pass);
}

void setup_motion(){
  pinMode(ledPin, OUTPUT);
  pinMode(pirPin, INPUT);
  digitalWrite(ledPin, LOW);
}

void setup_led(){
  //Onboard LED port Direction output
  pinMode(LED,OUTPUT); 
  //Power on LED state off
  digitalWrite(LED,LOW);
}

void setup_webserver(){
  
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/ledOn", handleLEDon); //as Per  <a href="ledOn">, Subroutine to be called
  server.on("/ledOff", handleLEDoff);
  server.begin();                  //Start server

  Serial.println("HTTP server started");
}

void setup_mqtt(){
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect(){
  // Entra no Loop ate estar conectado
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Usando um ID unico (Nota: IDs iguais causam desconexao no Mosquito)
    // Tentando conectar
    if (client.connect(USER_Room, USER_Room, PWD_Room)) {
      Serial.println("connected");
      // Subscrevendo no topico esperado
      client.subscribe(SUB_Temp);
    } else {
      Serial.print("Falhou! Codigo rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      // Esperando 5 segundos para tentar novamente
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  setup_wifi();
  setup_blynk();
  setup_motion();
  setup_led();
  setup_mqtt();
  setup_mqtt();
}

// le valor do sensor de movimento
void getPirValue(void)
{
  pirValue = digitalRead(pirPin);
  if(pirValue != lastMovement){
    if (pirValue == HIGH)
    { 
      lastMovement = pirValue;
      Blynk.notify("Movimento detectado no escritório");

      mensagem = jsonMQTTmsgDATA("My_favorite_motion_sensor", "Boolean", 1);
      client.publish(PUB_Room.concat(CHANNEL_Mov), mensagem);
      client.loop(); // executa mqtt
    } else {
      lastMovement = pirValue;
      Blynk.notify("Escritório parece estar vazio");
      
      mensagem = jsonMQTTmsgDATA("My_favorite_motion_sensor", "Boolean", 0);
      client.publish(PUB_Room.concat(CHANNEL_Mov), mensagem);
      client.loop(); // executa mqtt
    }
  }
  digitalWrite(ledPin, pirValue);
}

//Criando a funcao de callback
//Essa funcao eh rodada quando uma mensagem eh recebida via MQTT.
//Nesse caso ela eh muito simples: imprima via serial o que voce recebeu
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void readTemperature(){
  tensao = 0;
  
  //Tirando a media do valor lido no ADC
  for (i=0; i< AMOSTRAS; i++) {
   tensao += analogRead(0)/AMOSTRAS;
   delay(10);
  }
  //Calculando a resistencia do Termistor
  resistencia_termistor = RESISTOR*tensao/(1023-tensao);
  //Equacao de Steinhart-Hart
  temperature = (1 / (log(resistencia_termistor/TERMISTORNOMINAL) * 1/BETA + 1/(TEMPERATURANOMINAL + 273.15))) - 273.15;
  //Vamos imprimir via Serial o resultado para ajudar na verificacao
  Serial.print("Resistencia do Termistor: "); 
  Serial.println(resistencia_termistor);
  Serial.print("Temperatura: "); 
  Serial.println(temperature);
  
  //Enviando via MQTT o resultado calculado da temperatura
  mensagem = jsonMQTTmsgDATA("My_favorite_thermometer", "Celsius", temperature);
  client.publish(PUB_Room.concat(CHANNEL_Temp), mensagem);
  client.loop(); // executa mqtt
}

void loop()
{
  //O programa em si eh muito simples: 
  //se nao estiver conectado no Broker MQTT, se conecte!
  if (!client.connected()) {
    reconnect();
  }

  readTemperature();
  getPirValue(); // le valor do sensor de movimento
  server.handleClient(); // captura requisicoes do servidor web
  Blynk.run(); // executa o aplicativo
  delay(2000); //aguarda 2 segundos para executar novo loop
}
