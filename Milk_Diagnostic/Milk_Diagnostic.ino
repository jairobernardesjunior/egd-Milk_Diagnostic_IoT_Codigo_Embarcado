/*
  COLETA TEMPERATURA DO LEITE NO RESERVATÓRIO E ENVIA POR SMS
  ARDUINO MEGA 2560
  ===========================================================
  ===========================================================
*/

// RESET
#include <avr/wdt.h>

// SDCARD - CATALEX
#include <SD.h>
#include <SPI.h>
int pinCS = 53; // Connect with digital 53

int origem, int_seg, dif_dia, dif_hora;
String model, fone1, fone2, fone3;
float tmin, tmax, tideal;

// GPS - GPS6MV2 - Connect with pin TX18 and TX19
#include <TinyGPS.h>
#define GPSSerial Serial1
float lat, lon;
TinyGPS gps; // Cria o objeto gps

int year;
byte month, day, hour, minute, second;

// SENSOR TEMPERATURA EXTERNO - DHT11
#include "DHT.h"
#define DHTPIN 2 // Connect with digital 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float h, t, f, hif, hic;

// SENSOR TEMPERATURA SONDA - DS18B20
#include <OneWire.h>  
#include <DallasTemperature.h>
#define dados 33 // Connect with digital 33
OneWire oneWire(dados);  /*Protocolo OneWire*/
DallasTemperature sensors(&oneWire); /*encaminha referências OneWire para o sensor*/

// DISPOSITIVO SMS - SIM800L
#include <SoftwareSerial.h>
SoftwareSerial mySerial(4, 3); //Cria objeto mySerial passando como parâmetro as portas digitais 4 e 3

// -----------------------------------------------------------
void reset() {
  wdt_enable (WDTO_2S);
  wdt_reset ();  
  while (true) ; // fica aqui até resetar
  
}
// -----------------------------------------------------------
void ver_nro(String campo) {
  String nros = "0123456789.";
  String v1, v2;
  int i, ii;

  for (i=0; i<campo.length(); i++) {
    for (ii=0; ii<nros.length(); ii++) {
      v1=campo[i];
      v2=nros[ii];
      if (v1==v2) {
        break;
      }
    }

    if (ii>=nros.length()) {
      Serial.println("reset 05");
      reset();         
    }
  }

}
// -----------------------------------------------------------
void move_vr(int nro, String campo) {
  switch (nro) {
    case 1:
      ver_nro(campo);
      origem=campo.toInt();
      //Serial.println("1 ok");
      break;
    case 2:
      model=campo;
      //Serial.println("2 ok");
      break;
    case 3:
      ver_nro(campo);
      int_seg=campo.toInt();
      //Serial.println("3 ok");
      break;
    case 4:
      ver_nro(campo);
      dif_dia=campo.toInt();
      //Serial.println("4 ok");
      break;
    case 5:
      ver_nro(campo);
      dif_hora=campo.toInt();
      //Serial.println("5 ok");
      break;
    case 6:
      fone1=campo;
      //Serial.println("6 ok");
      break; 
    case 7:
      fone2=campo;
      //Serial.println("7 ok");
      break; 
    case 8:
      fone3=campo;
      //Serial.println("8 ok");
      break;   
    case 9:
      ver_nro(campo);
      tmin=campo.toFloat();
      //Serial.println("9 ok");
      break;   
    case 10:
      ver_nro(campo);
      tmax=campo.toFloat();
      //Serial.println("10 ok");
      break; 
    case 11:
      ver_nro(campo);
      tideal=campo.toFloat();
      //Serial.println("11 ok");
      break;                                            
  }

}
// -----------------------------------------------------------
void le_sdcard() {
  File myFile;
  String linha, campo, bytee;
  int i, ii;

  myFile = SD.open("md_param.txt");

  if (myFile) {
    while (myFile.available()) {
      linha = myFile.readString();
      //Serial.println(linha);
      break;
   }
    myFile.close();
  }
  else {
    Serial.println("***erro ao abrir md_param.txt");
    Serial.println("reset 02");
    reset();    
  }

  i=0;
  ii=0;
  while (true) {

    while (ii<800) {
      bytee = linha[ii];
      
      if (bytee==";") {
        ii+=1;
        break;
      }     

      ii+=1; 
    }

    if (ii>=800) {
      Serial.println("reset 03");
      reset();       
    }

    campo="";
    while (ii<800) {
      bytee = linha[ii];

      if (bytee==";") {
        ii+=1;    
        break;
      }
      
      campo=campo + linha[ii];
      ii+=1;
    }    

    if (ii>=800) {
      Serial.println("reset 04");
      reset();       
    }

    if (campo.length()>0) {
      Serial.println(campo);
      i+=1;
      move_vr(i, campo);
    }    

    if (i==11) {
      break;
    }
  }

}
// -----------------------------------------------------------
void le_gps() {
  bool ja_leu = false;

  while(true) {
    while(Serial1.available()){ // check for gps data 
      if(gps.encode(Serial1.read())) { // encode gps data

        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second);
        gps.f_get_position(&lat,&lon); // get latitude and longitude
      
        Serial.println("");  
        Serial.println("**************************************");  

        char sz[8];
        sprintf(sz, "%02d/%02d/%02d", day, month, year); 
        Serial.println("");
        Serial.print("Data: ");
        Serial.print(sz);

        sprintf(sz, "%02d:%02d:%02d", hour, minute, second);        
        Serial.println("");
        Serial.print("Hora: ");
        Serial.print(sz);      

        Serial.println("");      
        Serial.print("Latitude: ");
        Serial.print(lat,6);
        
        Serial.println("");
        Serial.print("Longitude: ");
        Serial.println(lon,6);     

        ja_leu=true;
        break;
      }
    }  

    if (ja_leu) {
      break;
    }
  }

}
// -----------------------------------------------------------
void le_tempex() {
  float h = dht.readHumidity(); // umidade %
  float t = dht.readTemperature(); // °C
  float f = dht.readTemperature(true); // °F

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Falha na leitura do SENSOR DHT11"));
    Serial.println("reset 06");
    reset();
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.println("");
  Serial.print("Umidade: ");
  Serial.print(h);
  Serial.print("%  Temperatura: ");
  Serial.print(t);
  Serial.print("°C ");
  Serial.print(f);
  Serial.print("°F  Heat index: ");
  Serial.print(hic);
  Serial.print("°C ");
  Serial.print(hif);
  Serial.println("°F");

}
// -----------------------------------------------------------
void le_tempsonda() { 
  sensors.requestTemperatures(); /* Envia o comando para leitura da temperatura */
  Serial.print("A temperatura do leite é: "); /* Printa "A temperatura é:" */
  Serial.print(sensors.getTempCByIndex(0)); /* Endereço do sensor */

}
// -----------------------------------------------------------
void updateSerial() {
  while (mySerial.available()) //Verifica se a comunicação serial está disponível
  {
    Serial.write(mySerial.read()); //Realiza leitura serial dos dados de entrada Arduino
  }
  delay(500);

}
// -----------------------------------------------------------
void envia_sms() {
  mySerial.println("AT"); //Teste de conexão 
  updateSerial();
  
  mySerial.println("AT+CMGF=1"); //Configuração do modo SMS text
  updateSerial();
  
  mySerial.println("AT+CMGS=\"+5534992528004\""); //Número de telefone que irá receber a mensagem, “ZZ” corresponde ao código telefônico do pais e “XXXXXXXXXXX” corresponde ao número de telefone com o DDD
  updateSerial();
  
  mySerial.print("Enviando dados do leite"); //Texto que será enviado para o usúario
  updateSerial();
  
  mySerial.write(26); //confirmação das configurações e envio dos dados para comunicação serial.

  Serial.println("");
  Serial.println("sms enviado");

}
// -----------------------------------------------------------
void ini_sms() {
  Serial.println("DISPOSITIVO sms SIM800L inicializado!");
  mySerial.begin(9600);

}
// -----------------------------------------------------------
void ini_tempsonda() {
  Serial.println("SENSOR temp DS18B20 inicializado!");
  sensors.begin();

}
// -----------------------------------------------------------
void ini_tempex() {
  Serial.println("SENSOR temp DHT11 inicializado!");
  dht.begin();

}
// -----------------------------------------------------------
void ini_gps() {
  Serial.println("GPS recebe o sinal:");
  Serial1.begin(9600); // conecta o sensor gps

}
// -----------------------------------------------------------
void ini_sdcard() {
  pinMode(pinCS, OUTPUT);
  
  if (SD.begin()) {
    Serial.println("SD card inicializado!");
  } 
  else {
    Serial.println("SD card ***NÃO inicializado.");
    Serial.println("reset 01");
    reset();
  }

}
// ####################################################
void setup() {
  Serial.begin(9600);

  ini_sdcard();
  le_sdcard();

  Serial.println("");

  ini_gps();
  ini_tempex();
  ini_tempsonda();
  ini_sms();
 
}
// ####################################################
void loop() {
  le_gps();
  le_tempex();
  le_tempsonda();
  envia_sms();

  abort();
  delay(3000);
  
}
