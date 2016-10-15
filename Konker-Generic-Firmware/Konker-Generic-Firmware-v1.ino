// Código de exemplo de comunicacao MQTT com configuração de rede via HTML
// Responsavel: Luis Fernando Gomez Gonzalez (luis@konkerlabs.com)

// Usando o gerenciador de conexão WiFiManagerK.

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
#include <ESP8266TelegramBOT.h>



extern "C" {
  #include "user_interface.h"
}

int configurechannel = 0;
int conf_channel = 0;


//ADC_MODE(ADC_VCC);

//Variaveis da Configuracao em Json
char api_key[17];
char device_id[17];
char mqtt_server[64];
char mqtt_port[5];
char mqtt_login[32];
char mqtt_pass[32];
char *mensagemjson;

//Variaveis Fisicas
float temperature;

//String de command
String pubString;
char message_buffer[20];
int reconnectcount=0;
char *type;
char typeC[32];
String Stype;
int disconnected=0;

int marked=0;

//Definindo os objetos de Wifi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//Ticker
Ticker timer;


//Criando as variaveis dentro do WiFiManager
WiFiManagerParameter custom_api_key("api", "api key", api_key, 17);
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 64);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
WiFiManagerParameter custom_mqtt_login("login", "mqtt login", mqtt_login, 32);
WiFiManagerParameter custom_mqtt_pass("password", "mqtt password", mqtt_pass, 32);
WiFiManagerParameter custom_mqtt_channel_in("channel_input", "mqtt channel data input", mqtt_channel_in, 32);
WiFiManagerParameter custom_mqtt_channel_out("channel_output", "mqtt channel data output", mqtt_channel_out, 32);
WiFiManagerParameter custom_mqtt_channel_cmd_in("command_channel_input", "mqtt channel command input", mqtt_channel_cmd_in, 32);
WiFiManagerParameter custom_mqtt_channel_cmd_out("command_channel_output", "mqtt channel command input", mqtt_channel_cmd_out, 32);


WiFiManager wifiManager;  

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
  strcpy(msgTopic, topic);
  received_msg = 1;
  Serial.println("");
}


// Setup do Microcontrolador: Vamos configurar o acesso serial, conectar no Wifi, configurar o MQTT e o GPIO do botao
void setup(){
  
  Serial.begin(115200);  
 
//------------------- Montando Sistema de arquivos e copiando as configuracoes  ----------------------
spiffsMount(mqtt_server, mqtt_port, mqtt_login, mqtt_pass, device_id, device_type, api_key, mqtt_channel_in, mqtt_channel_out, mqtt_channel_cmd_in, mqtt_channel_cmd_out, ack_topic, connect_topic, config_period);  

//Criando as variaveis dentro do WiFiManager
WiFiManagerParameter custom_api_key("api", "api key", api_key, 17);
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 64);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
WiFiManagerParameter custom_mqtt_login("login", "mqtt login", mqtt_login, 32);
WiFiManagerParameter custom_mqtt_pass("password", "mqtt pass", mqtt_pass, 32);
WiFiManagerParameter custom_mqtt_channel_in("channel_input", "mqtt channel data input", mqtt_channel_in, 32);
WiFiManagerParameter custom_mqtt_channel_out("channel_output", "mqtt channel data output", mqtt_channel_out, 32);
WiFiManagerParameter custom_mqtt_channel_cmd_in("command_channel_input", "mqtt channel command input", mqtt_channel_cmd_in, 32);
WiFiManagerParameter custom_mqtt_channel_cmd_out("command_channel_output", "mqtt channel command input", mqtt_channel_cmd_out, 32);


//------------------- Configuracao do WifiManager K ----------------------
  wifiManager.setTimeout(500);
  wifiManager.setBreakAfterConfig(1);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  wifiManager.addParameter(&custom_api_key);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_login);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_channel_in);
  wifiManager.addParameter(&custom_mqtt_channel_out);
  wifiManager.addParameter(&custom_mqtt_channel_cmd_in);
  wifiManager.addParameter(&custom_mqtt_channel_cmd_out);
  //wifiManager.resetSettings(); //Caso seja necessario resetar os valores para Debug
  

//------------------- Caso conecte, copie os dados para o FS ----------------------
  if(!wifiManager.autoConnect("KonkerConfig")) {
    
    //Copiando parametros  
    copyHTMLPar(api_key, mqtt_server, mqtt_port, mqtt_login, mqtt_pass, mqtt_channel_in, mqtt_channel_out, mqtt_channel_cmd_in, mqtt_channel_cmd_out, custom_api_key, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass, custom_mqtt_channel_in, custom_mqtt_channel_out, custom_mqtt_channel_cmd_in, custom_mqtt_channel_cmd_out);
    
    //Salvando Configuracao
    if (shouldSaveConfig) {  
                             saveConfigtoFile(api_key, device_id, mqtt_server, mqtt_port, mqtt_login, mqtt_pass, mqtt_channel_in, mqtt_channel_out, mqtt_channel_cmd_in, mqtt_channel_cmd_out);
                          }
    delay(2500);
    ESP.reset();
    } 

//------------------- Caso tudo mais falhe copie os dados para o FS ----------------------
//Copiando parametros  
   copyHTMLPar(api_key, mqtt_server, mqtt_port, mqtt_login, mqtt_pass, mqtt_channel_in, mqtt_channel_out, mqtt_channel_cmd_in, mqtt_channel_cmd_out, custom_api_key, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass, custom_mqtt_channel_in, custom_mqtt_channel_out, custom_mqtt_channel_cmd_in, custom_mqtt_channel_cmd_out);
 
//Salvando Configuracao
if (shouldSaveConfig) {  
  saveConfigtoFile(api_key,device_id, mqtt_server,mqtt_port,mqtt_login,mqtt_pass,mqtt_channel_in, mqtt_channel_out, mqtt_channel_cmd_in, mqtt_channel_cmd_out); 
  }

//------------------- Configurando MQTT e LED ----------------------
  client.setServer(mqtt_server, atol(mqtt_port));
  client.setCallback(callback);
  delay(200);
  if (!client.connected()) {
    disconnected=1;
    reconnectcount = reconnect(client, api_key, mqtt_login, mqtt_pass);
    if (reconnectcount>10) resetsettings=1;
  }
  client.subscribe(mqtt_topic_in);
  client.subscribe(mqtt_topic_cmd_in);

 

//-----------------------------------------------------------------------------------------
// Caso seja necessário enviar um valor com uma frequencia fixa.
//timer.attach_ms(1000*config_period_I,sendtemperature);

}
  
 // Loop com o programa principal
void loop(){
 delay(100);
 
// Se desconectado, reconectar.
 if (!client.connected()) {
                             disconnected = 1;
                             reconnectcount = reconnect(client, api_key, mqtt_login, mqtt_pass);
                             if (reconnectcount>10) resetsettings=1;
                             else {
                                    client.subscribe(mqtt_topic_in);
                                    client.subscribe(mqtt_topic_cmd_in);
                                  }
                           }
  
  client.loop();
  delay(10);  
  
 

}

