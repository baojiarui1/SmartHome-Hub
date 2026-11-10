/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-05-24                  the first version
 */

#include <rthw.h>
#include <rtthread.h>
#include "stm32f10x.h"  // 加上标准库头文件

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
/*
 * Please modify RT_HEAP_SIZE if you enable RT_USING_HEAP
 * the RT_HEAP_SIZE max value = (sram size - ZI size), 1024 means 1024 bytes
 */
#define RT_HEAP_SIZE (10*1024) // 改成 10KB，给 F103 留足动态内存
static rt_uint8_t rt_heap[RT_HEAP_SIZE];

RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

void rt_os_tick_callback(void)
{
    rt_interrupt_enter();
    
    rt_tick_increase();

    rt_interrupt_leave();
}

/**
 * This function will initial your board.
 */
void rt_hw_board_init(void)
{
    // TODO 1: 配置 OS Tick (利用 SysTick 定时器产生 1ms 中断)
    // F103 默认 72MHz。SysTick_Config 里的参数是 1ms 的计数值。
    // SystemCoreClock 在 system_stm32f10x.c 里应该被配置为了 72000000
    if (SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND) != 0) {
        while (1);
    }
    
    // 设置 SysTick 中断优先级为最低 (15)
    NVIC_SetPriority(SysTick_IRQn, 15);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif



}

// 告诉 RT-Thread，把 SysTick 中断交给它自己处理
void SysTick_Handler(void)
{
    rt_os_tick_callback();
}