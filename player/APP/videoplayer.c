#include "videoplayer.h" 
#include "string.h"  
#include "key.h" 
#include "usart.h"   
#include "delay.h"
#include "timer.h"
#include "lcd.h"
#include "led.h"
#include "key.h"
#include "malloc.h"
#include "i2s.h" 
#include "wm8978.h" 
#include "mjpeg.h" 
#include "avi.h"
#include "exfuns.h"
#include "text.h"
#include "touch.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//±¾³ÌĞòÖ»¹©Ñ§Ï°Ê¹ÓÃ£¬Î´¾­×÷ÕßĞí¿É£¬²»µÃÓÃÓÚÆäËüÈÎºÎÓÃÍ¾
//ALIENTEK STM32F407¿ª·¢°å
//ÊÓÆµ²¥·ÅÆ÷ Ó¦ÓÃ´úÂë	   
//ÕıµãÔ­×Ó@ALIENTEK
//¼¼ÊõÂÛÌ³:www.openedv.com
//´´½¨ÈÕÆÚ:2014/7/1
//°æ±¾£ºV1.0
//°æÈ¨ËùÓĞ£¬µÁ°æ±Ø¾¿¡£
//Copyright(C) ¹ãÖİÊĞĞÇÒíµç×Ó¿Æ¼¼ÓĞÏŞ¹«Ë¾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
 
    
extern u16 frame;
extern vu8 frameup;//ÊÓÆµ²¥·ÅÊ±Ï¶¿ØÖÆ±äÁ¿,µ±µÈÓÚ1µÄÊ±ºò,¿ÉÒÔ¸üĞÂÏÂÒ»Ö¡ÊÓÆµ


volatile u8 i2splaybuf;	//¼´½«²¥·ÅµÄÒôÆµÖ¡»º³å±àºÅ
u8* i2sbuf[4]; 			//ÒôÆµ»º³åÖ¡,¹²4Ö¡,4*5K=20K
  
//ÒôÆµÊı¾İI2S DMA´«Êä»Øµ÷º¯Êı
void audio_i2s_dma_callback(void) 
{      
	i2splaybuf++;
	if(i2splaybuf>3)i2splaybuf=0;
	if(DMA1_Stream4->CR&(1<<19))
	{	 
		DMA_MemoryTargetConfig(DMA1_Stream4,(u32)i2sbuf[i2splaybuf], DMA_Memory_0);
	}
	else 
	{  
    DMA_MemoryTargetConfig(DMA1_Stream4,(u32)i2sbuf[i2splaybuf], DMA_Memory_1); 		
	} 
} 
//µÃµ½pathÂ·¾¶ÏÂ,Ä¿±êÎÄ¼şµÄ×Ü¸öÊı
//path:Â·¾¶		    
//·µ»ØÖµ:×ÜÓĞĞ§ÎÄ¼şÊı
u16 video_get_tnum(u8 *path)
{	  
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//ÁÙÊ±Ä¿Â¼
	FILINFO tfileinfo;	//ÁÙÊ±ÎÄ¼şĞÅÏ¢		
	u8 *fn; 			 			   			     
    res=f_opendir(&tdir,(const TCHAR*)path); //´ò¿ªÄ¿Â¼
  	tfileinfo.lfsize=_MAX_LFN*2+1;						//³¤ÎÄ¼şÃû×î´ó³¤¶È
	tfileinfo.lfname=mymalloc(SRAMIN,tfileinfo.lfsize);	//Îª³¤ÎÄ¼ş»º´æÇø·ÖÅäÄÚ´æ
	if(res==FR_OK&&tfileinfo.lfname!=NULL)
	{
		while(1)//²éÑ¯×ÜµÄÓĞĞ§ÎÄ¼şÊı
		{
	        res=f_readdir(&tdir,&tfileinfo);       		//¶ÁÈ¡Ä¿Â¼ÏÂµÄÒ»¸öÎÄ¼ş
	        if(res!=FR_OK||tfileinfo.fname[0]==0)break;	//´íÎóÁË/µ½Ä©Î²ÁË,ÍË³ö		  
     		fn=(u8*)(*tfileinfo.lfname?tfileinfo.lfname:tfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X60)//È¡¸ßËÄÎ»,¿´¿´ÊÇ²»ÊÇÊÓÆµÎÄ¼ş	
			{
				rval++;//ÓĞĞ§ÎÄ¼şÊıÔö¼Ó1
			}	    
		}  
	} 
	myfree(SRAMIN,tfileinfo.lfname);
	return rval;
} 
//ÏÔÊ¾µ±Ç°²¥·ÅÊ±¼ä
//favi:µ±Ç°²¥·ÅµÄÊÓÆµÎÄ¼ş
//aviinfo;avi¿ØÖÆ½á¹¹Ìå
void video_time_show(FIL *favi,AVI_INFO *aviinfo)
{	 
	static u32 oldsec;	//ÉÏÒ»´ÎµÄ²¥·ÅÊ±¼ä
	u8* buf;
	u32 totsec=0;		//videoÎÄ¼ş×ÜÊ±¼ä 
	u32 cursec; 		//µ±Ç°²¥·ÅÊ±¼ä 
	totsec=(aviinfo->SecPerFrame/1000)*aviinfo->TotalFrame;	//¸èÇú×Ü³¤¶È(µ¥Î»:ms) 
	totsec/=1000;		//ÃëÖÓÊı. 
  	cursec=((double)favi->fptr/favi->fsize)*totsec;//µ±Ç°²¥·Åµ½¶àÉÙÃëÁË?  
	if(oldsec!=cursec)	//ĞèÒª¸üĞÂÏÔÊ¾Ê±¼ä
	{
		buf=mymalloc(SRAMIN,100);//ÉêÇë100×Ö½ÚÄÚ´æ
		oldsec=cursec; 
		POINT_COLOR=BLUE; 
		sprintf((char*)buf,"²¥·ÅÊ±¼ä:%02d:%02d:%02d/%02d:%02d:%02d",cursec/3600,(cursec%3600)/60,cursec%60,totsec/3600,(totsec%3600)/60,totsec%60);
 		Show_Str(10,90,lcddev.width-10,16,buf,16,0);	//ÏÔÊ¾¸èÇúÃû×Ö
		myfree(SRAMIN,buf);		
	} 		 
}
//ÏÔÊ¾µ±Ç°ÊÓÆµÎÄ¼şµÄÏà¹ØĞÅÏ¢ 
//aviinfo;avi¿ØÖÆ½á¹¹Ìå
void video_info_show(AVI_INFO *aviinfo)
{	  
	u8 *buf;
	buf=mymalloc(SRAMIN,100);//ÉêÇë100×Ö½ÚÄÚ´æ 
	POINT_COLOR=RED; 
	sprintf((char*)buf,"ÉùµÀÊı:%d,²ÉÑùÂÊ:%d",aviinfo->Channels,aviinfo->SampleRate*10); 
 	Show_Str(10,50,lcddev.width-48,16,buf,16,1);	//ÏÔÊ¾¸èÇúÃû×Ö
	sprintf((char*)buf,"Ö¡ÂÊ:%dÖ¡",1000/(aviinfo->SecPerFrame/1000)); 
 	Show_Str(10,70,lcddev.width-10,16,buf,16,1);	//ÏÔÊ¾¸èÇúÃû×Ö
	myfree(SRAMIN,buf);	  
}
//ÊÓÆµ»ù±¾ĞÅÏ¢ÏÔÊ¾
//name:ÊÓÆµÃû×Ö
//index:µ±Ç°Ë÷Òı
//total:×ÜÎÄ¼şÊı
void video_bmsg_show(u8* name,u16 index,u16 total)
{		
	u8* buf;
	buf=mymalloc(SRAMIN,100);//ÉêÇë100×Ö½ÚÄÚ´æ
	POINT_COLOR=RED;
	sprintf((char*)buf,"ÎÄ¼şÃû:%s",name);
	Show_Str(10,10,lcddev.width-10,16,buf,16,1);//ÏÔÊ¾ÎÄ¼şÃû
	sprintf((char*)buf,"Ë÷Òı:%d/%d",index,total);	
	Show_Str(10,30,lcddev.width-10,16,buf,16,1);//ÏÔÊ¾Ë÷Òı 		  	  
	myfree(SRAMIN,buf);		
}

//²¥·ÅÊÓÆµ
void video_play(void)
{
	u8 res;
 	DIR vdir;	 		//Ä¿Â¼
	FILINFO vfileinfo;	//ÎÄ¼şĞÅÏ¢
	u8 *fn;   			//³¤ÎÄ¼şÃû
	u8 *pname;			//´øÂ·¾¶µÄÎÄ¼şÃû
	u16 totavinum; 		//ÊÓÆµÎÄ¼ş×ÜÊı
	u16 curindex;		//ÊÓÆµÎÄ¼şµ±Ç°Ë÷Òı
	u8 key;				//¼üÖµ		  
 	u16 temp;
	u16 *vindextbl;		//ÊÓÆµÎÄ¼şË÷Òı±í 
	
	
 	while(f_opendir(&vdir,"0:/VIDEO"))//´ò¿ªÊÓÆµÎÄ¼ş¼Ğ
 	{	    
		Show_Str(60,190,240,16,"VIDEOÎÄ¼ş¼Ğ´íÎó!",16,0);
		delay_ms(200);				  
		LCD_Fill(60,190,240,206,WHITE);//Çå³ıÏÔÊ¾	     
		delay_ms(200);				  
	} 									  
	totavinum=video_get_tnum("0:/VIDEO"); //µÃµ½×ÜÓĞĞ§ÎÄ¼şÊı
  	while(totavinum==NULL)//ÊÓÆµÎÄ¼ş×ÜÊıÎª0		
 	{	    
		Show_Str(60,190,240,16,"Ã»ÓĞÊÓÆµÎÄ¼ş!",16,0);
		delay_ms(200);				  
		LCD_Fill(60,190,240,146,WHITE);//Çå³ıÏÔÊ¾	     
		delay_ms(200);				  
	}										   
  	vfileinfo.lfsize=_MAX_LFN*2+1;						//³¤ÎÄ¼şÃû×î´ó³¤¶È
	vfileinfo.lfname=mymalloc(SRAMIN,vfileinfo.lfsize);	//Îª³¤ÎÄ¼ş»º´æÇø·ÖÅäÄÚ´æ
 	pname=mymalloc(SRAMIN,vfileinfo.lfsize);			//Îª´øÂ·¾¶µÄÎÄ¼şÃû·ÖÅäÄÚ´æ
 	vindextbl=mymalloc(SRAMIN,2*totavinum);				//ÉêÇë2*totavinum¸ö×Ö½ÚµÄÄÚ´æ,ÓÃÓÚ´æ·ÅÊÓÆµÎÄ¼şË÷Òı
 	while(vfileinfo.lfname==NULL||pname==NULL||vindextbl==NULL)//ÄÚ´æ·ÖÅä³ö´í
 	{	    
		Show_Str(60,190,240,16,"ÄÚ´æ·ÖÅäÊ§°Ü!",16,0);
		delay_ms(200);				  
		LCD_Fill(60,190,240,146,WHITE);//Çå³ıÏÔÊ¾	     
		delay_ms(200);				  
	}  	 
 	//¼ÇÂ¼Ë÷Òı
    res=f_opendir(&vdir,"0:/VIDEO"); //´ò¿ªÄ¿Â¼
	if(res==FR_OK)
	{
		curindex=0;//µ±Ç°Ë÷ÒıÎª0
		while(1)//È«²¿²éÑ¯Ò»±é
		{
			temp=vdir.index;							//¼ÇÂ¼µ±Ç°index
	        res=f_readdir(&vdir,&vfileinfo);       		//¶ÁÈ¡Ä¿Â¼ÏÂµÄÒ»¸öÎÄ¼ş
	        if(res!=FR_OK||vfileinfo.fname[0]==0)break;	//´íÎóÁË/µ½Ä©Î²ÁË,ÍË³ö		  
     		fn=(u8*)(*vfileinfo.lfname?vfileinfo.lfname:vfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X60)//È¡¸ßËÄÎ»,¿´¿´ÊÇ²»ÊÇÒôÀÖÎÄ¼ş	
			{
				vindextbl[curindex]=temp;//¼ÇÂ¼Ë÷Òı
				curindex++;
			}	    
		} 
	}   
   	curindex=0;										//´Ó0¿ªÊ¼ÏÔÊ¾
   	res=f_opendir(&vdir,(const TCHAR*)"0:/VIDEO"); 	//´ò¿ªÄ¿Â¼
	while(res==FR_OK)//´ò¿ª³É¹¦
	{	
		dir_sdi(&vdir,vindextbl[curindex]);			//¸Ä±äµ±Ç°Ä¿Â¼Ë÷Òı	   
        res=f_readdir(&vdir,&vfileinfo);       		//¶ÁÈ¡Ä¿Â¼ÏÂµÄÒ»¸öÎÄ¼ş
        if(res!=FR_OK||vfileinfo.fname[0]==0)break;	//´íÎóÁË/µ½Ä©Î²ÁË,ÍË³ö
     	fn=(u8*)(*vfileinfo.lfname?vfileinfo.lfname:vfileinfo.fname);			 
		strcpy((char*)pname,"0:/VIDEO/");			//¸´ÖÆÂ·¾¶(Ä¿Â¼)z
		strcat((char*)pname,(const char*)fn);  		//½«ÎÄ¼şÃû½ÓÔÚºóÃæ 
		//LCD_Clear(WHITE);							//ÏÈÇåÆÁ
		LCD_Clear(GBLUE);
		video_bmsg_show(fn,curindex+1,totavinum);	//ÏÔÊ¾Ãû×Ö,Ë÷ÒıµÈĞÅÏ¢	
		
		key = video_play_mjpeg(pname,fn,curindex+1,totavinum);	//²¥·ÅÕâ¸öÒôÆµÎÄ¼ş
		if(key == CLOSE_PRES)
		{
			key=0;
			break;
		}else if(key==KEY2_PRES)		//ÉÏÒ»Çú
		{
			if(curindex)curindex--;
			else curindex=totavinum-1;
 		}else if(key==KEY0_PRES)//ÏÂÒ»Çú
		{
			curindex++;		   	
			if(curindex>=totavinum)curindex=0;//µ½Ä©Î²µÄÊ±ºò,×Ô¶¯´ÓÍ·¿ªÊ¼
 		}else break;	//²úÉúÁË´íÎó 	 
	} 											  
	f_closedir(&vdir);
	myfree(SRAMIN,vfileinfo.lfname);	//ÊÍ·ÅÄÚ´æ			    
	myfree(SRAMIN,pname);				//ÊÍ·ÅÄÚ´æ			    
	myfree(SRAMIN,vindextbl);			//ÊÍ·ÅÄÚ´æ	 
}
//²¥·ÅÒ»¸ömjpegÎÄ¼ş
//pname:ÎÄ¼şÃû
//·µ»ØÖµ:
//KEY0_PRES:ÏÂÒ»Çú
//KEY1_PRES:ÉÏÒ»Çú
//ÆäËû:´íÎó
u8 video_play_mjpeg(u8 *pname, u8 *fn, u16 index, u16 total)
{   
	u8* framebuf;	//ÊÓÆµ½âÂëbuf	 
	u8* pbuf;		//bufÖ¸Õë  
	FIL *favi;
	u8  res=0;
	u16 offset=0; 
	u32	nr; 
	//u8 key; 
	u8 tsflag;
    u8 i2ssavebuf;  
	u8* namebuf;
//	u16 tstik;
//	u8 tsres;
	namebuf=mymalloc(SRAMIN,20);//ÉêÇë100×Ö½ÚÄÚ´æ
	i2sbuf[0]=mymalloc(SRAMIN,AVI_AUDIO_BUF_SIZE);	//ÉêÇëÒôÆµÄÚ´æ
	i2sbuf[1]=mymalloc(SRAMIN,AVI_AUDIO_BUF_SIZE);	//ÉêÇëÒôÆµÄÚ´æ
	i2sbuf[2]=mymalloc(SRAMIN,AVI_AUDIO_BUF_SIZE);	//ÉêÇëÒôÆµÄÚ´æ
	i2sbuf[3]=mymalloc(SRAMIN,AVI_AUDIO_BUF_SIZE);	//ÉêÇëÒôÆµÄÚ´æ 
	framebuf=mymalloc(SRAMIN,AVI_VIDEO_BUF_SIZE);	//ÉêÇëÊÓÆµbuf
	favi=(FIL*)mymalloc(SRAMIN,sizeof(FIL));		//ÉêÇëfaviÄÚ´æ 
	memset(i2sbuf[0],0,AVI_AUDIO_BUF_SIZE);
	memset(i2sbuf[1],0,AVI_AUDIO_BUF_SIZE); 
	memset(i2sbuf[2],0,AVI_AUDIO_BUF_SIZE);
	memset(i2sbuf[3],0,AVI_AUDIO_BUF_SIZE);
	if(i2sbuf[3]==NULL||framebuf==NULL||favi==NULL)
	{
		printf("memory error!\r\n");
		res=0XFF;
	}   
	while(res==0)
	{ 
		res=f_open(favi,(char *)pname,FA_READ);
		if(res==0)
		{
			pbuf=framebuf;			
			res=f_read(favi,pbuf,AVI_VIDEO_BUF_SIZE,&nr);//¿ªÊ¼¶ÁÈ¡	
			if(res)
			{
				printf("fread error:%d\r\n",res);
				break;
			} 	 
			//¿ªÊ¼avi½âÎö
			res=avi_init(pbuf,AVI_VIDEO_BUF_SIZE);	//avi½âÎö
			if(res)
			{
				printf("avi err:%d\r\n",res);
				break;
			} 	
			
			video_info_show(&avix);
			LCD_ShowPlayerPic();
			//µÈ´ı²Ëµ¥Ñ¡Ôñ½çÃæ
			while(1){
				POINT_COLOR=RED;
				Show_Str(35,120,48,20,"ÉÏÒ»²¿",16,1);
				Show_Str(135,120,48,20,"ÏÂÒ»²¿",16,1);
				Show_Str(45,220,48,20,"²¥·Å",16,1);
				Show_Str(145,220,48,20,"¹Ø±Õ",16,1);
				if(TP_Scan(0))
				{
						if(tp_dev.x[0]>=135&&tp_dev.x[0]<190 &&tp_dev.y[0]>240 && tp_dev.y[0]<=300)
						{
								POINT_COLOR=BLUE;
								Show_Str(145,220,48,20,"¹Ø±Õ",16,1);
								delay_ms(300);
								res = CLOSE_PRES;
								break;
						}else if(tp_dev.x[0]>135&&tp_dev.x[0]<190 && tp_dev.y[0]>140 && tp_dev.y[0]<=190)
						{
								POINT_COLOR=BLUE;
								Show_Str(135,120,48,20,"ÏÂÒ»²¿",16,1);
								delay_ms(300);
								res = KEY0_PRES;
								break;
						}else if(tp_dev.x[0]>=50 &&tp_dev.x[0]<100 && tp_dev.y[0]>140 && tp_dev.y[0]<=190)
						{
								POINT_COLOR=BLUE;
								Show_Str(35,120,48,20,"ÉÏÒ»²¿",16,1);
								delay_ms(300);
								res = KEY2_PRES;
								break;
						}else if(tp_dev.x[0]>=50 &&tp_dev.x[0]<100 && tp_dev.y[0]>240 && tp_dev.y[0]<=300)
						{
								POINT_COLOR=BLUE;
								Show_Str(45,220,48,20,"²¥·Å",16,1);
								delay_ms(300);
								res = 0;
								break;
						}
				 }
			}
			if(res == 0){
				TIM6_Int_Init(avix.SecPerFrame/100-1,8400-1);//10Khz¼ÆÊıÆµÂÊ,¼Ó1ÊÇ100us 
				offset=avi_srarch_id(pbuf,AVI_VIDEO_BUF_SIZE,"movi");//Ñ°ÕÒmovi ID	 
				avi_get_streaminfo(pbuf+offset+4);			//»ñÈ¡Á÷ĞÅÏ¢ 
				f_lseek(favi,offset+12);					//Ìø¹ı±êÖ¾ID,¶ÁµØÖ·Æ«ÒÆµ½Á÷Êı¾İ¿ªÊ¼´¦	 
				res=mjpegdec_init((lcddev.width-avix.Width)/2,110+(lcddev.height-110-avix.Height)/2);//JPG½âÂë³õÊ¼»¯ 
				if(avix.SampleRate)							//ÓĞÒôÆµĞÅÏ¢,²Å³õÊ¼»¯
				{
					WM8978_I2S_Cfg(2,0);	//·ÉÀûÆÖ±ê×¼,16Î»Êı¾İ³¤¶È
					I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_16bextended);		//·ÉÀûÆÖ±ê×¼,Ö÷»ú·¢ËÍ,Ê±ÖÓµÍµçÆ½ÓĞĞ§,16Î»Ö¡³¤¶È
					I2S2_SampleRate_Set(avix.SampleRate);	//ÉèÖÃ²ÉÑùÂÊ
					I2S2_TX_DMA_Init(i2sbuf[1],i2sbuf[2],avix.AudioBufSize/2); //ÅäÖÃDMA
					i2s_tx_callback=audio_i2s_dma_callback;	//»Øµ÷º¯ÊıÖ¸ÏòI2S_DMA_Callback
					i2splaybuf=0;
					i2ssavebuf=0; 
					I2S_Play_Start(); //¿ªÆôI2S²¥·Å 
				}
				LCD_Display_Dir(1);//¿ªÆôºáÆÁÄ£Ê½
				while(1)//²¥·ÅÑ­»·
				{					
					if(avix.StreamID==AVI_VIDS_FLAG)	//ÊÓÆµÁ÷
					{
						pbuf=framebuf;
						f_read(favi,pbuf,avix.StreamSize+8,&nr);		//¶ÁÈëÕûÖ¡+ÏÂÒ»Êı¾İÁ÷IDĞÅÏ¢  
						res=mjpegdec_decode(pbuf,avix.StreamSize);
						if(res)
						{
							printf("decode error!\r\n");
						} 
						while(frameup==0);	//µÈ´ıÊ±¼äµ½´ï(ÔÚTIM6µÄÖĞ¶ÏÀïÃæÉèÖÃÎª1)
						frameup=0;			//±êÖ¾ÇåÁã
						frame++; 
					}else 	//ÒôÆµÁ÷
					{		  
						//video_time_show(favi,&avix); 	//ÏÔÊ¾µ±Ç°²¥·ÅÊ±¼ä
						i2ssavebuf++;
						if(i2ssavebuf>3)i2ssavebuf=0;
						do
						{
							nr=i2splaybuf;
							if(nr)nr--;
							else nr=3; 
						}while(i2ssavebuf==nr);//Åö×²µÈ´ı. 
						f_read(favi,i2sbuf[i2ssavebuf],avix.StreamSize+8,&nr);//Ìî³äi2sbuf	 
						pbuf=i2sbuf[i2ssavebuf];  
	//					delay_ms(10);
					}
					
					tp_dev.scan(0);
					if(tp_dev.sta&TP_PRES_DOWN){
						I2S_Play_Stop();	//¹Ø±ÕÒôÆµ
						delay_ms(300);
						tp_dev.scan(0);
						if(tp_dev.sta&TP_PRES_DOWN){
							delay_ms(1000);
							tp_dev.scan(0);
							if(tp_dev.sta&TP_PRES_DOWN){
								res = 0;
								LCD_Display_Dir(0);
								LCD_Clear(GBLUE);
								video_info_show(&avix);
								sprintf((char*)namebuf,"ÎÄ¼şÃû:%s",fn);
								Show_Str(10,10,lcddev.width-10,16,namebuf,16,1);//ÏÔÊ¾ÎÄ¼şÃû
								sprintf((char*)namebuf,"Ë÷Òı:%d/%d",index,total);	
								Show_Str(10,30,lcddev.width-10,16,namebuf,16,1);//ÏÔÊ¾Ë÷Ò
								void I2S_Play_Start(void);
								break;
							}
						}else{
							while(1){
								tp_dev.scan(0);
								if(tp_dev.sta&TP_PRES_DOWN){
									delay_ms(300);
									I2S_Play_Start();
									break;
								}
								delay_ms(10);
							}
						}
					} 	
	//				key=KEY_Scan(0);
	//				if(key==KEY0_PRES||key==KEY2_PRES)//KEY0/KEY2°´ÏÂ,²¥·ÅÏÂÒ»¸ö/ÉÏÒ»¸öÊÓÆµ
	//				{
	//					res=key;
	//					break; 
	//				}else if(key==KEY1_PRES||key==WKUP_PRES)
	//				{
	//					I2S_Play_Stop();//¹Ø±ÕÒôÆµ
	//					video_seek(favi,&avix,framebuf);
	//					pbuf=framebuf;
	//					I2S_Play_Start();//¿ªÆôDMA²¥·Å 
	//				}
					if(avi_get_streaminfo(pbuf+avix.StreamSize))//¶ÁÈ¡ÏÂÒ»Ö¡ Á÷±êÖ¾
					{
						printf("frame error \r\n"); 
						res=0;
						LCD_Display_Dir(0);
						LCD_Clear(GBLUE);
						video_info_show(&avix);
						sprintf((char*)namebuf,"ÎÄ¼şÃû:%s",fn);
						Show_Str(10,10,lcddev.width-10,16,namebuf,16,1);//ÏÔÊ¾ÎÄ¼şÃû
						sprintf((char*)namebuf,"Ë÷Òı:%d/%d",index,total);	
						Show_Str(10,30,lcddev.width-10,16,namebuf,16,1);//ÏÔÊ¾Ë÷Ò
						break; 
					} 					   	
				}
			}
			I2S_Play_Stop();	//¹Ø±ÕÒôÆµ
			TIM6->CR1&=~(1<<0); //¹Ø±Õ¶¨Ê±Æ÷6
			LCD_Set_Window(0,0,lcddev.width,lcddev.height);//»Ö¸´´°¿Ú
			mjpegdec_free();	//ÊÍ·ÅÄÚ´æ
			f_close(favi); 
		}
	}
	myfree(SRAMIN,i2sbuf[0]);
	myfree(SRAMIN,i2sbuf[1]);
	myfree(SRAMIN,i2sbuf[2]);
	myfree(SRAMIN,i2sbuf[3]);
	myfree(SRAMIN,framebuf);
	myfree(SRAMIN,favi);
	myfree(SRAMIN,namebuf);		
	return res;
}
//aviÎÄ¼ş²éÕÒ
u8 video_seek(FIL *favi,AVI_INFO *aviinfo,u8 *mbuf)
{
	u32 fpos=favi->fptr;
	u8 *pbuf;
	u16 offset;
	u32 br;
	u32 delta;
	u32 totsec;
	u8 key; 
	totsec=(aviinfo->SecPerFrame/1000)*aviinfo->TotalFrame;	
	totsec/=1000;//ÃëÖÓÊı.
	delta=(favi->fsize/totsec)*5;		//Ã¿´ÎÇ°½ø5ÃëÖÓµÄÊı¾İÁ¿ 
	while(1)
	{
		key=KEY_Scan(1); 
		if(key==WKUP_PRES)//¿ì½ø
		{
			if(fpos<favi->fsize)fpos+=delta; 
			if(fpos>(favi->fsize-AVI_VIDEO_BUF_SIZE))
			{
				fpos=favi->fsize-AVI_VIDEO_BUF_SIZE;
			}
		}else if(key==KEY1_PRES)//¿ìÍË
		{
			if(fpos>delta)fpos-=delta;
			else fpos=0; 
		}else break;
		f_lseek(favi,fpos);
		f_read(favi,mbuf,AVI_VIDEO_BUF_SIZE,&br);	//¶ÁÈëÕûÖ¡+ÏÂÒ»Êı¾İÁ÷IDĞÅÏ¢ 
		pbuf=mbuf; 
		if(fpos==0) //´Ó0¿ªÊ¼,µÃÏÈÑ°ÕÒmovi ID
		{
			offset=avi_srarch_id(pbuf,AVI_VIDEO_BUF_SIZE,"movi"); 
		}else offset=0;
		offset+=avi_srarch_id(pbuf+offset,AVI_VIDEO_BUF_SIZE,avix.VideoFLAG);	//Ñ°ÕÒÊÓÆµÖ¡	
		avi_get_streaminfo(pbuf+offset);			//»ñÈ¡Á÷ĞÅÏ¢ 
		f_lseek(favi,fpos+offset+8);				//Ìø¹ı±êÖ¾ID,¶ÁµØÖ·Æ«ÒÆµ½Á÷Êı¾İ¿ªÊ¼´¦	 
		if(avix.StreamID==AVI_VIDS_FLAG)
		{
			f_read(favi,mbuf,avix.StreamSize+8,&br);	//¶ÁÈëÕûÖ¡ 
			mjpegdec_decode(mbuf,avix.StreamSize); 		//ÏÔÊ¾ÊÓÆµÖ¡
		}else 
		{
			printf("error flag");
		}
		video_time_show(favi,&avix);  
	}
	return 0;
}






















