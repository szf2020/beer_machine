/*****************************************************************************
*  nxp serial uart hal driver                                                          
*  Copyright (C) 2019 wkxboot 1131204425@qq.com.                             
*                                                                            
*                                                                            
*  This program is free software; you can redistribute it and/or modify      
*  it under the terms of the GNU General Public License version 3 as         
*  published by the Free Software Foundation.                                
*                                                                            
*  @file     nxp_serial_uart_hal_driver.c                                                   
*  @brief    nxp serial uart hal driver                                                                                                                                                                                             
*  @author   wkxboot                                                      
*  @email    1131204425@qq.com                                              
*  @version  v1.0.0                                                  
*  @date     2019/1/10                                            
*  @license  GNU General Public License (GPL)                                
*                                                                            
*                                                                            
*****************************************************************************/


#include "fsl_usart.h"
#include "fsl_clock.h"
#include "pin_mux.h"
#include "nxp_serial_uart_hal_driver.h"


/* *****************************************************************************
*
*        lpc824 serial uart hal driver在IAR freertos下的移植
*
********************************************************************************/
/*nxp serial uart驱动结构体*/
serial_hal_driver_t nxp_serial_uart_hal_driver = {
.init = nxp_serial_uart_hal_init,
.deinit = nxp_serial_uart_hal_deinit,
.enable_txe_it = nxp_serial_uart_hal_enable_txe_it,
.disable_txe_it = nxp_serial_uart_hal_disable_txe_it,
.enable_rxne_it = nxp_serial_uart_hal_enable_rxne_it,
.disable_rxne_it = nxp_serial_uart_hal_disable_rxne_it
};

/*
* @brief 根据uart端口查找uart句柄
* @param port uart端口号
* @return uart句柄
* @note
*/
static USART_Type *nxp_serial_uart_search_handle_by_port(uint8_t port)
{
    USART_Type *nxp_uart_handle;

    if(port == 0){
    nxp_uart_handle = USART0;
    }else if(port == 1){
    nxp_uart_handle = USART1;
    serial_irq_num=USART1_IRQn;
    }else if(port == 2){
    nxp_uart_handle = USART2;
    }else{
    nxp_uart_handle = USART0;
    }

    return nxp_uart_handle;
}

/*
* @brief 根据uart端口查找uart中断号
* @param port uart端口号
* @return uart中断号
* @note
*/
static IRQn_Type nxp_serial_uart_search_irq_num_by_port(uint8_t port)
{
    IRQn_Type  nxp_uart_irq_num;

    if(port == 0){
    nxp_uart_irq_num = USART0_IRQn;
    }else if(port == 1){
    nxp_uart_irq_num=USART1_IRQn;
    }else if(port == 2){
    nxp_uart_irq_num = USART2_IRQn;
    }else{
    nxp_uart_irq_num = USART0_IRQn;
    }

    return nxp_uart_irq_num;
}


/*
* @brief 串口初始化驱动
* @param port uart端口号
* @param bauds 波特率
* @param data_bits 数据宽度
* @param stop_bit 停止位
* @return 
* @note
*/
int nxp_serial_uart_hal_init(uint8_t port,uint32_t baud_rates,uint8_t data_bits,uint8_t stop_bits)
{
    status_t status;
    usart_config_t config;
    IRQn_Type  nxp_uart_irq_num;
    USART_Type *nxp_uart_handle; 

    nxp_uart_irq_num = nxp_serial_uart_search_irq_num_by_port(port);
    nxp_uart_handle = nxp_serial_uart_search_handle_by_port(port);

    config.baudRate_Bps = baud_rates;
    if(data_bits == 8){
    config.bitCountPerChar = kUSART_8BitsPerChar;
    }else{
    config.bitCountPerChar = kUSART_7BitsPerChar;
    }
    if(stop_bits == 1){
    config.stopBitCount = kUSART_OneStopBit;
    }else{
    config.stopBitCount = kUSART_TwoStopBit;
    }
    
    config.parityMode = kUSART_ParityDisabled;
    config.syncMode = kUSART_SyncModeDisabled;
    
    config.loopback = false;
    config.enableRx = true;
    config.enableTx = true;
    /* Initialize the USART with configuration. */
    status=USART_Init(nxp_uart_handle, &config, CLOCK_GetFreq(kCLOCK_MainClk));
 
    if (status != kStatus_Success){
        return -1;
    } 
    EnableIRQ(nxp_uart_irq_num);

    return 0;
}


/*
* @brief 串口去初始化驱动
* @param port uart端口号
* @return = 0 成功
* @return < 0 失败
* @note
*/
int nxp_serial_uart_hal_deinit(uint8_t port)
{
    return 0; 
}


/*
* @brief 串口发送为空中断使能驱动
* @param port uart端口号
* @return 无
* @note
*/ 
void nxp_serial_uart_hal_enable_txe_it(uint8_t port)
{
    USART_Type *nxp_uart_handle; 

    nxp_uart_handle = nxp_serial_uart_search_handle_by_port(port);
    USART_EnableInterrupts(nxp_uart_handle,USART_INTENSET_TXRDYEN_MASK);
}
 

/*
* @brief 串口发送为空中断禁止驱动
* @param port uart端口号
* @return 无
* @note
*/
void nxp_serial_uart_hal_disable_txe_it(uint8_t port)
{
    USART_Type *nxp_uart_handle; 

    nxp_uart_handle = nxp_serial_uart_search_handle_by_port(port);
    USART_DisableInterrupts(nxp_uart_handle,USART_INTENSET_TXRDYEN_MASK);  
}


/*
* @brief 串口接收不为空中断使能驱动
* @param port uart端口号
* @return 无
* @note
*/ 
void nxp_serial_uart_hal_enable_rxne_it(uint8_t port)
{
    USART_Type *nxp_uart_handle; 

    nxp_uart_handle = nxp_serial_uart_search_handle_by_port(port);
    USART_EnableInterrupts(nxp_uart_handle,USART_INTENSET_RXRDYEN_MASK);
}


/*
* @brief 串口接收不为空中断禁止驱动
* @param port uart端口号
* @return 无
* @note
*/
void nxp_serial_uart_hal_disable_rxne_it(uint8_t port)
{
    USART_Type *nxp_uart_handle; 

    nxp_uart_handle = nxp_serial_uart_search_handle_by_port(port);
    USART_DisableInterrupts(nxp_uart_handle,USART_INTENSET_RXRDYEN_MASK);
}


/*
* @brief 串口中断routine驱动
* @param handle uart的serial句柄
* @return 无
* @note
*/
void nxp_serial_uart_hal_isr(int handle)
{
    int result;
    uint32_t tmp_flag = 0, tmp_it_source = 0; 
    char  send_byte,recv_byte;
    USART_Type *nxp_uart_handle; 

    nxp_uart_handle = nxp_serial_uart_search_handle_by_port(((serial_t *)handle)->port);

    tmp_flag = USART_GetEnabledInterrupts(nxp_uart_handle);
    tmp_it_source =USART_GetStatusFlags(nxp_uart_handle);
  
    /*接收中断处理*/
    if ((tmp_flag & USART_INTENSET_RXRDYEN_MASK) && (tmp_it_source & USART_STAT_RXRDY_MASK)){
        recv_byte=USART_ReadByte(nxp_uart_handle);
        isr_serial_put_byte_from_recv(handle,recv_byte);

    }
    /*发送中断处理*/
    if ((tmp_flag & USART_INTENSET_TXRDYEN_MASK) && (tmp_it_source & USART_STAT_TXRDY_MASK)){
        result =isr_serial_get_byte_to_send(handle,&send_byte);
        if (result == 1) {
            USART_WriteByte(nxp_uart_handle, send_byte);
        }
    }
}


/*串口中断处理*/
void USART1_IRQHandler()
{
  nxp_serial_uart_hal_isr();
}