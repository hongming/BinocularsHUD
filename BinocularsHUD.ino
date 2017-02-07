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
**/
#include "Math.h"

//定义变量
	//观测者所在纬度
float Latitude;

	//观测者所在经度
float Longitude;

	//天体方位角，N->W
	float A;

	//天体地平纬度
	float h;

	//本地时角，N->W
	float H;

	//望远镜指向的赤纬
	float Astro_HUD_RA;

	//望远镜指向的赤经  Astro_HUD_DEC=Sidereal_Time_Local-H
	float Astro_HUD_DEC;

	//儒略日和简化儒略日
  float JD,MJD;

	//本地时间
	float Year,Month,Day,Hour,Minute,Second;

  //格林尼治恒星时
  float Siderial_Time;
  
  //本地恒星时 LST
  float Siderial_Time_Local;
  
void setup() {
//启动串口
Serial.begin(9600);
Serial.println("Caculating.........");
//以下获得基础数据
	//观测者所在纬度
	Latitude=30.97761*(2*PI/360);

	//观测者所在经度
	Longitude=121.02539*(2*PI/360);

	//方位角，获取和计算,假设为0
	A=180.0*(2*PI/360);

	//地平纬度，获取和计算，假设为65度
	h=65.0*(2*PI/360);

	//当前时间
	Year=2017.0;
	Month=2.0;
	Day=6.0;
	Hour=20;
	Hour=Hour-8.0;
	Serial.println(Hour);
	Minute=0.0;
	Second=0.0;

	//儒略日，计算采用Navy.mil的计算试试看
  JD=367*Year-int((7*(Year+int((Month+9)/12)))/4) + int((275*Month)/9) + Day + 1721013.5 + Hour/24+Minute/1440+Second/86400 - 0.5*((((100*Year+Month-190002.5)>0)-((100*Year+Month-190002.5)<0))) + 0.5;
  Serial.print("JD is   ");
  Serial.println(JD,6);
	//简化儒略日，计算
	MJD=JD-2400000.5;
	Serial.print("MJD is   ");
  Serial.println(MJD);
	//格林尼治恒星时
	Siderial_Time=6.697374558 + 0.06570982441908*(JD-Hour/24- 2451545.0) + 1.00273790935*Hour + 0.000026*(JD - 2451545.0)*(JD - 2451545.0)/(36525*36525);
	while(Siderial_Time<0.0)
		Siderial_Time+=24.0;
	while(Siderial_Time>24.0)
		Siderial_Time-=24.0;
	//本地恒星时
	Siderial_Time_Local=Siderial_Time+8;
    while(Siderial_Time_Local<0.0)
    Siderial_Time_Local+=24.0;
  while(Siderial_Time_Local>24.0)
    Siderial_Time_Local-=24.0;
Serial.print("GST is:   ");
Serial.println(Siderial_Time);
Serial.print("LST is:   ");
Serial.println(Siderial_Time_Local);
//计算赤经
Astro_HUD_DEC=((Siderial_Time_Local*15-(atan(sin(A)/( cos(A)*sin(Latitude) +tan(h)*cos(Latitude) )))*360/(2*PI)))/15;
//计算赤纬
Astro_HUD_RA=(asin(sin(Latitude)*sin(h) - cos(Latitude)*cos(h)*cos(A)))*360/(2*PI);

Serial.print("Dec is  ");
Serial.println(Astro_HUD_DEC);
Serial.print("RA is  ");
Serial.println(Astro_HUD_RA);
Serial.print(" ^");
}

void loop(){
  
  }
