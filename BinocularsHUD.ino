/**
d  float精度不够，可考虑arduino due
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
**/
/*
http://item.taobao.com/item.htm?id=43511899945
Test on mega2560.
JY901   mega2560
TX <---> 0(Rx)
*/
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

//天体地平纬度Altitude，h
float Altitude;

//本地时角，N->W
float Local_Hour_A;

//望远镜指向的赤纬
float Astro_HUD_RA;

//望远镜指向的赤经  Astro_HUD_DEC=Sidereal_Time_Local-H
float Astro_HUD_DEC;

//儒略日和简化儒略日
double JD, MJD;

//本地时间
float Year, Month, Day, Hour, Minute, Second;

//格林尼治恒星时
double Siderial_Time;

//本地恒星时 LST
double Siderial_Time_Local;


void setup() {
  //启动串口
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial.println("Caculating.........");
  //提供观测者基础数据
  //观测者所在纬度
  Latitude = 36.10;
  //观测者所在经度
  Longitude = 120.34;

}
void loop() {
 

  //以下获得JY901实时方位角和地平维度基础数据


 //获得JY901角度数据
//Serial.print("Angle:"); 
//  Serial.print(-1*(float)JY901.stcAngle.Angle[0] / 32768 * 180); 
//  float az=(float)JY901.stcAngle.Angle[2] / 32768 * 180;
//  if (az<=0)
//    {az=-1*az;}
//  else
//    {az=360-az;}
//  Serial.print(" "); Serial.println(az);

  //JY901数据（ROLL,PITCH,YAW）与A方位角/地平纬度的关系变换

float jy_pitch=(float)JY901.stcAngle.Angle[0]/32768*180;

float jy_yaw=(float)JY901.stcAngle.Angle[2] / 32768 * 180;
 if (jy_yaw<=0)
    {jy_yaw=-1*jy_yaw;}
  else
    {jy_yaw=360-jy_yaw;}


Serial.print("Azimuth");
Serial.print(jy_yaw);
Serial.print("         ");
Serial.print("Altitude");
Serial.print(jy_pitch);
Serial.print("         ");
  //测试用方位角（弧度），获取和计算,假设为0
  Azimuth = jy_yaw*2 * PI / 360;

  //测试用地平纬度（弧度），获取和计算，假设为65度
  Altitude = jy_pitch* 2 * PI / 360;

  //以下获得实时时间
  /*
    RTC获取数据
  */

  //测试用时间
  Year = 2017;
  Month = 2;
  Day = 25;
  Hour = 20;
  Hour = Hour - 8;
  // Serial.println(Hour);
  Minute = 0;
  Second = 0;

  //儒略日，计算采用Navy.mil的计算试试看
  JD = 367 * Year - int((7 * (Year + int((Month + 9) / 12))) / 4) + int((275 * Month) / 9) + Day + 1721013.5 + Hour / 24 + Minute / 1440 + Second / 86400 - 0.5 * ((((100 * Year + Month - 190002.5) > 0) - ((100 * Year + Month - 190002.5) < 0))) + 0.5;
  Serial.print("JD is   ");
  Serial.println(JD, 6);
  //简化儒略日，计算
  MJD = JD - 2400000.5;
  Serial.print("MJD is   ");
  Serial.println(MJD, 6);
  //格林尼治恒星时
  //    Siderial_Time = 6.697374558 + 0.06570982441908 * (JD - Hour / 24 - 2451545.0) + 1.00273790935 * Hour + 0.000026 * (JD - 2451545.0) * (JD - 2451545.0) / (36525 * 36525);
  //http://aa.usno.navy.mil/faq/docs/GAST.php
  Siderial_Time = 18.697374558 + 24.06570982441908 * (JD - 2451545.0);
  while (Siderial_Time < 0.0)
    Siderial_Time += 24.0;
  while (Siderial_Time > 24.0)
    Siderial_Time -= 24.0;
  Serial.print("GST is: ");
  Serial.print("  ");
  Serial.print(Siderial_Time, 6);
  Serial.print("  ");
  //本地恒星时
   Siderial_Time_Local = Siderial_Time + Longitude/15;

  while (Siderial_Time_Local < 0.0)
    Siderial_Time_Local += 24.0;
  while (Siderial_Time_Local > 24.0)
    Siderial_Time_Local -= 24.0;
  Serial.print("LST is: ");
  Serial.print("  ");
  Serial.print(Siderial_Time_Local, 6);
  Serial.print("  ");
  //计算赤经
  Astro_HUD_RA = Siderial_Time_Local-atan(sin(Azimuth) / ( cos(Azimuth) * sin(Latitude*(2*PI/360)) + tan(Altitude) * cos(Latitude*(2*PI/360)) ))*180/(PI*15) ;
  //计算赤纬δ = 赤纬。天赤道以北为正，以南为负。
  Astro_HUD_DEC = asin(sin(Latitude*2*PI/360) * sin(Altitude) + cos(Latitude*(2*PI/360)) * cos(Altitude) * cos(Azimuth))*360/(2*PI);
  // Siderial_Time_Local * 15 -
  //* 360 / (2 * PI) ;
  Serial.print("RA:");
  Serial.print(Astro_HUD_RA);
  Serial.print("  ");
  Serial.print("Dec:");
  Serial.println(Astro_HUD_DEC);
  delay(2000);
    while (Serial1.available()) 
  {
    JY901.CopeSerialData(Serial1.read()); //Call JY901 data cope function
  }
  
}
