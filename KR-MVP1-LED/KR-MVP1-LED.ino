// Codigo do LED com ACK em comunicacao MQTT
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

//ACK phrase
const char dev_ack[4] = "ack";

//Variaveis da Configuracao em Json
char mqtt_server[16];
char mqtt_port[5];
char mqtt_login[32];
char mqtt_pass[32];
char *mensagemjson;

//Variaveis Fisicas
int LED1 = 2;//4;
int LED2 = 2;//5;
int LEDState = 0;

//String de command
char *cmd;
const char cmd1[] = "ButtonPressed";
const char cmd2[] = "ButtonPressedLong";
String pubString;
char message_buffer[20];


//Definindo os objetos de Wifi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);


// Funcao de Callback para o MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  int i;
  int state=0;
  
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  for (i = 0; i < length; i++) {
    msgBufferIN[i] = payload[i];
    Serial.print((char)payload[i]);
    
  }
  msgBufferIN[i] = '\0';
  received_msg = 1;
  Serial.println("");
}


// Setup do Microcontrolador: Vamos configurar o acesso serial, conectar no Wifi, configurar o MQTT e o GPIO do botao
void setup(){
  
  Serial.begin(115200);  

//------------------- Montando Sistema de arquivos e copiando as configuracoes  ----------------------

  spiffsMount(mqtt_server, mqtt_port, mqtt_login, mqtt_pass, device_id, device_type, api_key, in_topic, cmd_topic, data_topic, ack_topic);  

  //Criando as variaveis dentro do WiFiManager
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 16);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_login("login", "mqtt login", mqtt_login, 32);
  WiFiManagerParameter custom_mqtt_pass("password", "mqtt pass", mqtt_pass, 32);
 

//------------------- Configurando MQTT e LED ----------------------

  client.setServer(mqtt_server, atol(mqtt_port));
  client.setCallback(callback);
  client.subscribe(cmd_topic);
  Serial.print("Topico: ");
  Serial.println(in_topic);
  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  digitalWrite(LED1, 1);
  digitalWrite(LED2, 1);

//------------------- Configuracao do WifiManager K ----------------------
  WiFiManager wifiManager;  
  wifiManager.setTimeout(500);
  wifiManager.setBreakAfterConfig(1);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_login);
  wifiManager.addParameter(&custom_mqtt_pass);
  
  //wifiManager.resetSettings(); //Caso seja necessario resetar os valores para Debug
  

//------------------- Caso conecte, copie os dados para o FS ----------------------
  if(!wifiManager.autoConnect("KonkerConfig")) {
    
    //Copiando parametros  
    copyHTMLPar(mqtt_server, mqtt_port, mqtt_login, mqtt_pass, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass);
    
    //Salvando Configuracao
    if (shouldSaveConfig) {  
                             saveConfigtoFile(mqtt_server, mqtt_port, mqtt_login, mqtt_pass);
                          }
    delay(2500);
    ESP.reset();
    } 

//------------------- Caso tudo mais falhe copie os dados para o FS ----------------------
//Copiando parametros  
copyHTMLPar(mqtt_server, mqtt_port, mqtt_login, mqtt_pass, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass);

//Salvando Configuracao
if (shouldSaveConfig) {  
  saveConfigtoFile(mqtt_server,mqtt_port,mqtt_login,mqtt_pass); 
  }
//-----------------------------------------------------------------------------------------

}
  
 // Loop com o programa principal
void loop(){

  delay(100);
 
  // Se desconectado, reconectar.
  if (!client.connected()) {
    reconnect(client, api_key, mqtt_login, mqtt_pass);
  }
  client.loop();
  client.subscribe(in_topic);
  if (received_msg==1)
      {
        cmd=jsonMQTT_in_msg(msgBufferIN);
        //Serial.println(cmd);
        
        if ((String(cmd) == String(cmd1))||(String(cmd) == String(cmd2)))
        {
        LEDState = digitalRead(LED1);
        digitalWrite(LED1, !LEDState);
        digitalWrite(LED2, !LEDState); 
        //Serial.println("Switching LED");
        pubString = String(device_id) + String(dev_ack);
        pubString.toCharArray(message_buffer, pubString.length()+1);
        client.publish(ack_topic, message_buffer); 
        }
        received_msg=0;        
      }
  
 

}

