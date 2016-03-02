// Essa eh uma primeira versao do arquivo contendo as funcoes utilizadas para a comunicacao de dispositivos via MQTT na plataforma
// Konker. O proximo passo serah a tranformacao desse arquivo de funcoes para classes em C++.
// A adocao original dessa estrutura simples em C se deu pela velocidade de prototipacao e facil transformacao para uma estrutura
// de classes em um momento de maior especificidade do projeto.
//
// Responsavel: Luis Fernando Gomez Gonzalez (luis.gonzalez@inmetrics.com.br)
// Projeto Konker


// Vamos precisar de uma variavel Global para todas as funcoes, um enable para rodar todas as maquinas de estado.
int enable=1;

// Trigger para mensagens recebidas
int received_msg=0;

//Trigger da configuracao via Json -- Tambem precisa ser uma variavel global para rodar em duas maquinas de estado.
bool shouldSaveConfig = false;

//Buffer das mensagens MQTT
char bufferJ[256];

//Buffer de entrada MQTT;
char msgBufferIN[256];

char device_id[17]; 
char device_type[5];
char in_topic[32];
char cmd_topic[32];
char data_topic[32];
char ack_topic[32];

char dev_modifier[4] = "iot";
char cmd_channel[8] = "command";
char data_channel[5] = "data";
char ack_channel[4] = "ack";


Ticker t;
  
//----------------- Funcao de Trigger para salvar configuracao no FS ---------------------------
void saveConfigCallback() {
  Serial.println("Salvar a Configuracao");
  shouldSaveConfig = true;
}

//----------------- Copiando parametros configurados via HTML ---------------------------
void copyHTMLPar(char api_key[], char mqtt_server[], char mqtt_port[], char mqtt_login[], char mqtt_pass[], WiFiManagerParameter custom_api_key, WiFiManagerParameter custom_mqtt_server, WiFiManagerParameter custom_mqtt_port, WiFiManagerParameter custom_mqtt_login, WiFiManagerParameter custom_mqtt_pass){

  strcpy(api_key, custom_api_key.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_login, custom_mqtt_login.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());

}

//----------------- Montando o sistema de arquivo e lendo o arquivo config.json ---------------------------
void spiffsMount(char mqtt_server[], char mqtt_port[], char mqtt_login[], char mqtt_pass[], char device_id[], char device_type[], char api_key[], char in_topic[], char cmd_topic[], char data_topic[], char ack_topic[]) {

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

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_login, json["mqtt_login"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
          strcpy(device_id, json["device_id"]);
          strcpy(device_type, json["device_type"]);
          strcpy(api_key, json["api_key"]);
          strcpy(in_topic, json["in_topic"]);
          strcpy(cmd_topic, json["cmd_topic"]);
          strcpy(data_topic, json["data_topic"]);
          strcpy(ack_topic, json["ack_topic"]);
          

        } else {
          Serial.println("Falha em ler o Arquivo");
        }
      }
    }
  } else {
    Serial.println("Falha ao montar o sistema de arquivos");
  }
}

//----------------- Salvando arquivo de configuracao ---------------------------
void saveConfigtoFile(char api_key[], char mqtt_server[],char mqtt_port[],char mqtt_login[],char mqtt_pass[])
{
  String SString;
  SString = String(dev_modifier) + String("/") + String(api_key) + String("/") + String(cmd_channel);
  SString.toCharArray(cmd_topic, SString.length()+1);
  SString = String(dev_modifier) + String("/") + String(api_key) + String("/") + String(data_channel);
  SString.toCharArray(data_topic, SString.length()+1);
  SString = String(dev_modifier) + String("/") + String(api_key) + String("/") + String(ack_channel);
  SString.toCharArray(ack_topic, SString.length()+1);
        
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
  json["in_topic"] = in_topic;
  json["cmd_topic"] = cmd_topic;
  json["data_topic"] = data_topic;
  json["ack_topic"] = ack_topic;
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Falha em abrir o arquivo com permissao de gravacao");
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
}

//----------------- Criacao da Mensagem Json --------------------------------
char *jsonMQTTmsgDATA(const char *ts, const char *metric, int value, const char *unit)
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

char *jsonMQTTmsgCMD(const char *ts, const char *cmd)
  {
      StaticJsonBuffer<200> jsonMQTT;
      JsonObject& jsonMSG = jsonMQTT.createObject();
        jsonMSG["deviceId"] = device_id;
        jsonMSG["command"] = cmd;
        jsonMSG["ts"] = ts;
        jsonMSG.printTo(bufferJ, sizeof(bufferJ));
        return bufferJ;
  }

//----------------- Decodificacao da mensagem Json --------------------------------
char *jsonMQTT_in_msg(char* msg)
  {
  const char *device_id;
  const char *cmd;
  const char *ts;
  char *command;
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jsonMSG = jsonBuffer.parseObject(msg);
  device_id = jsonMSG["deviceId"];
  cmd = jsonMSG["command"];
  ts = jsonMSG["ts"];
  command=strdup(cmd);
  return command;
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
void reconnect(PubSubClient client, char id[], const char *mqtt_login, const char *mqtt_pass) {
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
    }
  }
}
