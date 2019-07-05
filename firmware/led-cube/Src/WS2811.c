#include "stm32f0xx_hal.h"
//#include "WS2812B.h"
//#include "sys.h"

//#define USE_DMA
#define nWs 64

#ifdef  USE_DMA
uint16_t PixelBuffer[2048] = {0};
uint16_t PixelPointer = 0;
#endif

void LED_SPI_LowLevel_Init(void)
{
    //GPIO_InitTypeDef  GPIO_InitStructure;
    //SPI_InitTypeDef   SPI_InitStructure;
#ifdef  USE_DMA
		int i = 0;
    DMA_InitTypeDef   DMA_InitStructure;
#endif

#ifdef  USE_DMA
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel3);
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (SPI1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)PixelBuffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_Init(DMA1_Channel3, &DMA_InitStructure); /* DMA1 CH3 = MEM -> DR */

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

    for (i = 0; i < 328; i++)
    {
        PixelBuffer[i] = 0xAAAA;
    }

    PixelPointer = 0;
#endif	
		
   //SPI_Cmd(SPI1, ENABLE);

}

void LED_SPI_WriteByte(uint16_t Data)
{
extern SPI_HandleTypeDef hspi1;
#ifdef  USE_DMA
    PixelBuffer[PixelPointer] = Data;
    PixelPointer++;
#else
		HAL_SPI_Transmit(&hspi1, (uint8_t*)&Data, 2, 1000);
#endif	
}

void LED_SPI_SendBits(uint8_t bits)
{
    int zero = 0x7000;  //111000000000000
    int one = 0x7F00;  //111111100000000
    int i = 0x00;

    for (i = 0x80; i >= 0x01; i >>= 1)
    {
        LED_SPI_WriteByte((bits & i) ? one : zero);
    }
}

void LED_SPI_SendPixel(uint16_t color)
{
    /*
     r7,r6,r5,r4,r3,r2,r1,r0,g7,g6,g5,g4,g3,g2,g1,g0,b7,b6,b5,b4,b3,b2,b1,b0
     \_____________________________________________________________________/
                               |      _________________...
                               |     /   __________________...
                               |    /   /   ___________________...
                               |   /   /   /
                              RGB,RGB,RGB,RGB,...,STOP
    */

    /*
    	BUG Fix : Actual is GRB,datasheet is something wrong.
    */
	  uint8_t r, g, b;  // 三原色
		// 绿 红 蓝 三原色分解
	  r   = (color>>8)&0xF8;
	  g = (color>>3)&0xFC;
	  b  = (color<<3);
    LED_SPI_SendBits(g);
    LED_SPI_SendBits(r);
    LED_SPI_SendBits(b);
}

void LED_SPI_Update(uint16_t buffer[], uint32_t length)
{
    uint8_t i = 0;

#ifdef USE_DMA
    if(DMA_GetCurrDataCounter(DMA1_Channel3) == 0)
    {
#endif
        for (i = 0; i < length; i++)
        {
            LED_SPI_SendPixel(buffer[i]);
        }

        if(length < nWs)
        {
            for(i = nWs - length; i < length; i++)
            {
							LED_SPI_SendPixel(0);
            }
        }

        //for (i = 0; i < 20; i++)   
				while(1)
        {
            LED_SPI_WriteByte(0xff);
        }
#ifdef USE_DMA
        PixelPointer = 0;

        DMA_Cmd(DMA1_Channel3, DISABLE);
        DMA_ClearFlag(DMA1_FLAG_TC3);
        DMA_SetCurrDataCounter(DMA1_Channel3, 24*length+20);
        DMA_Cmd(DMA1_Channel3, ENABLE);
    }
#endif
}



