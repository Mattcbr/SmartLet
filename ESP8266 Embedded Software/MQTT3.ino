#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient

//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_SUBSCRIBE "MQTTSendTopic"     //tópico MQTT de escuta
#define TOPICO_PUBLISH   "MQTTReadTopic"    //tópico MQTT de envio de informações para Broker

#define ID_MQTT  "MQTTID"     //id mqtt (para identificação de sessão)

//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1


// WIFI
const char* SSID = "SEU_SSID"; 	 // SSID / nome da rede WI-FI que 
 //deseja se conectar
const char* PASSWORD = "SUA_SENHA"; // Senha da rede WI-FI que deseja 
//se conectar
 
// MQTT
const char* BROKER_MQTT = "iot.eclipse.org"; //URL do broker MQTT que 
    //se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT


//Variáveis e objetos globais
WiFiClient espClient; 		 // Cria o objeto espClient
PubSubClient MQTT(espClient);  // Instancia o Cliente MQTT passando o 
 //objeto espClient
char EstadoSaida = '0';  //variável que armazena o estado atual da saída
 
//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
float getVPP();

//Variaveis para medição da energia
const int sensorIn = A0;
int mVperAmp = 66; // Sensibilidade do ACS712 de 30A
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

/* 
 *  Implementações das funções
 */
void setup() {
    //inicializações:
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    ESP.wdtDisable();
    ESP.wdtEnable(WDTO_8S);
}
 
void initSerial() {
    Serial.begin(115200);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    
    reconectWiFi();
}
 
//Função: inicializa parâmetros de conexão MQTT
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);  //informa qual broker e 
//porta deve ser 
//conectado

    MQTT.setCallback(mqtt_callback);           //atribui função de 
//callback
}
 
//Função: função de callback 
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;

    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) {
       char c = (char)payload[i];
       msg += c;
    }
  
    //toma ação dependendo da string recebida:
    if (msg.equals("L")){
        digitalWrite(D3, LOW);
        EstadoSaida = '1';
    }
	
    if (msg.equals("D")){
        digitalWrite(D3, HIGH);
        EstadoSaida = '0';
    }
}
 
//Função: reconecta-se ao broker MQTT
void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
 
//Função: reconecta-se ao WiFi
void reconectWiFi() {
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
  
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}



//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
void VerificaConexoesWiFIEMQTT(void){
    if (!MQTT.connected()) 
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é 
   //refeita
    
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

//Função: envia ao Broker o estado atual do output 
void EnviaEstadoOutputMQTT(void){
    //Novo Código
    char mensagem[8];
    int i;
    
    if (EstadoSaida == '0')
      mensagem[0] = 'L';

    if (EstadoSaida == '1')
      mensagem[0] = 'D';

    //Envio da Corrente
    float Voltage;
    Voltage = getVPP();
    (Voltage<0.4)? mensagem[1]='N':mensagem[1]='S';
    char voltchar[5];
    sprintf (voltchar, "%d.%02d A", (int)Voltage, (int)(Voltage*100)%100);
    
    Serial.println("Voltchar:");
    Serial.println(voltchar);

    for(i=2; i<9; i++){
      mensagem[i] = voltchar[i-2];    
    }

    Serial.print("Mensagem: ");
    Serial.print(mensagem);

    //Posso enviar várias coisas em um payload só e processar pelo site
    MQTT.publish(TOPICO_SUBSCRIBE, mensagem, 9); 

    Serial.println("- Estado da saida D3 e voltagem enviado ao broker!");
    delay(1000);
}

//Loop principal
void loop() {   
    //garante funcionamento das conexões WiFi e MQTT
    VerificaConexoesWiFIEMQTT();

    //envia o status de todos os outputs para o Broker no protocolo
    EnviaEstadoOutputMQTT();

    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
}

float getVPP(){
  float result;

  int readValue;             //valor lido no sensor
  int maxValue = 0;          // Valor maximo
  int minValue = 1024;       // Valor minimo

  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000){ //Captar valores durante 1 segundo
    readValue = analogRead(sensorIn);
	
    // Compara o valor atual com o maxValue
    if (readValue > maxValue){
      //Salva o valor máximo do sensor
      maxValue = readValue;
    }
	
	// Compara o valor atual com o minValue
    if (readValue < minValue){
      //Salva o valor minimo do sensor
      minValue = readValue;
    }
  }

  // Subtrai min do max, multiplica pela tensão e divide pela resolução do ACS712
  result = ((maxValue - minValue) * 5.0) / 1024.0;
  
  VRMS = (result / 2.0) * 0.707;       	//Equação da tensão RMS
  AmpsRMS = (VRMS * 1000) / mVperAmp;   //Equação da corrente RMs
  return AmpsRMS;
}
