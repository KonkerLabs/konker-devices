// Codigo do botao com ACK em comunicacao MQTT
// Responsavel: Luis Fernando Gomez Gonzalez (luis.gonzalez@inmetrics.com.br)
// Projeto Konker

// Agora usando o gerenciador de conexão WiFiManagerK.

#include <FS.h>  
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManagerK.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <ArduinoJson.h> 
#include "konker.h"

extern "C" {
  #include "user_interface.h"
}

#define sec             1000

//ACK phrase
const char dev_ack[4] = "ack";

//Variaveis da Configuracao em Json
char api_key[17];
char mqtt_server[16];
char mqtt_port[5];
char mqtt_login[32];
char mqtt_pass[32];
char *mensagemjson;

//Variaveis Fisicas
int buttonPin = 2;
int buttonState = 0;
float temperature;

//Resultado da soma do ACK
int name_ok = 0;

//String de command
char cmd[32];
const char cmd1[] = "ButtonPressed";
const char cmd2[] = "ButtonPressedLong";

//Buttonfloat
unsigned long timestart;
unsigned long timebutton;
unsigned int longpress = 4000;
int loop_enable;

//Definindo os objetos de Wifi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);


// Funcao de Callback para o MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  for (i = 0; i < 16; i++) {
    if ((char)payload[i]==device_id[i]) name_ok++;
  }
  for (i = 16; i < length; i++) {
    if ((char)payload[i]==dev_ack[i-16]) name_ok++;;
  }  
}


// Setup do Microcontrolador: Vamos configurar o acesso serial, conectar no Wifi, configurar o MQTT e o GPIO do botao
void setup(){
  
  Serial.begin(115200);
    
//------------------- Montando Sistema de arquivos e copiando as configuracoes  ----------------------

  spiffsMount(mqtt_server, mqtt_port, mqtt_login, mqtt_pass, device_id, device_type, api_key, in_topic, cmd_topic, data_topic, ack_topic);  

  //Criando as variaveis dentro do WiFiManager
  WiFiManagerParameter custom_api_key("api", "api key", api_key, 17);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 16);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_login("login", "mqtt login", mqtt_login, 32);
  WiFiManagerParameter custom_mqtt_pass("password", "mqtt pass", mqtt_pass, 32);
 

//------------------- Configurando MQTT e LED ----------------------

  client.setServer(mqtt_server, atol(mqtt_port));
  client.setCallback(callback);
  client.subscribe(cmd_topic);
  
  pinMode(buttonPin, INPUT);
  client.subscribe(ack_topic);
  
//------------------- Configuracao do WifiManager K ----------------------
  WiFiManager wifiManager;  
  wifiManager.setTimeout(500);
  wifiManager.setBreakAfterConfig(1);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  wifiManager.addParameter(&custom_api_key);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_login);
  wifiManager.addParameter(&custom_mqtt_pass);
  
  //wifiManager.resetSettings(); //Caso seja necessario resetar os valores para Debug
  

//------------------- Caso conecte, copie os dados para o FS ----------------------
  if(!wifiManager.autoConnect("KonkerConfig")) {
    
    //Copiando parametros  
    copyHTMLPar(api_key, mqtt_server, mqtt_port, mqtt_login, mqtt_pass, custom_api_key, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass);
    
    //Salvando Configuracao
    if (shouldSaveConfig) {  
                             saveConfigtoFile(api_key, mqtt_server, mqtt_port, mqtt_login, mqtt_pass);
                          }
    delay(2500);
    ESP.reset();
    } 

//------------------- Caso tudo mais falhe copie os dados para o FS ----------------------
//Copiando parametros  
copyHTMLPar(api_key, mqtt_server, mqtt_port, mqtt_login, mqtt_pass, custom_api_key, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass);

//Salvando Configuracao
if (shouldSaveConfig) {  
  saveConfigtoFile(api_key, mqtt_server,mqtt_port,mqtt_login,mqtt_pass); 
  }
//-----------------------------------------------------------------------------------------

}

  
 // Loop com o programa principal
void loop(){
  // Aguardando 0.3 segundos entre loops (PS.: O clique no botao deve ocorrer por um tempo superior a 0.3 segundos) 
  delay(100);
 
  // Se desconectado, reconectar.
  if (!client.connected()) {
    reconnect(client, api_key, mqtt_login, mqtt_pass);
  }
  client.loop();
  
  // Lendo o estado do botao. Como ele esta conectado em um divisor, o estado natural eh HIGH. Isso ocorre pois o GPIO2 nao pode ser inicializado em LOW.
  buttonState = digitalRead(buttonPin);
  if ((buttonState == LOW)&&(enable == 1)) {
      strcpy(cmd,cmd1);
      timestart = millis();
      loop_enable = 1;
      while ((buttonState == LOW)&&(loop_enable == 1))
        { 
          buttonState = digitalRead(buttonPin);
          timebutton = millis()-timestart;  
          delay(100);   
          if (timebutton>longpress)
            {
              strcpy(cmd,cmd2);
              loop_enable=0;
            }
        }
      
      //Por enquanto o TS é um placeholder.

      mensagemjson = jsonMQTTmsgCMD("1454853486000",cmd);        
      if (messageACK(client, cmd_topic, mensagemjson, ack_topic, name_ok)) name_ok=0;

      temperature = float(100*analogRead(A0)/1024);
      mensagemjson = jsonMQTTmsgDATA("000","temperature",temperature,"Celsius");
      if (messageACK(client, data_topic, mensagemjson, ack_topic, name_ok)) name_ok=0;
  }

}

