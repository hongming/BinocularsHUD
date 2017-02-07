#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "string.h"
 #include "LED.h"
 #include "stdio.h"
 /*
 Keil: MDK5.10.0.2
MCU:stm32f103c8
Ӳ���ӷ���
GY-955---STM32
1��GY-955_RX---STM32_TX,STM32��λ������AA 38 E2 ��ģ��
2��STM32_TX---FT232,STM32�������ϴ�����λ��
3��GY-955_TX---STM32_RX������ģ������
���˵��:
�ó�����ô��ڷ�ʽ��ȡģ�����ݣ�������115200

ע��ģ�鲨������͸ó�������һ��Ϊ115200���жϺ���λ��stm32f10x_it.c
��ϵ��ʽ��
http://shop62474960.taobao.com/?spm=a230r.7195193.1997079397.2.9qa3Ky&v=1
*/
static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_X;
  
  /* 4����ռ���ȼ���4����Ӧ���ȼ� */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  /*��ռ���ȼ��ɴ���жϼ���͵��ж�*/
	/*��Ӧ���ȼ����ȼ�ִ��*/
	NVIC_X.NVIC_IRQChannel = USART1_IRQn;//�ж�����
  NVIC_X.NVIC_IRQChannelPreemptionPriority = 0;//��ռ���ȼ�
  NVIC_X.NVIC_IRQChannelSubPriority = 0;//��Ӧ���ȼ�
  NVIC_X.NVIC_IRQChannelCmd = ENABLE;//ʹ���ж���Ӧ
  NVIC_Init(&NVIC_X);
}
void send_com(u8 data)
{
	u8 bytes[3]={0};
	bytes[0]=0xaa;
	bytes[1]=data;//�����ֽ�
	USART_Send(bytes,3);//����֡ͷ�������ֽڡ�У���
}

int fputc(int ch, FILE *f)
{
 while (!(USART1->SR & USART_FLAG_TXE));
 USART_SendData(USART1, (unsigned char) ch);// USART1 ���Ի��� USART2 ��
 return (ch);
}
int main(void)
{
  u8 sum=0,i=0;
	uint16_t data=0;
	int16_t DATA[7];
	float ROLL,PITCH,YAW;
  float Q4[4];
	delay_init(72);
	NVIC_Configuration();
	Usart_Int(115200);
	delay_ms(1000);//�ȴ�ģ���ʼ�����
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
         YAW= (float)((uint16_t)DATA[0])/100;
         ROLL=(float)DATA[1]/100;
         PITCH=  (float)(DATA[2])/100;
         Q4[0]= (float)DATA[3]/10000;
         Q4[1]= (float)DATA[4]/10000;
         Q4[2]= (float)DATA[5]/10000;
         Q4[3]= (float)DATA[6]/10000;
				send_3out(&raw_data[4],15,raw_data[2]&0X1F);//�ϴ�����λ��
//				printf("RPY: %.2f  ,",ROLL);
//		    printf("   %.2f  ,",PITCH);
//			  printf("   %.2f  ; ",YAW);
//		    printf("  Q4: %.4f  ,", Q4[0]);
//        printf("      %.4f  ,", Q4[1]);
//				printf("      %.4f  ,", Q4[2]);
//				printf("      %.4f  \r\n ", Q4[3]);
			}
			Receive_ok=0;//����������ϱ�־
			
		}
	}
}
