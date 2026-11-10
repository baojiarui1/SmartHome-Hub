#include "stm32f10x.h"
#include "FreeRTOS.h"  
#include "task.h"

/**
  * @brief  微秒级延时 (兼容 FreeRTOS 版)
  * @param  xus 延时时长
  */
void Delay_us(uint32_t xus)
{
    uint32_t reload = SysTick->LOAD;      // 获取当前重装值
    uint32_t ticks = xus * (reload / 1000); // 换算成节拍数
    uint32_t told, tnow, tcnt = 0;
    
    told = SysTick->VAL;                  // 获取当前计数值
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
            
            if (tcnt >= ticks) break;     // 延时够了就退出
        }
    }
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长
  */
void Delay_ms(uint32_t xms)
{
    // 直接使用 FreeRTOS 的毫秒延时，释放 CPU 给其他任务
    vTaskDelay(pdMS_TO_TICKS(xms)); 
}

void Delay_s(uint32_t xs)
{
    while(xs--)
    {
        Delay_ms(1000);
    }
}
