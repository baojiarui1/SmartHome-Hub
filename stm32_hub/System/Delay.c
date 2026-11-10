#include "stm32f10x.h"
#include <rtthread.h>

/**
  * @brief  微秒级延时 (兼容 RT-Thread 调度版)
  */
void Delay_us(uint32_t xus)
{
    uint32_t reload = SysTick->LOAD;      
    uint32_t ticks = xus * (reload / 1000); 
    uint32_t told, tnow, tcnt = 0;
    
    told = SysTick->VAL;                  
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += reload - tnow + told;
            told = tnow;
            
            if (tcnt >= ticks) break;     
        }
    }
}

/**
  * @brief  毫秒级延时
  */
void Delay_ms(uint32_t xms)
{
    // 直接使用 RT-Thread 的延时，释放 CPU
    rt_thread_mdelay(xms); 
}

void Delay_s(uint32_t xs)
{
    while(xs--)
    {
        Delay_ms(1000);
    }
}
