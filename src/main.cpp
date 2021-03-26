//FECHADURA SALA TECNICA 
//MILENA FREITAS
#include <Arduino.h>
#include <Keypad.h>
#include <U8x8lib.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WebServer.h>
String senha= "0000"; //senha de entrada
const int fechadura=14; 
const int botaoAbre=27; 
const int buzzer=13;
int estado=0;
const byte linha = 4;
const byte coluna = 4;
String digitada;
String usuarios;
byte pinolinha[linha] = {17, 5, 18, 23};       //Declara os pinos de interpretação das linha
byte pinocoluna[coluna] = {19, 22, 21, 0};      //Declara os pinos de interpretação das coluna
char keys [linha] [coluna]={
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'} };
Keypad keypad = Keypad(makeKeymap(keys), pinolinha, pinocoluna, linha, coluna);
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);

#define WIFI_NOME "Metropole" //rede wifi específica
#define WIFI_SENHA "908070Radio"
#define BROKER_MQTT "10.71.0.2"
#define DEVICE_TYPE "ESP32-TRM"
uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
uint16_t chip = (uint16_t)(chipid >> 32);
char DEVICE_ID[23];
char an = snprintf(DEVICE_ID, 23, "biit%04X%08X", chip, (uint32_t)chipid); // PEGA IP
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);
WiFiUDP udp;
IPAddress ip=WiFi.localIP();  
char topic[]= "fechadura"; // topico MQTT
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000); //Hr do Br
struct tm data; //armazena data 
char data_formatada[64];
char timeStamp; 
/////////////////////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length){
  //retorna infoMQTT
    char msg;
  if (topic) {
    for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    msg += (char)payload[i];
    }
  }
}
void conectaMQTT () {
  //Estabelece conexao c MQTT/WIFI
  while (!client.connected()) {
    Serial.println("Conectando MQTT...");
    if (client.connect("ESP32-TRM")) {
      Serial.println ("Conectado :)");
      //client.publish ("teste", "ola mundo");      
      client.subscribe ("fechadura");
    } else { //reconecta ate ter sucesso
      Serial.println("Falha na conexao");
      Serial.print(client.state());
      Serial.print("nova tentativa em 5s...");
      delay (5000);
    }
  }
}
void reconectaMQTT(){
  //reconecta MQTT caso desconecte
  if (!client.connected()) {
    conectaMQTT();
  }
  client.loop();
}
void hora(){
  time_t tt=time(NULL);
  data= *gmtime(&tt);
  strftime(data_formatada, 64, "%a - %d/%m/%y %H:%M:%S", &data);
  int ano_esp = data.tm_year;
  if (ano_esp<2020){
    ntp.forceUpdate();
  }
}
void pin(){
  pinMode(fechadura, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(botaoAbre, INPUT_PULLUP);
}
void visorInicio(){
  u8x8.clear();
  u8x8.setFont(u8x8_font_inb21_2x4_f);
  u8x8.setCursor(1,4);
  u8x8.print("Digite a Senha ");
}
void estadoSenha (int estado){
// 0=espera 1=aceito 2=negado 
  if (estado==0){ //standy by da porta, esperando ação
    digitalWrite(fechadura, HIGH); //trancada
    Serial.print("digite a senha: ");
    u8x8.setFont(u8x8_font_8x13B_1x2_f);
    u8x8.setCursor(1,4);
    u8x8.print("Digite a Senha ");
  } else if (estado==1){ //abre
    u8x8.clear();
    u8x8.setFont(u8x8_font_8x13B_1x2_f);
    u8x8.setCursor(3,2);
    u8x8.print("Bem Vindo!");
    digitalWrite(buzzer, LOW);
    digitalWrite(fechadura, LOW);
    delay(2000);
    digitalWrite(buzzer, HIGH);
    digitalWrite(fechadura, HIGH);
    
  } else if (estado==2){ //não abre
    digitalWrite(fechadura, HIGH); //TRANCADA
    Serial.print("falha na tentativa");
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);  
    u8x8.setFont(u8x8_font_8x13B_1x2_f);
    u8x8.setCursor(0,4);
    u8x8.clear();
    u8x8.print("Senha incorreta");
  }
}
bool verificaSenha (String sa, String sd){ //funçao chamada para comparar as senhas 
  bool resultado=false;
  if(sa.compareTo(sd)==0){//se a comparação for 0 é porque são iguais
    resultado=true; //se senha correta função verificaSenha fica verdadeira
  } else {
    resultado=false;
  }
  return resultado;
}
void payload (){
  time_t tt=time(NULL);
  String payload = "{\"local\":";
  payload += "\"PortaTransmisssor\"";
  payload += ",";
  payload += "\"hora\":";
  payload += tt;
  payload += ",";
  payload += "\"botao\":";
  payload += digitalRead(botaoAbre);
  payload += ",";
  payload += "\"fechadura\":";
  payload += fechadura;
  payload += ",";
  payload += "\"ip\":";
  payload +="\"";
  payload += ip.toString();
  payload +="\"";
  payload += ",";
  payload += "\"mac\":";
  payload +="\"";
  payload += DEVICE_ID;
  payload +="\"";
  payload += "}";
}
void setup(){
  Serial.begin(115200);
  visorInicio();
  // WiFi.begin(WIFI_NOME, WIFI_SENHA);
  // while(WiFi.status()!= WL_CONNECTED){
  //   Serial.println("conectando...");
  //   delay(500);
  // }
  // ntp.begin();
  // ntp.forceUpdate();
  // if (!ntp.forceUpdate()){
  //   while(1){
  //     Serial.print("Erro ao carregar a hora!");
  //     delay(1500);
  //   }
  // } else {
  //   timeval tv;
  //   tv.tv_sec=ntp.getEpochTime();
  //   settimeofday(&tv, NULL);
  // }
  // client.setServer (BROKER_MQTT, 1883);//define mqtt
  // client.setCallback(callback); 
  pin();
  u8x8.begin();
  u8x8.clear();
}
void loop(){
  // server.handleClient();
  // reconectaMQTT();
  if(digitalRead(botaoAbre) == HIGH){ //se apertar o botao abre a porta 
    digitalWrite(fechadura, LOW);
    delay(2000);
    digitalWrite(fechadura, HIGH);
    estado=0; //retorna para standy by
  }
  char key = keypad.getKey(); //le as teclas
  if (key !=NO_KEY){ //se digitar...
    digitalWrite (buzzer, LOW);
    delay(50);
    digitalWrite(buzzer, HIGH); 
    if (key=='A'){ //campainha
    digitalWrite (buzzer, LOW);
    delay(1500);
    digitalWrite(buzzer, HIGH); 
    digitada="";
    Serial.println("campainha");
  } else if (key=='C'){ //limpa a tela
    //limpa senha
    digitada="";
  } else if(key=='#'){ //depois do enter vai verificar a senha 
      if(verificaSenha(senha, digitada)){
        estado=1; //senha certa
        estadoSenha(estado);
        delay(2000);
        estado=0;
      } else {
        estado=2;
        estadoSenha(estado);
        delay(2000);
        estado=0;
      }
      digitada=""; //limpa o que foi digitado
  } else {
    digitada+=key;  //concatenar as info que são digitadas
  }
    estadoSenha(estado);
  }
  //  else {
  //   estado=0;
  // }
  // payload();
}
