
// INCLUSÃO DE BIBLIOTECA
#include <Nextion.h>   // https://github.com/itead/ITEADLIB_Arduino_Nextion
#include <WiFi.h>         
#include <FS.h>       
#include <IOXhop_AutomacaoESP32.h>
#include <WebServer.h>
#include <WiFiManager.h>  
#include "SPIFFS.h"       
#include <ArduinoJson.h>          
#include <EEPROM.h>

#define EEPROM_SIZE 2

char static_ip[16] = "192.168.0.218";         
char static_gw[16] = "192.168.0.1";
char static_sn[16] = "255.255.255.0";
#define AUTOMACAO_HOST "192.168.0."
bool shouldSaveConfig = false;            

boolean buttonPress = false;  

WebServer server(80);                                
//  Strings com HTML
String Q_1 = "<!DOCTYPE HTML><html><head></head><h1><center>AlexandreDev Automacao / NS Panel</center>";
String Q_2 = "</p></center><h3><BR></h3><html>\r\n";
String Ql = "";                                             
String Qd = "";                                             
String Qil = "";
String Q2 = "<p><center></p><p><h1>Equipamento resetado!</h1></p><p><center></p><p><h2>Conectar-se a rede wifi ''AlexandreDev-Device-Quarto'' e acesse http://192.168.4.1/<h2></p>";    
String Qid = "";
String Rele1;
String ResetEsp; // String para controle

void saveConfigCallback ()              
{
  shouldSaveConfig = true;
}

hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule(){
    ets_printf("(watchdog) reiniciar\n"); //imprime no log
    ESP.restart(); //reinicia o chip
}

uint32_t value = 0;
NexPage page0    = NexPage(0, 0, "page0");
NexPage home    = NexPage(1, 0, "home");
NexPage descanso = NexPage(2, 0, "descanso");
NexPage sala = NexPage(3, 0, "sala");
NexPage quarto = NexPage(4, 0, "quarto");
NexPage gourmet = NexPage(5, 0, "gourmet");
NexPage escritorio = NexPage(6, 0, "escritorio");

NexText        statusWifiText = NexText(0, 3, "t0");
NexText        hora = NexText(2, 3, "t1");
NexDSButton    portao        = NexDSButton(1, 11, "b0");
NexDSButton    luzGaragem        = NexDSButton(1, 5, "b4");
NexDSButton    luzArandela        = NexDSButton(1, 6, "b5");
NexDSButton    luzLed        = NexDSButton(1, 7, "b6");
NexDSButton    lustre        = NexDSButton(3, 2, "b0");
NexDSButton    sanca        = NexDSButton(3, 3, "b1");
NexDSButton    luzQuarto        = NexDSButton(4, 2, "b0");
NexDSButton    ledQuarto        = NexDSButton(4, 3, "b1");
NexDSButton    luzGourmet        = NexDSButton(5, 2, "b0");
NexDSButton    ledGourmet        = NexDSButton(5, 3, "b1");
NexDSButton    arandelaGourmet        = NexDSButton(5, 5, "b2");
NexDSButton    piscina         = NexDSButton(5, 6, "b3");
NexDSButton    som         = NexDSButton(5, 7, "b4");


//Lista dos objetos que podem enviar comandos ao Arduino (para o Arduino ficar ouvindo no Loop)
NexTouch *nex_listen_list[] = 
{
    &portao,
    &luzGaragem,
    &luzArandela,
    &luzLed,
    &lustre,
    &sanca,
    &luzQuarto,
    &ledQuarto,
    &luzGourmet,
    &ledGourmet,
    &arandelaGourmet,
    &piscina,
    &som,
    NULL
};

//Memoria para funcionamento da biblioteca Nextion
char buffer[100] = {0};
#define onLCD        4
#define bt1          27
#define bt2          14
#define rele1        19
#define rele2        22
#define ledWifi      2
#define buttomReset  33

#define DEBOUNCETIME 10 //tempo máximo de debounce para o botão (ms)

//É DECLARADA VOLÁTIL PORQUE SERÁ COMPARTILHADA PELO ISR E PELO CÓDIGO PRINCIPAL
volatile int numberOfButtonInterrupts1 = 0; //número de vezes que a interrupção foi executada
volatile int numberOfButtonInterrupts2 = 0; //número de vezes que a interrupção foi executada
volatile bool lastState1; //guarda o último estado do botão quando ocorreu a interrupção
volatile bool lastState2; //guarda o último estado do botão quando ocorreu a interrupção
volatile uint32_t debounceTimeout1 = 0; //guarda o tempo de debounce
volatile uint32_t debounceTimeout2 = 0; //guarda o tempo de debounce


uint32_t saveDebounceTimeout1;
uint32_t saveDebounceTimeout2;
bool saveLastState1;
bool saveLastState2;
int save1;
int save2;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR handleButton1Interrupt() {
    portENTER_CRITICAL_ISR(&mux); 
      numberOfButtonInterrupts1++;
      lastState1 = digitalRead(bt1);  
      debounceTimeout1 = xTaskGetTickCount(); //versão do millis () que funciona a partir da interrupção //version of millis() that works from interrupt
    portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR handleButton2Interrupt() {
    portENTER_CRITICAL_ISR(&mux); 
      numberOfButtonInterrupts2++;
      lastState2 = digitalRead(bt2);  
      debounceTimeout2 = xTaskGetTickCount(); //versão do millis () que funciona a partir da interrupção //version of millis() that works from interrupt
    portEXIT_CRITICAL_ISR(&mux);
}

int rele1State = LOW;    
int rele2State = LOW;    

int valorInt = 0;

String readString = String(30);
String valorReq;


void setup() {
  
  nexInit();

  EEPROM.begin(EEPROM_SIZE);
  rele1State = EEPROM.read(0);
  rele2State = EEPROM.read(1);
  
 
  pinMode(ledWifi, OUTPUT);
  pinMode(onLCD, OUTPUT);
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT); 
  pinMode(bt1, INPUT_PULLUP); 
  pinMode(bt2, INPUT_PULLUP);  
  pinMode(buttomReset, INPUT_PULLUP);  

  attachInterrupt(digitalPinToInterrupt(bt1), handleButton1Interrupt, CHANGE);   //configura a interrupção do botão no evento CHANGE para a função handleButtonInterrupt  
  attachInterrupt(digitalPinToInterrupt(bt2), handleButton2Interrupt, CHANGE);   //configura a interrupção do botão no evento CHANGE para a função handleButtonInterrupt  

  digitalWrite(rele1, rele1State); 
  digitalWrite(rele2, rele2State); 
  
  pinMode(onLCD, OUTPUT);
  digitalWrite(onLCD, LOW);

  if (SPIFFS.begin())
  {
  
    if (SPIFFS.exists("/config.json"))                      // carregar arquivo caso exista
    {
      
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);       
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          
          if (json["ip"])
          {
            
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
           
          }
          
        }
        
      }
    }
  }

  WiFiManager wifiManager;


  wifiManager.setSaveConfigCallback(saveConfigCallback);      //salva a configuração recebida
  IPAddress _ip, _gw, _sn;                                    //salvar IP
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  // wifiManager.resetSettings();                               //reset para apagar senha wifi

  wifiManager.setMinimumSignalQuality();                        //confiura qualidade minima de sinal do wifi padrão 8%
  
  if (!wifiManager.autoConnect("AlexandreDev-Device-Quarto", "91906245")) //verififcar se o wifi se conectou
  {
    delay(3000);
    ESP.restart();                                               //reinicia o ESP caso não conecte em nenhum rede
    delay(5000);
  }
                      
  if (shouldSaveConfig)                                         //salva as configuração recebidas
  {
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& jsonDoc = jsonBuffer.createObject();
    jsonDoc["ip"] = WiFi.localIP().toString();
    jsonDoc["gateway"] = WiFi.gatewayIP().toString();
    jsonDoc["subnet"] = WiFi.subnetMask().toString();
    
    File file = SPIFFS.open("/config.json", "w");
    
    if (!file) {
        //Serial.println("Falha ao abrir o arquivo para gravação");
        return;
    }
    // Serializa o objeto JSON no arquivo
    jsonDoc.prettyPrintTo(Serial);
    jsonDoc.printTo(file);
    file.close();


    //Serial.println("Arquivo JSON salvo com sucesso");
  }
  
  //Serial.println("local ip");
  //Serial.println(WiFi.localIP());
  //Serial.println(WiFi.gatewayIP());
  //Serial.println(WiFi.subnetMask());

  server.on("/", []()                                       
  {
    server.send(200, "text/html", Ql);                      
  });
     
  server.on("/Controle", []()                             
  {
    WiFiManager wifiManager;
    Rele1 = server.arg("Rele1"); 
    ResetEsp = server.arg("ResetEsp");

    if(Rele1 == "on") {
      digitalWrite(rele1, !digitalRead(rele1)); 
    }
   
    if(ResetEsp == "on"){
      server.send(200, "text/html", Q2);
      delay(2000);
      wifiManager.resetSettings();    
      delay(8000);
     ESP.restart();  
    }

    server.send(200, "text/html", Ql);  

    delay(100);                                          
  });
  server.begin();                                         
  
   Automacao.begin(AUTOMACAO_HOST);
 
 
      while (WiFi.status() != WL_CONNECTED)
  {
      digitalWrite(ledWifi, HIGH);
      delay(500);
      digitalWrite(ledWifi, LOW);
      delay(500);   
  }


    long tme = millis();

    timer = timerBegin(0, 80, true); //timerID 0, div 80
    //timer, callback, interrupção de borda
    timerAttachInterrupt(timer, &resetModule, true);
    //timer, tempo (us), repetição
    timerAlarmWrite(timer, 15000000, true); //igual a 15 segundos
    timerAlarmEnable(timer); //habilita a interrupção 
    


  portao.attachPush(portaoPushCallback, &portao);
  luzGaragem.attachPush(luzGaragemPushCallback, &luzGaragem);
  luzArandela.attachPush(luzArandelaPushCallback, &luzArandela);
  luzLed.attachPush(luzLedPushCallback, &luzLed);
  lustre.attachPush(lustrePushCallback, &lustre);
  sanca.attachPush(sancaPushCallback, &sanca);
  luzQuarto.attachPush(luzQuartoPushCallback, &luzQuarto);
  ledQuarto.attachPush(ledQuartoPushCallback, &ledQuarto);
  luzGourmet.attachPush(luzGourmetPushCallback, &luzGourmet);
  ledGourmet.attachPush(ledGourmetPushCallback, &ledGourmet);
  arandelaGourmet.attachPush(arandelaGourmetPushCallback, &arandelaGourmet);
  piscina.attachPush(piscinaPushCallback, &piscina);
  som.attachPush(somPushCallback, &som);
  
  statusWifiText.setText("Conectado");

  
}



void loop() {

  WiFiManager wifiManager;
  server.handleClient();   

  
  timerWrite(timer, 0); //reseta o temporizador (alimenta o watchdog) 
  long tme = millis(); //tempo inicial do loop
  nexLoop(nex_listen_list);

  
  if(digitalRead(buttomReset) == LOW && buttonPress == false){
    delay(50);
      timerWrite(timer, 0);
      buttonPress = true;
      digitalWrite(ledWifi, HIGH);
      server.send(200, "text/html", Q2);
      delay(2000);
      wifiManager.resetSettings();    
      delay(2000);
      ESP.restart();                     
    }

  portENTER_CRITICAL_ISR(&mux); // início da seção crítica
      save1  = numberOfButtonInterrupts1;
      save2  = numberOfButtonInterrupts2;
      saveDebounceTimeout1 = debounceTimeout1;
      saveDebounceTimeout2 = debounceTimeout2;
      saveLastState1  = lastState1;
      saveLastState2  = lastState2;
      timerWrite(timer, 0);
    portEXIT_CRITICAL_ISR(&mux); // fim da seção crítica

    bool currentState1 = digitalRead(bt1); //recupera o estado atual do botão
    bool currentState2 = digitalRead(bt2); //recupera o estado atual do botão

    //se o estado do botão mudou, atualiza o tempo de debounce
    if(currentState1 != saveLastState1)
    {
      saveDebounceTimeout1 = millis();
      timerWrite(timer, 0);
    }

    //se o tempo passado foi maior que o configurado para o debounce e o número de interrupções ocorridas é maior que ZERO (ou seja, ocorreu alguma), realiza os procedimentos
    if( (millis() - saveDebounceTimeout1) > DEBOUNCETIME && (save1 != 0) )
    {
           //se o botão está pressionado
           //liga o led verde e apaga o vermelho
           //caso contrário 
           //liga o led vermelho e apaga o verde

           if(currentState1) {              
              
           digitalWrite(rele1, !digitalRead(rele1));
           EEPROM.write(0, digitalRead(rele1));
           EEPROM.commit();
           timerWrite(timer, 0);
            }
            
            portENTER_CRITICAL_ISR(&mux);  //início da seção crítica
              numberOfButtonInterrupts1 = 0; // reconhece que o botão foi pressionado e reseta o contador de interrupção //acknowledge keypress and reset interrupt counter
            portEXIT_CRITICAL_ISR(&mux); //fim da seção crítica
    }
    if(currentState2 != saveLastState2)
    {
      saveDebounceTimeout2 = millis();
    }

    //se o tempo passado foi maior que o configurado para o debounce e o número de interrupções ocorridas é maior que ZERO (ou seja, ocorreu alguma), realiza os procedimentos
    if( (millis() - saveDebounceTimeout2) > DEBOUNCETIME && (save2 != 0) )
    {
          
           if(currentState2) {              
               Automacao.getString("200/relee");  
              delay(100);
              timerWrite(timer, 0);
        
            }
            
            portENTER_CRITICAL_ISR(&mux);  //início da seção crítica
              numberOfButtonInterrupts2 = 0; // reconhece que o botão foi pressionado e reseta o contador de interrupção //acknowledge keypress and reset interrupt counter
            portEXIT_CRITICAL_ISR(&mux); //fim da seção crítica
    }


  if(WiFi.status() == WL_CONNECTED){
    digitalWrite(ledWifi, HIGH);
    timerWrite(timer, 0);
    }else{
     resetModule();
    }

  timerWrite(timer, 0);
  delay(50);
   
}

void portaoPushCallback(void *ptr)
{
    Automacao.getString("200/relea");
    delay(100);
    timerWrite(timer, 0);
     
}
void luzGaragemPushCallback(void *ptr){
  Automacao.getString("200/rele3");  
   delay(100);
    timerWrite(timer, 0);
}
void luzArandelaPushCallback(void *ptr){
  Automacao.getString("200/rele4");  
   delay(100);
    timerWrite(timer, 0);
}
void luzLedPushCallback(void *ptr){
  Automacao.getString("200/rele1");
 delay(100);
    timerWrite(timer, 0);
}
void lustrePushCallback(void *ptr){
  Automacao.getString("200/rele6");
 delay(100);
    timerWrite(timer, 0);
}
void sancaPushCallback(void *ptr){
  Automacao.getString("200/rele5");  
   delay(100);
    timerWrite(timer, 0);
}
void luzQuartoPushCallback(void *ptr){
  Automacao.getString("100/rele4");  
   delay(100);
    timerWrite(timer, 0);
}
void ledQuartoPushCallback(void *ptr){
  Automacao.getString("100/fade");  
   delay(100);
    timerWrite(timer, 0);
}
void luzGourmetPushCallback(void *ptr){
  Automacao.getString("200/relee");  
   delay(100);
    timerWrite(timer, 0);
}
void ledGourmetPushCallback(void *ptr){
  Automacao.getString("200/relef");  
   delay(100);
    timerWrite(timer, 0);
}
void arandelaGourmetPushCallback(void *ptr){
  Automacao.getString("200/releg");
 delay(100);
    timerWrite(timer, 0);
}
void piscinaPushCallback(void *ptr){
  Automacao.getString("200/releh");  
   delay(100);
    timerWrite(timer, 0);
}
void somPushCallback(void *ptr){
  Automacao.getString("200/relej");  
   delay(100);
    timerWrite(timer, 0);
}

//http://192.168.0.95/Controle?Rele1=on
//http://192.168.0.95/Controle?ResetEsp=on
