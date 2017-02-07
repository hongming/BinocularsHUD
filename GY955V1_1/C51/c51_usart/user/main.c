#include <reg52.h>
#include "usart.h"
#include "iic.h"  
/*
Ӳ���ӷ���
GY-955---c51
1��GY-3955_RX---c51_TX,c51��λ������AA 38 E2 ��ģ��
2��c51_TX---FT232,STM32�������ϴ�����λ��
3��GY-955_TX---c51_RX������ģ��Ƕ�����
���˵��:
�ó�����ô��ڷ�ʽ��ȡģ�����ݣ�������9600
���Ե�����λ���Ƚ�ģ�鴮�����ó�9600��Ȼ���ٽ������ϲ�����
ָ��:A5 AE 53,ģ���踴λ��Ч

ע���жϺ���λ��stc_it.c
��ϵ��ʽ��
http://shop62474960.taobao.com/?spm=a230r.7195193.1997079397.2.9qa3Ky&v=1
*/
void send_com(u8 datas)
{
	u8 bytes[3]={0};
	bytes[0]=0xaa;
	bytes[1]=datas;//�����ֽ�
	USART_Send(bytes,3);//����֡ͷ�������ֽڡ�У���
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
	delay(60000);	   //�ȴ�ģ���ʼ�����
	send_com(0x38);//���Ͷ�ŷ���Ǻ���Ԫ��ָ��

 	while(1)
	{
		if(Receive_ok)//���ڽ������
		{
			for(sum=0,i=0;i<(raw_data[3]+4);i++)//rgb_data[3]=15
			sum+=raw_data[i];
	    	if(sum==raw_data[i])//У����ж�
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
				send_3out(&raw_data[4],15,raw_data[2]&0X1F);//�ϴ�����λ��
				
			}
			Receive_ok=0;//����������ϱ�־
		}	
	}
}
