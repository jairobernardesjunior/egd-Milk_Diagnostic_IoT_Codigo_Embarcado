/*
  COLETA TEMPERATURA DO LEITE NO RESERVATÓRIO E ENVIA POR SMS
  E EMAIL
  ARDUINO MEGA 2560 WIFI
  ===========================================================
  ===========================================================
*/

// RESET
#include <avr/wdt.h>

// SDCARD - CATALEX
#include <SD.h>
#include <SPI.h>
int pinCS = 53; // Connect with digital 53

int origem, int_seg, dif_dia, dif_hora, int_env, limite_sms;
String origems, model, fone1, fone2;
float tmin, tmax, tideal, dif_tbrusca;

// GPS - GPS6MV2 - Connect with pin TX18 and TX19
#include <TinyGPS.h>
#define GPSSerial Serial1
TinyGPS gps; // Cria o objeto gps
float lat, lon;
int year;
byte month, day, hour, minute, second;
String lats, lons; 
  
// SENSOR TEMPERATURA EXTERNO - DHT11
#include "DHT.h"
#define DHTPIN 2 // Connect with digital 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
//float h, t, f, hif, hic;

// SENSOR TEMPERATURA SONDA - DS18B20
#include <OneWire.h>  
#include <DallasTemperature.h>
#define dados 33 // Connect with digital 33
OneWire oneWire(dados);  /*Protocolo OneWire*/
DallasTemperature sensors(&oneWire); /*encaminha referências OneWire para o sensor*/

// DISPOSITIVO SMS - SIM800L
#include <SoftwareSerial.h>
SoftwareSerial mySerial(4, 3); //Cria objeto mySerial passando como parâmetro as portas digitais 4 e 3

// GERAL
File milk_arq;
byte day_ante, hour_ant, minute_ant, second_ant, tbrusca;
String dgps, tempex, tempsonda, tempmsg, sms_tbrusca, smsg, tldata, tlhora, reg_tbrusca, nro_reg_tbrusca;
int dif_sec, dif_sec_sms, tl_conta, conta_media, tl_conta2, dif_sec_tleite, int_env7;
int idx_int_env7, dif_sec_env7, dif_hora_salvo;
float h_acum, t_acum, f_acum, tleite_ac2, tleite_ac, tlmedia[10], tlmedia7[7], temp_leite;
bool envia_msg_temp, envia_dados_temp, envia_tbrusca;

// -----------------------------------------------------------
void reset() {
  wdt_enable (WDTO_2S);
  wdt_reset ();  
  while (true) ; // fica aqui até resetar
  
}
// -----------------------------------------------------------
void ver_nro(String campo) {
  String nros = "0123456789.-";
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
      //Serial.println("reset 05");
      smsg="*** parametro numerico informado no sdcard tem algum caractere nao numerico";
      grava_sdcard();
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
      ver_nro(campo);
      limite_sms=campo.toInt();
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
    case 12:
      ver_nro(campo);
      int_env=campo.toInt();
      //Serial.println("12 ok");
      break;    
    case 13:
      ver_nro(campo);
      dif_tbrusca=campo.toInt();
      //Serial.println("13 ok");
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
    //Serial.println("***erro ao abrir md_param.txt");
    //Serial.println("reset 02");
    smsg="*** erro ao abrir md_param.txt";
    grava_sdcard();  
    reset();    
  }

  i=0;
  ii=0;
  while (true) {

    while (ii<900) {
      bytee = linha[ii];
      
      if (bytee==";") {
        ii+=1;
        break;
      }     

      ii+=1; 
    }

    if (ii>=900) {
      //Serial.println("reset 03");
      smsg="*** texto cartao sdcard excedeu 900 caracteres";
      grava_sdcard();      
      reset();       
    }

    campo="";
    while (ii<900) {
      bytee = linha[ii];

      if (bytee==";") {
        ii+=1;    
        break;
      }
      
      campo=campo + linha[ii];
      ii+=1;
    }    

    if (ii>=900) {
      //Serial.println("reset 04");
      smsg="*** texto cartao sdcard excedeu 900 caracteres - final";
      grava_sdcard();       
      reset();       
    }

    if (campo.length()>0) {
      Serial.println(campo);
      i+=1;
      move_vr(i, campo);
    }    

    if (i==13) {
      myFile.close();
      break;
    }
  }  

}
// -----------------------------------------------------------
void le_gps() {
  //Serial.println("");
  //Serial.println("..........................................le_gps ");

  bool ja_leu = false; 

  hour_ant=hour;
  minute_ant=minute;
  second_ant=second;

  while(true) {
    while(Serial1.available()){ // check for gps data 
      if(gps.encode(Serial1.read())) { // encode gps data

        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second);       
        gps.f_get_position(&lat,&lon); // get latitude and longitude
      
        //Serial.println("");  
        //Serial.println("");
        //Serial.println("................................. le_gps ");  

        char sz[8];
        sprintf(sz, "%02d/%02d/%02d", day, month, year); 
        //Serial.println("");
        //Serial.print("Data: ");
        //Serial.print(sz);

        sprintf(sz, "%02d:%02d:%02d", hour, minute, second);        
        //Serial.println("");
        //Serial.print("Hora: ");
        //Serial.print(sz);      

        //Serial.println("");      
        //Serial.print("Latitude: ");
        //Serial.print(lat,6);
        
        //Serial.println("");
        //Serial.print("Longitude: ");
        //Serial.println(lon,6);     

        ja_leu=true;
        break;
      }
    }  

    if (ja_leu) {
      break;
    }
  }

  day+=dif_dia;
  hour+=dif_hora;

  if (((day!=day_ante) && (hour==0)) || (day_ante ==0)) { 
    day_ante=day;
  }
  
  char latc[5];
  dtostrf(lat, 10, 6, latc);
  lats = latc;
  char lonc[5];
  dtostrf(lon, 10, 6, lonc);
  lons = lonc;  

  float diaf=day;
  char diac[2];
  dtostrf(diaf, 2, 0, diac);
  String dias= diac;

  float mesf=month;
  char mesc[2];
  dtostrf(mesf, 2, 0, mesc);
  String mess= mesc;  

  float anof=year;
  char anoc[4];
  dtostrf(anof, 4, 0, anoc);
  String anos= anoc;

  float horf=hour;
  char horc[2];
  dtostrf(horf, 2, 0, horc);
  String hors= horc;

  float minf=minute;
  char minc[2];
  dtostrf(minf, 2, 0, minc);
  String mins= minc;  

  float segf=second;
  char segc[2];
  dtostrf(segf, 2, 0, segc);
  String segs= segc;

  //tldata = dias + "/" + mess + "/" + anos;
  //tlhora = hors + ":" + mins + ":" + segs;
  //dgps = "dt;" + dias + "/" + mess + "/" + anos + ";hr;" + hors + ":" + mins + ":" + segs + ";la;" + lats + ";lo;" + lons + ";";  

  //dgps = "dt" + dias + "/" + mess + "/" + anos + "phr" + hors + ":" + mins + ":" + segs + "pla" + lats + "plo" + lons + "p";  

  tldata = dias + "/" + mess + "/" + anos;
  tlhora = hors + ":" + mins + ":" + segs;
  dgps = dias + "/" + mess + "/" + anos + "p" + hors + ":" + mins + ":" + segs + "p" + lats + "p" + lons + "p";

  dif_sec=((hour*3600) + (minute*60) + second) - ((hour_ant*3600) + (minute_ant*60) + second_ant);

  if (dif_sec<0) {
    //Serial.println(dif_sec);
    dif_sec=0;
  }

  dif_sec_env7 += dif_sec;
  dif_sec_sms += dif_sec;
  dif_sec_tleite += dif_sec;

  //Serial.println("");
  //Serial.println("diferença em segundos legps: ");
  //Serial.println(dif_sec);
  //Serial.println(dif_sec_sms);
  //Serial.println(dif_sec_tleite);

  //Serial.println("................................. le_gps ");  

}
// -----------------------------------------------------------
void compoe_temp_leite() { 
  //Serial.println("");
  //Serial.println("..........................................compoe_temp_leite ");   

  float tlmx; 
  String tlsx; 
  char tlcx[9];

  for (int i=0; i<7; i++) {
    Serial.println(i);

    tlmx=tlmedia7[i];
    dtostrf(tlmx, 9, 6, tlcx);
    tlsx= tlcx;   
    tempsonda = tempsonda + tlsx + "p"; 
  }

}
// -----------------------------------------------------------
void formata_temp() {  
  //Serial.println("");
  //Serial.println("..........................................formata_temp "); 

  float hm, tm, fm, tlm;  

  hm = h_acum/tl_conta;
  tm = t_acum/tl_conta;
  fm = f_acum/tl_conta;
  tlm = tleite_ac/tl_conta;

  if (envia_tbrusca) {
    tlm=temp_leite;
  }

  //Serial.println(tlm);
  //Serial.println(tleite_ac);
  //Serial.println(tl_conta);

  char pumic[9];
  dtostrf(hm, 9, 6, pumic);
  String pumis = pumic;
  char texc[9];
  dtostrf(tm, 9, 6, texc);
  String texs = texc;  
  char fexc[9];
  dtostrf(fm, 9, 6, fexc);
  String fexs = fexc;  

  //tempex = "um" + pumis + "ptc" + texs + "ptf" + fexs + "p";  
  tempex = pumis + "p" + texs + "p";  

  //Serial.println("..........................................formata_temp2 ");

  char tlc[9];
  dtostrf(tlm, 9, 6, tlc);
  String tls= tlc;

  //tempsonda = "tl" + tls + "p";  
  tempsonda = tls + "p";  

  compoe_temp_leite();

  //Serial.println(dgps);
  //Serial.println(tempex);
  //Serial.println(tempsonda);
  //Serial.println("");
  //Serial.println("..........................................formata_temp ");  

}
// -----------------------------------------------------------
void le_tempex() {
  //Serial.println("");
  //Serial.println("........... le_tempex ");  

  float h = dht.readHumidity(); // umidade %
  float t = dht.readTemperature(); // °C
  float f = dht.readTemperature(true); // °F

  if (isnan(h) || isnan(t) || isnan(f)) {
    //Serial.println(F("Falha na leitura do SENSOR DHT11"));
    //Serial.println("reset 06");
    smsg="*** falha na leitura do sensor DHT11 - tempex";
    grava_sdcard();     
    reset();
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  //Serial.println("");
  //Serial.print(F("Umidade: "));
  //Serial.print(h);
  //Serial.print("%  Temperatura: ");
  //Serial.print(t);
  //Serial.print("°C ");
  //Serial.print(f);
  //Serial.print("°F  Heat index: ");
  //Serial.print(hic);
  //Serial.print("°C ");
  //Serial.print(hif);
  //Serial.println("°F");

  //Serial.println("........... le_tempex ");  

  h_acum += h;
  t_acum += t;
  f_acum += f;

  /*if (t<-50) {
    smsg="*** temperatura menor que -50 no sensor DHT11 - tempex";
    grava_sdcard();     
    reset();  
  } */

}
// -----------------------------------------------------------
void le_tempsonda() { 
  //Serial.println("");
  //Serial.println("......................... le_tempsonda ");  

  //Serial.println("");
  sensors.requestTemperatures(); /* Envia o comando para leitura da temperatura */
  //Serial.print("A temperatura do leite é: "); /* Printa "A temperatura é:" */
  //Serial.print(sensors.getTempCByIndex(0)); /* Endereço do sensor */

  float tl= sensors.getTempCByIndex(0);
  temp_leite=tl;

  /*if (tl<-50) {
    smsg="*** temperatura menor que -50 no sensor DS18B20 - tempsonda";
    grava_sdcard();     
    reset();  
  } */ 

  tleite_ac += tl;
  tleite_ac2 += tl;
  tl_conta += 1;  
  tl_conta2 += 1;  
  
  //Serial.println("");
  //Serial.println("tl - tlacum - tlconta - tlacum2 - tlconta2");
  //Serial.println(tl);
  //Serial.println(tleite_ac);  
  //Serial.println(tl_conta); 
  //Serial.println(tleite_ac2);  
  //Serial.println(tl_conta2);  
  //Serial.println("......................... le_tempsonda ");  
}
// -----------------------------------------------------------
void ini_sms() {
  digitalWrite(9, HIGH);
  
  Serial.println("");
  //Serial.println("DISPOSITIVO sms SIM800L inicializado!");
  mySerial.begin(9600);
  delay(1000);

  if (mySerial.available()) {
    Serial.println("DISPOSITIVO sms SIM800L inicializado!");
  }
  else {
    Serial.println("DISPOSITIVO sms SIM800L não inicializado!");
    Serial.println("reset 07");
    //reset();
  }
}
// -----------------------------------------------------------
void ini_tempsonda() {
  digitalWrite(10, HIGH);

  Serial.println("");
  Serial.println("SENSOR temp DS18B20 inicializado!");
  sensors.begin();
  delay(1000);

}
// -----------------------------------------------------------
void ini_tempex() {
  digitalWrite(11, HIGH);

  Serial.println("");
  Serial.println("SENSOR temp DHT11 inicializado!");
  dht.begin();
  delay(1000);

}
// -----------------------------------------------------------
void ini_gps() {
  digitalWrite(12, HIGH);  

  Serial.println("");
  Serial.println("GPS recebe o sinal:");
  Serial1.begin(9600); // conecta o sensor gps
  delay(1000);

}
// -----------------------------------------------------------
void ini_sdcard() {
  digitalWrite(13, HIGH);

  pinMode(pinCS, OUTPUT);
  delay(1000);
  
  Serial.println("");
  Serial.println("");

  if (SD.begin()) {
    Serial.println("SD card inicializado!");
  } 
  else {
    Serial.println("SD card ***NÃO inicializado.");
    Serial.println("reset 01");
    //smsg="*** SD card ***NAO inicializado.";
    //grava_sdcard();      
    reset();
  }

}
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void verifica_tbrusca() { 
  //Serial.println("");
  //Serial.println(".................... verifica_tbrusca ");  

  if (tbrusca==1) {
    if ((temp_leite - tideal) >= dif_tbrusca) {
      nro_reg_tbrusca= second_ant;   
      envia_tbrusca= true;
      reg_tbrusca= "rx31";
      tbrusca=2;
    }
  }
  else if (temp_leite <= tideal) {
      envia_tbrusca= true;
      reg_tbrusca= "rx32";
      tbrusca=1;
  }        

  //Serial.println(".................... verifica_tbrusca "); 

}
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void compara_varia() { 
  //Serial.println("");
  //Serial.println(".................... compara_varia ");  

  /*
  Serial.println(tlmedia[0]);
  Serial.println(tlmedia[1]);
  Serial.println(tmax);
  delay(2000); */

  //if (tlmedia[0] > tlmedia[1]) {
    //if (tlmedia[0] >= (tmax*0.90)) {
    if (temp_leite >= (tmax*0.90)) {
          
      //tempmsg = "A temperatura do leite esta alta - " + tldata + " as " + tlhora + " - temp: ";
      tempmsg = "A temperatura do leite esta alta (graus Celsius) =  ";
      /*
      for (int i=0; i<=9; i++) {
        char tlc[9];
        dtostrf(tlmedia[i], 9, 6, tlc);
        String tls= tlc;
        tempmsg = tempmsg + tls + " - ";
      } 
      */

      char tlc[9];
      //dtostrf(tlmedia[0], 9, 6, tlc);
      dtostrf(temp_leite, 9, 6, tlc);
      String tls= tlc;
      tempmsg = tempmsg + " __" + tls + "__";

      char tlc2[5];
      dtostrf(tmax, 5, 2, tlc2);
      String tls2= tlc2;
      tempmsg = tempmsg + "  tmax: __" + tls2 + "__";
      tempmsg = tempmsg + "  em __" + tldata + "__ as __" + tlhora + "__";
      envia_msg_temp=true;     

      //Serial.println("xxxxxxxxxxxxx " + tempmsg);
      //delay(2000);

    } 
  //}

  //Serial.println(".................... compara_varia "); 

}
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void media_tleite() {  
  //Serial.println("");
  //Serial.println("............ media_tleite "); 

  for (int i=9; i>0; i--) {
    tlmedia[i]=tlmedia[i-1];
  }
  tlmedia[0] = tleite_ac2/tl_conta2;
 
  //if (conta_media>=9) {
  if (conta_media>=0) {
    compara_varia();
  }
  else {
    conta_media += 1;
  }

  //Serial.println(tlmedia[0]);
  //Serial.println(conta_media);  
  //Serial.println(tl_conta2);  

  tleite_ac2=0;
  tl_conta2=0;

  //Serial.println("............ media_tleite ");

}
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void grava_sdcard() { 
  milk_arq = SD.open("milk_arq.txt", FILE_WRITE);
  if (milk_arq)
  {
    Serial.println("");
    Serial.println("                             grava sd card now.....................");
    Serial.println(smsg);
    milk_arq.println(smsg);
    milk_arq.close();
  } 

}
// ####################################################
void setup() {
  Serial.begin(9600);
  //Serial.println("");
  //Serial.print("setup");  

  //Define a porta do led como saida
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);  

  //apaga o led
  digitalWrite(13, LOW);
  digitalWrite(12, LOW);
  digitalWrite(11, LOW);
  digitalWrite(10, LOW);
  digitalWrite(9, LOW);
  digitalWrite(8, LOW);   

  ini_sdcard();
  le_sdcard();

  ini_gps();
  ini_tempex();
  ini_tempsonda();
  ini_sms();

  conta_media=0;
  tleite_ac2=0;
  tl_conta2=0;
  dif_sec_tleite=0;  

  h_acum=0;
  t_acum=0;
  f_acum=0;
  tleite_ac=0;
  tl_conta=0;
  dif_sec_sms=0;
  dif_sec_env7=0;
  idx_int_env7=0;
  day_ante=0;
  temp_leite=0;

  dif_hora_salvo=dif_hora;

  for (int i=0; i<7; i++) {
    tlmedia7[i]=0;
  }

  envia_msg_temp=false;
  envia_dados_temp=false; 
  envia_tbrusca=false;
  reg_tbrusca="";
  nro_reg_tbrusca="";
  tbrusca=1;



//.....................................................






  int_seg=15;
  int_env=30; 
  limite_sms=3;


  int_seg=2400;
  int_env=600; 
  limite_sms=50;


  int_seg=30;
  int_env=300; 
  limite_sms=3;
  tideal=30;
  dif_tbrusca=0.5;
  tmax=30;






//.....................................................

  float seg_dia= 86400;
  int_env7=int_env/8;


  Serial.println(seg_dia);
  Serial.println(int_env7);
}
// ####################################################
void loop() {
  //Serial.println("");
  //Serial.print("loop ");
  //Serial.println(tl_conta);
  //Serial.println(tl_conta2);

  float origemf=origem;
  char origemc[6];
  dtostrf(origemf, 6, 0, origemc);
  origems= origemc;
  origems= origems + "p";  

  le_gps(); 

  le_tempex();

  le_tempsonda(); 

  digitalWrite(9, HIGH);

  verifica_tbrusca();

  if (envia_tbrusca) {
    formata_temp();    
    sms_tbrusca= origems + dgps + tempex + tempsonda;
    smsg= sms_tbrusca;
    grava_sdcard();    
  }

  // ***** verifica temperatura do leite para enviar sms
  if (dif_sec_tleite>=int_seg) {
    media_tleite();
    
    if (envia_msg_temp) {
      smsg=tempmsg;
      grava_sdcard();
    }
    dif_sec_tleite=0;
  }

  // desloca temperatura do leite para frente na ltmedia7
  if (dif_sec_env7>=int_env7) {
    if (idx_int_env7<7) {
      tlmedia7[idx_int_env7]=tleite_ac/tl_conta;
      dif_sec_env7=0;
      idx_int_env7+= 1;
    }
  }  

  // reinicia variáveis de dados do leite marca para enviar sms 
  if (dif_sec_sms>=int_env) {
    formata_temp();
    smsg= origems + dgps + tempex + tempsonda;
    grava_sdcard();
    envia_dados_temp=true;

    dif_sec_sms=0;
    h_acum=0;
    t_acum=0;
    f_acum=0;
    tleite_ac=0;
    tl_conta=0;

    for (int i=0; i<7; i++) {
      tlmedia7[i]=0;
    }   
    dif_sec_env7=0;
    idx_int_env7=0;     
  }

  // ***** prepara para enviar dados por sms
  String str, str2;  
  bool envia_msgx= false;

  // ***** envia temperatura do leite com aumento brusco
  if (envia_tbrusca) {
    if (fone2!="sem") {
      str = fone2 + " ";
      str2 = reg_tbrusca + nro_reg_tbrusca + "p " + sms_tbrusca + " ";
      envia_msgx=true;
    }
    envia_tbrusca=false;
  }
  // ***** envia mensagem de aumento da temperatura do leite
  else  if (envia_msg_temp) {
          if (fone1!="sem") {
            str = fone1 + " ";
            str2 = "rx2p" + origems + "    " + tempmsg + " ";
            envia_msgx=true;
          }
          envia_msg_temp=false;
  }
  // ***** envia dados do leite    
  else  if (envia_dados_temp) {
          if (fone2!="sem") {
            str = fone2 + " ";
            str2 = "rx1p" + origems + dgps + tempex + tempsonda + " ";
            envia_msgx=true;
          }
          envia_dados_temp=false;              
  }       

  if (envia_msgx) {
    char fonex[str.length()];
    str.toCharArray(fonex,str.length());

    Serial.println("fonex...............................................");
    Serial.println(str);
    Serial.println(str2);

    char smsx[str2.length()];
    str2.toCharArray(smsx,str2.length());

    mySerial.write(""); //limpa
    delay(1000);       

    mySerial.write("AT+CMGF=1\r\n"); //Configuração do modo SMS text
    delay(1000);

    //mySerial.write("AT+CMGS=\"5534999700463\"\r\n");
    mySerial.write("AT+CMGS=\"");

    //mySerial.write("5534999700463");
    mySerial.write(fonex);
    
    mySerial.write("\"");
    mySerial.write("\r\n");
    delay(1000);
    
    //mySerial.write("Do bit Ao Byte");
    mySerial.write(smsx);
    delay(1000);
    
    mySerial.write((char)26);
    delay(1000);

    Serial.println("################### sms enviado");     
  } 

  //apaga o led
  digitalWrite(13, LOW);
  digitalWrite(12, LOW);
  digitalWrite(11, LOW);
  digitalWrite(10, LOW);
  digitalWrite(9, LOW);
  digitalWrite(8, LOW);

}