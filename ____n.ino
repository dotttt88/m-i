#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <Wire.h>                 //Thư viện giao tiếp I2C
#include <LiquidCrystal_I2C.h>    //Thư viện giao tiếp LCD theo chuẩn I2C
LiquidCrystal_I2C lcd(0x27,16,2); //Khai báo địa chỉ và thông tin LCD
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <EEPROM.h>
#include "HtmlHomePage.h"
#include "HtmlLoginPage.h"
#define DHTPIN 23
#define output1 25    //Bóng đèn
#define output2 26    //Phun sương
#define output3 27    //Quạt hút
#define output4 33    //Đèn cảnh báo
#define bt_set 16
#define bt_up  17
#define bt_down 18
#define bt_ok 19
#define buzzer 14     
#define timeReconnectWL 300000L         //Thời gian reconnect wifi là 5 phút
//#define timeReconnectBL 300000L         //Thời gian reconnect bynk là 5 phút
//#define timeUpdateBL 1000L              //Thời gian gửi dữ liệu lên blynk
#define timeUpdateWeb 1000L             //Thời gian gửi socket lên web
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

WebServer webServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);
String ssid_ap="DO-1"; 
String pass_ap="12345678";
String user_login="admin"; 
String pass_login="admin";
String ssid_sta="Wifi-Tang-5";  //Tên wifi sẽ kết nối tới
String pass_sta="so11ngach28ngo67";   //Mật khẩu wifi sẽ kết nối
boolean reConnectState=0;
boolean ledConnectWeb=0;
boolean reStoreConfigState=0;
unsigned long times1;                   //Thời gian reConnect
unsigned long times2;                   //Thời gian update socket lên web
//unsigned long times3;                   //Thời gian update dữ liệu lên blynk
unsigned long time_angiu;
unsigned long time_angiu_set;

float t,h;
float gioihan_tmin=37.5;  
float gioihan_tmax=38.5;
float gioihan_hmin=60;
float gioihan_hmax=80;
float giatrihieuchinh=0;
float muc_quanhiet=40;
float muc_quadoam=80;
int ngay=0;
int ngayover=0;
boolean setupMode=0;
int timerID1;
boolean bt_setState=1;
boolean bt_upState=1;
boolean bt_downState=1;
boolean bt_okState=1;
boolean canhbao_quanhiet=0;
boolean canhbao_quangay=0;
boolean canhbao_quadoam=0;
unsigned long time_canhbao=millis();
byte setnum=1;
byte degree[8] = {
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

void setup()
{
  Serial.begin(9600);
  EEPROM.begin(512);
  pinMode(output1,OUTPUT);
  pinMode(output2,OUTPUT);
  pinMode(output3,OUTPUT);
  pinMode(buzzer,OUTPUT);
  pinMode(output4,OUTPUT);
  pinMode(bt_set,INPUT_PULLUP);
  pinMode(bt_up,INPUT_PULLUP);
  pinMode(bt_down,INPUT_PULLUP);
  pinMode(bt_ok,INPUT_PULLUP);  
  digitalWrite(output1,HIGH);
  digitalWrite(output2,HIGH);
  digitalWrite(output3,HIGH);
  digitalWrite(buzzer,LOW);
  digitalWrite(output4,HIGH);
  //-------------------------------------------------
  reStoreConfig();
  reStoreConfigState=1;
  startWebServer();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  //-------------------------------------------------------
  dht.begin();

  
  timer.setInterval(2000L, readSensor);
  setSyncInterval(60*60);
  timer.setInterval(60000L,readTime);
  Wire.begin(21,22);         //Khởi tạo chân kết nối I2C
  lcd.init();                //Khởi tạo LCD
  lcd.clear();               //Xóa màn hình
  lcd.backlight();           //Bật đèn nền cho LCD
  lcd.createChar(0, degree);   //Tạo ký tự lưu vào byte thứ 0
  lcd.setCursor(2,0);        //Cột 2, dòng 0
  lcd.print("Welcom to");    //Ghi chữ bắt đầu từ cột thứ 2 dòng 0
  lcd.setCursor(0,1);        //Cột 0, dòng thứ 1
  lcd.print("E-Smart Channel!");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("IP Connected:");
  lcd.setCursor(1,1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear(); 
  homeDisplay();
}
void loop(){
  webServer.handleClient();
  webSocket.loop();
  timer.run();
  //------------------------------------------------------------------
  //-----------Update socket lên web-----------------------------------------------------------//
  if(millis()-times2>timeUpdateWeb){
    updateWebserver();
    times2=millis();
  }
  
  //--------------------------------------------------
  if(canhbao_quanhiet==1||canhbao_quangay==1||canhbao_quadoam==1){
    if(millis()-time_canhbao>200){
      time_canhbao=millis();
      beep();
      digitalWrite(output4,!digitalRead(output4));
    }
  }else{
    digitalWrite(output4,HIGH);
  }
  if(digitalRead(bt_set)==LOW){
    if(bt_setState==1){
      bt_setState=0;   
      beep();
      if(setupMode==0){
        setupMode=1;
        lcd.clear(); 
        setnum=1;   
      }else{
        setupMode=0;
        homeDisplay();
        save_setup();
      }
      Serial.print("Mode: ");Serial.println(setupMode);
    }
  }else{
    bt_setState=1;
  }
  if(setupMode==1){
    if(digitalRead(bt_ok)==LOW){
      if(bt_okState==1){
        beep();
        bt_okState=0;
        lcd.clear(); 
        if(setnum<8){
          setnum++;
        }else{
          setnum=1;
        }
        Serial.print("Set num: "); Serial.println(setnum);
      }
    }else{
      bt_okState=1;
    }
    if(setnum==1){
      lcd.setCursor(0,0);        
      lcd.print("Nhiet do Min: ");
      lcd.setCursor(0,1); 
      gioihan_tmin = setupValue(gioihan_tmin,30.0, 40.0); 
          
      lcd.print(gioihan_tmin,1);
      lcd.write(0);
      lcd.print("C");
      lcd.setCursor(9,1);
      lcd.print("30<T<40");
    }else if(setnum==2){
      lcd.setCursor(0,0);        
      lcd.print("Nhiet do Max: ");
      lcd.setCursor(0,1);  
      gioihan_tmax = setupValue(gioihan_tmax,35.0, 45.0);    
      lcd.print(gioihan_tmax,1);    
      lcd.write(0);
      lcd.print("C");
      lcd.setCursor(9,1);
      lcd.print("35<T<45");
    }else if(setnum==3){
      lcd.setCursor(0,0);        
      lcd.print("Nhiet do Over: ");
      lcd.setCursor(0,1);  
      muc_quanhiet = setupValue(muc_quanhiet,40.0, 50.0); 
         
      lcd.print(muc_quanhiet,1);
      lcd.write(0);
      lcd.print("C");
      lcd.setCursor(9,1);
      lcd.print("40<T<50");
    }else if(setnum==4){
      lcd.setCursor(0,0);        
      lcd.print("Do am min: ");
      lcd.setCursor(0,1);  
      gioihan_hmin = setupValue2(gioihan_hmin,40, 80);     
      lcd.print(gioihan_hmin,0);
      lcd.print("% ");
      lcd.setCursor(9,1);
      lcd.print("40<H<80");
    }else if(setnum==5){
      lcd.setCursor(0,0);        
      lcd.print("Do am max: ");
      lcd.setCursor(0,1);  
      gioihan_hmax = setupValue2(gioihan_hmax,60, 80);     
      lcd.print(gioihan_hmax,0);
      lcd.print("% ");
      lcd.setCursor(8,1);
      lcd.print("60<H<80");
    }else if(setnum==6){
      lcd.setCursor(0,0);        
      lcd.print("Do am over: ");
      lcd.setCursor(0,1);  
      muc_quadoam = setupValue2(muc_quadoam,80, 100);     
      lcd.print(muc_quadoam,0);
      lcd.print("% ");
      lcd.setCursor(8,1);
      lcd.print("80<H<100");
    }else if(setnum==7){
      lcd.setCursor(0,0);        
      lcd.print("So ngay ap: ");
      lcd.setCursor(0,1);  
      ngay = setupValue2(ngay,15, 45);     
      lcd.print(ngay);
      lcd.print("  ");
      lcd.setCursor(9,1);
      lcd.print("15<D<45");
    }else if(setnum==8){
      lcd.setCursor(0,0);        
      lcd.print("Hieu Chinh T:");
      lcd.setCursor(0,1);
      giatrihieuchinh = setupValue(giatrihieuchinh,-10.0, 10.0);       
      lcd.print(giatrihieuchinh,1);
      lcd.write(0);
      lcd.print("C  ");
      lcd.setCursor(8,1);
      lcd.print("-10<T<10");
    }
  }else{
    if(digitalRead(bt_ok)==LOW){
      if(bt_okState==1){
        bt_okState=0;
        Serial.println("Button Ok push!");
        if(canhbao_quangay==1){
          canhbao_quangay=0;
          ngayover=0;
          EEPROM.write(29,ngayover);
          EEPROM.commit();
          digitalWrite(output4,HIGH);
        }
      }
      if(millis()-time_angiu>2000){
        time_angiu=millis();
        ngayover=0;
        EEPROM.write(29,ngayover);
        EEPROM.commit();
        digitalWrite(output4,HIGH);
        beep();
      }
    }else{
      bt_okState=1;
      time_angiu=millis();
    }
    
  }
}
//==================CHƯƠNG TRÌNH CON=====================

void save_setup(){
  writeFloat(0,gioihan_tmin);  
  writeFloat(4,gioihan_tmax);
  writeFloat(8,gioihan_hmin);
  writeFloat(12,gioihan_hmax);
  writeFloat(16,giatrihieuchinh);
  writeFloat(20,muc_quanhiet);
  writeFloat(24,muc_quadoam);
  EEPROM.write(28,ngay);
  EEPROM.commit();
  Serial.println("Đã lưu cài đặt");
}
void reStoreConfig(){
  gioihan_tmin=readFloat(0);  
  gioihan_tmax=readFloat(4);
  gioihan_hmin=readFloat(8);
  gioihan_hmax=readFloat(12);
  giatrihieuchinh=readFloat(16);
  muc_quanhiet=readFloat(20);
  muc_quadoam=readFloat(24);
  ngay = EEPROM.read(28);
  ngayover = EEPROM.read(29);
}
boolean checkConnectWifi(){
  Serial.print("Connecting to a wifi network");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_sta.c_str(),pass_sta.c_str());
  int n=0;
  while(n<20){
    if(WiFi.status()==WL_CONNECTED){
      Serial.println();Serial.print("CONNECTED TO "); Serial.println(ssid_sta);
      Serial.print("IP CONNECTED: ");Serial.println(WiFi.localIP());
      return true;
    }
    n++;
    delay(500);
    Serial.print(".");
  }
  Serial.println();Serial.println("The wifi connection process is timed out!");
  return false;
}

void startAccesspointMode(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap.c_str(),pass_ap.c_str(), 1, false,4);
  Serial.println("**----------ACCESSPOINT MODE ACTIVATED----------**");
  Serial.println("Please connect to the device's wifi to configure!");
  Serial.print("SSID ACCESSPOINT: ");Serial.println(ssid_ap);
  Serial.print("PASSWORD: "); Serial.println(pass_ap);
  Serial.print("URL WEB SERVER: "); Serial.println(WiFi.softAPIP());
}
//----------------------------------WEBSERVER-------------------------------------------------//
void startWebServer(){
  webServer.on("/",[]{
    String s = FPSTR(LoginPage);
    webServer.send(200,"text/html",s);
  });
  webServer.on("/login",[]{
    String s = FPSTR(LoginPage);
    webServer.send(200,"text/html",s);
  });
  webServer.on("/home",[]{
    String s = FPSTR(HomePage);
    webServer.send(200,"text/html",s);
  });
  webServer.onNotFound([]() {
    String s = FPSTR(HomePage);
    webServer.send(200,"text/html",s);
  });
  webServer.on("/getSystem",getSystem);
  webServer.on("/getSetup",getSetup);
  webServer.on("/saveSetup",saveSetup);
  webServer.on("/reStart",reStart);
  
  webServer.begin();
}
void getSystem(){
  String s = "{\"user_login\": \""+ user_login +"\"," +
              "\"pass_login\": \""+ pass_login +"\"}";
  webServer.send(200,"application/json",s);
  Serial.println(s);
}
void getSetup(){
  int h_min = gioihan_hmin;
  int h_max = gioihan_hmax;
  int h_over = muc_quadoam;
  String s = "{\"nhietdo_min\": \""+ String(gioihan_tmin) + "\"," +
                "\"nhietdo_max\": \""+ gioihan_tmax + "\"," +
                "\"nhietdo_over\": \""+ muc_quanhiet + "\"," +
                "\"doam_min\": \""+ h_min + "\"," +
                "\"doam_max\": \""+ h_max + "\"," +
                "\"doam_over\": \""+ h_over + "\"," +
                "\"songayap\": \""+ ngay + "\"," +
                "\"hieuchinh\": \""+ giatrihieuchinh +"\"}";
  webServer.send(200,"application/json",s);
  Serial.println(s);
}
void saveSetup(){
  gioihan_tmin = webServer.arg("t_min").toFloat();
  gioihan_tmax = webServer.arg("t_max").toFloat();
  muc_quanhiet = webServer.arg("t_over").toFloat();
  
  gioihan_hmin = webServer.arg("h_min").toInt();
  gioihan_hmax = webServer.arg("h_max").toInt();
  muc_quadoam = webServer.arg("h_over").toInt();
  giatrihieuchinh = webServer.arg("hieuchinh").toFloat();
  ngay = webServer.arg("songayap").toInt();
  
  writeFloat(0,gioihan_tmin);  
  writeFloat(4,gioihan_tmax);
  writeFloat(8,gioihan_hmin);
  writeFloat(12,gioihan_hmax);
  writeFloat(16,giatrihieuchinh);
  writeFloat(20,muc_quanhiet);
  writeFloat(24,muc_quadoam);
  EEPROM.write(28,ngay);
  EEPROM.commit();
  String s = "Đã thiết lập!";
  webServer.send(200,"text/html",s);
}
void reStart(){
  ESP.restart();
}
void webSocketEvent(uint8_t num, WStype_t type,uint8_t * payload,size_t length){
  String payloadString = (const char *)payload;
  Serial.print("payloadString= ");
  Serial.println(payloadString);
}
void updateWebserver(){
  ledConnectWeb = !ledConnectWeb;
  String JSONtxt = "{\"ledconnect\": \""+ String(ledConnectWeb) +"\"," +
                    "\"output1\": \""+ digitalRead(output1) + "\"," +
                    "\"output2\": \""+ digitalRead(output2) + "\"," +
                    "\"output3\": \""+ digitalRead(output3) + "\"," +
                    "\"songayconlai\": \""+ (ngay-ngayover) + "\"," +
                    "\"nhietdo\": \""+ t + "\"," +
                    "\"doam\": \""+ h + "\"}";
  webSocket.broadcastTXT(JSONtxt);
  Serial.println(JSONtxt);
}
void beep(){
  digitalWrite(buzzer,HIGH);
  delay(200);
  digitalWrite(buzzer,LOW);
}
float setupValue(float value, float min, float max){
  if(digitalRead(bt_up)==LOW){
    if(bt_upState==1){
      bt_upState=0;
      beep(); 
      if(value<max){
        value=value+0.1;
      }else{
        value=min;
      }
    }
  }else{
    bt_upState=1;
  }
  if(digitalRead(bt_down)==LOW){
    if(bt_downState==1){
      bt_downState=0; 
      beep();
      if(value>min){
        value=value-0.1;
      }else{
        value=max;
      }
    }
  }else{
    bt_downState=1;
  }
  return value;
}
float setupValue2(float value, float min, float max){
  if(digitalRead(bt_up)==LOW){
    if(bt_upState==1){
      bt_upState=0;
      beep(); 
      if(value<max){
        value=value+1;
      }else{
        value=min;
      }
    }
  }else{
    bt_upState=1;
  }
  if(digitalRead(bt_down)==LOW){
    if(bt_downState==1){
      bt_downState=0; 
      beep();
      if(value>min){
        value=value-1;
      }else{
        value=max;
      }
    }
  }else{
    bt_downState=1;
  }
  return value;
}
void homeDisplay(){
  lcd.clear();
  lcd.setCursor(0,0);        
  lcd.print("T: ");  
  lcd.setCursor(0,1);        
  lcd.print("H: ");  
  lcd.setCursor(11,0);        
  lcd.print("D_Off"); 
  lcd.setCursor(12,1);
  if(ngay<10){
    lcd.print("0"); 
    lcd.print(ngay-ngayover); 
  }else{
    lcd.print(ngay-ngayover);
  }
}
void readSensor()
{
   float temp_h;
  float temp_t;
  temp_h = dht.readHumidity();
  temp_t = dht.readTemperature(); 
  //temp_h = dht.getHumidity();
  //temp_t = dht.getTemperature(); 
  //h = 64.36;
  //t= 35.63 + giatrihieuchinh;
  if (isnan(temp_h) || isnan(temp_t)) {
  //if(dht.getStatusString()=="OK"){
   Serial.println("Failed to read from DHT sensor!");
  }else{
    h=temp_h;
    t=temp_t + giatrihieuchinh;
    Serial.print("Nhiệt độ: ");Serial.println(t);Serial.println(" *C");
    Serial.print("Độ ẩm: ");Serial.println(h);Serial.print(" %\t");
  }
  if(setupMode==0){
    lcd.setCursor(3,0);
    lcd.print(t,1);
    lcd.write(0);
    lcd.print("C");
    lcd.setCursor(3,1);
    lcd.print(h,0);
    lcd.print("%");
    lcd.setCursor(12,1);
    if(ngay<10){
      lcd.print("0"); 
      lcd.print(ngay-ngayover); 
    }else{
      lcd.print(ngay-ngayover);
    }
  } 
  Serial.print("Temp: ");Serial.println(t);
  Serial.print("Humi: ");Serial.println(h);
  if(t<gioihan_tmin){
    digitalWrite(output1,LOW);
    //Serial.println("Bật relay1");
    digitalWrite(output3,HIGH);
  }else if(t>gioihan_tmax){
    digitalWrite(output1,HIGH);
    //Serial.println("Tắt relay1");
  }
  if(h<gioihan_hmin){
    digitalWrite(output2,LOW);
    //Serial.println("Bật relay2");
  }else if(h>gioihan_hmax){
    digitalWrite(output2,HIGH);
    //Serial.println("Tắt relay2");
  }
  if(t>muc_quanhiet){
    digitalWrite(output1,HIGH);
    digitalWrite(output3,LOW);
    String notifyText = "CẢNH BÁO QUÁ NHIỆT esp32";
    canhbao_quanhiet=1;
  }else{
    canhbao_quanhiet=0;
  }
   if(h>muc_quadoam){
    digitalWrite(output2,HIGH);
    String notifyText = "CẢNH BÁO QUÁ ĐỘ ẨM esp32";
    canhbao_quadoam=1;
  }else{
    canhbao_quadoam=0;
  }
}
void readTime(){
  if(minute()==0&&hour()==7){
    if(ngayover<ngay){
      ngayover++;
      EEPROM.write(29,ngayover);
      EEPROM.commit();
      canhbao_quangay=0;
    }else{
      String notifyText = "QUÁ NGÀY ẤP TRỨNG RỒI BẠN NHÉ!esp32";
      canhbao_quangay=1;
    }
  }
}

//-------------Ghi dữ liệu kiểu float vào bộ nhớ EEPROM----------------------//
float readFloat(unsigned int addr){
  union{
    byte b[4];
    float f;
  }data;
  for(int i=0; i<4; i++){
    data.b[i]=EEPROM.read(addr+i);
  }
  return data.f;
}
void writeFloat(unsigned int addr, float x){
  union{
    byte b[4];
    float f;
    
  }data;
  data.f=x;
  for(int i=0; i<4;i++){
    EEPROM.write(addr+i,data.b[i]);
  }
}
