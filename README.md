# NSpanel
Criando um sistema de automação próprio para o sonof NSpanel 

Dentro da pasta da biblioteca ITEADLIB_Arduino_Nextion-master, precisamos alterar algumas linhas para poder realizar o teste com esp32 e o emulador do NEXTION.

NexConfig.h => 
#define nexSerial Serial2 // usar com a placa original do NSPANEL
#define nexSerial Serial // usar com msu esp32

NexHardware.cpp => 
nexSerial.begin(115200, SERIAL_8N1, 17, 16); usar com a placa original do NSPANEL
nexSerial.begin(115200, SERIAL_8N1, 3, 1);   usar com msu esp32
