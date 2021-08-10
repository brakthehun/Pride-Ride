
uint8_t flagger[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Adafruit_NeoPixel.h>

#define buttons 4

#define pix 27
Adafruit_NeoPixel strip = Adafruit_NeoPixel(pix, 16, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel ftrip = Adafruit_NeoPixel(pix, 17, NEO_RGB + NEO_KHZ800);


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

int color=0;
int spd=10;
int pgm=0;
int bright=255;
int pinputs[buttons]={26,18,19,23};
boolean hold[buttons];
long unsigned holdTimer[buttons];



void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.macAddress(myMAC);
  
  if (esp_now_init() != ESP_OK) {
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, flagger, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  for(int a=0;a<6;a++){
    dataSend.MAC[a]=myMAC[a];
  }
  dataSend.ID=0;
  Serial.println(WiFi.macAddress());

  for(int a=0;a<buttons;a++){
    pinMode(pinputs[a],INPUT_PULLUP);
  }
  
  strip.begin();
  strip.show();
  ftrip.begin();
  ftrip.show();
}

long unsigned colorTimer=millis()+50;
long unsigned pulseTimer=millis()+50;
long unsigned flashTimer=millis()+15;
long unsigned sendTimer=millis()+500;

long unsigned currentTime;
int pulse;
int pulseDir=5;
boolean flash;
boolean colorUp;
boolean pulseUp;
boolean pulseReset;
int pulseColor;

boolean newd=0;

void loop() {
  currentTime=millis();
  if(currentTime>colorTimer){
    colorTimer+=spd/5;
    color+=1;
    color%=768;
    colorUp=1;
  }
  if(currentTime>pulseTimer){
    pulseTimer+=spd;
    pulseUp=1;
    pulse+=pulseDir;
    if(pulse>=255){pulseDir=-5;}
    if(pulse<=0){pulseDir=5;pulseColor=random(768);}
    pulse+=pulseDir;;
  }
  if(currentTime>flashTimer){
    flashTimer+=spd;
    flash=1;
  }
  for(int a=0;a<buttons;a++){
    if(!digitalRead(pinputs[a])&&!hold[a]&&millis()>holdTimer[a]){
      holdTimer[a]=millis()+2;
      hold[a]=1;
      if(a==0){pgm=(pgm+1)%9;}
      if(a==1){spd=(spd+15)%60;}
      if(a==2){color=(color+25)%768;}
      if(a==3){bright=(bright+32)%256;}
      }
    if(digitalRead(pinputs[a])&&hold[a]&&millis()>holdTimer[a]){
      holdTimer[a]=millis()+2;
      hold[a]=0;
    }
  }
  if(pgm==0&&colorUp){
    for(int a=0;a<pix;a++){
      setPixel(a,((768/pix)*a)+color,bright);
    }
    colorUp=0;
    strip.show();
    ftrip.show();
  }
  if(pgm==1&&colorUp){
    for(int a=0;a<pix;a++){
      setPixel(a,color,bright);
    }
    colorUp=0;
    ftrip.show();
    strip.show();
  }
  if(pgm==2&&colorUp){
    for(int a=0;a<pix;a++){
      dimPixel(a,bright);
    }
    colorUp=0;
    strip.show();
    ftrip.show();
  }
  if(pgm==3&&flash){
    dimAll();
    flash=0;
    int flashLength=random(pix/4)+3;
    int poz=random(pix);
    for(int a=0;a<flashLength;a++){
      setPixel(a+poz,color,bright);
    }
    strip.show();
    ftrip.show();
  }
  if(pgm==4&&flash){
    dimAll();
    flash=0;
    int flashLength=random(pix/4)+3;
    int poz=random(pix);
    int colors=random(768);
    for(int a=0;a<flashLength;a++){
      setPixel(a+poz,colors,bright);
    }
    strip.show();
    ftrip.show();
  }
  if(pgm==5&&flash){
    dimAll();
    flash=0;
    int flashLength=random(pix/4)+3;
    int poz=random(pix);
    for(int a=0;a<flashLength;a++){
      dimPixel(a+poz,bright);
    }
    strip.show();
    ftrip.show();
  }
  if(pgm==6&&pulseUp){
    pulseUp=0;
    for(int a=0;a<pix;a++){
      setPixel(a,color,pulse);
    }
    strip.show();
    ftrip.show();
  }
  if(pgm==7&&pulseUp){
    pulseUp=0;
    for(int a=0;a<pix;a++){
      setPixel(a,pulseColor,pulse);
    }
    strip.show();
    ftrip.show();
  }
  if(pgm==8&&pulseUp){
    pulseUp=0;
    for(int a=0;a<pix;a++){
      dimPixel(a,pulse);
    }
    strip.show();
    ftrip.show();
  }
  if(pgm==9&&flash){
    dimAll();
    flash=0;
    int flashLength=random(pix/4)+3;
    int poz=random(pix);
    for(int a=0;a<flashLength;a++){
      setPixel(a+poz,dataRecv.color,bright);
    }
    strip.show();
    ftrip.show();
  }
  if(pgm==10&&pulseUp){
    pulseUp=0;
    for(int a=0;a<pix;a++){
      setPixel(a,dataRecv.color,pulse);
    }
    strip.show();
    ftrip.show();
  }

  if(currentTime>sendTimer){
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(flagger, (uint8_t *) &dataSend, sizeof(dataSend));
    if (result == ESP_OK) {
      sendTimer+=2500;
    }
    else {
      sendTimer+=200;
    }
  }
  
  if(newd){
    newd=0;
    Serial.println("New Dongle");
    //delay(50);
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, dataRecv.MAC, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }
    esp_err_t result = esp_now_send(dataRecv.MAC, (uint8_t *) &dataSend, sizeof(dataSend));
    if (result == ESP_OK) {
    Serial.print("^");
    }
    else {
      Serial.print("x");
    }
  }
}

void setPixel(int pixelz, int colorz, long unsigned fadez){
  colorz%=768;
  pixelz%=pix;
  fadez*=fadez;
  fadez/=255;
  
  if(colorz<256){
    colorz=(colorz*fadez)/255;
    strip.setPixelColor(pixelz,colorz,0,fadez-colorz);
    ftrip.setPixelColor(pixelz,colorz,0,fadez-colorz);
  }
  if(colorz>=256&&colorz<512){
    colorz-=256;
    colorz=(colorz*fadez)/255;
    strip.setPixelColor(pixelz,fadez-colorz,colorz,0);
    ftrip.setPixelColor(pixelz,fadez-colorz,colorz,0);
  }
  if(colorz>=512){
    colorz-=512;
    colorz=(colorz*fadez)/255;
    strip.setPixelColor(pixelz,0,fadez-colorz,colorz);
    ftrip.setPixelColor(pixelz,0,fadez-colorz,colorz);
  }
}

void dimPixel(int pixelz, long unsigned fadez){
  fadez*=fadez;
  fadez/=255;
  pixelz%=pix;
  strip.setPixelColor(pixelz,fadez,fadez,fadez);
  ftrip.setPixelColor(pixelz,fadez,fadez,fadez);
}

void dimAll(){
  for(int a=0;a<pix;a++){
    dimPixel(a,0);
  }
}


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status==0){
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&dataRecv, incomingData, sizeof(dataRecv));
  if(!dataSend.ID){
    for(int a=0;a<6;a++){
      flagger[a]=dataRecv.MAC[a];
    }
    Serial.print("new!!");
    newd=1;
    dataSend.ID=dataRecv.ID;
  }
  pgm=dataRecv.program;
  bright=dataRecv.bright;
  spd=dataRecv.speed;
  color=dataRecv.color*3;
  pulse=dataRecv.pulse*2;
  pulseDir=5;
  if(pulse>255){
    pulse=510-pulse;
    pulseDir=-5;
  }
}
