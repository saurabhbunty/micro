/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-08-23     Bernard      first implementation
 */

#include <rthw.h>
#include <rtthread.h>

#include "stm32f10x.h"
#include "board.h"

#ifdef RT_USING_SPI
#include "stm32f10x_spi.h"
#include "rt_stm32f10x_spi.h"
#endif

struct rt_semaphore spi1_lock;

/**
 * @addtogroup STM32
 */

/*@{*/

static void delay(void)
{
    volatile unsigned int dl;
    for(dl=0; dl<2500000; dl++);
}

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
    /*
     * set priority group:
     * 2 bits for pre-emption priority
     * 2 bits for subpriority
     */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

extern void rt_hw_interrupt_thread_switch(void);
/**
 * This is the timer interrupt service routine.
 *
 */
void rt_hw_timer_handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

/* NAND Flash */
#include "fsmc_nand.h"

static void all_device_reset(void)
{
    /* RESET */
    /* DM9000A          PE5  */
    /* LCD              PF10 */
    /* SPI-FLASH        PA3  */

    /*  CS */
    /* DM9000A FSMC_NE4 PG12 */
    /* LCD     FSMC_NE2 PG9  */
    /* SPI_FLASH        PA4  */
    /* CODEC            PC5  */
    /* TOUCH            PC4  */

    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOE
                           | RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG,ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    /* SDIO POWER */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOC,&GPIO_InitStructure);
    GPIO_SetBits(GPIOC,GPIO_Pin_6); /* SD card power down */

    /* SPI_FLASH CS */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOA,&GPIO_InitStructure);
    GPIO_SetBits(GPIOA,GPIO_Pin_4);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    /* CODEC && TOUCH CS */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_Init(GPIOC,&GPIO_InitStructure);
    GPIO_SetBits(GPIOC,GPIO_Pin_4 | GPIO_Pin_5);

    /*  DM9000A RESET */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOE,&GPIO_InitStructure);
    GPIO_ResetBits(GPIOE,GPIO_Pin_5);

    /* LCD RESET */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOF,&GPIO_InitStructure);
    GPIO_ResetBits(GPIOF,GPIO_Pin_10);

    /* LCD brightness */
#if ( LCD_USE_PWM == 0 )
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOF,&GPIO_InitStructure);
    GPIO_ResetBits(GPIOF,GPIO_Pin_9); /* LCD brightness power off */
#elif ( LCD_USE_PWM == 1 )
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
    GPIO_ResetBits(GPIOB,GPIO_Pin_9); /* LCD brightness power off */
#elif ( LCD_USE_PWM == 2 )
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
    GPIO_ResetBits(GPIOB,GPIO_Pin_9); /* LCD brightness power off */
#endif

    /* SPI_FLASH RESET */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA,&GPIO_InitStructure);
    GPIO_ResetBits(GPIOA,GPIO_Pin_3);

    /* FSMC GPIO configure */
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF
                               | RCC_APB2Periph_GPIOG, ENABLE);
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /*
        FSMC_D0 ~ FSMC_D3
        PD14 FSMC_D0   PD15 FSMC_D1   PD0  FSMC_D2   PD1  FSMC_D3
        */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_Init(GPIOD,&GPIO_InitStructure);

        /*
        FSMC_D4 ~ FSMC_D12
        PE7 ~ PE15  FSMC_D4 ~ FSMC_D12
        */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10
                                      | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_Init(GPIOE,&GPIO_InitStructure);

        /* FSMC_D13 ~ FSMC_D15   PD8 ~ PD10 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
        GPIO_Init(GPIOD,&GPIO_InitStructure);

        /*
        FSMC_A0 ~ FSMC_A5   FSMC_A6 ~ FSMC_A9
        PF0     ~ PF5       PF12    ~ PF15
        */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3
                                      | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_Init(GPIOF,&GPIO_InitStructure);

        /* FSMC_A10 ~ FSMC_A15  PG0 ~ PG5 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
        GPIO_Init(GPIOG,&GPIO_InitStructure);

        /* FSMC_A16 ~ FSMC_A18  PD11 ~ PD13 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
        GPIO_Init(GPIOD,&GPIO_InitStructure);

        /* RD-PD4 WR-PD5 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
        GPIO_Init(GPIOD,&GPIO_InitStructure);

        /* NBL0-PE0 NBL1-PE1 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
        GPIO_Init(GPIOE,&GPIO_InitStructure);

        /* NE1/NCE2 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
        GPIO_Init(GPIOD,&GPIO_InitStructure);
        /* NE2 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
        GPIO_Init(GPIOG,&GPIO_InitStructure);
        /* NE3 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_Init(GPIOG,&GPIO_InitStructure);
        /* NE4 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
        GPIO_Init(GPIOG,&GPIO_InitStructure);
    }
    /* FSMC GPIO configure */

    delay();
    GPIO_SetBits(GPIOE,GPIO_Pin_5);   /* DM9000A          */
    GPIO_SetBits(GPIOF,GPIO_Pin_10);  /* LCD              */
    GPIO_SetBits(GPIOA,GPIO_Pin_3);   /* SPI_FLASH        */
    GPIO_ResetBits(GPIOC,GPIO_Pin_6); /* SD card power up */
}

/**
 * This function will initial STM32 Radio board.
 */
extern void FSMC_SRAM_Init(void);
void rt_hw_board_init(void)
{
    //NAND_IDTypeDef NAND_ID;

    /* Configure the system clocks */
    SystemInit();

    all_device_reset();

    /* NVIC Configuration */
    NVIC_Configuration();

    /* Configure the SysTick */
    SysTick_Config( SystemCoreClock / RT_TICK_PER_SECOND );

    /* Console Initialization*/
    rt_hw_usart_init();
#if STM32_CONSOLE_USART == 1
    rt_console_set_device("uart1");
#elif STM32_CONSOLE_USART == 2
    rt_console_set_device("uart2");
#elif STM32_CONSOLE_USART == 3
    rt_console_set_device("uart3");
#endif

    rt_kprintf("\r\n\r\nSystemInit......\r\n");

    // show SN
    {
        uint8_t * sn = (uint8_t *)0x1FFFF7E8;
        uint32_t i;

        rt_kprintf("CPU SN: ");
        for(i=0;i<12;i++)
        {
            rt_kprintf("%02X",*sn++);
        }
        rt_kprintf("\r\n");
    }

    /* SRAM init */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
    FSMC_SRAM_Init();

    /* memtest */
    {
        unsigned char * p_extram = (unsigned char *)STM32_EXT_SRAM_BEGIN;
        unsigned int temp;

        rt_kprintf("\r\nmem testing....");
        for(temp=0; temp<(STM32_EXT_SRAM_END-STM32_EXT_SRAM_BEGIN); temp++)
        {
            *p_extram++ = (unsigned char)temp;
        }

        p_extram = (unsigned char *)STM32_EXT_SRAM_BEGIN;
        for(temp=0; temp<(STM32_EXT_SRAM_END-STM32_EXT_SRAM_BEGIN); temp++)
        {
            if( *p_extram++ != (unsigned char)temp )
            {
                rt_kprintf("\rmemtest fail @ %08X\r\nsystem halt!!!!!",(unsigned int)p_extram);
                while(1);
            }
        }
        rt_kprintf("\rmem test pass!!\r\n");
    }/* memtest */

    /* SPI1 config */
    {
        if (rt_sem_init(&spi1_lock, "spi1lock", 1, RT_IPC_FLAG_FIFO) != RT_EOK)
        {
            rt_kprintf("init spi1 lock semaphore failed\n");
        }
    } /* SPI1 config */

#ifdef RT_USING_SPI
    rt_stm32f10x_spi_init();

#   ifdef USING_SPI1
    /* attach spi10 : CS PA4 */
    {
        static struct rt_spi_device rt_spi_device;
        static struct stm32_spi_cs  stm32_spi_cs;
        GPIO_InitTypeDef GPIO_InitStructure;

        stm32_spi_cs.GPIOx = GPIOA;
        stm32_spi_cs.GPIO_Pin = GPIO_Pin_4;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(stm32_spi_cs.GPIOx, &GPIO_InitStructure);

        GPIO_SetBits(stm32_spi_cs.GPIOx, stm32_spi_cs.GPIO_Pin);

        rt_spi_bus_attach_device(&rt_spi_device, "spi10", "spi1", (void*)&stm32_spi_cs);
    }

    /* attach spi11 : CS PC4 */
    {
        static struct rt_spi_device rt_spi_device;
        static struct stm32_spi_cs  stm32_spi_cs;
        GPIO_InitTypeDef GPIO_InitStructure;

        stm32_spi_cs.GPIOx = GPIOC;
        stm32_spi_cs.GPIO_Pin = GPIO_Pin_4;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(stm32_spi_cs.GPIOx, &GPIO_InitStructure);

        GPIO_SetBits(stm32_spi_cs.GPIOx, stm32_spi_cs.GPIO_Pin);

        rt_spi_bus_attach_device(&rt_spi_device, "spi11", "spi1", (void*)&stm32_spi_cs);
    }

    /* attach spi12 : CS PC5 */
    {
        static struct rt_spi_device rt_spi_device;
        static struct stm32_spi_cs  stm32_spi_cs;
        GPIO_InitTypeDef GPIO_InitStructure;

        stm32_spi_cs.GPIOx = GPIOC;
        stm32_spi_cs.GPIO_Pin = GPIO_Pin_5;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(stm32_spi_cs.GPIOx, &GPIO_InitStructure);

        GPIO_SetBits(stm32_spi_cs.GPIOx, stm32_spi_cs.GPIO_Pin);

        rt_spi_bus_attach_device(&rt_spi_device, "spi12", "spi1", (void*)&stm32_spi_cs);
    }
#   endif
#endif /* #ifdef RT_USING_SPI */
}/* rt_hw_board_init */

void rt_hw_spi1_baud_rate(uint16_t SPI_BaudRatePrescaler)
{
    SPI1->CR1 &= ~SPI_BaudRatePrescaler_256;
    SPI1->CR1 |= SPI_BaudRatePrescaler;
}

/*@}*/