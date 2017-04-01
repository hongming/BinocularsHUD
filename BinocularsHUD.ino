/**
  float精度不够，可考虑arduino due
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
  增加屏幕
**/
/*
<<<<<<< HEAD
http://item.taobao.com/item.htm?id=43511899945
Test on mega2560.
JY901   mega2560
TX <---> 0(Rx)
屏幕：https://github.com/olikraus/u8g2
=======
  http://item.taobao.com/item.htm?id=43511899945
  Test on mega2560.
  JY901   mega2560
  TX <---> 0(Rx)
>>>>>>> origin/master
*/
//U8G2显示屏幕
#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

#include <Wire.h>
#include <JY901.h>  //JY901姿态板
#include "Math.h"
//定义变量
//观测者所在纬度
//观测者所在经度
double Longitude;
double Latitude;

//天体方位角Azimuth，N->W,A
float Azimuth;
float Azimuth_raw;

//天体地平纬度Altitude，h
float Altitude;
float Altitude_raw;

//本地时角，N->W
float Local_Hour_A;

//望远镜指向的赤纬
float Astro_HUD_RA;

//望远镜指向的赤经  Astro_HUD_DEC=Sidereal_Time_Local-H
float Astro_HUD_DEC;
int Astro_HUD_DEC_integer;
int Astro_HUD_DEC_decimal;

//儒略日和简化儒略日
double JD, MJD;

//本地时间
float Year, Month, Day, Hour, Minute, Second;

//格林尼治恒星时
double Siderial_Time;

//本地恒星时 LST
double Siderial_Time_Local;

//滤波
float Filter_Value_Pitch;
float Filter_Value_Yaw;
void setup() {
  //启动图形库
    u8g2.begin();
  //启动串口
  Serial.begin(9600);
  Serial1.begin(9600);
  //提供观测者基础数据
  //观测者所在纬度
  Latitude = 31.0456;
  //观测者所在经度
  Longitude = 121.3997;
randomSeed(analogRead(0)); 
}
void loop(void) {


  //以下获得JY901实时方位角和地平维度基础数据,JY901数据（ROLL,PITCH,YAW）与A方位角/地平纬度的关系变换

<<<<<<< HEAD
float jy_pitch=(float)JY901.stcAngle.Angle[0]/32768*180;
float jy_yaw=(float)JY901.stcAngle.Angle[2] / 32768 * 180;
=======
  float jy_pitch = (float)JY901.stcAngle.Angle[0] / 32768 * 180;

  float jy_yaw = (float)JY901.stcAngle.Angle[2] / 32768 * 180;
  // if (jy_yaw<=0)
  //    {jy_yaw=-1*jy_yaw;}
  //  else
  //    {jy_yaw=360-jy_yaw;}
>>>>>>> origin/master

  //  Serial.print("Azimuth");
  //  Serial.print(jy_yaw);
  //  Serial.print("         ");
  //  Serial.print("Altitude");
  //  Serial.print(jy_pitch);
  //  Serial.print("         ");
  //测试用方位角（弧度），获取和计算,假设为0
<<<<<<< HEAD
  Azimuth = jy_yaw*2 * PI / 360;
//  Azimuth=218.061611*2 * PI / 360;

  //测试用地平纬度（弧度），获取和计算，假设为65度
  Altitude = jy_pitch* 2 * PI / 360;
//  Altitude=66.642722* 2 * PI / 360;
=======
  Azimuth_raw=jy_yaw;
  Azimuth = Azimuth_raw * 2 * PI / 360;
  Serial.print("Azimuth");
  Serial.print("\t");
  Serial.print(Azimuth_raw);
  Serial.print("\t");
  //测试用地平纬度（弧度），获取和计算，假设为65度
  //  Altitude = jy_pitch* 2 * PI / 360;
  Altitude_raw=jy_pitch;
  Altitude = Altitude_raw * 2 * PI / 360;
  Serial.print("Altitude");
  Serial.print("\t");
  Serial.print(Altitude_raw);
  Serial.print("\t");
>>>>>>> origin/master
  //以下获得实时时间
  /*
    RTC获取数据
  */

  //测试用时间
  Year = 2017;
  Month = 3;
<<<<<<< HEAD
  Day = 23;
=======
  Day = 25;
>>>>>>> origin/master
  Hour = 23;
  Hour = Hour - 8;
  // Serial.println(Hour);
  Minute = 20;
  Second = 0;

<<<<<<< HEAD
=======
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
>>>>>>> origin/master
  //本地恒星时
  Siderial_Time_Local = Siderial_Time + Longitude / 15;
  //Siderial_Time_Local = Siderial_Time + 8.0;

  while (Siderial_Time_Local < 0.0)
    Siderial_Time_Local += 24.0;
  while (Siderial_Time_Local > 24.0)
    Siderial_Time_Local -= 24.0;
  //  Serial.print("LST is: ");
  //  Serial.print("  ");
  //  Serial.print(Siderial_Time_Local, 6);
  //  Serial.print("  ");
  //计算赤经
  Astro_HUD_RA = Siderial_Time_Local - atan(sin(Azimuth) / ( cos(Azimuth) * sin(Latitude * (2 * PI / 360)) - tan(Altitude) * cos(Latitude * (2 * PI / 360)) )) * 180 / (PI * 15) ;

  //对赤经数据做计算
  if (Azimuth_raw >= 0 && Azimuth_raw < 90) {
    Astro_HUD_RA = Astro_HUD_RA + 12;
  }
  else if (Azimuth_raw >= 270 && Azimuth_raw < 360) {
    if (Astro_HUD_RA + 12 > 24) {
      Astro_HUD_RA = Astro_HUD_RA - 12;
    } else
    {
      Astro_HUD_RA = Astro_HUD_RA + 12;
    }
  }

  //计算赤纬δ = 赤纬。天赤道以北为正，以南为负。
  Astro_HUD_DEC = asin(sin(Latitude * 2 * PI / 360) * sin(Altitude) + cos(Latitude * (2 * PI / 360)) * cos(Altitude) * cos(Azimuth)) * 360 / (2 * PI);
  //计算赤纬的整数和小数部分
  if (Astro_HUD_DEC >= 0) {
    Astro_HUD_DEC_integer = floor(Astro_HUD_DEC);
    Astro_HUD_DEC_decimal = (Astro_HUD_DEC - floor(Astro_HUD_DEC)) * 60;
  }
  else {
    Astro_HUD_DEC_integer = int(abs(Astro_HUD_DEC)) * (-1);
    Astro_HUD_DEC_decimal = abs((Astro_HUD_DEC + int(abs(Astro_HUD_DEC)))) * 60 ;
  }

  Serial.print("RA:");
  Serial.print("\t");
  Serial.print(int(floor(Astro_HUD_RA)));
  Serial.print("\t");
  Serial.print("h");
  Serial.print("\t");
  Serial.print(round((Astro_HUD_RA - floor(Astro_HUD_RA)) * 60));
  Serial.print("\t");
  Serial.print("m");
  Serial.print("\t");
  Serial.print(" Dec:");
<<<<<<< HEAD
  Serial.print(int(floor(Astro_HUD_DEC)));
  Serial.print("°");
  Serial.print(round((Astro_HUD_DEC-floor(Astro_HUD_DEC))*60));  
   Serial.println("'");
  delay(100);
  //在屏幕上显示
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.firstPage();
  do {
    u8g2.setCursor(5, 10);
    u8g2.print(jy_yaw);
    u8g2.setCursor(60, 10);
    u8g2.print(jy_pitch);
    u8g2.setCursor(5, 40);
    u8g2.print(F("RA: "));
    u8g2.setCursor(60, 40);
    u8g2.print(Astro_HUD_RA);   
    u8g2.setCursor(5, 60);
    u8g2.print(F("DEC: "));
    u8g2.setCursor(60, 60);
    u8g2.print(Astro_HUD_DEC);   
  } while ( u8g2.nextPage() );
  
    while (Serial1.available())
=======
  Serial.print("\t");
    Serial.print(Astro_HUD_DEC);
  Serial.print("\t");
  Serial.print(Astro_HUD_DEC_integer);
  Serial.print("#");
  Serial.print(Astro_HUD_DEC_decimal);
  Serial.println("'");

  delay(500);
  while (Serial1.available())
>>>>>>> origin/master
  {
    JY901.CopeSerialData(Serial1.read()); //Call JY901 data cope function
  }
}
