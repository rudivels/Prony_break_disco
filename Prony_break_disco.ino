//  Baseado em Exemplo de https://domoticx.com/esp8266-wifi-modbus-tcp-ip-slave/
//  Hardware Wemos D1 ESP8266
//             
//  Programa desenvolvido para ser incluido no sensor com Modbus-IP de 
//  peso, rotacao e temperatura Freio de Prony
//
//  29/11/2020 Versao com medicao de peso, temperatura e rotacao para bancada hidrolab

#include "HX711.h"
#include <ESP8266WiFi.h>

#define TIMER_INTERRUPT_DEBUG      1
#include "ESP8266TimerInterrupt.h"
#define TIMER_INTERVAL_MS  1000
ESP8266Timer ITimer;
void ICACHE_RAM_ATTR handleInterrupt();

const int LOADCELL_DOUT_PIN = D5;
const int LOADCELL_SCK_PIN = D6;
HX711 scale;

#include "acesso_wifi.h"  // define ssid & password

int ModbusTCP_port = 502;

int sensorValue; 
float Temperatura;
float gramas;

//////// Required for Modbus TCP / IP /// Requerido para Modbus TCP/IP /////////
#define maxInputRegister 20
#define maxHoldingRegister 20
 
#define MB_FC_NONE 0
#define MB_FC_READ_REGISTERS 3 //implemented
#define MB_FC_WRITE_REGISTER 6 //implemented
#define MB_FC_WRITE_MULTIPLE_REGISTERS 16 //implemented
//
// MODBUS Error Codes
//
#define MB_EC_NONE 0
#define MB_EC_ILLEGAL_FUNCTION 1
#define MB_EC_ILLEGAL_DATA_ADDRESS 2
#define MB_EC_ILLEGAL_DATA_VALUE 3
#define MB_EC_SLAVE_DEVICE_FAILURE 4
//
// MODBUS MBAP offsets
//
#define MB_TCP_TID 0
#define MB_TCP_PID 2
#define MB_TCP_LEN 4
#define MB_TCP_UID 6
#define MB_TCP_FUNC 7
#define MB_TCP_REGISTER_START 8
#define MB_TCP_REGISTER_NUMBER 10
 
byte ByteArray[260];
unsigned int MBHoldingRegister[maxHoldingRegister];
 
//////////////////////////////////////////////////////////////////////////

WiFiServer MBServer(ModbusTCP_port);

const byte interruptPin = 13;  // D7 da placa Wemos
volatile byte interruptCounter = 0;
int rotacao;
 
void ICACHE_RAM_ATTR handleInterrupt() {
  interruptCounter++;
}

void ICACHE_RAM_ATTR TimerHandler()
{
 rotacao=interruptCounter;
 interruptCounter=0;
 // #if (TIMER_INTERRUPT_DEBUG > 0)
 // Serial.print("Delta ms = " + String(millis() - lastMillis));
 // lastMillis = millis();
 // Serial.print(" rotacao = "); Serial.println(rotacao);
 // #endif
}

void setup() {
 pinMode(14, OUTPUT);  // ainda não descobri a funcao deste pino...
 pinMode(interruptPin, INPUT_PULLUP);

 attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
 if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.println("Starting  ITimer OK, millis() = " + String(millis()));
  }
  else
    Serial.println("Can't set ITimer correctly. Select another freq. or interval");
    
 scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
 // Procedimento para calibrar
 scale.set_scale();
 scale.tare();
 // coloca um peso conhecido 
 scale.get_units(10);  // pega dez medidas e retorna float
 // divide o valor retornado pelo peso conhecido
 // passa a resposta no set_scale()
 scale.set_scale(2280.f);
 scale.tare(); 
 // calibrado 
 Serial.begin(9600);
 delay(100);
 WiFi.begin(ssid, password);
 Serial.println(".");
 while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
 MBServer.begin();
 Serial.println("Connected ");
 Serial.print("ESP8266 Slave Modbus TCP/IP ");
 Serial.print(WiFi.localIP()); Serial.print(":"); Serial.println(String(ModbusTCP_port));
 Serial.println("Modbus TCP/IP Online"); 
 Serial.println("Pronto  ");
}


void loop() {
 WiFiClient client = MBServer.available();
 if (!client) { return; }
 
 boolean flagClientConnected = 0;
 byte byteFN = MB_FC_NONE;
 int Start;
 int WordDataLength;
 int ByteDataLength;
 int MessageLength;
 
 // Debugando
 /*while(1){
 sensorValue = analogRead(A0);
 Temperatura=sensorValue*3.3/10.24;
 Temp_int=Temperatura*1000;
 
 gramas=scale.get_units();

 Serial.print("Sens Value = "); Serial.print(sensorValue);
 Serial.print("  , Temperatura = "); Serial.print(Temperatura); 
 Serial.print("  , Gramas = "); Serial.print(gramas,1); 
 Serial.print("interruptCounter); 
 delay(1000);
 }
 } */
 
 
 // Modbus TCP/IP
 while (client.connected()) {
   if(client.available()) {
      flagClientConnected = 1;
      int i = 0;
      while(client.available()) {
        ByteArray[i] = client.read();
        i++;
      }
      client.flush();

      sensorValue = analogRead(A0);
      Temperatura=sensorValue*3.3/10.24;
      gramas=scale.get_units();
      //rotacao=digitalRead(interruptPin);

      Serial.print("Sens Value = "); Serial.print(sensorValue);
      Serial.print("  , Temperatura = "); Serial.print(Temperatura); 
      Serial.print("  , Gramas = "); Serial.print(gramas,1);
      Serial.print("  , counter = "); Serial.println(rotacao); 
      
      ///// code here --- codigo aqui
 
      ///////// Holding Register [0] A [9] = 10 Holding Registers Escritura
      ///////// Holding Register [0] A [9] = 10 Holding Registers Writing
 
      MBHoldingRegister[0] = sensorValue; // random(0,12);
      MBHoldingRegister[1] = (int)Temperatura; // dec_gramas; // random(0,12);
      MBHoldingRegister[2] = (int)gramas; //random(0,12);
      MBHoldingRegister[3] = rotacao; // random(0,12);
      MBHoldingRegister[4] = 0; //random(0,12);
      MBHoldingRegister[5] = 0; //random(0,12);
      MBHoldingRegister[6] = 0; //random(0,12);
      MBHoldingRegister[7] = 0; //random(0,12);
      MBHoldingRegister[8] = 0; //random(0,12);
      MBHoldingRegister[9] = 0; //random(0,12);
 
      ///////// Holding Register [10] A [19] = 10 Holding Registers Lectura
      ///// Holding Register [10] A [19] = 10 Holding Registers Reading
 
      int Temporal[10];
 
      Temporal[0] = MBHoldingRegister[10];
      Temporal[1] = MBHoldingRegister[11];
      Temporal[2] = MBHoldingRegister[12];
      Temporal[3] = MBHoldingRegister[13];
      Temporal[4] = MBHoldingRegister[14];
      Temporal[5] = MBHoldingRegister[15];
      Temporal[6] = MBHoldingRegister[16];
      Temporal[7] = MBHoldingRegister[17];
      Temporal[8] = MBHoldingRegister[18];
      Temporal[9] = MBHoldingRegister[19];
 
      /// Enable Output 14
      digitalWrite(14, MBHoldingRegister[14] );
 
      //// debug
      for (int i = 0; i < 10; i++) {
       Serial.print("[");
        Serial.print(i);
        Serial.print("] ");
        Serial.print(Temporal[i]);
      }
      Serial.println("");
 
      //// end code - fin 
 
      //// routine Modbus TCP
      byteFN = ByteArray[MB_TCP_FUNC];
      Start = word(ByteArray[MB_TCP_REGISTER_START],ByteArray[MB_TCP_REGISTER_START+1]);
      WordDataLength = word(ByteArray[MB_TCP_REGISTER_NUMBER],ByteArray[MB_TCP_REGISTER_NUMBER+1]);
    }
 
    // Handle request
    switch(byteFN) {
 
      case MB_FC_NONE:
        break;
 
      case MB_FC_READ_REGISTERS: // 03 Read Holding Registers
        ByteDataLength = WordDataLength * 2;
        ByteArray[5] = ByteDataLength + 3; //Number of bytes after this one.
        ByteArray[8] = ByteDataLength; //Number of bytes after this one (or number of bytes of data).
        for(int i = 0; i < WordDataLength; i++) {
          ByteArray[ 9 + i * 2] = highByte(MBHoldingRegister[Start + i]);
          ByteArray[10 + i * 2] = lowByte(MBHoldingRegister[Start + i]);
        }
        MessageLength = ByteDataLength + 9;
        client.write((const uint8_t *)ByteArray,MessageLength);
        byteFN = MB_FC_NONE;
        break;
  
      case MB_FC_WRITE_REGISTER: // 06 Write Holding Register
        MBHoldingRegister[Start] = word(ByteArray[MB_TCP_REGISTER_NUMBER],ByteArray[MB_TCP_REGISTER_NUMBER+1]);
        ByteArray[5] = 6; //Number of bytes after this one.
        MessageLength = 12;
        client.write((const uint8_t *)ByteArray,MessageLength);
        byteFN = MB_FC_NONE;
        break;
 
      case MB_FC_WRITE_MULTIPLE_REGISTERS: //16 Write Holding Registers
        ByteDataLength = WordDataLength * 2;
        ByteArray[5] = ByteDataLength + 3; //Number of bytes after this one.
        for(int i = 0; i < WordDataLength; i++) {
          MBHoldingRegister[Start + i] = word(ByteArray[ 13 + i * 2],ByteArray[14 + i * 2]);
        }
        MessageLength = 12;
        client.write((const uint8_t *)ByteArray,MessageLength); 
        byteFN = MB_FC_NONE;
        break;
    }
  }
}
