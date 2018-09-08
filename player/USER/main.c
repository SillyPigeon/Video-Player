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
//ALIENTEK 探索者STM32F407开发板 实验45
//视频播放器实验 -库函数版本
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com
//广州市星翼电子科技有限公司    
//作者：正点原子 @ALIENTEK 
 
int main(void)
{ 
	usmart_dev.init(84);		//初始化USMART
	my_mem_init(SRAMIN);		//初始化内部内存池 
	my_mem_init(SRAMCCM);		//初始化CCM内存池 
	exfuns_init();			//为fatfs相关变量申请内存  
  	f_mount(fs[0],"0:",1); 	//挂载SD卡 
 	f_mount(fs[1],"1:",1); 	//挂载FLASH.
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  //初始化延时函数
	uart_init(115200);		//初始化串口波特率为115200
	
	LED_Init();					//初始化LED 
	usmart_dev.init(84);		//初始化USMART
 	LCD_Init();					//LCD初始化  
 	KEY_Init();					//按键初始化   
	W25QXX_Init();				//初始化W25Q128
	WM8978_Init();				//初始化WM8978	
	
	WM8978_ADDA_Cfg(1,0);		//开启DAC
	WM8978_Input_Cfg(0,0,0);	 //关闭输入通道
	WM8978_Output_Cfg(1,0);		//开启DAC输出  
	WM8978_HPvol_Set(25,25);
	WM8978_SPKvol_Set(60);
	TIM3_Int_Init(10000-1,8400-1);//10Khz计数,1秒钟中断一次
	
	my_mem_init(SRAMIN);		//初始化内部内存池 
	my_mem_init(SRAMCCM);		//初始化CCM内存池 
	exfuns_init();				//为fatfs相关变量申请内存  
  	f_mount(fs[0],"0:",1); 		//挂载SD卡  
	POINT_COLOR=RED;      
	while(font_init()) 			//检查字库
	{	    
		LCD_ShowString(30,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(30,50,240,66,WHITE);//清除显示	     
		delay_ms(200);	
		LED0=!LED0;
	}  	 
//  update_font(20,110,16,"0:");//更新字库
	tp_dev.init();				//触摸屏初始化
	
	delay_ms(1500);	
	
	
	
	while(1)
	{
		//LCD_Clear(BLACK);
		//LCD_ShowPic(80,70,48,48); 
//		LCD_Clear(GBLUE);
		LCD_ShowHomePic(); 
		//ai_load_picfile("0:/PICTURE/left_240320.jpg", 0, 0, 240, 320,1);//显示图片  
		POINT_COLOR=RED;
		Show_Str(20,20,120,16,"视频播放器",16,1);		
		Show_Str(135,20,120,16,"文件查看器",16,1);		  
//		printf("tp_dev.xfac=%lf tp_dev.yfac=%lf\r\n",tp_dev.xfac,tp_dev.yfac);
//		printf("tp_dev.xoff=%d tp_dev.yoff=%d\r\n",tp_dev.xoff,tp_dev.yoff);
//		printf("tp_dev.touchtype=%d\r\n",tp_dev.touchtype);//获取默认信息
		
		while(1)
		{
			if(TP_Scan(0))//先判断有没有触摸事件
			{
				if(tp_dev.x[0]>20 && tp_dev.x[0]<70 && tp_dev.y[0]>40 && tp_dev.y[0]<100)
				{
					  POINT_COLOR=BLUE;
					  Show_Str(20,20,120,16,"视频播放器",16,1);	
						delay_ms(300);
						video_play();
						break;
				}else if(tp_dev.x[0]>135 && tp_dev.x[0]<220 && tp_dev.y[0]>40 && tp_dev.y[0]<70)
				{
						POINT_COLOR=BLUE;
					  Show_Str(135,20,120,16,"文件查看器",16,1);	
						delay_ms(300);
						file_manage();
						break;
				}
			}
		}
	} 
}




