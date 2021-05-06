//FECHADURA SALA TECNICA COM SENHA
//MILENA FREITAS 2021
#include <Arduino.h>
#include <Keypad.h>
#include <U8x8lib.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

const int tamanho_array=4;
const int STARTING_EEPROM_ADDRESS = 10; //primeiro endereço 
int senhas[]= {6950, 7845, 6569, 1984};
String usuario[] ={"MAURICIO", "MILENA", "MEIRA", "LEONARDO"};
String novasSenhas[tamanho_array];
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

#define EEPROM_SIZE 1024
#define WIFI_NOME "Metropole" //rede wifi específica
#define WIFI_SENHA "908070Radio"
#define BROKER_MQTT "10.71.0.2"
#define DEVICE_TYPE "ESP32"
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
bool acerteiSenha=false;
/////////////////////////////////////////////////////////////////
/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";
/* Login page */
String loginIndex =
"<form name=loginForm>"
"<h1>ESP32 Login</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='servidor908070')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
/* Server Index Page */
String serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;
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
  while(!client.connected()){
    Serial.println("conectando...");
    if (client.connect("ESP32")){
      Serial.println("CONECTADO! :)");
      client.publish ("teste", "hello word");
      client.subscribe ("fechadura");         
    } else {
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
void escreveEEPROM(int endereco, int senhas[], unsigned int length) {
  int adressIndex=endereco;
  for (int i=0; i<5; i++){
    EEPROM.write(adressIndex, senhas[i]>>8);
    EEPROM.write(adressIndex+1, senhas[i] & 0xFF);// escreve o byte menos significativo
    adressIndex+=2;
  }
}
void leEEPROM (int endereco, String senhas[], unsigned int length){
  int adressIndex=endereco;
  for (int i=0; i<5; i++){
    senhas[i] = (EEPROM.read(adressIndex)<<8) + EEPROM.read(adressIndex+1);
    adressIndex+=2;
  }
}
void pin(){
  pinMode(fechadura, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(botaoAbre, INPUT_PULLUP);
}
void visorInicio(){
  u8x8.clear();
  u8x8.setFont(u8x8_font_8x13B_1x2_f);
  u8x8.setCursor(1,4);
  u8x8.print("digite a senha: ");
}
void estadoSenha (int estado){
// 0=espera 1=aceito 2=negado 
  if (estado==0){ //standy by da porta, esperando ação
    digitalWrite(buzzer, HIGH);
    digitalWrite(fechadura, HIGH); //trancada
    u8x8.clear();
    u8x8.setFont(u8x8_font_8x13B_1x2_f);
    u8x8.setCursor(1,4);
    u8x8.print("digite a senha: ");
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
    u8x8.clear(); 
    u8x8.setFont(u8x8_font_8x13B_1x2_f);
    u8x8.setCursor(0,4);
    u8x8.print("Senha incorreta");
    digitalWrite(fechadura, HIGH); //TRANCADA
    Serial.print("falha na tentativa");
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(500);
    digitalWrite(buzzer, HIGH); 
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
  client.publish (topic, (char*) payload.c_str());
}
void setup(){
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  escreveEEPROM(STARTING_EEPROM_ADDRESS, senhas, tamanho_array);
  leEEPROM(STARTING_EEPROM_ADDRESS, novasSenhas, tamanho_array);
  WiFi.begin(WIFI_NOME, WIFI_SENHA);
  while(WiFi.status()!= WL_CONNECTED){
    Serial.println("conectando...");
    delay(500);
  }
  ntp.begin();
  ntp.forceUpdate();
  if (!ntp.forceUpdate()){
    while(1){
      Serial.print("Erro ao carregar a hora!");
      delay(1500);
    }
  } else {
    timeval tv;
    tv.tv_sec=ntp.getEpochTime();
    settimeofday(&tv, NULL);
  }
  client.setServer (BROKER_MQTT, 1883);//define mqtt
  client.setCallback(callback); 
  pin();
  u8x8.begin();
  u8x8.setFont(u8x8_font_8x13B_1x2_f);
  u8x8.setCursor(1,4);
  u8x8.print("digite a senha: ");
  Serial.print("digite a senha");
  estado=0;//inicializa com porta travada
  estadoSenha(estado);
  payload();
}
void loop(){
  Serial.println(ip.toString());
  server.handleClient();
  reconectaMQTT();
  if(digitalRead(botaoAbre)==HIGH){ //se apertar o botao abre a porta 
    digitalWrite(fechadura, LOW);
    delay(2000);
    digitalWrite(fechadura, HIGH);
    estado=0; //retorna para standy by
    estadoSenha(estado);
  }
  char key = keypad.getKey(); //le as teclas
  if (key!=NO_KEY){ //se digitar...
    digitalWrite (buzzer, LOW);
    delay(50);
    digitalWrite(buzzer, HIGH); 
    if (key=='A'){ //campainha
    digitalWrite (buzzer, LOW);
    delay(1500);
    digitalWrite(buzzer, HIGH);
    digitada="";
  } else if (key=='C'){
    //limpa senha
    digitada="";
  } else if(key=='#'){ //depois do enter vai verificar a senha 
    for (int i = 0; i < 5; i++) { //VERIFICAR SE ESTA SAINDO DESSE LAÇO
      if(verificaSenha(novasSenhas[i], digitada)){
        estado=1; //senha certa
        acerteiSenha = true;
        String payload = "{\"Entrada do usuario\":";
        payload += usuario[i];
        payload += "}";
        client.publish (topic, (char*) payload.c_str());
        estadoSenha(estado);
        estado=0;
        delay(2000); 
        break;
      }
    } 
    if (acerteiSenha==false) {
      estado=2;
      estadoSenha(estado);
      delay(2000);
      estado=0;
    }
    digitada=""; //limpa o que foi digitado
    acerteiSenha=false;
  } else {
    digitada+=key;  //concatenar as info que são digitadas
  }
    estadoSenha(estado);
  }
}
