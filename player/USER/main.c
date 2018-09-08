#include "sys.h"
#include "delay.h"  
#include "usart.h"   
#include "led.h"
#include "lcd.h"
#include "key.h"  
#include "usmart.h"  
#include "malloc.h" 
#include "w25qxx.h"    
#include "sdio_sdcard.h"
#include "ff.h"  
#include "exfuns.h"    
#include "fontupd.h"
#include "text.h"	 
#include "wm8978.h"	 
#include "videoplayer.h" 
#include "timer.h" 
#include "touch.h" 
#include "piclib.h"	
#include "fattester.h"	 
//ALIENTEK ̽����STM32F407������ ʵ��45
//��Ƶ������ʵ�� -�⺯���汾
//����֧�֣�www.openedv.com
//�Ա����̣�http://eboard.taobao.com
//������������ӿƼ����޹�˾    
//���ߣ�����ԭ�� @ALIENTEK 
 
int main(void)
{ 
	usmart_dev.init(84);		//��ʼ��USMART
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ�� 
	my_mem_init(SRAMCCM);		//��ʼ��CCM�ڴ�� 
	exfuns_init();			//Ϊfatfs��ر��������ڴ�  
  	f_mount(fs[0],"0:",1); 	//����SD�� 
 	f_mount(fs[1],"1:",1); 	//����FLASH.
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  //��ʼ����ʱ����
	uart_init(115200);		//��ʼ�����ڲ�����Ϊ115200
	
	LED_Init();					//��ʼ��LED 
	usmart_dev.init(84);		//��ʼ��USMART
 	LCD_Init();					//LCD��ʼ��  
 	KEY_Init();					//������ʼ��   
	W25QXX_Init();				//��ʼ��W25Q128
	WM8978_Init();				//��ʼ��WM8978	
	
	WM8978_ADDA_Cfg(1,0);		//����DAC
	WM8978_Input_Cfg(0,0,0);	 //�ر�����ͨ��
	WM8978_Output_Cfg(1,0);		//����DAC���  
	WM8978_HPvol_Set(25,25);
	WM8978_SPKvol_Set(60);
	TIM3_Int_Init(10000-1,8400-1);//10Khz����,1�����ж�һ��
	
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ�� 
	my_mem_init(SRAMCCM);		//��ʼ��CCM�ڴ�� 
	exfuns_init();				//Ϊfatfs��ر��������ڴ�  
  	f_mount(fs[0],"0:",1); 		//����SD��  
	POINT_COLOR=RED;      
	while(font_init()) 			//����ֿ�
	{	    
		LCD_ShowString(30,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(30,50,240,66,WHITE);//�����ʾ	     
		delay_ms(200);	
		LED0=!LED0;
	}  	 
//  update_font(20,110,16,"0:");//�����ֿ�
	tp_dev.init();				//��������ʼ��
	
	delay_ms(1500);	
	
	
	
	while(1)
	{
		//LCD_Clear(BLACK);
		//LCD_ShowPic(80,70,48,48); 
//		LCD_Clear(GBLUE);
		LCD_ShowHomePic(); 
		//ai_load_picfile("0:/PICTURE/left_240320.jpg", 0, 0, 240, 320,1);//��ʾͼƬ  
		POINT_COLOR=RED;
		Show_Str(20,20,120,16,"��Ƶ������",16,1);		
		Show_Str(135,20,120,16,"�ļ��鿴��",16,1);		  
//		printf("tp_dev.xfac=%lf tp_dev.yfac=%lf\r\n",tp_dev.xfac,tp_dev.yfac);
//		printf("tp_dev.xoff=%d tp_dev.yoff=%d\r\n",tp_dev.xoff,tp_dev.yoff);
//		printf("tp_dev.touchtype=%d\r\n",tp_dev.touchtype);//��ȡĬ����Ϣ
		
		while(1)
		{
			if(TP_Scan(0))//���ж���û�д����¼�
			{
				if(tp_dev.x[0]>20 && tp_dev.x[0]<70 && tp_dev.y[0]>40 && tp_dev.y[0]<100)
				{
					  POINT_COLOR=BLUE;
					  Show_Str(20,20,120,16,"��Ƶ������",16,1);	
						delay_ms(300);
						video_play();
						break;
				}else if(tp_dev.x[0]>135 && tp_dev.x[0]<220 && tp_dev.y[0]>40 && tp_dev.y[0]<70)
				{
						POINT_COLOR=BLUE;
					  Show_Str(135,20,120,16,"�ļ��鿴��",16,1);	
						delay_ms(300);
						file_manage();
						break;
				}
			}
		}
	} 
}




