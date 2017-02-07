#include <reg52.h>
#include "usart.h"
#include "iic.h"  
/*
硬件接法：
GY-955---c51
1、GY-3955_RX---c51_TX,c51复位将发送AA 38 E2 给模块
2、c51_TX---FT232,STM32将数据上传给上位机
3、GY-955_TX---c51_RX，接收模块角度数据
软件说明:
该程序采用串口方式获取模块数据，波特率9600
所以得用上位机先将模块串口设置成9600，然后再进行以上操作：
指令:A5 AE 53,模块需复位生效

注：中断函数位于stc_it.c
联系方式：
http://shop62474960.taobao.com/?spm=a230r.7195193.1997079397.2.9qa3Ky&v=1
*/
void send_com(u8 datas)
{
	u8 bytes[3]={0};
	bytes[0]=0xaa;
	bytes[1]=datas;//功能字节
	USART_Send(bytes,3);//发送帧头、功能字节、校验和
}
void delay(uint16_t x)
{
  while(x--);
}
int main(void)
{
  
    u8 sum=0,i=0;
	int16_t DATA[7];
//	float ROLL,PITCH,YAW;
   // float Q4[4];
	Usart_Int(9600);
	delay(60000);
	delay(60000);
	delay(60000);
	delay(60000);	   //等待模块初始化完成
	send_com(0x38);//发送读欧拉角和四元数指令

 	while(1)
	{
		if(Receive_ok)//串口接收完毕
		{
			for(sum=0,i=0;i<(raw_data[3]+4);i++)//rgb_data[3]=15
			sum+=raw_data[i];
	    	if(sum==raw_data[i])//校验和判断
			{
				DATA[0]=(raw_data[4]<<8)|raw_data[5];
        		DATA[1]=(raw_data[6]<<8)|raw_data[7];
         		DATA[2]=(raw_data[8]<<8)|raw_data[9];
         		DATA[3]=(raw_data[10]<<8)|raw_data[11];
        		DATA[4]=(raw_data[12]<<8)|raw_data[13];
        		DATA[5]=(raw_data[14]<<8)|raw_data[15];
         		DATA[6]=(raw_data[16]<<8)|raw_data[17];
         	/*	YAW= (float)((uint16_t)DATA[0])/100;
         		ROLL=(float)DATA[1]/100;
         		PITCH=  (float)(DATA[2])/100;
         		Q4[0]= (float)DATA[3]/10000;
         		Q4[1]= (float)DATA[4]/10000;
         		Q4[2]= (float)DATA[5]/10000;
         		Q4[3]= (float)DATA[6]/10000;  */
				send_3out(&raw_data[4],15,raw_data[2]&0X1F);//上传给上位机
				
			}
			Receive_ok=0;//处理数据完毕标志
		}	
	}
}
