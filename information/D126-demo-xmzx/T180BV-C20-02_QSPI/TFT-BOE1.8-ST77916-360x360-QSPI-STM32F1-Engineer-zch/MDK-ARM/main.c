#include "stm32f10x.h"                  // Device header
#include "sys.h"
#include "delay.h"


//#include "led.h"
//#include "oled.h"
#include "timer.h"
////#include "usart.h"
#include "tftlcd.h"
#include "pic.h"
//#include "exti.h"

int flg=1;
int sec=55,min=59,hour=23;
u16 count;
#define JTAG_SWD_DISABLE   0X02
#define SWD_ENABLE         0X01
#define JTAG_SWD_ENABLE    0X00	

void JTAG_Set(u8 mode)
{
	u32 temp;
	temp=mode;
	temp<<=25;
	RCC->APB2ENR|=1<<0;     //开启辅助时钟	   
	AFIO->MAPR&=0XF8FFFFFF; //清除MAPR的[26:24]
	AFIO->MAPR|=temp;       //设置jtag模式
} 




int main(void)
{
	int speed = 1;
	
	SystemInit();	
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级

	JTAG_Set(JTAG_SWD_DISABLE);     
	JTAG_Set(SWD_ENABLE);  
	delay_ms(10);	
	Timer3_Init(9999,7199);
	TFTLCD_Init();

		//显示红色
		DispColor(RED);	
		delay_ms(speed);
		//显示绿色
		DispColor(GREEN);
		delay_ms(speed);
		//显示蓝色
		DispColor(BLUE);
	//	DispColorQSPI(0x00,0x1f);
		delay_ms(speed);
		TFTLCD_QSPI_DrawPoint(100,100,YELLOW);
		delay_ms(speed);
		TFTLCD_QSPI_DrawLine(150,150,260,260,YELLOW);
		delay_ms(speed);
		TFTLCD_QSPI_DrawRectangle(100,100,250,250,BRED);
		delay_ms(speed);
		TFTLCD_QSPI_DrawCircle(180,180,100,GREEN);
		delay_ms(speed);
		TFTLCD_QSPI_DrawCircle(180,180,50,WHITE);
		delay_ms(speed);
		TFTLCD_QSPI_ShowChar(150,150,'a',BLACK,RED,24,0);
		delay_ms(speed);
		TFTLCD_QSPI_DrawPic(100,100,40,40,pic_eval);
		delay_ms(speed);
		TFTLCD_QSPI_ShowString(90,70,"i love you",BLACK,YELLOW,16,0);
		delay_ms(speed);
		TFTLCD_QSPI_ShowIntNum(160,160,13,2,RED,GREEN,24);
		delay_ms(speed);
		TFTLCD_QSPI_ShowFloatNum1(120,120,12.34,4,BLUE,BLACK,24);

		delay_ms(speed);

	while(1)
	{

			TFTLCD_QSPI_ShowString(100,100,"time:",BLACK,GREEN,12,0);
			TFTLCD_QSPI_ShowIntNum(100,115,hour/10,1,RED,GREEN,12);
			TFTLCD_QSPI_ShowIntNum(106,115,hour%10,1,RED,GREEN,12);
			TFTLCD_QSPI_ShowChar(112,115,':',RED,GREEN,12,0);
			TFTLCD_QSPI_ShowIntNum(118,115,min/10,1,RED,GREEN,12);
			TFTLCD_QSPI_ShowIntNum(124,115,min%10,1,RED,GREEN,12);
			TFTLCD_QSPI_ShowChar(130,115,':',RED,GREEN,12,0);
			TFTLCD_QSPI_ShowIntNum(136,115,sec/10,1,RED,GREEN,12);
			TFTLCD_QSPI_ShowIntNum(142,115,sec%10,1,RED,GREEN,12);
			if(sec==20)
			{
				sec=55;min=59;hour=23;
				break;
			}
	}
}

