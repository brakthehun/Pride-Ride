#define fobMax 16

uint8_t flagger[] = {0x7C, 0x9E, 0xBD, 0xF2, 0x79, 0x84};
uint8_t connect[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
SSD1306Wire display(0x3c, 5, 4); 

int pinputs[12]={25,26,27,14,23,22,21,19,16,17,15,18};
boolean hold[12];
long unsigned holdTimer[12];


boolean sendit;
byte myMAC[6];
byte masterAddress[6];

typedef struct struct_message {
  byte color;
  byte program;
  byte speed;
  byte bright;
  byte pulse;
  byte ID;
  uint8_t MAC[6];
} struct_message;
struct_message dataSend;
struct_message dataRecv;


long unsigned blinkTimer;
int blinks;

int assign=0;
uint8_t dongles[fobMax][6];
boolean reg[fobMax];

boolean update;


void setup() {
  for(int a=0;a<12;a++){
    pinMode(pinputs[a],INPUT_PULLUP);
  }
  
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.macAddress(myMAC);
  
  for(int a=0;a<6;a++){
    dataSend.MAC[a]=myMAC[a];
  }
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP OK");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, flagger, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Lame");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  for(int a=0;a<6;a++){
    dataSend.MAC[a]=myMAC[a];
  }
  Serial.println(" ");
  Serial.println(WiFi.macAddress());
  
  display.init();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "MAC Address");
  display.drawString(0,20, WiFi.macAddress());
  display.display();
  delay(1000);
  display.clear();
  update=1;
}


long unsigned timer;
long unsigned searchTimer=millis()+1200;

int pgm;
int color;
int speed=5;
int bright=255;
int skater=0;
boolean search=1;


boolean newd=0;
boolean backup=0;
boolean ignore[fobMax];
long unsigned FOBwatch[fobMax];
byte lastDongle;

void loop() {
  timer=millis();
  for(int a=0;a<12;a++){
    if(!digitalRead(pinputs[a])&&!hold[a]&&timer>holdTimer[a]){
      holdTimer[a]=timer+2;
      hold[a]=1;
      if(a==0){update=1;sendit=1;}
      if(a==1){
        update=1;
        searchTimer=timer+1200;
        search=1;
        esp_wifi_set_mac(WIFI_IF_STA, &connect[0]);
        Serial.println(WiFi.macAddress());
        }
      if(a==2){update=1;skater=(skater+1)%(assign+1);}
      if(a==3){update=1;skater=(skater+assign)%(assign+1);}
      if(a==4&&!hold[5]){update=1;pgm=max(0,pgm-1);}
      if(a==5&&!hold[4]){update=1;pgm=min(14,pgm+1);}
      if(a==6&&!hold[7]){update=1;color=max(0,color-3);}
      if(a==7&&!hold[6]){update=1;color=min(255,color+3);}
      if(a==8&&!hold[9]){update=1;speed=max(1,speed-1);}
      if(a==9&&!hold[8]){update=1;speed=min(12,speed+1);}
      if(a==10&&!hold[11]){update=1;bright=max(0,bright-5);}
      if(a==11&&!hold[10]){update=1;bright=min(255,bright+5);}
      }
    if(digitalRead(pinputs[a])&&hold[a]&&timer>holdTimer[a]){
      holdTimer[a]=timer+2;
      hold[a]=0;
    }
  }
  if(search){
    if(timer>searchTimer){
      search=0;
      esp_wifi_set_mac(WIFI_IF_STA, &myMAC[0]);
      Serial.println(WiFi.macAddress());
      update=1;
    }
  }
  if(update){
    display.clear();
    display.drawString(0,0,"Pgm:"+String(pgm+1));
    display.drawString(0,10,"Color: "+String(color));
    display.drawString(0,20,"Speed: "+String(13-speed));
    display.drawString(0,30,"Brightness: "+String(bright));
    display.drawString(0,40,"Skater: "+String(skater)+"/"+String(assign));
    display.drawString(0,50,"Search: "+String(search)+"  D:"+String(lastDongle));
    display.display();
    update=0;
  }
  if(sendit){
    sendit=0;
    if(pgm<11){
      dataSend.color=color;
      dataSend.program=pgm;
      dataSend.speed=speed*speed;
      dataSend.bright=bright;
    }
    for(int a=0;a<assign;a++){
      if(pgm<11){dataSend.color=((255/assign)*a)+color;}
      if(pgm<11){dataSend.pulse=((255/assign)*a);}
      esp_err_t result = esp_now_send(dongles[a], (uint8_t *) &dataSend, sizeof(dataSend));
      if (result == ESP_OK) { }
    }
  }

  if(newd){
    newd=0;
    //Serial.println("New Dongle");
    //delay(50);
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, dataRecv.MAC, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      //Serial.println("Failed to add peer");
      dataSend.ID=dataRecv.ID;
      reg[dataRecv.ID-1]=1;
      assign--;
      return;
    }
    //if(!backup){
    esp_err_t result = esp_now_send(dataRecv.MAC, (uint8_t *) &dataSend, sizeof(dataSend));
    if (result == ESP_OK) {
      Serial.print("^");
    }
    else {
      Serial.print("x");
    }
  backup=0;
  Serial.print("Added");
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status==0){
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&dataRecv, incomingData, sizeof(dataRecv));
  while(reg[assign]){assign++;}
  if(!reg[dataRecv.ID-1]){reg[dataRecv.ID-1]=1;}
  if(assign<fobMax){
    if(!dataRecv.ID){
      for(int a=0;a<6;a++){
        dongles[assign][a]=dataRecv.MAC[a];
      }
      reg[assign]=1;
      assign+=1;
      dataSend.ID=assign;
      newd=1; 
    }
    if(dataRecv.ID){
      lastDongle=dataRecv.ID;
    }
    Serial.print(dataRecv.ID);
    Serial.print(":");
  }
}
