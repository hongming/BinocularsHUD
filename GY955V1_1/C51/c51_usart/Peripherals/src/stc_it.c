#include "stc_it.h"
#include "usart.h"
#include "string.h"
//�ⲿ�ж�1
uint8_t stata=0;
void EXTI1_IRQHandler(void)interrupt 2
{
}
//�����ж�
void USART_IRQHandler(void)interrupt 4
{
	static uint8_t i=0,rebuf[20]={0};
	uint8_t sum=0;
    if(TI)//������ɱ�־
	{
	  TI=0;//�巢����ɱ�־
	  send_ok=0;//�����־��0 
	}
	if(RI)//������ɱ�־
	{
		rebuf[i++]=SBUF;
		RI=0;//���жϽ��ձ�־
		if (rebuf[0]!=0x5a)//֡ͷ����
			i=0;	
		if ((i==2)&&(rebuf[1]!=0x5a))//֡ͷ����
			i=0;
	
		if(i>3)//i����4ʱ���Ѿ����յ��������ֽ�rebuf[3]
		{
			if(i!=(rebuf[3]+5))//�ж��Ƿ����һ֡�������
				return;	
			
					if(!Receive_ok)//�����ݴ�����ɺ�Ž����µ�����
					{
						memcpy(raw_data,rebuf,20);//�������յ�������
						Receive_ok=1;//������ɱ�־
					}
			
			i=0;//������0
		}
	
	}

}
