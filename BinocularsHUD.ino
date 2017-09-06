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
/*
  校正，几个思路
  1、使用上位机软件校正后，指向几个特定的方位，比如北极星、水平正南，修正方位角和高度角
  2、使用弼马温1984（陈）赠送的EQASCOM Alignment文档中的Nearest Point Transformation，调节访问同1
  3、3–Point Transformation，来源同上
  矩阵计算
  验证  http://matrix.reshish.com/multCalculation.php 和https://matrixcalc.org/zh/#%7B%7B195119%2F10000,1649%2F100,87941%2F5000%7D,%7B279667%2F10000,-%28264333%2F10000%29,62833%2F5000%7D,%7B1,1,1%7D%7D%2A%7B%7B31%2F50,-%281%2F50%29,-%28517%2F50%29%7D,%7B1%2F4,-%283%2F100%29,-%28369%2F100%29%7D,%7B-%2887%2F100%29,1%2F20,1503%2F100%7D%7D
  原理 https://en.wikipedia.org/wiki/Affine_transformation和https://zhuanlan.zhihu.com/p/23199679?utm_source=wechat_session&utm_medium=social

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
U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

//引入库
#include <Wire.h>
#include <JY901.h>  //JY901姿态板
#include "Math.h"
#include <stdio.h>
#include <DS1302.h> //时钟库
#include <MatrixMath.h> //矩阵预算库

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

//感应器的数值，jy_yaw_m为考虑了磁偏角后的方位角，jy_pitch为传感器的高度角
float jy_yaw, jy_pitch, jy_yaw_m;

//望远镜指向的赤纬、赤经，浮点数和拆分后的整数
float Astro_HUD_RA;
int Mod_RA_HH;
int Mod_RA_MM;
int Mod_RA_SS;
float Astro_HUD_DEC;
int Mod_DEC_DD;
int Mod_DEC_MM;
int Mod_DEC_SS;
//用电位器，做单星校准，RA、DEC人工校正值
int RA_AlignPin = A0;
int DEC_AlignPin = A1;
int RA_AlignPin_Offset = 0;
int DEC_AlignPin_Offset = 0;
float RA_AlignPin_Offset_F = 0;
float DEC_AlignPin_Offset_F = 0;

//目标矩阵--》星图的三颗星体的标准坐标，
float Matrix_Atlas[3][3];
//三颗校准星体的序号
int Alignment_Star_One, Alignment_Star_Two, Alignment_Star_Three;
//原始矩阵--》感应器对准的三颗星体后得到的原始坐标
float Matrix_Sensor[3][3];
//三颗校准星体的实际位置的序号，可能用不上
int Point_Star_One, Point_Star_Two, Point_Star_Three;
//转换矩阵,自动计算出来
float Matrix_T[3][3];
//转换矩阵,使用计算数据，纠正系统偏移
float Matrix_T_Offset[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
//屏幕显示矩阵
float Matrix_Atlas_Goto[3][1];
//传感器原始输出矩阵
float Matrix_Sensor_Goto[3][1];


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

//望远镜指向在The Cambridge Star Atlas中的星图区域（Index to the star charts）
int TCSA_Star_Chart;

//Nexstarsite.com上的Star list星体表中的Alignment Stars
void AlignmentStars_Print(const float [][3]);
const int AlignmentStars_rows = 40;
const int AlignmentStars_colums = 3;
String  AlignmentStars_Name;
float AlignmentStars_Altitude, AlignmentStars_Azimuth; //校准星体的实时地平高度角、方位角
float AlignmentStars_Array[AlignmentStars_rows][AlignmentStars_colums] = {
  {0, 2.10 , 29.10},
  {1, 17.43 , 35.60},
  {2, 297.70 , 8.90},
  {3, 31.80 , 23.50},
  {4, 79.18 , 46.00},
  {5, 213.93 , 19.20},
  {6, 96.00 , -52.70},
  {7, 2.30 , 59.10},
  {8, 14.18 , 60.70},
  {9, 219.90 , -60.80},
  {10, 210.95 , -60.40},
  {11, 101.28 , -16.70},
  {12, 114.83 , 5.20},
  {13, 233.68 , 26.70},
  {14, 186.65 , -63.10},
  {15, 191.93 , -59.70},
  {16, 292.68 , 28.00},
  {17, 310.35 , 45.30},
  {18, 24.43 , -57.20},
  {19, 113.65 , 31.90},
  {20, 116.33 , 28.00},
  {21, 141.90 , -8.70},
  {22, 177.28 , 14.60},
  {23, 152.10 , 12.00},
  {24, 279.23 , 38.80},
  {25, 263.73 , 12.60},
  {26, 88.80 , 7.40},
  {27, 78.63 , -8.20},
  {28, 306.40 , -56.70},
  {29, 3.30 , 15.20},
  {30, 345.95 , 28.10},
  {31, 51.08 , 49.90},
  {32, 344.43 , -29.60},
  {33, 247.35 , -26.40},
  {34, 68.98 , 16.50},
  {35, 165.93 , 61.80},
  {36, 200.98 , 54.90},
  {37, 37.95 , 89.30},
  {38, 137.00 , -43.40},
  {39, 201.30 , -11.20},
};

void setup() {
  //启动图形库
  u8g2.begin();
  delay(1000);
  //启动串口
  Serial.begin(9600);
  delay(1000);
  Serial.println("Welcome to BianocularsHUD!");
  //启动姿态板串口
  Serial1.begin(9600);
  delay(100);
  //提供观测者所在纬度、经度
  Latitude = 31.0456;
  Longitude = 121.3997;
  //观测者所在位置的磁偏角
  //Magnetic_Delination = 5.9;
   Magnetic_Delination=0;
  //启动RTC、设置时间
  // Initialize a new chip by turning off write protection and clearing the
  // clock halt flag. These methods needn't always be called. See the DS1302
  // datasheet for details.
  rtc.writeProtect(false);
  rtc.halt(false);


  //  以下仅用于重置时钟的时间，平时需注释掉
  // Make a new time object to set the date and time.
  // Sunday, September 22, 2013 at 01:38:50.
  //    Time t(2017, 6, 29,22, 34, 57, Time::kSunday);
  // Set the time and date on the chip.
  //    rtc.time(t);

}
void loop() {

  //以下获得JY901实时方位角和地平维度基础数据,JY901数据（ROLL,PITCH,YAW）与A方位角/地平纬度的关系变换
  //原公式，20170629验证后取消，jy_yaw = 180.0 - Magnetic_Delination - (float)JY901.stcAngle.Angle[2] / 32768 * 180;
  jy_yaw = (float)JY901.stcAngle.Angle[2] / 32768 * 180;
  if (jy_yaw > (180.0 - Magnetic_Delination)) {
    jy_yaw_m = 360 + 180.0 - Magnetic_Delination - jy_yaw; //磁北极附近，如果不做处理，会出现负值的方位角
  }
  else {
    jy_yaw_m = 180.0 - Magnetic_Delination - jy_yaw;
  }
  jy_pitch = -1 * (float)JY901.stcAngle.Angle[0] / 32768 * 180;

  //校准星体计算
  //jy_yaw_m=63.5;
  //jy_pitch=40.8;
//    Serial.print("Azimuth");
//    Serial.println(jy_yaw);
//    Serial.print("         ");
//    Serial.print("Altitude");
//    Serial.print(jy_pitch);
//    Serial.println("         ");

  //测试用方位角（弧度），获取和计算,假设为0
  Azimuth = jy_yaw_m * 2 * PI / 360;

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
  //    Serial.print("JD is   ");
  //    Serial.print(JD, 6);
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

  //打印LST本地时间
  //        Serial.print("Local Siderial Time is: ");
  //        Serial.print("  ");
  //        Serial.println(Siderial_Time_Local, 6);

  //获取两个电位器的校正值
  RA_AlignPin_Offset = analogRead(RA_AlignPin);
  DEC_AlignPin_Offset = analogRead(DEC_AlignPin);
  RA_AlignPin_Offset_F = map(RA_AlignPin_Offset, 0, 1023, -80, 80) * 0.01;
  DEC_AlignPin_Offset_F = map(DEC_AlignPin_Offset, 0, 1023, -312, 311) * 0.01;

//RA_AlignPin_Offset_F=0;
//DEC_AlignPin_Offset_F=0;
  //计算赤经
  Astro_HUD_RA = RA_AlignPin_Offset_F + Siderial_Time_Local - atan(sin(Azimuth) / ( cos(Azimuth) * sin(Latitude * (2 * PI / 360)) - tan(Altitude) * cos(Latitude * (2 * PI / 360)) )) * 180 / (PI * 15) ;
  //计算赤纬δ = 赤纬。天赤道以北为正，以南为负。
  Astro_HUD_DEC = DEC_AlignPin_Offset_F + asin(sin(Latitude * 2 * PI / 360) * sin(Altitude) + cos(Latitude * (2 * PI / 360)) * cos(Altitude) * cos(Azimuth)) * 360 / (2 * PI);
  //Serial.print("The Binoculars is point at RA: ");
  //Serial.print(Astro_HUD_RA);
  //Serial.print(" RA in Degree ");
  //Serial.print(Astro_HUD_RA*15);
  //Serial.print(" DEC: ");
  //Serial.println(Astro_HUD_DEC);
  //偏移矩阵，在Setup中定义，默认使用E矩阵，应参照Matrix_T进行填写
  //  传感器获得的数值，直接作为原始数据录入
  Matrix_Sensor_Goto[0][0] = Astro_HUD_RA * 15; //角度
  Matrix_Sensor_Goto[1][0] = Astro_HUD_DEC;
  Matrix_Sensor_Goto[2][0] = 1;

  MatrixD.Multiply((float*)Matrix_T_Offset, (float*)Matrix_Sensor_Goto, 3, 3, 1, (float*)Matrix_Atlas_Goto);

  //经过仿射偏移后的值
  Astro_HUD_RA = Matrix_Atlas_Goto[0][0] / 15;
  Astro_HUD_DEC = Matrix_Atlas_Goto[0][1];

  //以下获取赤经、赤纬的独立显示值
  Mod_RA_HH = int(Astro_HUD_RA);
  Mod_RA_MM = int(fmod(Astro_HUD_RA, 1) * 60);
  Mod_RA_MM = int(Mod_RA_MM / 10) * 10; //在精度允许范围内，以20分钟为基础分段显示
  Mod_RA_SS = int((Astro_HUD_RA - Mod_RA_HH - Mod_RA_MM / 60) * 60);
  Mod_DEC_DD = int(Astro_HUD_DEC);
  Mod_DEC_MM = int(fmod(abs(Astro_HUD_DEC), 1) * 60);
  Mod_DEC_MM = int(Mod_DEC_MM / 10) * 10; //在精度允许范围内，以20角分为基础分段显示
  Mod_DEC_SS = int((abs(Astro_HUD_DEC) - abs(Mod_DEC_DD) - Mod_DEC_MM / 60) * 60);

  //在The Cambridge Star Atlas中的星图区域（Index to the star charts）中，望远镜指向的区域
  if (Mod_DEC_DD >= 65) {
    TCSA_Star_Chart = 1;
  }
  else if (Mod_DEC_DD >= 20 && Mod_DEC_DD < 65
          ) {
    TCSA_Star_Chart = ceil(Mod_RA_HH / 4) + 1;
  }
  else if (Mod_DEC_DD >= -20 && Mod_DEC_DD < 20
          ) {
    TCSA_Star_Chart = ceil(Mod_RA_HH / 4) + 7;
  }
  else if (Mod_DEC_DD >= -65 && Mod_DEC_DD < -20
          ) {
    TCSA_Star_Chart = ceil(Mod_RA_HH / 4) + 13;
  }
  else
  {
    TCSA_Star_Chart = 20;
  }

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
    /*
      打印南北星环
      int Star_Chart_H_1=32;
      int Star_Chart_C_1=8;
      int Star_Chart_C_2=20;
      int Star_Chart_C_3=28;
      int Star_Chart_H_2=96;
      int Star_Chart_v=32;

      u8g2.drawCircle(Star_Chart_H_1,Star_Chart_v,Star_Chart_C_1, U8G2_DRAW_ALL);
      u8g2.drawCircle(Star_Chart_H_1,Star_Chart_v,Star_Chart_C_2, U8G2_DRAW_ALL);
      u8g2.drawCircle(Star_Chart_H_1,Star_Chart_v,Star_Chart_C_3, U8G2_DRAW_ALL);
    */


    //打印RA赤纬
    u8g2.setCursor(3, 15);
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

    //显示RA手动调整值
    u8g2.setCursor(30, 30);
    u8g2.setFont(u8g2_font_profont10_tf);
    u8g2.print(RA_AlignPin_Offset_F);
    //显示DEC手动调整值
    u8g2.setCursor(99, 30);
    u8g2.setFont(u8g2_font_profont10_tf);
    u8g2.print(DEC_AlignPin_Offset_F);

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
    u8g2.drawCircle(13, 40, 10, U8G2_DRAW_ALL);
    u8g2.drawLine(13, 40, 10 - 10 * sin(Azimuth), 40 + 10 * cos(Azimuth));
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.setCursor(30, 50);
    u8g2.print(jy_yaw_m);

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
    u8g2.setCursor(110, 63);
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.print(TCSA_Star_Chart);
  } while ( u8g2.nextPage() );

  delay(10);

  //第一步，重新启用以下代码，查看实时有效校准星体，一般30度以上无遮挡的较好，RA间距越大越好，选择3个星体
  //第二步，操作望远镜，指向这三个星体，记下实际RA、DEC，填写到Matrix_Sensor矩阵中
  //第三步，再次导入，获得仿射矩阵Matrix_T的值，填写到Setup()中；
  //第四步，再次导入，之后将自动校准
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉
  //进行Three Stars Alignments三星校准，获取Matrix_T偏移矩阵后，以下至Loop结束可注释掉

  //在串口中输出Nextstarsite的Alignment Star校准星体信息
  //    Serial.println("Alignment Star Values in arry by row are: ");
  //    delay(10);
  //    AlignmentStars_Print(AlignmentStars_Array);
  //    Serial.println();
  //    Serial.println("Which Alignment Stars Tonight？");
  //
  //  for (int i = 0; i < AlignmentStars_rows; i++) {
  //    AlignmentStars_Azimuth = 180+atan(sin((Siderial_Time_Local * 15 - AlignmentStars_Array[i][1]) * 2 * PI / 360) /(cos((Siderial_Time_Local * 15 - AlignmentStars_Array[i][1]) * 2 * PI / 360) * sin(Latitude * 2 * PI / 360) - tan(AlignmentStars_Array[i][2] * 2 * PI / 360) * cos(Latitude * 2 * PI / 360)))*360/(2*PI) ;
  ////方位角起算点的原因，增加180度。
  //    AlignmentStars_Altitude = asin(sin(Latitude * 2 * PI / 360) * sin(AlignmentStars_Array[i][2] * 2 * PI / 360) + cos(Latitude * 2 * PI / 360) * cos(AlignmentStars_Array[i][2] * 2 * PI / 360) * cos((Siderial_Time_Local * 15 - AlignmentStars_Array[i][1])*2*PI/360))*360/(2*PI);
  //
  //    if (AlignmentStars_Altitude >= 45) {
  //      //地平高度大于45度时，才列出来星座、名字
  //      AlignmentStars_Names(i);
  //      Serial.print("\n");
  //      Serial.print(i);
  //      Serial.print("\t");
  //      Serial.print(AlignmentStars_Name);
  //      Serial.print("\t");
  //      Serial.print(AlignmentStars_Azimuth); //校准星体的方位角
  //      Serial.print("\t");
  //      Serial.print(AlignmentStars_Altitude); //校准星体的高度角
  //      Serial.print("\t");
  //
  //      for (int j = 1; j < AlignmentStars_colums; j++) {
  //        Serial.print(AlignmentStars_Array[i][j]);
  //        Serial.print("\t");
  //      }//校准星体的赤经、赤纬
  //    }
  //    delay(50);
  //  }
  //
  //  //选择标准校准星体的序号
  //  Alignment_Star_One=2;
  //  Alignment_Star_Two=16;
  //  Alignment_Star_Three=25;
  //
  //
  //
  //   //  传感器获得的数值，直接作为原始数据录入
  //      Matrix_Sensor_Goto[0][0]=Astro_HUD_RA*15;//角度
  //      Matrix_Sensor_Goto[1][0]=Astro_HUD_DEC;
  //      Matrix_Sensor_Goto[2][0]=1;
  //
  //  // 应显示的坐标，对应星图RA/DEC
  //  //  Matrix_Atlas_Goto[0][1] = 0;
  //  //  Matrix_Atlas_Goto[1][1] = 0;
  //  //  Matrix_Atlas_Goto[2][1] = 1;
  //
  //  //  /*三星校准-目标矩阵的数值
  //  //    A_RA  B_RA  C_RA
  //  //    A_Dec B_Dec C_Dec
  //  //    1   1   1
  //  //  */
  //    Matrix_Atlas[0][0] = AlignmentStars_Array[Alignment_Star_One][1];
  //    Matrix_Atlas[0][1] = AlignmentStars_Array[Alignment_Star_Two][1];
  //    Matrix_Atlas[0][2] = AlignmentStars_Array[Alignment_Star_Three][1];
  //    Matrix_Atlas[1][0] = AlignmentStars_Array[Alignment_Star_One][2];
  //    Matrix_Atlas[1][1] = AlignmentStars_Array[Alignment_Star_Two][2];
  //    Matrix_Atlas[1][2] = AlignmentStars_Array[Alignment_Star_Three][2];
  //    Matrix_Atlas[2][0] = 1;
  //    Matrix_Atlas[2][1] = 1;
  //    Matrix_Atlas[2][2] = 1;
  //  //
  //  //  /*三星校准-传感器瞄准矩阵的数值
  //  //    原始矩阵--》感应器计算后的原始坐标,Matrix_Sensor
  //  //    a_RA  b_RA  c_RA
  //  //    a_Dec b_Dec c_Dec
  //  //    1   1   1
  //  //  */
  //    Matrix_Sensor[0][0] = 302.70; //Alignment_Star_One序号星体的实际测量赤经
  //    Matrix_Sensor[1][0] = 13.90; //Alignment_Star_One序号星体的实际测量赤纬
  //    Matrix_Sensor[0][1] = 297.68; //Alignment_Star_Two序号星体的实际测量赤经
  //    Matrix_Sensor[1][1] = 33.00;//Alignment_Star_Two序号星体的实际测量赤纬
  //    Matrix_Sensor[0][2] = 268.73; //Alignment_Star_Three序号星体的实际测量赤经
  //    Matrix_Sensor[1][2] = 17.60;//Alignment_Star_Three序号星体的实际测量赤纬
  //    Matrix_Sensor[2][0] = 1;
  //    Matrix_Sensor[2][1] = 1;
  //    Matrix_Sensor[2][2] = 1;
  //
  ////计算实际值
  //
  //    MatrixD.Invert((float*)Matrix_Sensor, 3);  //计算逆矩阵
  //    MatrixD.Print((float*)Matrix_Sensor, 3, 3, "Matrix_Sensor_inver");
  //    MatrixD.Multiply((float*)Matrix_Atlas, (float*)Matrix_Sensor, 3, 3, 3, (float*)Matrix_T);
  //    MatrixD.Print((float*)Matrix_T, 3, 3, "T");//获得转换矩阵
  //    MatrixD.Multiply((float*)Matrix_T, (float*)Matrix_Sensor_Goto, 3, 3, 1, (float*)Matrix_Atlas_Goto);
  //    MatrixD.Print((float*)Matrix_Sensor_Goto, 3, 1, "JY901_Sensor");//实际测量赤经赤纬值
  //    MatrixD.Print((float*)Matrix_Atlas_Goto, 3, 1, "On_OLED");//获得最终显示的内容矩阵
}

//打印数组中的RA、Dec信息
void AlignmentStars_Print(const float a[][AlignmentStars_colums]) {
  for (int i = 0; i < AlignmentStars_rows; i++) {

    Serial.println();
    AlignmentStars_Names(i);

    Serial.print(AlignmentStars_Name);
    Serial.print("\t");
    for (int j = 1; j < AlignmentStars_colums; j++) {
      Serial.print(a[i][j]);
      Serial.print("\t");
    }

  }
}

//Nexstarsite.com上的Star list星体表中的Alignment Stars的星座、名字
void AlignmentStars_Names(int x) {
  switch (x) {
    case 0 : AlignmentStars_Name = "And Alpheratz"; break;
    case 1 : AlignmentStars_Name = "And Mirach"; break;
    case 2 : AlignmentStars_Name = "Aql_Altair"; break;
    case 3 : AlignmentStars_Name = "Ari_Hamal"; break;
    case 4 : AlignmentStars_Name = "Aur_Capella"; break;
    case 5 : AlignmentStars_Name = "Boo_Arcturus"; break;
    case 6 : AlignmentStars_Name = "Car_Canopus"; break;
    case 7 : AlignmentStars_Name = "Cas_Caph"; break;
    case 8 : AlignmentStars_Name = "Cas_Navi"; break;
    case 9 : AlignmentStars_Name = "Cen_Alpha Centauri"; break;
    case 10: AlignmentStars_Name = "Cen_Hadar"; break;
    case 11: AlignmentStars_Name = "Cma_Sirius"; break;
    case 12: AlignmentStars_Name = "Cmi_Procyon"; break;
    case 13: AlignmentStars_Name = "CrB_Alphecca"; break;
    case 14: AlignmentStars_Name = "Cru_Acrux"; break;
    case 15: AlignmentStars_Name = "Cru_Mimosa"; break;
    case 16: AlignmentStars_Name = "Cyg_Albireo"; break;
    case 17: AlignmentStars_Name = "Cyg_Deneb"; break;
    case 18: AlignmentStars_Name = "Eri_Achernar"; break;
    case 19: AlignmentStars_Name = "Gem_Castor"; break;
    case 20: AlignmentStars_Name = "Gem_Pollux"; break;
    case 21: AlignmentStars_Name = "Hya_Alphard"; break;
    case 22: AlignmentStars_Name = "Leo_Denebola"; break;
    case 23: AlignmentStars_Name = "Leo_Regulus"; break;
    case 24: AlignmentStars_Name = "Lyr_Vega"; break;
    case 25: AlignmentStars_Name = "Oph_Rasalhague"; break;
    case 26: AlignmentStars_Name = "Ori_Betelgeuse"; break;
    case 27: AlignmentStars_Name = "Ori_Rigel"; break;
    case 28: AlignmentStars_Name = "Pav_Peacock"; break;
    case 29: AlignmentStars_Name = "Peg_Algenib"; break;
    case 30: AlignmentStars_Name = "Peg_Scheat"; break;
    case 31: AlignmentStars_Name = "Per_Mirfak"; break;
    case 32: AlignmentStars_Name = "PsA_Fomalhaut"; break;
    case 33: AlignmentStars_Name = "Sco_Antares"; break;
    case 34: AlignmentStars_Name = "Tau_Aldebaran"; break;
    case 35: AlignmentStars_Name = "Uma_Dubhe"; break;
    case 36: AlignmentStars_Name = "Uma_Mizar"; break;
    case 37: AlignmentStars_Name = "Umi_Polaris"; break;
    case 38: AlignmentStars_Name = "Vel_Suhail"; break;
    case 39: AlignmentStars_Name = "Vir_Spica"; break;
    default:
      break;
  }
}

void serialEvent1() {

  //获取姿态板数据
  while (Serial1.available())
  {
    JY901.CopeSerialData(Serial1.read()); //Call JY901 data cope function
  }
}
