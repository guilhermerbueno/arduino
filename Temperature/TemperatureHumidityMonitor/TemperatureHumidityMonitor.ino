#include <DHT.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <BlynkSimpleEsp8266.h>
 
//Criamos uma variável do tipo ESP8266WebServer que já possui funções
//que auxiliam na criação das rotas que o ESP8266 vai responder
ESP8266WebServer server(80);
//Variável do tipo DHT que possui funções para controlarmos o módulo dht 
//permitindo ler a temperatura e a umidade
DHT dht(0, DHT11);

//SSID and Password of your WiFi router
const char* ssid = "bueno";
const char* password = "12345678";
const char* auth = "553ddce9a0eb45ff8e6466d578b14b2f";
String ip = "";


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
#define LED 13

//Função que definimos para ser chamada quando o caminho requisitado não foi registrado 
void onNotFound() 
{
  server.send(404, "text/plain", "Not Found" );
}

//Função que definimos que será executada quando o cliente fizer uma requisição
//do tipo GET no caminho http://192.168.2.8/temperature (pode ser outro ip dependendo da sua configuração) 
void getTemperature()
{
  //Fazemos a leitura da temperatura através do módulo dht
  float t = dht.readTemperature();
  //Cria um json com os dados da temperatura
  String json = "{\"temperature\":"+String(t)+"}";
  //Envia o json para o cliente com o código 200, que é o código quando a requisição foi realizada com sucesso
  Serial.println("Entrou no metodo de temperatura e a temperatura lida é: " + String(t));
  server.send (200, "application/json", json);
}

//Função que definimos que será executada quando o cliente fizer uma requisição
//do tipo GET no caminho http://192.168.2.8/humidity (pode ser outro ip dependendo da sua configuração) 
void getHumidity()
{
  //Fazemos a leitura da umidade através do módulo dht
  float h = dht.readHumidity();
  //Cria um json com os dados da umidade
  String json = "{\"humidity\":"+String(h)+"}";
  //Envia o json para o cliente com o código 200, que é o código quando a requisição foi realizada com sucesso
  server.send(200, "application/json", json);
}

//Função que definimos que será executada quando o cliente fizer uma requisição
//do tipo GET no caminho http://192.168.2.8/monitor (pode ser outro ip dependendo da sua configuração) 
void showMonitor()
{
  String html =
  "<html>"
  "<head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'/>"
    "<title>DHT Monitor</title>"
    "<style type='text/css'>"
      "body{"
            "padding: 35px;"
            "background-color: #222222;"
      "}"
      "h1{"
        "color: #FFFFFF;"
        "font-family: sans-serif;"
      "}"
      "p{"
        "color: #EEEEEE;"
        "font-family: sans-serif;"
            "font-size:18px;"
      "}"
   "</style>"
  "</head>"
  "<body>"
   "<h1>DHT Monitor</h1>"
   "<p id='temperature'>Temperature: </p>"
   "<p id='humidity'>Humidity: </p>"
 "</body>"
 "<script type='text/javascript'>"
   "refresh();" 
   "setInterval(refresh, 5000);"
   "function refresh()"
   "{"
     "refreshTemperature()"
     "refreshHumidity();"
   "}"
   "function refreshTemperature()"
   "{"
     "var xmlhttp = new XMLHttpRequest();"
     "xmlhttp.onreadystatechange = function() {"
       "if (xmlhttp.readyState == XMLHttpRequest.DONE && xmlhttp.status == 200){"
         "document.getElementById('temperature').innerHTML = 'Temperature: ' + JSON.parse(xmlhttp.responseText).temperature + 'C';"
       "}"
     "};"
     "xmlhttp.open('GET', '" + ip + "'/temperature', true);"
     "xmlhttp.send();"
   "}"
   "function refreshHumidity()"
    "{"
      "var xmlhttp = new XMLHttpRequest();"
      "xmlhttp.onreadystatechange = function() {"
        "if (xmlhttp.readyState == XMLHttpRequest.DONE && xmlhttp.status == 200){"
          "document.getElementById('humidity').innerHTML = 'Humidity: ' + JSON.parse(xmlhttp.responseText).humidity + '%';"
        "}"
      "};"
      "xmlhttp.open('GET', '" + ip + "'/humidity', true);"
      "xmlhttp.send();"
    "}"
  "</script>"
  "</html>";
  //Envia o html para o cliente com o código 200, que é o código quando a requisição foi realizada com sucesso
  server.send(200, "text/html", html);
}

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

void setup() {
  //Inicialize a Serial apenas caso esteja com o ESP8266 conectado ao computador pela serla queira ter um log 
  //para facilitar saber o que está acontecendo com o ESP8266
  Serial.begin(115200);
 
  Blynk.begin(auth, ssid, password);
  //Instrução para o ESP8266 se conectar à rede. 
  //No nosso caso o nome da rede é TesteESP e a senha é 87654321. 
  //Você deve alterar com as informações da sua rede
  WiFi.begin(ssid, password);
 
  //Feedback caso esteja usando o Monitor Serial 
  Serial.println("");
  Serial.print("Conectando");
  
  //Esperamos até que o módulo se conecte à rede
  while (WiFi.status() != WL_CONNECTED)
  {
   delay(500);
   Serial.print(".");
  }
 
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  ip = WiFi.localIP().toString();

  //Aqui definimos qual a função será executada para o caminho e tipo dado.
  //Nesse caso quando houver uma requisição http do tipo GET no caminho http://192.168.2.8/temperature
  //(pode ser outro ip dependendo da sua configuração) a função getTemperature será executada
  server.on("/temperature", HTTP_GET, getTemperature);

  //Nesse outo caso quando houver uma requisição http do tipo GET no caminho http://192.168.2.8/humidity
  //(pode ser outro ip dependendo da sua configuração) a função getHumidity será executada
  server.on("/humidity", HTTP_GET, getHumidity);

  //Nesse caso quando houver uma requisição http do tipo GET no caminho http://192.168.2.8/monitor
  //(pode ser outro ip dependendo da sua configuração) a função showMonitor será executada. 
  //Esta função retornará a página principal que mostrará os valores
  //da temperatura e da umidade e recarregará essas informações de tempos em tempos
  server.on("/monitor", HTTP_GET, showMonitor);
  
  //Aqui definimos qual função será executada caso o caminho que o cliente requisitou não tenha sido registrado
  server.onNotFound(onNotFound);

  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/ledOn", handleLEDon); //as Per  <a href="ledOn">, Subroutine to be called
  server.on("/ledOff", handleLEDoff);

  //Inicializamos o server que criamos na porta 80
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  // put your main code here, to run repeatedly:
  Blynk.run();
  server.handleClient();          //Handle client requests
  

}
