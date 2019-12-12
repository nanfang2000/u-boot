/*
 * (C) Copyright 2012-2013 Henrik Nordstrom <henrik@henriknordstrom.net>
 * (C) Copyright 2013 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 *
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * Some board init for the Allwinner A10-evb board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#include <axp_pmic.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include <asm/arch/display.h>
#include <asm/arch/dram.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/spl.h>
#include <asm/arch/usb_phy.h>
#ifndef CONFIG_ARM64
#include <asm/armv7.h>
#endif
#include <asm/gpio.h>
#include <asm/io.h>
#include <crc.h>
#include <environment.h>
#include <libfdt.h>
#include <nand.h>
#include <net.h>
#include <spl.h>
#include <sy8106a.h>
#include <asm/setup.h>

#if defined CONFIG_VIDEO_LCD_PANEL_I2C && !(defined CONFIG_SPL_BUILD)
/* So that we can use pin names in Kconfig and sunxi_name_to_gpio() */
int soft_i2c_gpio_sda;
int soft_i2c_gpio_scl;

static int soft_i2c_board_init(void)
{
	int ret;

	soft_i2c_gpio_sda = sunxi_name_to_gpio(CONFIG_VIDEO_LCD_PANEL_I2C_SDA);
	if (soft_i2c_gpio_sda < 0) {
		printf("Error invalid soft i2c sda pin: '%s', err %d\n",
		       CONFIG_VIDEO_LCD_PANEL_I2C_SDA, soft_i2c_gpio_sda);
		return soft_i2c_gpio_sda;
	}
	ret = gpio_request(soft_i2c_gpio_sda, "soft-i2c-sda");
	if (ret) {
		printf("Error requesting soft i2c sda pin: '%s', err %d\n",
		       CONFIG_VIDEO_LCD_PANEL_I2C_SDA, ret);
		return ret;
	}

	soft_i2c_gpio_scl = sunxi_name_to_gpio(CONFIG_VIDEO_LCD_PANEL_I2C_SCL);
	if (soft_i2c_gpio_scl < 0) {
		printf("Error invalid soft i2c scl pin: '%s', err %d\n",
		       CONFIG_VIDEO_LCD_PANEL_I2C_SCL, soft_i2c_gpio_scl);
		return soft_i2c_gpio_scl;
	}
	ret = gpio_request(soft_i2c_gpio_scl, "soft-i2c-scl");
	if (ret) {
		printf("Error requesting soft i2c scl pin: '%s', err %d\n",
		       CONFIG_VIDEO_LCD_PANEL_I2C_SCL, ret);
		return ret;
	}

	return 0;
}
#else
static int soft_i2c_board_init(void) { return 0; }
#endif

#define LCD_SPI_CS(a)	gpio_set_value(SUNXI_GPE(7), a)
#define SPI_DCLK(a)	gpio_set_value(SUNXI_GPE(9), a)
#define SPI_SDA(a)	gpio_set_value(SUNXI_GPE(8), a)

volatile void LCD_delay(int j)
{
	volatile uint16_t i;	
	while(j--)
	for(i=7200;i>0;i--);
}
void LCD_WriteByteSPI(unsigned char byte)
{
		unsigned char n;
   
   for(n=0; n<8; n++)			
   {  
	  if(byte&0x80) SPI_SDA(1);
      	else SPI_SDA(0);
      byte<<= 1;
		 
	  SPI_DCLK(0);
    SPI_DCLK(1);
   }
}
static void SPI_WriteComm(uint16_t CMD)
{	
    LCD_SPI_CS(0);
	LCD_WriteByteSPI(0X20);
	LCD_WriteByteSPI(CMD>>8);
	LCD_WriteByteSPI(0X00);
	LCD_WriteByteSPI(CMD);
	
	LCD_SPI_CS(1);
	// uint8_t cmd[4] = {0x20, CMD>>8, 0x00, CMD};		
	// spi_device_select(spi_lcd_dev);
	// spi_device_write_then_read(spi_lcd_dev, &cmd, 4, 0, 0);
	// spi_device_deselect(spi_lcd_dev);
}
static void SPI_WriteData(uint16_t tem_data)
{	
    LCD_SPI_CS(0);
	LCD_WriteByteSPI(0x40);
	LCD_WriteByteSPI(tem_data);
	LCD_SPI_CS(1);

	// uint8_t cmd[2] = {0x40, tem_data};			
	// spi_device_select(spi_lcd_dev);
	// spi_device_write_then_read(spi_lcd_dev, &cmd, 2, 0, 0);
	// spi_device_deselect(spi_lcd_dev);
}
static void LCD_Reset(void)
{			
	//注意，现在科学发达，有的屏不用复位也行 
    gpio_set_value(SUNXI_GPE(5), 0);
    LCD_delay(200);					   
    gpio_set_value(SUNXI_GPE(5), 1);	 
    LCD_delay(200);	
}

static int soft_spi_board_init(void)
{
	int ret;
	// SPI 3 lines for LCD driver chipe initialization
	sunxi_gpio_set_cfgpin(SUNXI_GPE(5), SUNXI_GPIO_OUTPUT); //RST
	sunxi_gpio_set_cfgpin(SUNXI_GPE(7), SUNXI_GPIO_OUTPUT); //CS
	sunxi_gpio_set_cfgpin(SUNXI_GPE(8), SUNXI_GPIO_OUTPUT); //DAT
	sunxi_gpio_set_cfgpin(SUNXI_GPE(9), SUNXI_GPIO_OUTPUT); //CLK

    LCD_Reset();
	
    SPI_WriteComm(0xf000);SPI_WriteData(0x0055);	
    SPI_WriteComm(0xf001);SPI_WriteData(0x00aa);	
    SPI_WriteComm(0xf002);SPI_WriteData(0x0052);	
    SPI_WriteComm(0xf003);SPI_WriteData(0x0008);	
    SPI_WriteComm(0xf004);SPI_WriteData(0x0001);	
                                            
    SPI_WriteComm(0xbc01);SPI_WriteData(0x0086);	
    SPI_WriteComm(0xbc02);SPI_WriteData(0x006a);	
    SPI_WriteComm(0xbd01);SPI_WriteData(0x0086);	
    SPI_WriteComm(0xbd02);SPI_WriteData(0x006a);	
    SPI_WriteComm(0xbe01);SPI_WriteData(0x0067);	
                                            
    SPI_WriteComm(0xd100);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd101);SPI_WriteData(0x005d);	
    SPI_WriteComm(0xd102);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd103);SPI_WriteData(0x006b);	
    SPI_WriteComm(0xd104);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd105);SPI_WriteData(0x0084);	
    SPI_WriteComm(0xd106);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd107);SPI_WriteData(0x009c);	
    SPI_WriteComm(0xd108);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd109);SPI_WriteData(0x00b1);	
    SPI_WriteComm(0xd10a);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd10b);SPI_WriteData(0x00d9);	
    SPI_WriteComm(0xd10c);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd10d);SPI_WriteData(0x00fd);	
    SPI_WriteComm(0xd10e);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd10f);SPI_WriteData(0x0038);	
    SPI_WriteComm(0xd110);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd111);SPI_WriteData(0x0068);	
    SPI_WriteComm(0xd112);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd113);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd114);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd115);SPI_WriteData(0x00fb);	
    SPI_WriteComm(0xd116);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd117);SPI_WriteData(0x0063);	
    SPI_WriteComm(0xd118);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd119);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd11a);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd11b);SPI_WriteData(0x00bb);	
    SPI_WriteComm(0xd11c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd11d);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd11e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd11f);SPI_WriteData(0x0046);	
    SPI_WriteComm(0xd120);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd121);SPI_WriteData(0x0069);	
    SPI_WriteComm(0xd122);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd123);SPI_WriteData(0x008f);	
    SPI_WriteComm(0xd124);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd125);SPI_WriteData(0x00a4);	
    SPI_WriteComm(0xd126);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd127);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd128);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd129);SPI_WriteData(0x00c7);	
    SPI_WriteComm(0xd12a);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd12b);SPI_WriteData(0x00c9);	
    SPI_WriteComm(0xd12c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd12d);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd12e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd12f);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd130);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd131);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd132);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd133);SPI_WriteData(0x00cc);	
                                            
    SPI_WriteComm(0xd200);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd201);SPI_WriteData(0x005d);	
    SPI_WriteComm(0xd202);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd203);SPI_WriteData(0x006b);	
    SPI_WriteComm(0xd204);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd205);SPI_WriteData(0x0084);	
    SPI_WriteComm(0xd206);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd207);SPI_WriteData(0x009c);	
    SPI_WriteComm(0xd208);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd209);SPI_WriteData(0x00b1);	
    SPI_WriteComm(0xd20a);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd20b);SPI_WriteData(0x00d9);	
    SPI_WriteComm(0xd20c);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd20d);SPI_WriteData(0x00fd);	
    SPI_WriteComm(0xd20e);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd20f);SPI_WriteData(0x0038);	
    SPI_WriteComm(0xd210);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd211);SPI_WriteData(0x0068);	
    SPI_WriteComm(0xd212);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd213);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd214);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd215);SPI_WriteData(0x00fb);	
    SPI_WriteComm(0xd216);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd217);SPI_WriteData(0x0063);	
    SPI_WriteComm(0xd218);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd219);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd21a);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd21b);SPI_WriteData(0x00bb);	
    SPI_WriteComm(0xd21c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd21d);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd21e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd21f);SPI_WriteData(0x0046);	
    SPI_WriteComm(0xd220);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd221);SPI_WriteData(0x0069);	
    SPI_WriteComm(0xd222);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd223);SPI_WriteData(0x008f);	
    SPI_WriteComm(0xd224);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd225);SPI_WriteData(0x00a4);	
    SPI_WriteComm(0xd226);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd227);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd228);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd229);SPI_WriteData(0x00c7);	
    SPI_WriteComm(0xd22a);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd22b);SPI_WriteData(0x00c9);	
    SPI_WriteComm(0xd22c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd22d);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd22e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd22f);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd230);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd231);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd232);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd233);SPI_WriteData(0x00cc);	
                                            
                                            
    SPI_WriteComm(0xd300);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd301);SPI_WriteData(0x005d);	
    SPI_WriteComm(0xd302);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd303);SPI_WriteData(0x006b);	
    SPI_WriteComm(0xd304);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd305);SPI_WriteData(0x0084);	
    SPI_WriteComm(0xd306);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd307);SPI_WriteData(0x009c);	
    SPI_WriteComm(0xd308);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd309);SPI_WriteData(0x00b1);	
    SPI_WriteComm(0xd30a);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd30b);SPI_WriteData(0x00d9);	
    SPI_WriteComm(0xd30c);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd30d);SPI_WriteData(0x00fd);	
    SPI_WriteComm(0xd30e);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd30f);SPI_WriteData(0x0038);	
    SPI_WriteComm(0xd310);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd311);SPI_WriteData(0x0068);	
    SPI_WriteComm(0xd312);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd313);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd314);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd315);SPI_WriteData(0x00fb);	
    SPI_WriteComm(0xd316);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd317);SPI_WriteData(0x0063);	
    SPI_WriteComm(0xd318);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd319);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd31a);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd31b);SPI_WriteData(0x00bb);	
    SPI_WriteComm(0xd31c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd31d);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd31e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd31f);SPI_WriteData(0x0046);	
    SPI_WriteComm(0xd320);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd321);SPI_WriteData(0x0069);	
    SPI_WriteComm(0xd322);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd323);SPI_WriteData(0x008f);	
    SPI_WriteComm(0xd324);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd325);SPI_WriteData(0x00a4);	
    SPI_WriteComm(0xd326);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd327);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd328);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd329);SPI_WriteData(0x00c7);	
    SPI_WriteComm(0xd32a);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd32b);SPI_WriteData(0x00c9);	
    SPI_WriteComm(0xd32c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd32d);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd32e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd32f);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd330);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd331);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd332);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd333);SPI_WriteData(0x00cc);	
                                            
    SPI_WriteComm(0xd400);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd401);SPI_WriteData(0x005d);	
    SPI_WriteComm(0xd402);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd403);SPI_WriteData(0x006b);	
    SPI_WriteComm(0xd404);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd405);SPI_WriteData(0x0084);	
    SPI_WriteComm(0xd406);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd407);SPI_WriteData(0x009c);	
    SPI_WriteComm(0xd408);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd409);SPI_WriteData(0x00b1);	
    SPI_WriteComm(0xd40a);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd40b);SPI_WriteData(0x00d9);	
    SPI_WriteComm(0xd40c);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd40d);SPI_WriteData(0x00fd);	
    SPI_WriteComm(0xd40e);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd40f);SPI_WriteData(0x0038);	
    SPI_WriteComm(0xd410);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd411);SPI_WriteData(0x0068);	
    SPI_WriteComm(0xd412);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd413);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd414);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd415);SPI_WriteData(0x00fb);	
    SPI_WriteComm(0xd416);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd417);SPI_WriteData(0x0063);	
    SPI_WriteComm(0xd418);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd419);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd41a);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd41b);SPI_WriteData(0x00bb);	
    SPI_WriteComm(0xd41c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd41d);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd41e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd41f);SPI_WriteData(0x0046);	
    SPI_WriteComm(0xd420);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd421);SPI_WriteData(0x0069);	
    SPI_WriteComm(0xd422);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd423);SPI_WriteData(0x008f);	
    SPI_WriteComm(0xd424);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd425);SPI_WriteData(0x00a4);	
    SPI_WriteComm(0xd426);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd427);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd428);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd429);SPI_WriteData(0x00c7);	
    SPI_WriteComm(0xd42a);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd42b);SPI_WriteData(0x00c9);	
    SPI_WriteComm(0xd42c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd42d);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd42e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd42f);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd430);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd431);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd432);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd433);SPI_WriteData(0x00cc);	
                                            
                                            
    SPI_WriteComm(0xd500);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd501);SPI_WriteData(0x005d);	
    SPI_WriteComm(0xd502);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd503);SPI_WriteData(0x006b);	
    SPI_WriteComm(0xd504);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd505);SPI_WriteData(0x0084);	
    SPI_WriteComm(0xd506);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd507);SPI_WriteData(0x009c);	
    SPI_WriteComm(0xd508);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd509);SPI_WriteData(0x00b1);	
    SPI_WriteComm(0xd50a);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd50b);SPI_WriteData(0x00D9);	
    SPI_WriteComm(0xd50c);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd50d);SPI_WriteData(0x00fd);	
    SPI_WriteComm(0xd50e);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd50f);SPI_WriteData(0x0038);	
    SPI_WriteComm(0xd510);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd511);SPI_WriteData(0x0068);	
    SPI_WriteComm(0xd512);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd513);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd514);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd515);SPI_WriteData(0x00fb);	
    SPI_WriteComm(0xd516);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd517);SPI_WriteData(0x0063);	
    SPI_WriteComm(0xd518);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd519);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd51a);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd51b);SPI_WriteData(0x00bb);	
    SPI_WriteComm(0xd51c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd51d);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd51e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd51f);SPI_WriteData(0x0046);	
    SPI_WriteComm(0xd520);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd521);SPI_WriteData(0x0069);	
    SPI_WriteComm(0xd522);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd523);SPI_WriteData(0x008f);	
    SPI_WriteComm(0xd524);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd525);SPI_WriteData(0x00a4);	
    SPI_WriteComm(0xd526);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd527);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd528);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd529);SPI_WriteData(0x00c7);	
    SPI_WriteComm(0xd52a);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd52b);SPI_WriteData(0x00c9);	
    SPI_WriteComm(0xd52c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd52d);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd52e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd52f);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd530);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd531);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd532);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd533);SPI_WriteData(0x00cc);	
                                            
    SPI_WriteComm(0xd600);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd601);SPI_WriteData(0x005d);	
    SPI_WriteComm(0xd602);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd603);SPI_WriteData(0x006b);	
    SPI_WriteComm(0xd604);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd605);SPI_WriteData(0x0084);	
    SPI_WriteComm(0xd606);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd607);SPI_WriteData(0x009c);	
    SPI_WriteComm(0xd608);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd609);SPI_WriteData(0x00b1);	
    SPI_WriteComm(0xd60a);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd60b);SPI_WriteData(0x00d9);	
    SPI_WriteComm(0xd60c);SPI_WriteData(0x0000);	
    SPI_WriteComm(0xd60d);SPI_WriteData(0x00fd);	
    SPI_WriteComm(0xd60e);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd60f);SPI_WriteData(0x0038);	
    SPI_WriteComm(0xd610);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd611);SPI_WriteData(0x0068);	
    SPI_WriteComm(0xd612);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd613);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd614);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xd615);SPI_WriteData(0x00fb);	
    SPI_WriteComm(0xd616);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd617);SPI_WriteData(0x0063);	
    SPI_WriteComm(0xd618);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd619);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd61a);SPI_WriteData(0x0002);	
    SPI_WriteComm(0xd61b);SPI_WriteData(0x00bb);	
    SPI_WriteComm(0xd61c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd61d);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd61e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd61f);SPI_WriteData(0x0046);	
    SPI_WriteComm(0xd620);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd621);SPI_WriteData(0x0069);	
    SPI_WriteComm(0xd622);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd623);SPI_WriteData(0x008f);	
    SPI_WriteComm(0xd624);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd625);SPI_WriteData(0x00a4);	
    SPI_WriteComm(0xd626);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd627);SPI_WriteData(0x00b9);	
    SPI_WriteComm(0xd628);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd629);SPI_WriteData(0x00c7);	
    SPI_WriteComm(0xd62a);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd62b);SPI_WriteData(0x00c9);	
    SPI_WriteComm(0xd62c);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd62d);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd62e);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd62f);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd630);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd631);SPI_WriteData(0x00cb);	
    SPI_WriteComm(0xd632);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xd633);SPI_WriteData(0x00cc);	
                                            
    SPI_WriteComm(0xba00);SPI_WriteData(0x0024);	
    SPI_WriteComm(0xba01);SPI_WriteData(0x0024);	
    SPI_WriteComm(0xba02);SPI_WriteData(0x0024);	
                                            
    SPI_WriteComm(0xb900);SPI_WriteData(0x0024);	
    SPI_WriteComm(0xb901);SPI_WriteData(0x0024);	
    SPI_WriteComm(0xb902);SPI_WriteData(0x0024);	

    SPI_WriteComm(0xf000);SPI_WriteData(0x0055);          
    SPI_WriteComm(0xf001);SPI_WriteData(0x00aa);	
    SPI_WriteComm(0xf002);SPI_WriteData(0x0052);	
    SPI_WriteComm(0xf003);SPI_WriteData(0x0008);	
    SPI_WriteComm(0xf004);SPI_WriteData(0x0000);	
                                            
                                            
    SPI_WriteComm(0xb100);SPI_WriteData(0x00cc);	
                                            
                                            
    SPI_WriteComm(0xbc00);SPI_WriteData(0x0005);	
    SPI_WriteComm(0xbc01);SPI_WriteData(0x0005);	
    SPI_WriteComm(0xbc02);SPI_WriteData(0x0005);	
                                            
    SPI_WriteComm(0xb800);SPI_WriteData(0x0001);	
    SPI_WriteComm(0xb801);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xb802);SPI_WriteData(0x0003);	
    SPI_WriteComm(0xb803);SPI_WriteData(0x0003);	
                                            
                                            
    SPI_WriteComm(0xbd02);SPI_WriteData(0x0007);	
    SPI_WriteComm(0xbd03);SPI_WriteData(0x0031);	
    SPI_WriteComm(0xbe02);SPI_WriteData(0x0007);	
    SPI_WriteComm(0xbe03);SPI_WriteData(0x0031);	
    SPI_WriteComm(0xbf02);SPI_WriteData(0x0007);	
    SPI_WriteComm(0xbf03);SPI_WriteData(0x0031);	
                                            
                                            
    SPI_WriteComm(0xff00);SPI_WriteData(0x00aa);	
    SPI_WriteComm(0xff01);SPI_WriteData(0x0055);	
    SPI_WriteComm(0xff02);SPI_WriteData(0x0025);	
    SPI_WriteComm(0xff03);SPI_WriteData(0x0001);	


    SPI_WriteComm(0xf304);SPI_WriteData(0x0011);	
    SPI_WriteComm(0xf306);SPI_WriteData(0x0010);	
    SPI_WriteComm(0xf308);SPI_WriteData(0x0000);	
                                            
    SPI_WriteComm(0x3500);SPI_WriteData(0x0000);	

    SPI_WriteComm(0x3A00);SPI_WriteData(0x0077);
    #if CONFIG_FB_HW_ROTATE_90
    SPI_WriteComm(0x3600);SPI_WriteData(0x00A0);
    #else
    SPI_WriteComm(0x3600);SPI_WriteData(0x0000);
    #endif

    SPI_WriteComm(0x2a00);SPI_WriteData(0x0000);
    SPI_WriteComm(0x2a01);SPI_WriteData(0x0000);
    #if CONFIG_FB_HW_ROTATE_90
    SPI_WriteComm(0x2a02);SPI_WriteData(0x0003);	
    SPI_WriteComm(0x2a03);SPI_WriteData(0x001f);	    
    #else
    SPI_WriteComm(0x2a02);SPI_WriteData(0x0001);	
    SPI_WriteComm(0x2a03);SPI_WriteData(0x00df);	
    #endif	

                                            
    SPI_WriteComm(0x2b00);SPI_WriteData(0x0000);   
    SPI_WriteComm(0x2b01);SPI_WriteData(0x0000);	
    #if CONFIG_FB_HW_ROTATE_90
    SPI_WriteComm(0x2b02);SPI_WriteData(0x0001);	
    SPI_WriteComm(0x2b03);SPI_WriteData(0x00df);	    
    #else
    SPI_WriteComm(0x2b02);SPI_WriteData(0x0003);	
    SPI_WriteComm(0x2b03);SPI_WriteData(0x001f);   
    #endif
	



    SPI_WriteComm(0x1100);
    LCD_delay(120);

    SPI_WriteComm(0x2900);

    SPI_WriteComm(0x2c00);
    SPI_WriteComm(0x3c00);

	return 0;
}

DECLARE_GLOBAL_DATA_PTR;

void i2c_init_board(void)
{
#ifdef CONFIG_I2C0_ENABLE
#if defined(CONFIG_MACH_SUN4I) || \
    defined(CONFIG_MACH_SUN5I) || \
    defined(CONFIG_MACH_SUN7I) || \
    defined(CONFIG_MACH_SUN8I_R40)
	sunxi_gpio_set_cfgpin(SUNXI_GPB(0), SUN4I_GPB_TWI0);
	sunxi_gpio_set_cfgpin(SUNXI_GPB(1), SUN4I_GPB_TWI0);
	clock_twi_onoff(0, 1);
#elif defined(CONFIG_MACH_SUN6I)
	sunxi_gpio_set_cfgpin(SUNXI_GPH(14), SUN6I_GPH_TWI0);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(15), SUN6I_GPH_TWI0);
	clock_twi_onoff(0, 1);
#elif defined(CONFIG_MACH_SUN8I)
	sunxi_gpio_set_cfgpin(SUNXI_GPH(2), SUN8I_GPH_TWI0);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(3), SUN8I_GPH_TWI0);
	clock_twi_onoff(0, 1);
#endif
#endif

#ifdef CONFIG_I2C1_ENABLE
#if defined(CONFIG_MACH_SUN4I) || \
    defined(CONFIG_MACH_SUN7I) || \
    defined(CONFIG_MACH_SUN8I_R40)
	sunxi_gpio_set_cfgpin(SUNXI_GPB(18), SUN4I_GPB_TWI1);
	sunxi_gpio_set_cfgpin(SUNXI_GPB(19), SUN4I_GPB_TWI1);
	clock_twi_onoff(1, 1);
#elif defined(CONFIG_MACH_SUN5I)
	sunxi_gpio_set_cfgpin(SUNXI_GPB(15), SUN5I_GPB_TWI1);
	sunxi_gpio_set_cfgpin(SUNXI_GPB(16), SUN5I_GPB_TWI1);
	clock_twi_onoff(1, 1);
#elif defined(CONFIG_MACH_SUN6I)
	sunxi_gpio_set_cfgpin(SUNXI_GPH(16), SUN6I_GPH_TWI1);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(17), SUN6I_GPH_TWI1);
	clock_twi_onoff(1, 1);
#elif defined(CONFIG_MACH_SUN8I)
	sunxi_gpio_set_cfgpin(SUNXI_GPH(4), SUN8I_GPH_TWI1);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(5), SUN8I_GPH_TWI1);
	clock_twi_onoff(1, 1);
#endif
#endif

#ifdef CONFIG_I2C2_ENABLE
#if defined(CONFIG_MACH_SUN4I) || \
    defined(CONFIG_MACH_SUN7I) || \
    defined(CONFIG_MACH_SUN8I_R40)
	sunxi_gpio_set_cfgpin(SUNXI_GPB(20), SUN4I_GPB_TWI2);
	sunxi_gpio_set_cfgpin(SUNXI_GPB(21), SUN4I_GPB_TWI2);
	clock_twi_onoff(2, 1);
#elif defined(CONFIG_MACH_SUN5I)
	sunxi_gpio_set_cfgpin(SUNXI_GPB(17), SUN5I_GPB_TWI2);
	sunxi_gpio_set_cfgpin(SUNXI_GPB(18), SUN5I_GPB_TWI2);
	clock_twi_onoff(2, 1);
#elif defined(CONFIG_MACH_SUN6I)
	sunxi_gpio_set_cfgpin(SUNXI_GPH(18), SUN6I_GPH_TWI2);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(19), SUN6I_GPH_TWI2);
	clock_twi_onoff(2, 1);
#elif defined(CONFIG_MACH_SUN8I)
	sunxi_gpio_set_cfgpin(SUNXI_GPE(12), SUN8I_GPE_TWI2);
	sunxi_gpio_set_cfgpin(SUNXI_GPE(13), SUN8I_GPE_TWI2);
	clock_twi_onoff(2, 1);
#endif
#endif

#ifdef CONFIG_I2C3_ENABLE
#if defined(CONFIG_MACH_SUN6I)
	sunxi_gpio_set_cfgpin(SUNXI_GPG(10), SUN6I_GPG_TWI3);
	sunxi_gpio_set_cfgpin(SUNXI_GPG(11), SUN6I_GPG_TWI3);
	clock_twi_onoff(3, 1);
#elif defined(CONFIG_MACH_SUN7I) || \
      defined(CONFIG_MACH_SUN8I_R40)
	sunxi_gpio_set_cfgpin(SUNXI_GPI(0), SUN7I_GPI_TWI3);
	sunxi_gpio_set_cfgpin(SUNXI_GPI(1), SUN7I_GPI_TWI3);
	clock_twi_onoff(3, 1);
#endif
#endif

#ifdef CONFIG_I2C4_ENABLE
#if defined(CONFIG_MACH_SUN7I) || \
    defined(CONFIG_MACH_SUN8I_R40)
	sunxi_gpio_set_cfgpin(SUNXI_GPI(2), SUN7I_GPI_TWI4);
	sunxi_gpio_set_cfgpin(SUNXI_GPI(3), SUN7I_GPI_TWI4);
	clock_twi_onoff(4, 1);
#endif
#endif

#ifdef CONFIG_R_I2C_ENABLE
	clock_twi_onoff(5, 1);
	sunxi_gpio_set_cfgpin(SUNXI_GPL(0), SUN8I_H3_GPL_R_TWI);
	sunxi_gpio_set_cfgpin(SUNXI_GPL(1), SUN8I_H3_GPL_R_TWI);
#endif
}

/* add board specific code here */
int board_init(void)
{
	__maybe_unused int id_pfr1, ret, satapwr_pin, macpwr_pin;

	gd->bd->bi_boot_params = (PHYS_SDRAM_0 + 0x100);

#if !defined(CONFIG_ARM64) && !defined(CONFIG_MACH_SUNIV)
	asm volatile("mrc p15, 0, %0, c0, c1, 1" : "=r"(id_pfr1));
	debug("id_pfr1: 0x%08x\n", id_pfr1);
	/* Generic Timer Extension available? */
	if ((id_pfr1 >> CPUID_ARM_GENTIMER_SHIFT) & 0xf) {
		uint32_t freq;

		debug("Setting CNTFRQ\n");

		/*
		 * CNTFRQ is a secure register, so we will crash if we try to
		 * write this from the non-secure world (read is OK, though).
		 * In case some bootcode has already set the correct value,
		 * we avoid the risk of writing to it.
		 */
		asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(freq));
		if (freq != COUNTER_FREQUENCY) {
			debug("arch timer frequency is %d Hz, should be %d, fixing ...\n",
			      freq, COUNTER_FREQUENCY);
#ifdef CONFIG_NON_SECURE
			printf("arch timer frequency is wrong, but cannot adjust it\n");
#else
			asm volatile("mcr p15, 0, %0, c14, c0, 0"
				     : : "r"(COUNTER_FREQUENCY));
#endif
		}
	}
#endif /* !CONFIG_ARM64 && !CONFIG_MACH_SUNIV */

	ret = axp_gpio_init();
	if (ret)
		return ret;

#ifdef CONFIG_SATAPWR
	satapwr_pin = sunxi_name_to_gpio(CONFIG_SATAPWR);
	gpio_request(satapwr_pin, "satapwr");
	gpio_direction_output(satapwr_pin, 1);
	/* Give attached sata device time to power-up to avoid link timeouts */
	mdelay(500);
#endif
#ifdef CONFIG_MACPWR
	macpwr_pin = sunxi_name_to_gpio(CONFIG_MACPWR);
	gpio_request(macpwr_pin, "macpwr");
	gpio_direction_output(macpwr_pin, 1);
#endif

#ifdef CONFIG_DM_I2C
	/*
	 * Temporary workaround for enabling I2C clocks until proper sunxi DM
	 * clk, reset and pinctrl drivers land.
	 */
	i2c_init_board();
#endif

#ifdef CONFIG_VIDEO_LCD_PANEL_SPI
	soft_spi_board_init();
#endif
	/* Uses dm gpio code so do this here and not in i2c_init_board() */
	return soft_i2c_board_init();
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)PHYS_SDRAM_0, PHYS_SDRAM_0_SIZE);

	return 0;
}

#if defined(CONFIG_NAND_SUNXI)
static void nand_pinmux_setup(void)
{
	unsigned int pin;

	for (pin = SUNXI_GPC(0); pin <= SUNXI_GPC(19); pin++)
		sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_NAND);

#if defined CONFIG_MACH_SUN4I || defined CONFIG_MACH_SUN7I
	for (pin = SUNXI_GPC(20); pin <= SUNXI_GPC(22); pin++)
		sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_NAND);
#endif
	/* sun4i / sun7i do have a PC23, but it is not used for nand,
	 * only sun7i has a PC24 */
#ifdef CONFIG_MACH_SUN7I
	sunxi_gpio_set_cfgpin(SUNXI_GPC(24), SUNXI_GPC_NAND);
#endif
}

static void nand_clock_setup(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	setbits_le32(&ccm->ahb_gate0, (CLK_GATE_OPEN << AHB_GATE_OFFSET_NAND0));
#ifdef CONFIG_MACH_SUN9I
	setbits_le32(&ccm->ahb_gate1, (1 << AHB_GATE_OFFSET_DMA));
#else
	setbits_le32(&ccm->ahb_gate0, (1 << AHB_GATE_OFFSET_DMA));
#endif
	setbits_le32(&ccm->nand0_clk_cfg, CCM_NAND_CTRL_ENABLE | AHB_DIV_1);
}

void board_nand_init(void)
{
	nand_pinmux_setup();
	nand_clock_setup();
#ifndef CONFIG_SPL_BUILD
	sunxi_nand_init();
#endif
}
#endif

#ifdef CONFIG_MMC
static void mmc_pinmux_setup(int sdc)
{
	__maybe_unused unsigned int pin;
	__maybe_unused int pins;

	switch (sdc) {
	case 0:
		/* SDC0: PF0-PF5 */
#ifndef CONFIG_UART0_PORT_F
		for (pin = SUNXI_GPF(0); pin <= SUNXI_GPF(5); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPF_SDC0);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
#endif
		break;

	case 1:
		pins = sunxi_name_to_gpio_bank(CONFIG_MMC1_PINS);

#if defined(CONFIG_MACH_SUN4I) || defined(CONFIG_MACH_SUN7I) || \
    defined(CONFIG_MACH_SUN8I_R40)
		if (pins == SUNXI_GPIO_H) {
			/* SDC1: PH22-PH-27 */
			for (pin = SUNXI_GPH(22); pin <= SUNXI_GPH(27); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN4I_GPH_SDC1);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		} else {
			/* SDC1: PG0-PG5 */
			for (pin = SUNXI_GPG(0); pin <= SUNXI_GPG(5); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN4I_GPG_SDC1);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		}
#elif defined(CONFIG_MACH_SUN5I)
		/* SDC1: PG3-PG8 */
		for (pin = SUNXI_GPG(3); pin <= SUNXI_GPG(8); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUN5I_GPG_SDC1);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
#elif defined(CONFIG_MACH_SUN6I)
		/* SDC1: PG0-PG5 */
		for (pin = SUNXI_GPG(0); pin <= SUNXI_GPG(5); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUN6I_GPG_SDC1);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
#elif defined(CONFIG_MACH_SUN8I)
		if (pins == SUNXI_GPIO_D) {
			/* SDC1: PD2-PD7 */
			for (pin = SUNXI_GPD(2); pin <= SUNXI_GPD(7); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN8I_GPD_SDC1);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		} else {
			/* SDC1: PG0-PG5 */
			for (pin = SUNXI_GPG(0); pin <= SUNXI_GPG(5); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN8I_GPG_SDC1);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		}
#endif
		break;

	case 2:
		pins = sunxi_name_to_gpio_bank(CONFIG_MMC2_PINS);

#if defined(CONFIG_MACH_SUN4I) || defined(CONFIG_MACH_SUN7I)
		/* SDC2: PC6-PC11 */
		for (pin = SUNXI_GPC(6); pin <= SUNXI_GPC(11); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
#elif defined(CONFIG_MACH_SUN5I)
		if (pins == SUNXI_GPIO_E) {
			/* SDC2: PE4-PE9 */
			for (pin = SUNXI_GPE(4); pin <= SUNXI_GPD(9); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN5I_GPE_SDC2);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		} else {
			/* SDC2: PC6-PC15 */
			for (pin = SUNXI_GPC(6); pin <= SUNXI_GPC(15); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		}
#elif defined(CONFIG_MACH_SUN6I)
		if (pins == SUNXI_GPIO_A) {
			/* SDC2: PA9-PA14 */
			for (pin = SUNXI_GPA(9); pin <= SUNXI_GPA(14); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN6I_GPA_SDC2);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		} else {
			/* SDC2: PC6-PC15, PC24 */
			for (pin = SUNXI_GPC(6); pin <= SUNXI_GPC(15); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}

			sunxi_gpio_set_cfgpin(SUNXI_GPC(24), SUNXI_GPC_SDC2);
			sunxi_gpio_set_pull(SUNXI_GPC(24), SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(SUNXI_GPC(24), 2);
		}
#elif defined(CONFIG_MACH_SUN8I_R40)
		/* SDC2: PC6-PC15, PC24 */
		for (pin = SUNXI_GPC(6); pin <= SUNXI_GPC(15); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}

		sunxi_gpio_set_cfgpin(SUNXI_GPC(24), SUNXI_GPC_SDC2);
		sunxi_gpio_set_pull(SUNXI_GPC(24), SUNXI_GPIO_PULL_UP);
		sunxi_gpio_set_drv(SUNXI_GPC(24), 2);
#elif defined(CONFIG_MACH_SUN8I) || defined(CONFIG_MACH_SUN50I)
		/* SDC2: PC5-PC6, PC8-PC16 */
		for (pin = SUNXI_GPC(5); pin <= SUNXI_GPC(6); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}

		for (pin = SUNXI_GPC(8); pin <= SUNXI_GPC(16); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
#elif defined(CONFIG_MACH_SUN9I)
		/* SDC2: PC6-PC16 */
		for (pin = SUNXI_GPC(6); pin <= SUNXI_GPC(16); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
#endif
		break;

	case 3:
		pins = sunxi_name_to_gpio_bank(CONFIG_MMC3_PINS);

#if defined(CONFIG_MACH_SUN4I) || defined(CONFIG_MACH_SUN7I) || \
    defined(CONFIG_MACH_SUN8I_R40)
		/* SDC3: PI4-PI9 */
		for (pin = SUNXI_GPI(4); pin <= SUNXI_GPI(9); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPI_SDC3);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
#elif defined(CONFIG_MACH_SUN6I)
		if (pins == SUNXI_GPIO_A) {
			/* SDC3: PA9-PA14 */
			for (pin = SUNXI_GPA(9); pin <= SUNXI_GPA(14); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN6I_GPA_SDC3);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}
		} else {
			/* SDC3: PC6-PC15, PC24 */
			for (pin = SUNXI_GPC(6); pin <= SUNXI_GPC(15); pin++) {
				sunxi_gpio_set_cfgpin(pin, SUN6I_GPC_SDC3);
				sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
				sunxi_gpio_set_drv(pin, 2);
			}

			sunxi_gpio_set_cfgpin(SUNXI_GPC(24), SUN6I_GPC_SDC3);
			sunxi_gpio_set_pull(SUNXI_GPC(24), SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(SUNXI_GPC(24), 2);
		}
#endif
		break;

	default:
		printf("sunxi: invalid MMC slot %d for pinmux setup\n", sdc);
		break;
	}
}

int board_mmc_init(bd_t *bis)
{
	__maybe_unused struct mmc *mmc0, *mmc1;
	__maybe_unused char buf[512];

	mmc_pinmux_setup(CONFIG_MMC_SUNXI_SLOT);
	mmc0 = sunxi_mmc_init(CONFIG_MMC_SUNXI_SLOT);
	if (!mmc0)
		return -1;

#if CONFIG_MMC_SUNXI_SLOT_EXTRA != -1
	mmc_pinmux_setup(CONFIG_MMC_SUNXI_SLOT_EXTRA);
	mmc1 = sunxi_mmc_init(CONFIG_MMC_SUNXI_SLOT_EXTRA);
	if (!mmc1)
		return -1;
#endif

	return 0;
}
#endif

#ifdef CONFIG_SPL_BUILD
void sunxi_board_init(void)
{
	int power_failed = 0;

#ifdef CONFIG_SY8106A_POWER
	power_failed = sy8106a_set_vout1(CONFIG_SY8106A_VOUT1_VOLT);
#endif

#if defined CONFIG_AXP152_POWER || defined CONFIG_AXP209_POWER || \
	defined CONFIG_AXP221_POWER || defined CONFIG_AXP809_POWER || \
	defined CONFIG_AXP818_POWER
	power_failed = axp_init();

#if defined CONFIG_AXP221_POWER || defined CONFIG_AXP809_POWER || \
	defined CONFIG_AXP818_POWER
	power_failed |= axp_set_dcdc1(CONFIG_AXP_DCDC1_VOLT);
#endif
	power_failed |= axp_set_dcdc2(CONFIG_AXP_DCDC2_VOLT);
	power_failed |= axp_set_dcdc3(CONFIG_AXP_DCDC3_VOLT);
#if !defined(CONFIG_AXP209_POWER) && !defined(CONFIG_AXP818_POWER)
	power_failed |= axp_set_dcdc4(CONFIG_AXP_DCDC4_VOLT);
#endif
#if defined CONFIG_AXP221_POWER || defined CONFIG_AXP809_POWER || \
	defined CONFIG_AXP818_POWER
	power_failed |= axp_set_dcdc5(CONFIG_AXP_DCDC5_VOLT);
#endif

#if defined CONFIG_AXP221_POWER || defined CONFIG_AXP809_POWER || \
	defined CONFIG_AXP818_POWER
	power_failed |= axp_set_aldo1(CONFIG_AXP_ALDO1_VOLT);
#endif
	power_failed |= axp_set_aldo2(CONFIG_AXP_ALDO2_VOLT);
#if !defined(CONFIG_AXP152_POWER)
	power_failed |= axp_set_aldo3(CONFIG_AXP_ALDO3_VOLT);
#endif
#ifdef CONFIG_AXP209_POWER
	power_failed |= axp_set_aldo4(CONFIG_AXP_ALDO4_VOLT);
#endif

#if defined(CONFIG_AXP221_POWER) || defined(CONFIG_AXP809_POWER) || \
	defined(CONFIG_AXP818_POWER)
	power_failed |= axp_set_dldo(1, CONFIG_AXP_DLDO1_VOLT);
	power_failed |= axp_set_dldo(2, CONFIG_AXP_DLDO2_VOLT);
#if !defined CONFIG_AXP809_POWER
	power_failed |= axp_set_dldo(3, CONFIG_AXP_DLDO3_VOLT);
	power_failed |= axp_set_dldo(4, CONFIG_AXP_DLDO4_VOLT);
#endif
	power_failed |= axp_set_eldo(1, CONFIG_AXP_ELDO1_VOLT);
	power_failed |= axp_set_eldo(2, CONFIG_AXP_ELDO2_VOLT);
	power_failed |= axp_set_eldo(3, CONFIG_AXP_ELDO3_VOLT);
#endif

#ifdef CONFIG_AXP818_POWER
	power_failed |= axp_set_fldo(1, CONFIG_AXP_FLDO1_VOLT);
	power_failed |= axp_set_fldo(2, CONFIG_AXP_FLDO2_VOLT);
	power_failed |= axp_set_fldo(3, CONFIG_AXP_FLDO3_VOLT);
#endif

#if defined CONFIG_AXP809_POWER || defined CONFIG_AXP818_POWER
	power_failed |= axp_set_sw(IS_ENABLED(CONFIG_AXP_SW_ON));
#endif
#endif
	printf("DRAM:");
	gd->ram_size = sunxi_dram_init();
	printf(" %d MiB\n", (int)(gd->ram_size >> 20));
	if (!gd->ram_size)
		hang();

	/*
	 * Only clock up the CPU to full speed if we are reasonably
	 * assured it's being powered with suitable core voltage
	 */
	if (!power_failed) {
		printf("Set CPU frequency: %d\n", CONFIG_SYS_CLK_FREQ);
		clock_set_pll1(CONFIG_SYS_CLK_FREQ);
	}
	else
		printf("Failed to set core voltage! Can't set CPU frequency\n");
}
#endif

#ifdef CONFIG_USB_GADGET
int g_dnl_board_usb_cable_connected(void)
{
	return sunxi_usb_phy_vbus_detect(0);
}
#endif

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	char *serial_string;
	unsigned long long serial;

	serial_string = env_get("serial#");

	if (serial_string) {
		serial = simple_strtoull(serial_string, NULL, 16);

		serialnr->high = (unsigned int) (serial >> 32);
		serialnr->low = (unsigned int) (serial & 0xffffffff);
	} else {
		serialnr->high = 0;
		serialnr->low = 0;
	}
}
#endif

/*
 * Check the SPL header for the "sunxi" variant. If found: parse values
 * that might have been passed by the loader ("fel" utility), and update
 * the environment accordingly.
 */
static void parse_spl_header(const uint32_t spl_addr)
{
	struct boot_file_head *spl = (void *)(ulong)spl_addr;
	if (memcmp(spl->spl_signature, SPL_SIGNATURE, 3) != 0)
		return; /* signature mismatch, no usable header */

	uint8_t spl_header_version = spl->spl_signature[3];
	if (spl_header_version != SPL_HEADER_VERSION) {
		printf("sunxi SPL version mismatch: expected %u, got %u\n",
		       SPL_HEADER_VERSION, spl_header_version);
		return;
	}
	if (!spl->fel_script_address)
		return;

	if (spl->fel_uEnv_length != 0) {
		/*
		 * data is expected in uEnv.txt compatible format, so "env
		 * import -t" the string(s) at fel_script_address right away.
		 */
		himport_r(&env_htab, (char *)(uintptr_t)spl->fel_script_address,
			  spl->fel_uEnv_length, '\n', H_NOCLEAR, 0, 0, NULL);
		return;
	}
	/* otherwise assume .scr format (mkimage-type script) */
	env_set_hex("fel_scriptaddr", spl->fel_script_address);
}

/*
 * Note this function gets called multiple times.
 * It must not make any changes to env variables which already exist.
 */
static void setup_environment(const void *fdt)
{
	char serial_string[17] = { 0 };
	unsigned int sid[4];
	uint8_t mac_addr[6];
	char ethaddr[16];
	int i, ret;

	ret = sunxi_get_sid(sid);
	if (ret == 0 && sid[0] != 0) {
		/*
		 * The single words 1 - 3 of the SID have quite a few bits
		 * which are the same on many models, so we take a crc32
		 * of all 3 words, to get a more unique value.
		 *
		 * Note we only do this on newer SoCs as we cannot change
		 * the algorithm on older SoCs since those have been using
		 * fixed mac-addresses based on only using word 3 for a
		 * long time and changing a fixed mac-address with an
		 * u-boot update is not good.
		 */
#if !defined(CONFIG_MACH_SUN4I) && !defined(CONFIG_MACH_SUN5I) && \
    !defined(CONFIG_MACH_SUN6I) && !defined(CONFIG_MACH_SUN7I) && \
    !defined(CONFIG_MACH_SUN8I_A23) && !defined(CONFIG_MACH_SUN8I_A33)
		sid[3] = crc32(0, (unsigned char *)&sid[1], 12);
#endif

		/* Ensure the NIC specific bytes of the mac are not all 0 */
		if ((sid[3] & 0xffffff) == 0)
			sid[3] |= 0x800000;

		for (i = 0; i < 4; i++) {
			sprintf(ethaddr, "ethernet%d", i);
			if (!fdt_get_alias(fdt, ethaddr))
				continue;

			if (i == 0)
				strcpy(ethaddr, "ethaddr");
			else
				sprintf(ethaddr, "eth%daddr", i);

			if (env_get(ethaddr))
				continue;

			/* Non OUI / registered MAC address */
			mac_addr[0] = (i << 4) | 0x02;
			mac_addr[1] = (sid[0] >>  0) & 0xff;
			mac_addr[2] = (sid[3] >> 24) & 0xff;
			mac_addr[3] = (sid[3] >> 16) & 0xff;
			mac_addr[4] = (sid[3] >>  8) & 0xff;
			mac_addr[5] = (sid[3] >>  0) & 0xff;

			eth_env_set_enetaddr(ethaddr, mac_addr);
		}

		if (!env_get("serial#")) {
			snprintf(serial_string, sizeof(serial_string),
				"%08x%08x", sid[0], sid[3]);

			env_set("serial#", serial_string);
		}
	}
}

int misc_init_r(void)
{
	__maybe_unused int ret;
	uint boot;

	env_set("fel_booted", NULL);
	env_set("fel_scriptaddr", NULL);
	env_set("mmc_bootdev", NULL);

	boot = sunxi_get_boot_device();
	/* determine if we are running in FEL mode */
	if (boot == BOOT_DEVICE_BOARD) {
		env_set("fel_booted", "1");
		parse_spl_header(SPL_ADDR);
	/* or if we booted from MMC, and which one */
	} else if (boot == BOOT_DEVICE_MMC1) {
		env_set("mmc_bootdev", "0");
	} else if (boot == BOOT_DEVICE_MMC2) {
		env_set("mmc_bootdev", "1");
	}

	setup_environment(gd->fdt_blob);

#ifndef CONFIG_MACH_SUN9I
	ret = sunxi_usb_phy_probe();
	if (ret)
		return ret;
#endif

#ifdef CONFIG_USB_ETHER
	usb_ether_init();
#endif

	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	int __maybe_unused r;

	/*
	 * Call setup_environment again in case the boot fdt has
	 * ethernet aliases the u-boot copy does not have.
	 */
	setup_environment(blob);

#ifdef CONFIG_VIDEO_DT_SIMPLEFB
	r = sunxi_simplefb_setup(blob);
	if (r)
		return r;
#endif
	return 0;
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	struct boot_file_head *spl = (void *)(ulong)SPL_ADDR;
	const char *cmp_str = (void *)(ulong)SPL_ADDR;

	/* Check if there is a DT name stored in the SPL header and use that. */
	if (spl->dt_name_offset) {
		cmp_str += spl->dt_name_offset;
	} else {
#ifdef CONFIG_DEFAULT_DEVICE_TREE
		cmp_str = CONFIG_DEFAULT_DEVICE_TREE;
#else
		return 0;
#endif
	};

/* Differentiate the two Pine64 board DTs by their DRAM size. */
	if (strstr(name, "-pine64") && strstr(cmp_str, "-pine64")) {
		if ((gd->ram_size > 512 * 1024 * 1024))
			return !strstr(name, "plus");
		else
			return !!strstr(name, "plus");
	} else {
		return strcmp(name, cmp_str);
	}
}
#endif
