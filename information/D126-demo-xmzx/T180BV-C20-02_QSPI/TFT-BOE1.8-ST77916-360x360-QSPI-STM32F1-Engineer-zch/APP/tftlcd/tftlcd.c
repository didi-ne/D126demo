#include "tftlcd.h"
#include "delay.h"
#include "lcdfont.h"
//1.3寸LCD  240X240  IPS
//1.8寸TFT  128x160  RGB_TFT
//tftlcd gpio初始化
void TFTLCD_GPIO_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	
	GPIO_InitStructure.GPIO_Pin = TFTLCD_SCL_GPIO_PIN|TFTLCD_SDA_GPIO_PIN|TFTLCD_SDA1_GPIO_PIN|TFTLCD_SDA2_GPIO_PIN|TFTLCD_RST_GPIO_PIN|TFTLCD_CS_GPIO_PIN|TFTLCD_BLK_GPIO_PIN;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//50MHz
 	GPIO_Init(TFTLCD_SCL_GPIO_PORT, &GPIO_InitStructure);	 
 	GPIO_SetBits(TFTLCD_SCL_GPIO_PORT,TFTLCD_SCL_GPIO_PIN|TFTLCD_SDA_GPIO_PIN|TFTLCD_SDA1_GPIO_PIN|TFTLCD_SDA2_GPIO_PIN|TFTLCD_RST_GPIO_PIN|TFTLCD_CS_GPIO_PIN|TFTLCD_BLK_GPIO_PIN);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	GPIO_InitStructure.GPIO_Pin = TFTLCD_SDA3_GPIO_PIN;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//50MHz
 	GPIO_Init(TFTLCD_SDA3_GPIO_PORT, &GPIO_InitStructure);	 
 	GPIO_SetBits(TFTLCD_SDA3_GPIO_PORT,TFTLCD_SDA3_GPIO_PIN);

}



void TFTLCD_Writ_Bus(u8 dat) 
{	
	u8 i;
	//TFTLCD_CS_RESET;
	for(i=0;i<8;i++)
	{			  	
		if(dat&0x80)
		{
			TFTLCD_SDA_SET;
		}
		else
		{
			TFTLCD_SDA_RESET;
		}
		TFTLCD_SCL_RESET;delay_us(3);
		TFTLCD_SCL_SET;
		dat<<=1;
	}	
	//TFTLCD_CS_SET;	
}
 
void TFTLCD_WR_DATA8(u8 dat)
{
//	TFTLCD_CS_RESET;
	TFTLCD_SDA_SET;
	TFTLCD_SCL_RESET;
	delay_us(5);
	TFTLCD_SCL_SET;
	TFTLCD_Writ_Bus(dat);
//	TFTLCD_CS_SET;
}
 
void TFTLCD_WR_DATA(u16 dat)
{
	TFTLCD_WR_DATA8(dat>>8);
	TFTLCD_WR_DATA8(dat);
}
 
void TFTLCD_WR_REG(u8 dat)
{
	//TFTLCD_CS_RESET;
	TFTLCD_SDA_RESET;
	TFTLCD_SCL_RESET;
	delay_us(5);
	TFTLCD_SCL_SET;
	TFTLCD_Writ_Bus(dat);
//TFTLCD_CS_SET;
}

void TFTLCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
//	if(USE_HORIZONTAL==0)
//	{
		TFTLCD_WR_REG(0x2a);
		TFTLCD_WR_DATA(x1+TFT_COLUMN_OFFSET);
		TFTLCD_WR_DATA(x2+TFT_COLUMN_OFFSET);
		TFTLCD_WR_REG(0x2b);
		TFTLCD_WR_DATA(y1+TFT_LINE_OFFSET);
		TFTLCD_WR_DATA(y2+TFT_LINE_OFFSET);
		TFTLCD_WR_REG(0x2c);
//	}
//	else if(USE_HORIZONTAL==1)
//	{
//		TFTLCD_WR_REG(0x2a);
//		TFTLCD_WR_DATA(x1+TFT_COLUMN_OFFSET);
//		TFTLCD_WR_DATA(x2+TFT_COLUMN_OFFSET);
//		TFTLCD_WR_REG(0x2b);
//		TFTLCD_WR_DATA(y1+TFT_LINE_OFFSET);
//		TFTLCD_WR_DATA(y2+TFT_LINE_OFFSET);
//		TFTLCD_WR_REG(0x2c);
//	}
//	else if(USE_HORIZONTAL==2)
//	{
//		TFTLCD_WR_REG(0x2a);
//		TFTLCD_WR_DATA(x1+TFT_COLUMN_OFFSET);
//		TFTLCD_WR_DATA(x2+TFT_COLUMN_OFFSET);
//		TFTLCD_WR_REG(0x2b);
//		TFTLCD_WR_DATA(y1+TFT_LINE_OFFSET);
//		TFTLCD_WR_DATA(y2+TFT_LINE_OFFSET);
//		TFTLCD_WR_REG(0x2c);
//	}
//	else
//	{
//		TFTLCD_WR_REG(0x2a);
//		TFTLCD_WR_DATA(x1);
//		TFTLCD_WR_DATA(x2);
//		TFTLCD_WR_REG(0x2b);
//		TFTLCD_WR_DATA(y1);
//		TFTLCD_WR_DATA(y2);
//		TFTLCD_WR_REG(0x2c);
//	}
}






//初始化
void TFTLCD_Init(void)
{
	TFTLCD_GPIO_Init();
	
	TFTLCD_RST_RESET;
	delay_ms(100);
	TFTLCD_RST_SET;
	delay_ms(100);
	
	TFTLCD_BLK_SET;
	delay_ms(100);

	TFTLCD_CS_RESET;
	write_cmd(0xF0);
	write_dat(0x28);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF2);
	write_dat(0x28);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x73);
	write_dat(0xF0);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x76);
	write_dat(0x0F);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x7C);
	write_dat(0xD1);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x83);
	write_dat(0xE0);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x84);
	write_dat(0x61);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF2);
	write_dat(0x82);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF0);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF0);
	write_dat(0x01);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF1);
	write_dat(0x01);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB0);
	write_dat(0x52);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB1);
	write_dat(0x49);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB2);
	write_dat(0x24);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB3);
	write_dat(0x01);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB4);
	write_dat(0x66);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB5);
	write_dat(0x44);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB6);
	write_dat(0xC5);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB7);
	write_dat(0x40);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB8);
	write_dat(0x86);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB9);
	write_dat(0x15);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBA);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBB);
	write_dat(0x08);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBC);
	write_dat(0x08);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBD);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBE);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBF);
	write_dat(0x07);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC0);
	write_dat(0x80);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC1);
	write_dat(0x10);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC2);
	write_dat(0x37);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC3);
	write_dat(0x80);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC4);
	write_dat(0x10);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC5);
	write_dat(0x37);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC6);
	write_dat(0xA9);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC7);
	write_dat(0x41);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC8);
	write_dat(0x01);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC9);
	write_dat(0xA9);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xCA);
	write_dat(0x41);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xCB);
	write_dat(0x01);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xCC);
	write_dat(0x7F);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xCD);
	write_dat(0x7F);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xCE);
	write_dat(0xFF);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD0);
	write_dat(0x91);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD1);
	write_dat(0x68);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD2);
	write_dat(0x68);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF5);
	write_dat(0x00);
	write_dat(0xA5);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF1);
	write_dat(0x10);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF0);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF0);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE0);
	write_dat(0xF0);
	write_dat(0x0E);
	write_dat(0x14);
	write_dat(0x0B);
	write_dat(0x0B);
	write_dat(0x16);
	write_dat(0x3A);
	write_dat(0x44);
	write_dat(0x4E);
	write_dat(0x18);
	write_dat(0x14);
	write_dat(0x13);
	write_dat(0x2F);
	write_dat(0x35);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE1 );
	write_dat(0xF0);
	write_dat(0x0D);
	write_dat(0x13);
	write_dat(0x0B);
	write_dat(0x0A);
	write_dat(0x16);
	write_dat(0x39);
	write_dat(0x43);
	write_dat(0x4E);
	write_dat(0x17);
	write_dat(0x13);
	write_dat(0x13);
	write_dat(0x2E);
	write_dat(0x34);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF0);
	write_dat(0x10);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF3);
	write_dat(0x10);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE0);
	write_dat(0x09);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE1);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE2);
	write_dat(0x03);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE3);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE4);
	write_dat(0xE0);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE5);
	write_dat(0x06);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE6);
	write_dat(0x21);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE7);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE8);
	write_dat(0x05);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xE9);
	write_dat(0x82);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xEA);
	write_dat(0xDE);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xEB);
	write_dat(0xC0);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xEC);
	write_dat(0x40);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xED);
	write_dat(0x84);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xEE);
	write_dat(0xFF);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xEF);
	write_dat(0x71);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF8);
	write_dat(0xFF);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF9);
	write_dat(0x50);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xFA);
	write_dat(0xFF);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xFB);
	write_dat(0xF3);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xFC);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xFD);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xFE);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xFF);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x60);
	write_dat(0x42);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x61);
	write_dat(0xDF);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x62);
	write_dat(0x40);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x63);
	write_dat(0x40);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x64);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x65);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x66);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x67);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x68);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x69);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x6A);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x6B);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x70);
	write_dat(0x42);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x71);
	write_dat(0xDF);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x72);
	write_dat(0x40);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x73);
	write_dat(0x40);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x74);
	write_dat(0x01);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x75);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x76);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x77);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x78);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x79);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x7A);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x7B);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x80);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x81);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x82);
	write_dat(0x04);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x83);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x84);
	write_dat(0xDC);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x85);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x86);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x87);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x88);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x89);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x8A);
	write_dat(0x06);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x8B);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x8C);
	write_dat(0xDE);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x8D);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x8E);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x8F);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x90);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x91);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x92);
	write_dat(0x08);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x93);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x94);
	write_dat(0xE0);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x95);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x96);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x97);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x98);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x99);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x9A);
	write_dat(0x0A);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x9B);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x9C);
	write_dat(0xE2);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x9D);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x9E);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x9F);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA0);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA1);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA2);
	write_dat(0x03);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA3);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA4);
	write_dat(0xDB);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA5);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA6);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA7);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA8);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xA9);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xAA);
	write_dat(0x05);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xAB);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xAC);
	write_dat(0xDD);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xAD);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xAE);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xAF);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB0);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB1);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB2);
	write_dat(0x07);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB3);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB4);
	write_dat(0xDF);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB5);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB6);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB7);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB8);
	write_dat(0x48);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xB9);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBA);
	write_dat(0x09);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBB);
	write_dat(0x02);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBC);
	write_dat(0xE1);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBD);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBE);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xBF);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC0);
	write_dat(0x65);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC1);
	write_dat(0x74);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC2);
	write_dat(0x47);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC3);
	write_dat(0x56);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC4);
	write_dat(0xAA);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC5);
	write_dat(0x11);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC6);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC7);
	write_dat(0x2A);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC8);
	write_dat(0xA2);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xC9);
	write_dat(0x33);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD0);
	write_dat(0x65);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD1);
	write_dat(0x74);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD2);
	write_dat(0x47);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD3);
	write_dat(0x56);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD4);
	write_dat(0xAA);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD5);
	write_dat(0x11);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD6);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD7);
	write_dat(0x2A);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD8);
	write_dat(0xA2);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xD9);
	write_dat(0x33);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF3);
	write_dat(0x01);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0xF0);
	write_dat(0x00);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x21);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x11);
	TFTLCD_CS_SET;

	delay_ms(120);

	TFTLCD_CS_RESET;
	write_cmd(0x29);
	TFTLCD_CS_SET;

	TFTLCD_CS_RESET;
	write_cmd(0x3A);
	write_dat(0x55);
	TFTLCD_CS_SET;

} 






#if TFT==153

//发送命令
void write_cmd(unsigned char cmd)
{	
	TFTLCD_Writ_Bus(0x02);
	TFTLCD_Writ_Bus(0x00);
	TFTLCD_Writ_Bus(cmd);
	TFTLCD_Writ_Bus(0x00);__NOP(); __NOP();__NOP();__NOP();
}

//发送数据
void write_dat(unsigned char dat)
{	
	TFTLCD_Writ_Bus(dat);
}

void SPI_WriteData1(unsigned char i)
{
	TFTLCD_SCL_RESET;__NOP(); __NOP();
	if(i&0x80) TFTLCD_SDA3_SET;
	else TFTLCD_SDA3_RESET;

	if(i&0x40) TFTLCD_SDA2_SET;
	else TFTLCD_SDA2_RESET;

	if(i&0x20) TFTLCD_SDA1_SET;
	else TFTLCD_SDA1_RESET;

	if(i&0x10)TFTLCD_SDA_SET;
	else TFTLCD_SDA_RESET;

	TFTLCD_SCL_SET;__NOP();__NOP();

	TFTLCD_SCL_RESET;__NOP(); __NOP();

	if(i&0x08) TFTLCD_SDA3_SET;
	else TFTLCD_SDA3_RESET;

	if(i&0x04) TFTLCD_SDA2_SET;
	else TFTLCD_SDA2_RESET;


	if(i&0x02) TFTLCD_SDA1_SET;
	else TFTLCD_SDA1_RESET;


	if(i&0x01) TFTLCD_SDA_SET;
	else TFTLCD_SDA_RESET;

	TFTLCD_SCL_SET;__NOP();__NOP();

}



//4路数据写入 4-wire Pixel Write Data Waveform 
void write_lcd(unsigned char dat)
{
	TFTLCD_SCL_RESET;
	
	if(dat & 0x80){ TFTLCD_SDA3_SET; }else{ TFTLCD_SDA3_RESET; }

	if(dat & 0x40){ TFTLCD_SDA2_SET; }else{ TFTLCD_SDA2_RESET; }
		
	if(dat & 0x20){ TFTLCD_SDA1_SET; }else{ TFTLCD_SDA1_RESET; }
	
	if(dat & 0x10){ TFTLCD_SDA_SET; }else{ TFTLCD_SDA_RESET; }
		
	TFTLCD_SCL_SET;

	TFTLCD_SCL_RESET;

	if(dat & 0x08){ TFTLCD_SDA3_SET; }else{TFTLCD_SDA3_RESET; }

	if(dat & 0x04){ TFTLCD_SDA2_SET; }else{ TFTLCD_SDA2_RESET; }
		
	if(dat & 0x02){ TFTLCD_SDA1_SET; }else{ TFTLCD_SDA1_RESET; }
	
	if(dat & 0x01){ TFTLCD_SDA_SET; }else{ TFTLCD_SDA_RESET; }

	TFTLCD_SCL_SET;
}

void BlockWrite(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend)
{
	//Ystart += 0;
	//Yend +=  30;
	TFTLCD_CS_RESET;
	write_cmd(0x2a);   
	write_dat(Xstart>>8);
	write_dat(Xstart&0xff);
	write_dat(Xend>>8);
	write_dat(Xend&0xff);
	TFTLCD_CS_SET;
	
	TFTLCD_CS_RESET;
	write_cmd(0x2b);   
	write_dat(Ystart>>8);
	write_dat(Ystart&0xff);
	write_dat(Yend>>8);
	write_dat(Yend&0xff);
	TFTLCD_CS_SET;
	
	TFTLCD_CS_RESET;
	write_cmd(0x2c);
	TFTLCD_CS_SET;
}

void TFTLCD_QSPI_DrawPoint(u16 x,u16 y,u16 color)
{
	unsigned int DataHs,DataLs;
	DataHs=(color>>8)&0xff;
	DataLs=color&0xff;

	BlockWrite(x,x,y,y);
	TFTLCD_CS_RESET;
	TFTLCD_Writ_Bus(0x32);//SPI_SendData(0x12);
 //0x002C00
	TFTLCD_Writ_Bus(0x00);
	TFTLCD_Writ_Bus(0x2c);
	TFTLCD_Writ_Bus(0x00);

	SPI_WriteData1(DataHs);
	SPI_WriteData1(DataLs);


	TFTLCD_CS_SET;
} 

void TFTLCD_QSPI_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; 
	delta_y=y2-y1;
	uRow=x1;
	uCol=y1;
	if(delta_x>0)incx=1; 
	else if(delta_x==0)incx=0;
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if(delta_y==0)incy=0;
	else {incy=-1;delta_y=-delta_y;}
	if(delta_x>delta_y)distance=delta_x;
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		TFTLCD_QSPI_DrawPoint(uRow,uCol,color);
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

void TFTLCD_QSPI_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	TFTLCD_QSPI_DrawLine(x1,y1,x2,y1,color);
	TFTLCD_QSPI_DrawLine(x1,y1,x1,y2,color);
	TFTLCD_QSPI_DrawLine(x1,y2,x2,y2,color);
	TFTLCD_QSPI_DrawLine(x2,y1,x2,y2,color);
}
 
void TFTLCD_QSPI_DrawCircle(u16 x0,u16 y0,u8 r,u16 color)
{
	int a,b;
	a=0;b=r;	  
	while(a<=b)
	{
		TFTLCD_QSPI_DrawPoint(x0-b,y0-a,color);             //3           
		TFTLCD_QSPI_DrawPoint(x0+b,y0-a,color);             //0           
		TFTLCD_QSPI_DrawPoint(x0-a,y0+b,color);             //1                
		TFTLCD_QSPI_DrawPoint(x0-a,y0-b,color);             //2             
		TFTLCD_QSPI_DrawPoint(x0+b,y0+a,color);             //4               
		TFTLCD_QSPI_DrawPoint(x0+a,y0-b,color);             //5
		TFTLCD_QSPI_DrawPoint(x0+a,y0+b,color);             //6 
		TFTLCD_QSPI_DrawPoint(x0-b,y0+a,color);             //7
		a++;
		if((a*a+b*b)>(r*r))
		{
			b--;
		}
	}
}

void TFTLCD_QSPI_DrawPic(u16 x,u16 y,u16 w,u16 h,unsigned int *picture)
{
	unsigned int i,j;

	BlockWrite(x,x+w-1,y,y+h-1);

	TFTLCD_CS_RESET;
 
	TFTLCD_Writ_Bus(0x32);//SPI_SendData(0x12);
 //0x002C00
	TFTLCD_Writ_Bus(0x00);
	TFTLCD_Writ_Bus(0x2c);
	TFTLCD_Writ_Bus(0x00);


	for(i=0;i<w;i++)
	{ 
		for(j=0;j<h;j++)
		{
			SPI_WriteData1(*picture++);
			SPI_WriteData1(*picture++);
		}
	} 

	TFTLCD_CS_SET;

}

void TFTLCD_QSPI_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 temp,sizex,t,m=0;
	u16 i,TypefaceNum;
	u16 x0=x;
	unsigned int DataHs,DataLs;

	sizex=sizey/2;
	TypefaceNum=(sizex/8+((sizex%8)?1:0))*sizey;
	num=num-' ';
	BlockWrite(x,x+sizex-1,y,y+sizey-1);

	TFTLCD_CS_RESET;
 
	TFTLCD_Writ_Bus(0x32);//SPI_SendData(0x12);
 //0x002C00
	TFTLCD_Writ_Bus(0x00);
	TFTLCD_Writ_Bus(0x2c);
	TFTLCD_Writ_Bus(0x00);


	for(i=0;i<TypefaceNum;i++)
	{ 
		if(sizey==12)temp=ascii_1206[num][i];		    
		else if(sizey==16)temp=ascii_1608[num][i];
		else if(sizey==24)temp=ascii_2412[num][i];
		else return;
		for(t=0;t<8;t++)
		{
			if(!mode)
			{
				if(temp&(0x01<<t)){	
					DataHs=(fc>>8)&0xff;
					DataLs=fc&0xff;
					SPI_WriteData1(DataHs);
					SPI_WriteData1(DataLs);
 
				}
				else{
					DataHs=(bc>>8)&0xff;
					DataLs=bc&0xff;
					SPI_WriteData1(DataHs);
					SPI_WriteData1(DataLs);
				}
				m++;
				if(m%sizex==0)
				{
					m=0;
					break;
				}
			}
			else
			{
				if(temp&(0x01<<t))TFTLCD_QSPI_DrawPoint(x,y,fc);
				x++;
				if((x-x0)==sizex)
				{
					x=x0;
					y++;
					break;
				}
			}
		}
	} 
	
	TFTLCD_CS_SET;		
}

void TFTLCD_QSPI_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode)
{         
	while(*p!='\0')
	{       
		TFTLCD_QSPI_ShowChar(x,y,*p,fc,bc,sizey,mode);
		x+=sizey/2;
		p++;
	}  
}

u32 mypowQSPI(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;
	return result;
}


void TFTLCD_QSPI_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey)
{         	
	u8 t,temp;
	u8 enshow=0;
	u8 sizex=sizey/2;
	for(t=0;t<len;t++)
	{
		temp=(num/mypowQSPI(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				TFTLCD_QSPI_ShowChar(x+t*sizex,y,' ',fc,bc,sizey,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	TFTLCD_QSPI_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
} 
 
void TFTLCD_QSPI_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey)
{         	
	u8 t,temp,sizex;
	u16 num1;
	sizex=sizey/2;
	num1=num*100;
	for(t=0;t<len;t++)
	{
		temp=(num1/mypow(10,len-t-1))%10;
		if(t==(len-2))
		{
			TFTLCD_QSPI_ShowChar(x+(len-2)*sizex,y,'.',fc,bc,sizey,0);
			t++;
			len+=1;
		}
	 	TFTLCD_QSPI_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
}


void DispColorsQspi(unsigned int xstart,unsigned int xend,unsigned int ystart,unsigned int yend,unsigned int color)
{
	unsigned int i,j,DataHs,DataLs;
	DataHs=(color>>8)&0xff;
	DataLs=color&0xff;

	BlockWrite(xstart,xend,ystart,yend);
	
	
	TFTLCD_CS_RESET;
	TFTLCD_Writ_Bus(0x32);//SPI_SendData(0x12);
 //0x002C00
	TFTLCD_Writ_Bus(0x00);
	TFTLCD_Writ_Bus(0x2c);
	TFTLCD_Writ_Bus(0x00);

	for(i=100;i<200;i++)
	{
		for(j=100;j<300;j++)
		{
			SPI_WriteData1(DataHs);
			SPI_WriteData1(DataLs);
		}
	}
	TFTLCD_CS_SET;
} 



void DispColor(unsigned int color)
{
	unsigned int i,j,DataH,DataL;
	DataH=(color>>8)&0xff;
	DataL=color&0xff;

	BlockWrite(0,COL-1,0,ROW-1);
	
	
	TFTLCD_CS_RESET;
	TFTLCD_Writ_Bus(0x32);//SPI_SendData(0x12);
 //0x002C00
	TFTLCD_Writ_Bus(0x00);
	TFTLCD_Writ_Bus(0x2c);
	TFTLCD_Writ_Bus(0x00);

	for(i=0;i<COL;i++)
	{
		for(j=0;j<ROW;j++)
		{
			SPI_WriteData1(DataH);
			SPI_WriteData1(DataL);
		}
	}
	TFTLCD_CS_SET;
} 


void DispColorQSPI(unsigned char data1,unsigned char data2)
{
	unsigned int i,j;
	BlockWrite(0,COL-1,0,ROW-1);
	
	TFTLCD_CS_RESET;
	
	TFTLCD_Writ_Bus(0x12);
	write_lcd(0x00);
	write_lcd(0x2C);
	write_lcd(0x00);


	for(i=0;i<COL;i++)
	{	
		for(j=0;j<ROW;j++)
		{
			write_lcd(data1);
			write_lcd(data2);
		}
	}	
	 
	TFTLCD_CS_SET;
}

#endif

void TFTLCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{          
	u16 i,j; 
	TFTLCD_Address_Set(xsta,ysta,xend-1,yend-1);
	for(i=ysta;i<yend;i++)
	{													   	 	
		for(j=xsta;j<xend;j++)
		{
			TFTLCD_WR_DATA(color);
		}
	} 					  	    
}
 
void TFTLCD_DrawPoint(u16 x,u16 y,u16 color)
{
	TFTLCD_Address_Set(x,y,x,y);
	TFTLCD_WR_DATA(color);
} 
 
void TFTLCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; 
	delta_y=y2-y1;
	uRow=x1;
	uCol=y1;
	if(delta_x>0)incx=1; 
	else if(delta_x==0)incx=0;
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if(delta_y==0)incy=0;
	else {incy=-1;delta_y=-delta_y;}
	if(delta_x>delta_y)distance=delta_x;
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		TFTLCD_DrawPoint(uRow,uCol,color);
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

void TFTLCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	TFTLCD_DrawLine(x1,y1,x2,y1,color);
	TFTLCD_DrawLine(x1,y1,x1,y2,color);
	TFTLCD_DrawLine(x1,y2,x2,y2,color);
	TFTLCD_DrawLine(x2,y1,x2,y2,color);
}
 
void TFTLCD_DrawCircle(u16 x0,u16 y0,u8 r,u16 color)
{
	int a,b;
	a=0;b=r;	  
	while(a<=b)
	{
		TFTLCD_DrawPoint(x0-b,y0-a,color);             //3           
		TFTLCD_DrawPoint(x0+b,y0-a,color);             //0           
		TFTLCD_DrawPoint(x0-a,y0+b,color);             //1                
		TFTLCD_DrawPoint(x0-a,y0-b,color);             //2             
		TFTLCD_DrawPoint(x0+b,y0+a,color);             //4               
		TFTLCD_DrawPoint(x0+a,y0-b,color);             //5
		TFTLCD_DrawPoint(x0+a,y0+b,color);             //6 
		TFTLCD_DrawPoint(x0-b,y0+a,color);             //7
		a++;
		if((a*a+b*b)>(r*r))
		{
			b--;
		}
	}
}

void TFTLCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 temp,sizex,t,m=0;
	u16 i,TypefaceNum;
	u16 x0=x;
	sizex=sizey/2;
	TypefaceNum=(sizex/8+((sizex%8)?1:0))*sizey;
	num=num-' ';
	TFTLCD_Address_Set(x,y,x+sizex-1,y+sizey-1);
	for(i=0;i<TypefaceNum;i++)
	{ 
		if(sizey==12)temp=ascii_1206[num][i];		    
		else if(sizey==16)temp=ascii_1608[num][i];
		else if(sizey==24)temp=ascii_2412[num][i];
		else return;
		for(t=0;t<8;t++)
		{
			if(!mode)
			{
				if(temp&(0x01<<t))TFTLCD_WR_DATA(fc);
				else TFTLCD_WR_DATA(bc);
				m++;
				if(m%sizex==0)
				{
					m=0;
					break;
				}
			}
			else
			{
				if(temp&(0x01<<t))TFTLCD_DrawPoint(x,y,fc);
				x++;
				if((x-x0)==sizex)
				{
					x=x0;
					y++;
					break;
				}
			}
		}
	}   	 	  
}
 
 
void TFTLCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode)
{         
	while(*p!='\0')
	{       
		TFTLCD_ShowChar(x,y,*p,fc,bc,sizey,mode);
		x+=sizey/2;
		p++;
	}  
}
 
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;
	return result;
}


void TFTLCD_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey)
{         	
	u8 t,temp;
	u8 enshow=0;
	u8 sizex=sizey/2;
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				TFTLCD_ShowChar(x+t*sizex,y,' ',fc,bc,sizey,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	TFTLCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
} 
 
void TFTLCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey)
{         	
	u8 t,temp,sizex;
	u16 num1;
	sizex=sizey/2;
	num1=num*100;
	for(t=0;t<len;t++)
	{
		temp=(num1/mypow(10,len-t-1))%10;
		if(t==(len-2))
		{
			TFTLCD_ShowChar(x+(len-2)*sizex,y,'.',fc,bc,sizey,0);
			t++;
			len+=1;
		}
	 	TFTLCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
}
 
void TFTLCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[])
{
	u16 i,j;
	u32 k=0;
	TFTLCD_Address_Set(x,y,x+length-1,y+width-1);
	for(i=0;i<length;i++)
	{
		for(j=0;j<width;j++)
		{
			TFTLCD_WR_DATA8(pic[k*2]);
			TFTLCD_WR_DATA8(pic[k*2+1]);
			k++;
		}
	}			
}




void TFTLCD_Image(u16 x, u16 y, u16 width, u16 height, const u8 *image) 
{  
	u16 colorData=0;
	u32 cnt=0;
	u16 i=0,j=0;
 	for(i=0; i<height; i++)                     // 一行一行地显示
	{
 		TFTLCD_Address_Set(x, y+i, x+width, y+height);  // 重新设置光标位置 
		for(j=0; j<width; j++)                  // 一行中，从左到事，逐个像素处理
		{		    
			colorData=(image[cnt*2+1]<<8) | image[cnt*2]; 
			TFTLCD_WR_DATA(colorData);             // 写入16位颜色数据 
			cnt++;	
		}
	}	  
}  










