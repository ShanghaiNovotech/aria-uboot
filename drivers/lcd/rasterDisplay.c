/**
 * \file  rasterDisplay.c
 *
 * \brief Sample application for raster
 *
*/

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/ 
*/
/* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "soc_AM335x.h"
#include "interrupt.h"
#include "evmAM335x.h"
#include "raster.h"
#include "image.h"
#include "ecap.h"
//#include "hsi2c.h"

#define FOUR_INCH_LCD 1
#define FIVE_INCH_LCD 0

/******************************************************************************
**                      INTERNAL FUNCTION PROTOTYPES
*******************************************************************************/
static void LCDIsr(void);
static void SetUpLCD(void);
static void LCDAINTCConfigure(void);
/******************************************************************************
**              FUNCTION DEFINITIONS
******************************************************************************/
	
int Lcd_Init(void)
{
    //IntMasterIRQEnable();

    //IntAINTCInit();

    LCDAINTCConfigure();
    printf("Configured Interrupt\n");

    

    LCDBackLightEnable();
    printf("Cofigured Backlight\n");

    SetUpLCD();
    printf("Configured LCD\n");
  
    // Configuring the base ceiling 
    RasterDMAFBConfig(SOC_LCDC_0_REGS, 
                      (unsigned int)image1,
                      (unsigned int)image1 + sizeof(image1) - 2,
                      0);

    RasterDMAFBConfig(SOC_LCDC_0_REGS, 
                      (unsigned int)image1,
                      (unsigned int)image1 + sizeof(image1) - 2,
                      1);

    // Enable End of frame0/frame1 interrupt 
    RasterIntEnable(SOC_LCDC_0_REGS, RASTER_END_OF_FRAME0_INT |
                                     RASTER_END_OF_FRAME1_INT);

    // Enable raster 
    RasterEnable(SOC_LCDC_0_REGS);
}

/*
** Configures raster to display image 
*/
static void SetUpLCD(void)
{
    /* Enable clock for LCD Module */ 
    LCDModuleClkConfig();

    LCDPinMuxSetup();

    /* 
    **Clock for DMA,LIDD and for Core(which encompasses
    ** Raster Active Matrix and Passive Matrix logic) 
    ** enabled.
    */
    RasterClocksEnable(SOC_LCDC_0_REGS);

    /* Disable raster */
    RasterDisable(SOC_LCDC_0_REGS);

    /* Configure the pclk */
    //RasterClkConfig(SOC_LCDC_0_REGS, 9000000, 92000000);
    RasterClkConfig(SOC_LCDC_0_REGS, 7833600, 150000000);

    /* Configuring DMA of LCD controller */ 
    RasterDMAConfig(SOC_LCDC_0_REGS, RASTER_DOUBLE_FRAME_BUFFER,
                    RASTER_BURST_SIZE_16, RASTER_FIFO_THRESHOLD_8,
                    RASTER_BIG_ENDIAN_DISABLE);

    /* Configuring modes(ex:tft or stn,color or monochrome etc) for raster controller */
    RasterModeConfig(SOC_LCDC_0_REGS, RASTER_DISPLAY_MODE_TFT_UNPACKED,
                     RASTER_PALETTE_DATA, RASTER_COLOR, RASTER_RIGHT_ALIGNED);


     /* Configuring the polarity of timing parameters of raster controller */
    RasterTiming2Configure(SOC_LCDC_0_REGS, RASTER_FRAME_CLOCK_LOW |
                                            RASTER_LINE_CLOCK_LOW  |
                                            RASTER_PIXEL_CLOCK_HIGH|
                                            RASTER_SYNC_EDGE_RISING|
                                            RASTER_SYNC_CTRL_ACTIVE|
                                            RASTER_AC_BIAS_HIGH     , 0, 255);

#if FIVE_INCH_LCD
    /* Configuring horizontal timing parameter */
    RasterHparamConfig(SOC_LCDC_0_REGS, 800, 48, 40, 40);

    /* Configuring vertical timing parameters */
    RasterVparamConfig(SOC_LCDC_0_REGS, 480, 3, 13, 29);
#endif
#if FOUR_INCH_LCD //NHD-4.3-ATXI#-T-1
    /* Configuring horizontal timing parameter */
    RasterHparamConfig(SOC_LCDC_0_REGS, 480, 4, 8, 43);

    /* Configuring vertical timing parameters */
    RasterVparamConfig(SOC_LCDC_0_REGS, 272, 10, 4, 12);
#endif

    RasterFIFODMADelayConfig(SOC_LCDC_0_REGS, 128);

}


/*
** configures arm interrupt controller to generate raster interrupt 
*/
static void LCDAINTCConfigure(void)
{
    /* Register the ISR in the Interrupt Vector Table.*/
    IntRegister(SYS_INT_LCDCINT, LCDIsr);

    IntPrioritySet(SYS_INT_LCDCINT, 0, AINTC_HOSTINT_ROUTE_IRQ );

    /* Enable the System Interrupts for AINTC.*/
    IntSystemEnable(SYS_INT_LCDCINT);
}

/*
** For each end of frame interrupt base and ceiling is reconfigured 
*/
static void LCDIsr(void)
{
    unsigned int  status;

    status = RasterIntStatus(SOC_LCDC_0_REGS,RASTER_END_OF_FRAME0_INT_STAT |
                                             RASTER_END_OF_FRAME1_INT_STAT );

    status = RasterClearGetIntStatus(SOC_LCDC_0_REGS, status);   

    if (status & RASTER_END_OF_FRAME0_INT_STAT)
    {
        RasterDMAFBConfig(SOC_LCDC_0_REGS, 
                          (unsigned int)image1,
                          (unsigned int)image1 + sizeof(image1) - 2,
                          0);
    }

    if(status & RASTER_END_OF_FRAME1_INT_STAT)
    {
        RasterDMAFBConfig(SOC_LCDC_0_REGS, 
                          (unsigned int)image1,
                          (unsigned int)image1 + sizeof(image1) - 2,
                          1);
    }
}

/***************************** End Of File ************************************/
