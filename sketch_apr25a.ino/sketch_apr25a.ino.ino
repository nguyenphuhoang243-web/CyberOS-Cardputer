#include <M5Cardputer.h>
#include <WiFi.h>
#include <time.h>

M5Canvas canvas(&M5.Display);

// ================= SCREEN =================
enum Screen {BOOT, MENU, WIFI_MENU, WIFI_SCAN, WIFI_LIST, WIFI_PASS, RADAR, CLOCK, SECURITY};
Screen screen = BOOT;

// ================= MENU =================
int menuIndex = 0;
float scrollY = 0, scrollTarget = 0, scrollVel = 0;

// ================= WIFI =================
String ssid[20];
int netCount = 0;
int wifiIndex = 0;
int wifiMenuIndex = 0;
String pass = "";

// ================= SCAN =================
bool scanning = false;
unsigned long scanT = 0;

// ================= TRANSITION =================
float transX = 0;
bool transitioning = false;

// ================= KEY =================
char lastKey = 0;

// ================= MATRIX =================
int mx[80], my[80], mv[80];

// ================= RADAR =================
float radarAngle = 0;
int radarX[20], radarY[20], radarStrength[20];

// ================= BATTERY =================
int batteryLevel = 100;
bool charging = false;
int chargeAnim = 0;

// ================= BOOT =================
unsigned long bootT = 0;

// =====================================================
// TIME INIT
// =====================================================
void initTime(){
  configTime(7 * 3600, 0, "pool.ntp.org");
}

// =====================================================
// BATTERY
// =====================================================
int getBattery(){
  float v = M5.Power.getBatteryVoltage();

  batteryLevel = map(v, 3300, 4200, 0, 100);
  batteryLevel = constrain(batteryLevel, 0, 100);

  charging = M5.Power.isCharging();

  return batteryLevel;
}

void drawBattery(){

  int lv = getBattery();

  canvas.drawRect(200,5,30,10,GREEN);
  canvas.fillRect(230,7,3,6,GREEN);

  int w = map(lv,0,100,0,28);
  canvas.fillRect(201,6,w,8,GREEN);

  if(charging){
    chargeAnim = (chargeAnim + 2) % 30;
    canvas.drawLine(200 + (chargeAnim % 28), 3,
                    200 + (chargeAnim % 28), 15, GREEN);
  }

  canvas.setCursor(150,5);
  canvas.print(lv);
  canvas.print("%");
}

// =====================================================
// MATRIX
// =====================================================
void drawMatrix(){
  for(int i=0;i<80;i++){
    canvas.drawPixel(mx[i],my[i],0x0200);
    my[i]+=mv[i];
    if(my[i]>135){
      my[i]=0;
      mx[i]=random(0,240);
    }
  }
}

// =====================================================
// WIFI SCAN
// =====================================================
void startScan(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true,true);
  delay(100);

  scanning = true;
  scanT = millis();
}

void updateScan(){
  if(!scanning) return;

  canvas.setCursor(20,60);
  canvas.print("SCANNING...");

  if(millis()-scanT>500){

    int n = WiFi.scanNetworks();
    netCount = (n>20)?20:n;

    for(int i=0;i<netCount;i++){
      ssid[i]=WiFi.SSID(i);

      float a = map(i,0,20,0,360)*0.017;
      int d = map(WiFi.RSSI(i),-90,-30,60,10);

      radarX[i]=120+cos(a)*d;
      radarY[i]=70+sin(a)*d;
      radarStrength[i]=WiFi.RSSI(i);
    }

    scanning=false;
    screen=WIFI_LIST;
  }
}

// =====================================================
// CLOCK FIX (REAL)
// =====================================================
void drawClock(){

  struct tm t;
  if(!getLocalTime(&t)){
    canvas.setCursor(40,60);
    canvas.print("SYNC...");
    return;
  }

  canvas.setTextSize(2);
  canvas.setCursor(40,60);

  if(t.tm_hour<10)canvas.print("0");
  canvas.print(t.tm_hour);
  canvas.print(":");

  if(t.tm_min<10)canvas.print("0");
  canvas.print(t.tm_min);
  canvas.print(":");

  if(t.tm_sec<10)canvas.print("0");
  canvas.print(t.tm_sec);
}

// =====================================================
// RADAR FULL FIX
// =====================================================
void drawRadar(){

  int cx=120, cy=70;

  canvas.drawCircle(cx,cy,20,GREEN);
  canvas.drawCircle(cx,cy,40,GREEN);
  canvas.drawCircle(cx,cy,60,GREEN);

  canvas.drawLine(cx-60,cy,cx+60,cy,GREEN);
  canvas.drawLine(cx,cy-60,cx,cy+60,GREEN);

  radarAngle += 0.05;

  canvas.drawLine(cx,cy,
    cx+cos(radarAngle)*60,
    cy+sin(radarAngle)*60,
    GREEN);

  if(netCount<=0){
    canvas.setCursor(40,60);
    canvas.print("NO SIGNAL");
    return;
  }

  for(int i=0;i<netCount && i<20;i++){
    canvas.fillCircle(radarX[i],radarY[i],3,GREEN);
  }
}

// =====================================================
// SECURITY
// =====================================================
void drawSecurity(){
  canvas.setTextSize(2);
  canvas.setCursor(30,60);
  canvas.print("SECURITY");

  canvas.setCursor(30,80);
  canvas.print("COMING SOON");
}

// =====================================================
// MENU
// =====================================================
void drawMenu(){
  String item[4]={"WiFi","Radar","Clock","Security"};

  scrollVel+=(scrollTarget-scrollY)*0.15;
  scrollVel*=0.75;
  scrollY+=scrollVel;

  for(int i=0;i<4;i++){
    int y=20+i*35-scrollY;

    if(i==menuIndex)
      canvas.fillRoundRect(30,y,170,28,6,0x0200);
    else
      canvas.drawRoundRect(30,y,170,28,6,GREEN);

    canvas.setCursor(50,y+8);
    canvas.print(item[i]);
  }
}

// =====================================================
// WIFI MENU
// =====================================================
void drawWifiMenu(){
  const char* item[2]={"SCAN","LIST"};

  for(int i=0;i<2;i++){
    int y=60+i*30;

    if(i==wifiMenuIndex)
      canvas.fillRoundRect(30,y,180,25,6,0x0200);
    else
      canvas.drawRoundRect(30,y,180,25,6,GREEN);

    canvas.setCursor(60,y+7);
    canvas.print(item[i]);
  }
}

// =====================================================
// WIFI LIST
// =====================================================
void drawWifiList(){
  for(int i=0;i<netCount && i<6;i++){
    int y=25+i*18;

    if(i==wifiIndex)
      canvas.fillRect(10,y-10,220,16,0x0100);

    canvas.setCursor(15,y);
    canvas.print(ssid[i]);
  }
}

// =====================================================
// PASS
// =====================================================
void drawPass(){
  canvas.setCursor(10,50);
  canvas.print("SSID:");
  canvas.print(ssid[wifiIndex]);

  canvas.setCursor(10,80);
  canvas.print("PASS:");
  canvas.print(pass);
  canvas.print("_");
}

// =====================================================
// INPUT FIX FULL
// =====================================================
void handleInput(){

  auto st=M5Cardputer.Keyboard.keysState();

  if(!st.word.empty()){
    char k=tolower(st.word[0]);

    if(k==lastKey)return;
    lastKey=k;

    if(k=='\n'||k=='\r')return;

    if(k=='b')screen=MENU;

    if(screen==MENU){
      if(k=='w')menuIndex=(menuIndex+3)%4;
      if(k=='s')menuIndex=(menuIndex+1)%4;

      scrollTarget=menuIndex*35;

      if(k=='e'){
        transX=240;
        transitioning=true;

        if(menuIndex==0)screen=WIFI_MENU;
        if(menuIndex==1)screen=RADAR;
        if(menuIndex==2)screen=CLOCK;
        if(menuIndex==3)screen=SECURITY;
      }
    }

    else if(screen==WIFI_MENU){
      if(k=='e'){
        if(wifiMenuIndex==0){
          startScan();
          screen=WIFI_SCAN;
        }else screen=WIFI_LIST;
      }
    }

    else if(screen==WIFI_LIST){
      if(netCount>0){
        if(k=='w')wifiIndex=(wifiIndex-1+netCount)%netCount;
        if(k=='s')wifiIndex=(wifiIndex+1)%netCount;

        if(k=='e'){
          pass="";
          screen=WIFI_PASS;
        }
      }
    }

    else if(screen==WIFI_PASS){
      if(k=='e'){
        WiFi.begin(ssid[wifiIndex].c_str(),pass.c_str());
        screen=MENU;
      }
      else if(k=='b'){
        if(pass.length())pass.remove(pass.length()-1);
      }
      else{
        if(k>=32 && k<=126)pass+=k;
      }
    }
  }else lastKey=0;
}

// =====================================================
// SETUP
// =====================================================
void setup(){

  auto cfg=M5.config();
  M5.begin(cfg);
  M5Cardputer.begin();

  canvas.createSprite(240,135);

  bootT=millis();
  initTime();

  WiFi.mode(WIFI_STA);

  for(int i=0;i<80;i++){
    mx[i]=random(0,240);
    my[i]=random(0,135);
    mv[i]=random(1,3);
  }

  for(int i=0;i<10;i++){
    radarX[i]=random(40,200);
    radarY[i]=random(30,110);
    radarStrength[i]=random(-90,-40);
  }
}

// =====================================================
// LOOP
// =====================================================
void loop(){

  M5Cardputer.update();

  canvas.fillScreen(BLACK);

  drawMatrix();
  handleInput();
  updateScan();

  if(screen==BOOT){
    canvas.setTextSize(2);
    canvas.setCursor(50,60);
    canvas.print("CYBER OS");

    if(millis()-bootT>1500)screen=MENU;
  }

  if(screen==MENU)drawMenu();
  if(screen==WIFI_MENU)drawWifiMenu();
  if(screen==WIFI_LIST)drawWifiList();
  if(screen==WIFI_PASS)drawPass();
  if(screen==CLOCK)drawClock();
  if(screen==RADAR)drawRadar();
  if(screen==SECURITY)drawSecurity();

  drawBattery();

  canvas.pushSprite(0,0);

  delay(16);
}