// Essa eh uma primeira versao do arquivo contendo as funcoes utilizadas para a comunicacao de dispositivos via MQTT na plataforma
// Konker. O proximo passo serah a tranformacao desse arquivo de funcoes para classes em C++.
// A adocao original dessa estrutura simples em C se deu pela velocidade de prototipacao e facil transformacao para uma estrutura
// de classes em um momento de maior especificidade do projeto.
//
// Responsavel: Luis Fernando Gomez Gonzalez (luis@konkerlabs.com)



// Vamos precisar de uma variavel Global para todas as funcoes, um enable para rodar todas as maquinas de estado.
int enable=1;

// Vamos precisar de uma variavel Global para todas voltar os settings para o valor inicial.
int resetsettings=0;

// Vamos precisar de uma variavel Global para avisar que uma configuracao foi feita.
int configured=0;

int senddata=0;

// Trigger para mensagens recebidas
int received_msg=0;

//Trigger da configuracao via Json -- Tambem precisa ser uma variavel global para rodar em duas maquinas de estado.
bool shouldSaveConfig = false;

//Buffer das mensagens MQTT
char bufferJ[256];

//Buffer de entrada MQTT;
char msgBufferIN[2048];
char msgTopic[32];

char device_type[5];
char mqtt_channel_in[32];
char mqtt_channel_out[32];
char mqtt_channel_cmd_in[32];
char mqtt_channel_cmd_out[32];
char ack_topic[32];
char connect_topic[32];
char config_period[5];
int config_period_I;

char mqtt_topic_in[32];
char mqtt_topic_out[32];
char mqtt_topic_cmd_in[32];
char mqtt_topic_cmd_out[32];

const char in_modifier[4] = "pub";
const char out_modifier[4] = "sub";
const char ack_channel[4] = "ack";
const char config_channel[7] = "config";
const char connect_channel[8] = "connect";

//Opcoes de commandos
const char *IR_type = "IR";
const char *Button_type1 = "ButtonPressed";
const char *Button_type2 = "ButtonPressedLong";
const char *Hue_type = "Hue";
const char *LED_ON = "LED ON";
const char *LED_OFF = "LED OFF";
const char *LED_SWITCH = "LED SWITCH";


//IR Variables

int long irdata[512];
int commaIndex[512];

//LED
int LEDState=0;

Ticker t;
  
//----------------- Funcao de Trigger para salvar configuracao no FS ---------------------------
void saveConfigCallback() {
  Serial.println("Salvar a Configuracao");
  shouldSaveConfig = true;
}

//----------------- Copiando parametros configurados via HTML ---------------------------
void copyHTMLPar(char api_key[], char mqtt_server[], char mqtt_port[], char mqtt_login[], char mqtt_pass[], char mqtt_channel_in[], char mqtt_channel_out[], char mqtt_channel_cmd_in[], char mqtt_channel_cmd_out[], WiFiManagerParameter custom_api_key, WiFiManagerParameter custom_mqtt_server, WiFiManagerParameter custom_mqtt_port, WiFiManagerParameter custom_mqtt_login, WiFiManagerParameter custom_mqtt_pass, WiFiManagerParameter custom_mqtt_channel_in, WiFiManagerParameter custom_mqtt_channel_out, WiFiManagerParameter custom_mqtt_channel_cmd_in, WiFiManagerParameter custom_mqtt_channel_cmd_out){

  strcpy(api_key, custom_api_key.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_login, custom_mqtt_login.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_channel_in, custom_mqtt_channel_in.getValue());
  strcpy(mqtt_channel_out, custom_mqtt_channel_out.getValue());
  strcpy(mqtt_channel_cmd_in, custom_mqtt_channel_cmd_in.getValue());
  strcpy(mqtt_channel_cmd_out, custom_mqtt_channel_cmd_out.getValue());
}

//----------------- Montando o sistema de arquivo e lendo o arquivo config.json ---------------------------
void spiffsMount(char mqtt_server[], char mqtt_port[], char mqtt_login[], char mqtt_pass[], char device_id[], char device_type[], char api_key[], char mqtt_channel_in[], char mqtt_channel_out[], char mqtt_channel_cmd_in[], char mqtt_channel_cmd_out[], char ack_topic[], char connect_topic[], char config_period[]) {

String config_periodS;

  if (SPIFFS.begin()) {
    Serial.println("Sistema de Arquivos Montado");
    if (SPIFFS.exists("/config.json")) {
      Serial.println("Arquivo já existente, lendo..");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Arquivo aberto com sucesso");
        size_t size = configFile.size();
        // Criando um Buffer para alocar os dados do arquivo.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          if (json.containsKey("mqtt_server")) strcpy(mqtt_server, json["mqtt_server"]);
          if (json.containsKey("mqtt_port")) strcpy(mqtt_port, json["mqtt_port"]);
          if (json.containsKey("mqtt_login")) strcpy(mqtt_login, json["mqtt_login"]);
          if (json.containsKey("mqtt_pass")) strcpy(mqtt_pass, json["mqtt_pass"]);
          if (json.containsKey("device_id")) strcpy(device_id, json["device_id"]);
          if (json.containsKey("device_type")) strcpy(device_type, json["device_type"]);
          if (json.containsKey("api_key")) strcpy(api_key, json["api_key"]);
          if (json.containsKey("mqtt_channel_in")) strcpy(mqtt_channel_in, json["mqtt_channel_in"]);
          if (json.containsKey("mqtt_channel_out")) strcpy(mqtt_channel_out, json["mqtt_channel_out"]);
          if (json.containsKey("mqtt_channel_cmd_in")) strcpy(mqtt_channel_cmd_in, json["mqtt_channel_cmd_in"]);
          if (json.containsKey("mqtt_channel_cmd_out")) strcpy(mqtt_channel_cmd_out, json["mqtt_channel_cmd_out"]);
          if (json.containsKey("config_period")) strcpy(config_period, json["config_period"]); 

        } 
          else {
          Serial.println("Falha em ler o Arquivo");
        }
      }
    }
  } else {
    Serial.println("Falha ao montar o sistema de arquivos");
  }
  config_periodS = config_period;
  config_period_I = config_periodS.toInt();

    String SString;
  SString = String(in_modifier) + String("/") + String(api_key) + String("/") + String(mqtt_channel_in);
  SString.toCharArray(mqtt_topic_in, SString.length()+1);
  
  SString = String(out_modifier) + String("/") + String(api_key) + String("/") + String(mqtt_channel_out);
  SString.toCharArray(mqtt_topic_out, SString.length()+1);
    
  SString = String(in_modifier) + String("/") + String(api_key) + String("/") + String(mqtt_channel_cmd_in);
  SString.toCharArray(mqtt_topic_cmd_in, SString.length()+1);
  
  SString = String(out_modifier) + String("/") + String(api_key) + String("/") + String(mqtt_channel_cmd_out);
  SString.toCharArray(mqtt_topic_cmd_out, SString.length()+1);
  

}

//----------------- Salvando arquivo de configuracao ---------------------------
void saveConfigtoFile(char api_key[], char device_id[], char mqtt_server[],char mqtt_port[],char mqtt_login[],char mqtt_pass[], char mqtt_channel_in[], char mqtt_channel_out[], char mqtt_channel_cmd_in[], char mqtt_channel_cmd_out[])
{

  Serial.println("Salvando Configuracao");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_login"] = mqtt_login;
  json["mqtt_pass"] = mqtt_pass;
  json["device_id"] = device_id;
  json["device_type"] = device_type;
  json["api_key"] = api_key;
  json["mqtt_channel_in"] = mqtt_channel_in;
  json["mqtt_channel_out"] = mqtt_channel_out;
  json["mqtt_channel_cmd_in"] = mqtt_channel_cmd_in;
  json["mqtt_channel_cmd_out"] = mqtt_channel_cmd_out;
  json["config_period"] = config_period;
  
File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Falha em abrir o arquivo com permissao de gravacao");
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
}

//----------------- Configuracao --------------------------------
void configuration(char* configs, int value)
{
  String config_periodS;
  const char *rst = "reset";
  const char *prd = "period";
  char api_key[17];
  char device_id[17];
  char mqtt_server[64];
  char mqtt_port[5];
  char mqtt_login[32];
  char mqtt_pass[32];
   
  if (String(configs) == String(rst)) 
                                      {
                                        resetsettings=1;
                                      }
                                      
  else if (String(configs) == String(prd))
                                          {
                                            //spiffsMount(mqtt_server, mqtt_port, mqtt_login, mqtt_pass, device_id, device_type, api_key, in_topic, cmd_topic, data_topic, ack_topic, config_topic, connect_topic, config_period);  
                                            config_period_I = value;
                                            config_periodS = value;
                                            config_periodS.toCharArray(config_period,5);          
                                            delay(500);
                                            saveConfigtoFile(api_key, device_id, mqtt_server, mqtt_port, mqtt_login, mqtt_pass,mqtt_channel_in, mqtt_channel_out, mqtt_channel_cmd_in, mqtt_channel_cmd_out);
                                          }
}

//----------------------------------------------------------------

//----------------- Criacao da Mensagem Json --------------------------------
char *jsonMQTTmsgDATA(const char *ts, const char *device_id, const char *metric, int value, const char *unit)
  {
      StaticJsonBuffer<200> jsonMQTT;
      JsonObject& jsonMSG = jsonMQTT.createObject();
        jsonMSG["deviceId"] = device_id;
        jsonMSG["metric"] = metric;
        jsonMSG["value"] = value;
        jsonMSG["unit"] = unit;
        jsonMSG["ts"] = ts;
        jsonMSG.printTo(bufferJ, sizeof(bufferJ));
        return bufferJ;
  }

char *jsonMQTTmsgCMD(const char *ts, const char *device_id, const char *cmd)
  {
      StaticJsonBuffer<200> jsonMQTT;
      JsonObject& jsonMSG = jsonMQTT.createObject();
        jsonMSG["deviceId"] = device_id;
        jsonMSG["command"] = cmd;
        jsonMSG["ts"] = ts;
        jsonMSG.printTo(bufferJ, sizeof(bufferJ));
        return bufferJ;
  }

char *jsonMQTTmsgCONNECT(const char *ts, const char *device_id, const char *connect_msg, const char *metric, int value, const char *unit)
  {
      StaticJsonBuffer<200> jsonMQTT;
      JsonObject& jsonMSG = jsonMQTT.createObject();
        jsonMSG["deviceId"] = device_id;
        jsonMSG["connectmsg"] = connect_msg;
        jsonMSG["metric"] = metric;
        jsonMSG["value"] = value;
        jsonMSG["unit"] = unit;
        jsonMSG["ts"] = ts;
        jsonMSG.printTo(bufferJ, sizeof(bufferJ));
        return bufferJ;
  }  
//----------------- Decodificacao da mensagem Json In -----------------------------
char *jsonMQTT_in_msg(const char msg[])
  {
  const char *jdevice_id;
  const char *cmd = "null";
  const char *ts;

  StaticJsonBuffer<2048> jsonBuffer;
  JsonObject& jsonMSG = jsonBuffer.parseObject(msg);
  if (jsonMSG.containsKey("deviceId")) jdevice_id = jsonMSG["deviceId"];
  if (jsonMSG.containsKey("command")) cmd = jsonMSG["command"];
  if (jsonMSG.containsKey("ts")) ts = jsonMSG["ts"];
  char *command = (char*)cmd;
  return command;
  }
  
//----------------- Decodificacao dos dados da mensagem Json In --------------------
char *jsonMQTT_in_data_msg(const char msg[])
  {
  const char *jdevice_id;
  const char *cmd;
  const char *ts;
  const char *dt;
  
  StaticJsonBuffer<2048> jsonBuffer;
  JsonObject& jsonMSG = jsonBuffer.parseObject(msg);
  if (jsonMSG.containsKey("deviceId")) jdevice_id = jsonMSG["deviceId"];
  if (jsonMSG.containsKey("command")) cmd = jsonMSG["command"];
  if (jsonMSG.containsKey("value")) dt = jsonMSG["value"];
  if (jsonMSG.containsKey("ts")) ts = jsonMSG["ts"];
  char *data = (char*)dt;
  return data;
  }
  

//----------------- Decodificacao da mensagem Json CFG -----------------------------
void *jsonMQTT_config_msg(const char msg[])
  {
  int value = 0;
  const char *configs;
  char *conf;
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& jsonMSG = jsonBuffer.parseObject(msg);
  if (jsonMSG.containsKey("config")) 
                                    {
                                      configs = jsonMSG["config"];
                                      conf=strdup(configs);
                                      if (jsonMSG.containsKey("value")) value = jsonMSG["value"];
                                      configuration(conf, value);
                                      configured=1;
                                    }
  }

//---------------------------------------------------------------------------


//----------------- Feedback para o ACK (no momento eh um placeholder: soh escreve via serial)--------------------------------
void simplef(int charcount) {
  enable=1;
  //int charcount=0;
  if (charcount>18) {
                   //sucess = 1;
                   Serial.println("Mensagem recebida pelo device B com sucesso");
                  }
  else            {
                   //sucess = 0;
                   Serial.println("Mensagem nao entregue");
                  }
}

//----------------- Publica a mensagem e aguarda o ACK--------------------------------
int messageACK(PubSubClient client, char topic[], char message[],char ack[], int charcount){
  
  client.publish(topic, message);
  Serial.println("Mensagem enviada!");
  enable=0;
  t.once_ms(3000,simplef,charcount);
  //name_ok=0;
  return 1;
}

//----------------- Funcao para conectar ao broker MQTT e reconectar quando perder a conexao --------------------------------
int reconnect(PubSubClient client, char id[], const char *mqtt_login, const char *mqtt_pass) {
  int i=0;
  // Loop ate estar conectado
  while (!client.connected()) {
    Serial.print("Tentando conectar no Broker MQTT...");
    // Tentando conectar no broker MQTT (PS.: Nao usar dois clientes com o mesmo nome. Causa desconexao no Mosquitto)
    if (client.connect(id, mqtt_login, mqtt_pass)) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println("Tentando conectar novamente em 3 segundos");
      // Esperando 3 segundos antes de re-conectar
      delay(3000);
      if (i==19) break;
      i++;
    }
  }
  return i;
}


//----------------- Funcao para enviar a temperatura --------------------------------

void sendtemperature()
{
senddata=1;
}

//----------------- Funcao para testar as mensagens de entrada ----------------------

char *testCMD(const char msgBufferIN[])
{
  char *cmd;
  String msgString ="null";
  String type;
  char typeC[32];
  int commaIndex;
  
  cmd=jsonMQTT_in_msg(msgBufferIN);
  msgString = String(cmd);
  commaIndex = msgString.indexOf('-');
  type = msgString.substring(0, commaIndex);
  if (commaIndex == -1) 
          {
            type = msgString.substring(0, msgString.length());
          }
  type.toCharArray(typeC,type.length()+1);
  typeC[type.length()+1] = '\0';

  return typeC;
    
}
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
void LEDSwitch(int LED1, int LED2)
{       
   //LEDState = digitalRead(LED1);
   Serial.print("Digital State: ");
   Serial.println(LEDState);
   if (LEDState)
   {
   analogWrite(LED1, 0);
   analogWrite(LED2, 0); 
   }
   else 
   {
   analogWrite(LED1, 1024);
   analogWrite(LED2, 1024);
   }
   LEDState=!LEDState;
 }

void LEDOn(int LED1, int LED2, int pwm)
{
   
   analogWrite(LED1, pwm);
   analogWrite(LED2, pwm); 
   LEDState=1;
}

void LEDOff(int LED1, int LED2)
{
   analogWrite(LED1, 0);
   analogWrite(LED2, 0);
   LEDState=0;
}

//----------------------------- Philips Hue -------------------------------------------

boolean setHue(WiFiClient client, int lightNum,String command)
{
  const char hueHubIP[] = "192.168.0.31";  // Hue hub IP -- https://www.meethue.com/api/nupnp
  const char hueUsername[] = "newdeveloper";  // Hue username
  const int hueHubPort = 80;
  
  if (client.connect(hueHubIP, hueHubPort))
  {
    while (client.connected())
    {
      client.print("PUT /api/");
      client.print(hueUsername);
      client.print("/lights/");
      client.print(lightNum);  
      client.println("/state HTTP/1.1");
      client.println("keep-alive");
      client.print("Host: ");
      client.println(hueHubIP);
      client.print("Content-Length: ");
      client.println(command.length());
      client.println("Content-Type: text/plain;charset=UTF-8");
      client.println();  // blank line before body
      client.println(command);  // Hue command
    }
    client.stop();
    return true;  // command executed
  }
  else
    return false;  // command failed
}

//----------------------------- Philips Hue -------------------------------------------

void ICACHE_FLASH_ATTR sendHue(WiFiClient client, const char msgBufferIN[]) //ICACHE_FLASH_ATTR 
{
  bool _on = true;
  bool _off = false;
  const char *jdevice_id;
  const char *cmd;
  const char *ts;
  const char *dt;
  const char *Hue_on;
  const char *Hue_bri;
  const char *Hue_hue;
  const char *Hue_sat;
  
  StaticJsonBuffer<2048> jsonBuffer;
  JsonObject& jsonMSG = jsonBuffer.parseObject(msgBufferIN);
  if (jsonMSG.containsKey("deviceId")) jdevice_id = jsonMSG["deviceId"];
  if (jsonMSG.containsKey("command")) cmd = jsonMSG["command"];
  if (jsonMSG.containsKey("on")) Hue_on = jsonMSG["on"];
  else Hue_on = "false";
  if (jsonMSG.containsKey("bri")) Hue_bri = jsonMSG["bri"];
  else Hue_bri = "0";
  if (jsonMSG.containsKey("hue")) Hue_hue = jsonMSG["hue"];
  else Hue_hue = "0";
  if (jsonMSG.containsKey("sat")) Hue_sat = jsonMSG["sat"];
  else Hue_sat = "0";
  if (jsonMSG.containsKey("ts")) ts = jsonMSG["ts"];

  StaticJsonBuffer<200> jsonMQTT;
  JsonObject& jsonMSG2 = jsonMQTT.createObject();
  if (String(Hue_on)==String("true")) jsonMSG2["on"] = _on;
  else jsonMSG2["on"] = _off;
  jsonMSG2["hue"] = atol(Hue_hue);
  jsonMSG2["sat"] = atol (Hue_sat);
  jsonMSG2["bri"] = atol (Hue_bri);
  jsonMSG2.printTo(bufferJ, sizeof(bufferJ));

  const char hueHubIP[] = "10.10.4.31";  // Hue hub IP -- https://www.meethue.com/api/nupnp
  const char hueUsername[] = "newdeveloper";  
  const int hueHubPort = 80;
  const int lightNum = 2;

  String command = String(bufferJ);
  //String command = "{\"on\": true,\"hue\": 65280,\"sat\":255,\"bri\":255,\"transitiontime\":0}"; 
  //Serial.print("Comando: ");
  //Serial.println(command);
  String ShueUsername = String(hueUsername);
  String ShueHubIP = String(hueHubIP);
  String SlightNum = String(lightNum);
  String ShueHubPort = String(hueHubPort);

  
  
  if (client.connect(hueHubIP, hueHubPort))
  {
    delay(50);
    Serial.println("Conectado!!");
    Serial.println("Pronto para enviar!! \n Heap: ");
    Serial.println(ESP.getFreeHeap());
      String allMSG = "PUT /api/";
      allMSG += ShueUsername;
      allMSG += "/lights/";
      allMSG += SlightNum;
      allMSG += "/state";
      allMSG += "\n";
      //allMSG += "keep-alive";
      //allMSG += "\n";
      allMSG += "Host: ";
      allMSG += String(hueHubIP);
      allMSG += ":";
      allMSG += String(hueHubPort);
      allMSG += "\n";
      allMSG += "User-Agent: ESP8266/1.0";
      allMSG += "\n";
      allMSG += "Connection: close";
      allMSG += "\n";
      allMSG += "Content-type: text/xml; charset=\"utf-8\"";
      allMSG += "\n";
      allMSG += "Content-Length: ";
      allMSG += String(command.length());
      allMSG += "\n";
      allMSG += "\n";
      allMSG += command;

    //if (client.connected())
    {
      //Serial.println(allMSG);
      
//      client.println("PUT /api/" + ShueUsername + "/lights/" + SlightNum + "/state");
//      client.println("Host: " + String(hueHubIP) + ":" + String(hueHubPort));
//      client.println("User-Agent: ESP8266/1.0");
//      client.println("Connection: close");
//      client.println("Content-type: text/xml; charset=\"utf-8\"");
//      client.print("Content-Length: ");
//      client.println(command.length()); 
//      client.println();
//      client.println(command); 
      client.println(allMSG);
      Serial.println("Enviado!!");
      delay(50);
    }
    client.stop();
//    while(client.available()){
//                              String line = cliente.readStringUntil('\r');
//                              Serial.print(line);   
//                              }
  }
}
