/*
Exemplo básico de conexão a Konker Plataform via MQTT, baseado no https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_auth/mqtt_auth.ino. Este exemplo se utiliza das bibliotecas do ESP8266 programado via Arduino IDE (https://github.com/esp8266/Arduino) e a biblioteca PubSubClient que pode ser obtida em: https://github.com/knolleary/pubsubclient/
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Vamos primeiramente conectar o ESP8266 com a rede Wireless (mude os parâmetros abaixo para sua rede).

const char* ssid = "<SSID>";
const char* password = "<Senha da Rede Wifi>";

//Criando a função de callback
void callback(char* topic, byte* payload, unsigned int length) {
  // Essa função trata das mensagens que são recebidas no tópico no qual o Arduino esta inscrito.
}

//Criando os objetos de conexão com a rede e com o servidor MQTT.
WiFiClient espClient;
PubSubClient client("mqtt.hackathon.konkerlabs.net", 1883, callback,espClient);
ADC_MODE(ADC_VCC); //Essa linha diz para o ESP usar o ADC para ler a tensão VCC. Caso o ADC seja usado para leituras externas, essa linha deve ser comentada ou apagada.
char mensagemC[100];

void setup()
{
  //Conectando na Rede
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
      {
        delay(500);
        Serial.print(".");
      }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    //Configurando a conexão com o servidor MQTT
    if (client.connect("arduinoClient", "<login>", "<senha>")) {

    //Subscrevendo no tópico <tópico para subscrição>
    client.subscribe("<tópico para subscrição>");

    //Agora vamos publicar uma mensagem no <tópico para pubicação>. Nota: a mensagem deve estar no formato JSON.
    String mensagem = "{\"id\":\"02\", \"value\":\"";
    mensagem+= String(ESP.getVcc());
    mensagem+= "\" , \"metric\":\"voltage\", \"unit\":\"milivolts\"}"; 
    mensagem.toCharArray(mensagemC,mensagem.length()+1);
    Serial.println(mensagemC);
    client.publish("<tópico para pubicação>",mensagemC);

  }
}

void loop()
{
  //Vamos manter o cliente ativo para receber mensagens no tópico de subscrição
  client.loop();
}
