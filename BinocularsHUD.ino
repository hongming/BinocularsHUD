/**
  float精度不够，从Mega换为Arduino due
  计算公式来源：
  1，http://www.fjptsz.com/xxjs/xjw/rj/117/12.htm
  地平坐标转到赤道坐标
  tan(H) = sin(A)/( cos(A)*sin(Latitude) +tan(h)*cos(Latitude) )
  sin(Astro_HUD_RA)= sin(Latitude)*sin(h) - cos(Latitude)*cos(h)*cos(A)
  2，恒星时 	https://zh.wikipedia.org/wiki/%E6%81%92%E6%98%9F%E6%97%B6
  3,GMST恒星时 http://aa.usno.navy.mil/faq/docs/GAST.php
  4，JD时间:http://aa.usno.navy.mil/faq/docs/JD_Formula.php
  5，查询本地时间 http://aa.usno.navy.mil/data/docs/siderealtime.php
  6，RTC库文件，使用https://github.com/adafruit/RTClib
  使用JY901库文件
  尝试使用tinygps++获取位置和时间
  计划加入磁力计校正
**/
/*姿态板信息
  http://item.taobao.com/item.htm?id=43511899945
  Test on mega2560.
  JY901   mega2560
  TX <---> 0(Rx)
  屏幕：https://github.com/olikraus/u8g2
*/
/*
  数据计算来自：磁偏角 https://www.ngdc.noaa.gov/geomag-web/#declination
*/
/* Example sketch for interfacing with the DS1302 timekeeping chip.

  Copyright (c) 2009, Matt Sparks
  All rights reserved.

  https://github.com/msparks/arduino-ds1302
*/


//U8G2显示屏幕配置文件
#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

//引入库
#include <Wire.h>
#include <JY901.h>  //JY901姿态板
#include "Math.h"
#include <stdio.h>
#include <DS1302.h> //时钟库


//以下定义变量

//观测者所在经纬度
double Longitude;
double Latitude;
//观测者所在位置的磁偏角
float Magnetic_Delination;
//天体方位角Azimuth，以北方为起点，N->W,A
float Azimuth;

//天体地平纬度Altitude，h
float Altitude;

//本地时角，N->W
float Local_Hour_A;

//望远镜指向的赤纬，浮点数和拆分后的整数
float Astro_HUD_RA;
int Mod_RA_HH;
int Mod_RA_MM;
int Mod_RA_SS;
//望远镜指向的赤经，浮点数和拆分后的整数 Astro_HUD_DEC=Sidereal_Time_Local-H
float Astro_HUD_DEC;
int Mod_DEC_DD;
int Mod_DEC_MM;
int Mod_DEC_SS;

//儒略日和简化儒略日
double JD, MJD;

//本地时间
float Year, Month, Day, Hour, Minute, Second;

//格林尼治恒星时
double Siderial_Time;

//本地恒星时 LST
double Siderial_Time_Local;

//RTC时间相关
namespace {

// Set the appropriate digital I/O pin connections. These are the pin
// assignments for the Arduino as well for as the DS1302 chip. See the DS1302
// datasheet:
//
//   http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
const int kCePin   = 5;  // Chip Enable
const int kIoPin   = 6;  // Input/Output
const int kSclkPin = 7;  // Serial Clock

// Create a DS1302 object.
DS1302 rtc(kCePin, kIoPin, kSclkPin);

}

//将串口获得的数据，存储到一个字符串内，在Stellarium内使用
char inChar;
String Stellarium = "";

/*
  其他信息
  屏幕显示偏转270度，https://github.com/olikraus/u8g2/wiki/u8g2reference#setdisplayrotation
*/

void setup() {
  //启动图形库
  u8g2.begin();
  //启动串口
  Serial.begin(9600);
  //启动姿态板串口
  Serial1.begin(9600);
  //提供观测者所在纬度、经度
  Latitude = 31.0456;
  Longitude = 121.3997;
  //观测者所在位置的磁偏角
  Magnetic_Delination = 5.9;
  // Magnetic_Delination=0;
  //启动RTC、设置时间
  // Initialize a new chip by turning off write protection and clearing the
  // clock halt flag. These methods needn't always be called. See the DS1302
  // datasheet for details.
  rtc.writeProtect(false);
  rtc.halt(false);

  //  以下仅用于重置时钟的时间，平时需注释掉
  // Make a new time object to set the date and time.
  // Sunday, September 22, 2013 at 01:38:50.
  //  Time t(2017, 6, 12, 1, 22,35, Time::kSunday);
  // Set the time and date on the chip.
  //  rtc.time(t);
}
void loop() {

  //以下获得JY901实时方位角和地平维度基础数据,JY901数据（ROLL,PITCH,YAW）与A方位角/地平纬度的关系变换

  float jy_yaw = 180.0 - Magnetic_Delination - (float)JY901.stcAngle.Angle[2] / 32768 * 180;
  float jy_pitch = -1 * (float)JY901.stcAngle.Angle[0] / 32768 * 180;
  /*
    显示原始方位角、高度角，隐藏
    Serial.print("Azimuth");
    Serial.print(jy_yaw);
    Serial.print("         ");
    Serial.print("Altitude");
    Serial.print(jy_pitch);
    Serial.print("         ");
  */
  //测试用方位角（弧度），获取和计算,假设为0
  Azimuth = jy_yaw * 2 * PI / 360;

  //测试用地平纬度（弧度），获取和计算，假设为65度
  Altitude = jy_pitch * 2 * PI / 360;

  //以下获得实时时间
  Time t = rtc.time();

  //时间按格式分拆
  Year = t.yr;
  Month = t.mon;
  Day = t.date;
  Hour = t.hr;
  Hour = Hour - 8;
  Minute = t.min;
  Second = t.sec;

  //儒略日，计算采用Navy.mil的计算试试看
  JD = 367 * Year - int((7 * (Year + int((Month + 9) / 12))) / 4) + int((275 * Month) / 9) + Day + 1721013.5 + Hour / 24 + Minute / 1440 + Second / 86400 - 0.5 * ((((100 * Year + Month - 190002.5) > 0) - ((100 * Year + Month - 190002.5) < 0))) + 0.5;
  //  Serial.print("JD is   ");
  //  Serial.print(JD, 6);
  //简化儒略日，计算
  MJD = JD - 2400000.5;
  //  Serial.print("     MJD is   ");
  //  Serial.println(MJD, 6);
  //格林尼治恒星时
  //    Siderial_Time = 6.697374558 + 0.06570982441908 * (JD - Hour / 24 - 2451545.0) + 1.00273790935 * Hour + 0.000026 * (JD - 2451545.0) * (JD - 2451545.0) / (36525 * 36525);
  //http://aa.usno.navy.mil/faq/docs/GAST.php
  Siderial_Time = 18.697374558 + 24.06570982441908 * (JD - 2451545.0);
  while (Siderial_Time < 0.0)
    Siderial_Time += 24.0;
  while (Siderial_Time > 24.0)
    Siderial_Time -= 24.0;
  //  Serial.print("GST is: ");
  //  Serial.print("  ");
  //  Serial.print(Siderial_Time, 6);
  //  Serial.print("  ");
  //本地恒星时
  Siderial_Time_Local = Siderial_Time + Longitude / 15;

  while (Siderial_Time_Local < 0.0)
    Siderial_Time_Local += 24.0;
  while (Siderial_Time_Local > 24.0)
    Siderial_Time_Local -= 24.0;
  /*
     打印LST本地时间
      Serial.print("LST is: ");
      Serial.print("  ");
      Serial.print(Siderial_Time_Local, 6);
      Serial.print("  ");
  */
  //计算赤经
  Astro_HUD_RA = Siderial_Time_Local - atan(sin(Azimuth) / ( cos(Azimuth) * sin(Latitude * (2 * PI / 360)) - tan(Altitude) * cos(Latitude * (2 * PI / 360)) )) * 180 / (PI * 15) ;
  Mod_RA_HH = int(Astro_HUD_RA);
  Mod_RA_MM = int(fmod(Astro_HUD_RA, 1) * 60);
  Mod_RA_MM = int(Mod_RA_MM / 10) * 10; //在精度允许范围内，以10分钟为基础分段显示
  Mod_RA_SS = int((Astro_HUD_RA - Mod_RA_HH - Mod_RA_MM / 60) * 60);

  //计算赤纬δ = 赤纬。天赤道以北为正，以南为负。
  Astro_HUD_DEC = asin(sin(Latitude * 2 * PI / 360) * sin(Altitude) + cos(Latitude * (2 * PI / 360)) * cos(Altitude) * cos(Azimuth)) * 360 / (2 * PI);
  Mod_DEC_DD = int(Astro_HUD_DEC);
  Mod_DEC_MM = int(fmod(abs(Astro_HUD_DEC), 1) * 60);
  Mod_DEC_MM = int(Mod_DEC_MM / 10) * 10; //在精度允许范围内，以10角分为基础分段显示
  Mod_DEC_SS = int((abs(Astro_HUD_DEC) - abs(Mod_DEC_DD) - Mod_DEC_MM / 60) * 60);

  /*串口输出方位角、高度角等信息
    Serial.print("RA:");
    Serial.print(Astro_HUD_RA);
    Serial.print("->");
    Serial.print(Mod_RA_HH);
    Serial.print(" h ");
    Serial.print(Mod_RA_MM);
    Serial.print(" m ");
    Serial.print(Mod_RA_SS);
    Serial.print(" s");
    Serial.print("DEC:");
    Serial.print(Astro_HUD_DEC);
    Serial.print("->");
    Serial.print(Mod_DEC_DD);
    Serial.print(" d ");
    Serial.print(Mod_DEC_MM);
    Serial.print(" m ");
    Serial.print(Mod_DEC_SS);
    Serial.println(" s");
  */

  //读取Stellarium数据
  while (Serial.available() > 0) {
    inChar = Serial.read();
    Stellarium += String(inChar);
    delay(5);
  }

  //向Stellarium传送RA赤经值
  if (Stellarium == "#:GR#") {
    if (Mod_RA_HH < 10) {
      Serial.print("0");
    }
    Serial.print(Mod_RA_HH);
    Serial.print(":");
    if (Mod_RA_MM < 10) {
      Serial.print("0");
    }
    Serial.print(Mod_RA_MM);
    Serial.print(":00#");
    /*
      不要Mod_RA_SS秒
        if (Mod_RA_SS < 10) {
          Serial.print("0");
        }
        Serial.print(Mod_RA_SS);
        Serial.print("#");
    */
    Stellarium = "";
  }
  //向Stellarium传送DEC赤经值
  if (Stellarium == "#:GD#") {
    if ((Mod_DEC_DD >= 0 && Mod_DEC_DD < 10) ) {
      Serial.print ("+0");
      Serial.print(Mod_DEC_DD);
    } else if (Mod_DEC_DD >= 10) {
      Serial.print ("+");
      Serial.print(Mod_DEC_DD);
    }
    else if ((Mod_DEC_DD < 0) && abs(Mod_DEC_DD) < 10 ) {
      Serial.print("-0");
      Serial.print(abs(Mod_DEC_DD));
    } else {
      Serial.print(Mod_DEC_DD);
    }
    Serial.print("*");
    if (Mod_DEC_MM < 10) {
      Serial.print("0");
    }
    Serial.print(Mod_DEC_MM);
    Serial.print(":00#");
    /*
      不要Mod_DEC_SS角秒
        if (Mod_DEC_SS < 10) {
          Serial.print("0");
        }
        Serial.print(Mod_DEC_SS);
        Serial.print("#");
    */
    Stellarium = "";
  }

  //在屏幕上显示

  u8g2.firstPage();
  do {
    //打印RA赤纬
    u8g2.setCursor(1, 15);
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.print(F("R"));
    u8g2.setCursor(10, 15);
    u8g2.setFont(u8g2_font_helvB12_tf);
    u8g2.print(Mod_RA_HH);
    u8g2.setCursor(30, 15);
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.print(F("h"));
    u8g2.setCursor(37, 15);
    u8g2.print(Mod_RA_MM);
    u8g2.setCursor(50, 15);
    u8g2.print(F("m"));
    //打印DEC赤经
    u8g2.setCursor(69, 15);
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.print(F("D"));
    u8g2.setCursor(77, 15);
    u8g2.setFont(u8g2_font_helvB12_tf);
    u8g2.print(Mod_DEC_DD);
    u8g2.setCursor(99, 15);
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawGlyph(99, 15, 0x00b0);
    u8g2.setCursor(106, 15);
    u8g2.print(Mod_DEC_MM);
    u8g2.setCursor(125, 15);
    u8g2.setFont(u8g2_font_helvB12_tf);
    u8g2.print(F("'"));

    //打印地平圈和方位角
    u8g2.drawCircle(10, 40, 10, U8G2_DRAW_ALL);
    u8g2.drawLine(10, 40, 10 - 10 * sin(Azimuth), 40 + 10 * cos(Azimuth));
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.setCursor(30, 50);
    u8g2.print(jy_yaw);

    //打印高度角
    u8g2.drawCircle(69, 50, 20, U8G2_DRAW_UPPER_RIGHT);
    u8g2.drawLine(69, 50, 89, 50);
    u8g2.drawLine(69, 50, 69, 30);
    u8g2.drawLine(69, 50, 69 + 20 * cos(Altitude), 50 - 20 * sin(Altitude));
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.setCursor(95, 50);
    u8g2.print(jy_pitch);

    //打印时间
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.setCursor(21, 63);
    u8g2.print(t.mon);
    u8g2.setCursor(30, 63);
    u8g2.print(t.date);
    u8g2.setCursor(50, 63);
    u8g2.print(t.hr);
    u8g2.setCursor(70, 63);
    u8g2.print(t.min);
    u8g2.setCursor(90, 63);
    u8g2.print(t.sec);
  } while ( u8g2.nextPage() );

  delay(10);

  //获取姿态板数据
  while (Serial1.available())
  {
    JY901.CopeSerialData(Serial1.read()); //Call JY901 data cope function
  }

}

