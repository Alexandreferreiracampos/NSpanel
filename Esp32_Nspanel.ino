/*
     CÓDIGO:    Q0961
     AUTOR:     BrincandoComIdeias
     APRENDA:   https://cursodearduino.net/
     SKETCH:    ESP32 Com Display Touch
     DATA:      31/10/2022
*/

// INCLUSÃO DE BIBLIOTECA
#include <Nextion.h>   // https://github.com/itead/ITEADLIB_Arduino_Nextion
#include <WiFi.h>         
#include <IOXhop_AutomacaoESP32.h>
#include <ArduinoJson.h>          

#define WIFI_SSID "Alexandre"                   
#define WIFI_PASSWORD "91906245"         
#define AUTOMACAO_HOST "192.168.0.100"

uint32_t value = 0;
NexPage page0    = NexPage(0, 0, "page0");
NexPage home    = NexPage(1, 0, "home");
NexPage page2    = NexPage(2, 0, "page2");

NexText        statusWifiText = NexText(0, 3, "t0");
NexDSButton    bt0        = NexDSButton(1, 11, "b0");



//Lista dos objetos que podem enviar comandos ao Arduino (para o Arduino ficar ouvindo no Loop)
NexTouch *nex_listen_list[] = 
{
    &bt0,
    NULL
};

//Memoria para funcionamento da biblioteca Nextion
char buffer[100] = {0};
int ledWifi = 2;
int bt1 = 27;
int bt2 = 14;
int relay1 = 19;
int relay2 = 22;
int sensor = 38;

const int ledPin = 5; 
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
int valorInt = 0;

String readString = String(30);
String valorReq;


void setup() {
  nexInit();
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(ledPin, ledChannel);
  pinMode(ledWifi, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   Automacao.begin(AUTOMACAO_HOST);

     while (WiFi.status() != WL_CONNECTED)
  {
      digitalWrite(ledWifi, HIGH);
      delay(500);
      digitalWrite(ledWifi, LOW);
      delay(500);
      

  }

  digitalWrite(ledWifi, HIGH);

  bt0.attachPush(bt0PushCallback, &bt0);
  statusWifiText.setText("Conectado");
  
}

void loop() {
  

  nexLoop(nex_listen_list);
   
}


void bt0PushCallback(void *ptr)
{
    Automacao.getString("rele4");
     
}
