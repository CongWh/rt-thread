/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-5      SummerGift   change to new framework
 * 2018-12-11     greedyhao    Porting for stm32f7xx
 */

#include "board.h"

#ifdef RT_USING_SPI

#if defined(BSP_USING_SPI1) || defined(BSP_USING_SPI2) || defined(BSP_USING_SPI3) || defined(BSP_USING_SPI4) || defined(BSP_USING_SPI5) || defined(BSP_USING_SPI6) 
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */

#include "drv_spi.h"
#include "drv_config.h"

//#define DRV_DEBUG
#define LOG_TAG              "drv.spi"
#include <drv_log.h>

enum
{
#ifdef BSP_USING_SPI1
    SPI1_INDEX,
#endif
#ifdef BSP_USING_SPI2
    SPI2_INDEX,
#endif
#ifdef BSP_USING_SPI3
    SPI3_INDEX,
#endif
#ifdef BSP_USING_SPI4
    SPI4_INDEX,
#endif
#ifdef BSP_USING_SPI5
    SPI5_INDEX,
#endif
#ifdef BSP_USING_SPI6
    SPI6_INDEX,
#endif
};

static struct stm32_spi_config spi_config[] =
{
#ifdef BSP_USING_SPI1
    SPI1_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI2
    SPI2_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI3
    SPI3_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI4
    SPI4_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI5
    SPI5_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI6
    SPI6_BUS_CONFIG,
#endif
};

static struct stm32_spi spi_bus_obj[sizeof(spi_config) / sizeof(spi_config[0])];

static rt_err_t stm32_spi_init(struct stm32_spi *spi_drv, struct rt_spi_configuration *cfg)
{
    RT_ASSERT(spi_drv != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    SPI_HandleTypeDef *spi_handle = &spi_drv->handle;

    if (cfg->mode & RT_SPI_SLAVE)
    {
        spi_handle->Init.Mode = SPI_MODE_SLAVE;
    }
    else
    {
        spi_handle->Init.Mode = SPI_MODE_MASTER;
    }

    if (cfg->mode & RT_SPI_3WIRE)
    {
        spi_handle->Init.Direction = SPI_DIRECTION_1LINE;
    }
    else
    {
        spi_handle->Init.Direction = SPI_DIRECTION_2LINES;
    }

    if (cfg->data_width == 8)
    {
        spi_handle->Init.DataSize = SPI_DATASIZE_8BIT;
        spi_handle->TxXferSize = 8;
        spi_handle->RxXferSize = 8;
    }
    else if (cfg->data_width == 16)
    {
        spi_handle->Init.DataSize = SPI_DATASIZE_16BIT;
    }
    else
    {
        return RT_EIO;
    }

    if (cfg->mode & RT_SPI_CPHA)
    {
        spi_handle->Init.CLKPhase = SPI_PHASE_2EDGE;
    }
    else
    {
        spi_handle->Init.CLKPhase = SPI_PHASE_1EDGE;
    }

    if (cfg->mode & RT_SPI_CPOL)
    {
        spi_handle->Init.CLKPolarity = SPI_POLARITY_HIGH;
    }
    else
    {
        spi_handle->Init.CLKPolarity = SPI_POLARITY_LOW;
    }

    if (cfg->mode & RT_SPI_NO_CS)
    {
        spi_handle->Init.NSS = SPI_NSS_SOFT;
    }
    else
    {
        spi_handle->Init.NSS = SPI_NSS_SOFT;
    }

    uint32_t SPI_APB_CLOCK;

    SPI_APB_CLOCK = HAL_RCC_GetPCLK2Freq();

    if (cfg->max_hz >= SPI_APB_CLOCK / 2)
    {
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    }
    else if (cfg->max_hz >= SPI_APB_CLOCK / 4)
    {
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    }
    else if (cfg->max_hz >= SPI_APB_CLOCK / 8)
    {
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    }
    else if (cfg->max_hz >= SPI_APB_CLOCK / 16)
    {
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    }
    else if (cfg->max_hz >= SPI_APB_CLOCK / 32)
    {
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    }
    else if (cfg->max_hz >= SPI_APB_CLOCK / 64)
    {
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    }
    else if (cfg->max_hz >= SPI_APB_CLOCK / 128)
    {
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    }
    else
    {
        /*  min prescaler 256 */
        spi_handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    }

    LOG_D("sys freq: %d, pclk2 freq: %d, SPI limiting freq: %d, BaudRatePrescaler: %d",
          HAL_RCC_GetSysClockFreq(),
          SPI_APB_CLOCK,
          cfg->max_hz,
          spi_handle->Init.BaudRatePrescaler);

    if (cfg->mode & RT_SPI_MSB)
    {
        spi_handle->Init.FirstBit = SPI_FIRSTBIT_MSB;
    }
    else
    {
        spi_handle->Init.FirstBit = SPI_FIRSTBIT_LSB;
    }

    spi_handle->Init.TIMode = SPI_TIMODE_DISABLE;
    spi_handle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi_handle->State = HAL_SPI_STATE_RESET;
    if (HAL_SPI_Init(spi_handle) != HAL_OK)
    {
        return RT_EIO;
    }

#if defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32F7)
    SET_BIT(spi_handle->Instance->CR2, SPI_RXFIFO_THRESHOLD_HF);
#endif

    __HAL_SPI_ENABLE(spi_handle);

    LOG_D("%s init done", spi_drv->config->bus_name);
    return RT_EOK;
}

#ifdef BSP_SPI_USING_DMA
static uint8_t dummy = 0xFF;
static void spi_dma_transfer_prepare(struct rt_spi_bus * spi_bus, struct rt_spi_message* message)
{
    struct stm32_spi *spi_drv =  rt_container_of(spi_bus, struct stm32_spi, spi_bus);

    DMA_HandleTypeDef * hdma_tx = (DMA_HandleTypeDef *)&spi_drv->dma.handle_tx;
    DMA_HandleTypeDef * hdma_rx = (DMA_HandleTypeDef *)&spi_drv->dma.handle_rx;

    HAL_DMA_DeInit(hdma_tx);
    HAL_DMA_DeInit(hdma_rx);

    /*
     * Check if the DMA Stream is disabled before enabling it.
     * Note that this step is useful when the same Stream is used multiple times.
     */
#if defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7)
    while (hdma_tx->Instance->CR & DMA_SxCR_EN);
    while (hdma_rx->Instance->CR & DMA_SxCR_EN);
#endif

    if(message->recv_buf != RT_NULL)
    {
        hdma_rx->Init.MemInc = DMA_MINC_ENABLE;
    }
    else
    {
        message->recv_buf = &dummy;
        hdma_rx->Init.MemInc = DMA_MINC_DISABLE;
    }
    HAL_DMA_Init(hdma_rx);

    __HAL_LINKDMA(&spi_drv->handle, hdmarx, spi_drv->dma.handle_rx);

    if(message->send_buf != RT_NULL)
    {
        hdma_tx->Init.MemInc = DMA_MINC_ENABLE;
    }
    else
    {
        dummy = 0xFF;
        message->send_buf = &dummy;
        hdma_tx->Init.MemInc = DMA_MINC_DISABLE;
    }
    HAL_DMA_Init(hdma_tx);

    /* link DMA with SPI */
    __HAL_LINKDMA(&spi_drv->handle, hdmatx, spi_drv->dma.handle_tx);

    LOG_D("%s RX Instance: %x, TX Instance: %x", spi_drv->config->bus_name, hdma_rx->Instance, hdma_tx->Instance);
    LOG_D("%s dma config done, TX dma_irq number: %d, RX dma_irq number: %d",
          spi_drv->config->bus_name,
          spi_drv->config->dma_tx.dma_irq,
          spi_drv->config->dma_rx.dma_irq);

    /* NVIC configuration for DMA transfer complete interrupt*/
    HAL_NVIC_SetPriority(spi_drv->config->dma_tx.dma_irq, 0, 1);
    HAL_NVIC_EnableIRQ(spi_drv->config->dma_tx.dma_irq);

    /* NVIC configuration for DMA transfer complete interrupt*/
    HAL_NVIC_SetPriority(spi_drv->config->dma_rx.dma_irq, 0, 0);
    HAL_NVIC_EnableIRQ(spi_drv->config->dma_rx.dma_irq);
}
#endif

static rt_uint32_t spixfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->parent.user_data != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    struct stm32_spi *spi_drv =  rt_container_of(device->bus, struct stm32_spi, spi_bus);
    SPI_HandleTypeDef * spi_handle = &spi_drv->handle;
    struct stm32_hw_spi_cs *cs = device->parent.user_data;
    rt_int32_t length = message->length;
    rt_int32_t data_width = spi_drv->cfg->data_width;

    if (message->cs_take)
    {
        HAL_GPIO_WritePin(cs->GPIOx, cs->GPIO_Pin, GPIO_PIN_RESET);
    }

#ifdef BSP_SPI_USING_DMA
    if(message->length > 32)
    {
        if(data_width <= 8)
        {
            HAL_StatusTypeDef state;
            LOG_D("%s dma transfer prepare and start", spi_drv->config->bus_name);
            LOG_D("%s sendbuf: %X, recvbuf: %X, length: %d",
                   spi_drv->config->bus_name,
                  (uint32_t)message->send_buf,
                  (uint32_t)message->recv_buf, message->length);

            spi_dma_transfer_prepare(device->bus, message);
            /* start once data exchange in DMA mode */
            state = HAL_SPI_TransmitReceive_DMA(spi_handle,
                                                (uint8_t*)message->send_buf,
                                                (uint8_t*)message->recv_buf,
                                                message->length);
            if (state != HAL_OK)
            {
                LOG_D("spi flash configuration error : %d", state);
                message->length = 0;
                //while(1);
            }
            else
            {
                LOG_D("%s dma transfer done", spi_drv->config->bus_name);
            }

            /* For simplicity reasons, this example is just waiting till the end of the
               transfer, but application may perform other tasks while transfer operation
               is ongoing. */
            while (HAL_SPI_GetState(spi_handle) != HAL_SPI_STATE_READY);
            LOG_D("%s get state done", spi_drv->config->bus_name);
        }
        else
        {
            // TODO
        }
    } else
#endif
    {
        if (data_width == 8)
        {
            const rt_uint8_t * send_ptr = message->send_buf;
            rt_uint8_t * recv_ptr = message->recv_buf;
            
            while (length--)
            {
                rt_uint8_t data = ~0;

                if(send_ptr != RT_NULL)
                {
                    data = *send_ptr++;
                }
                
                /* send data once */
                while (__HAL_SPI_GET_FLAG(spi_handle, SPI_FLAG_TXE) == RESET);
                *(volatile rt_uint8_t *)(&spi_handle->Instance->DR) = data;

                /* receive data once */
#if defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32F7)
                SET_BIT(spi_handle->Instance->CR2, SPI_RXFIFO_THRESHOLD_HF);
#endif
                while (__HAL_SPI_GET_FLAG(spi_handle, SPI_FLAG_RXNE) == RESET);
                data = *(volatile rt_uint8_t *)(&spi_handle->Instance->DR);
                
                if(recv_ptr != RT_NULL)
                {
                    *recv_ptr++ = data;
                }
            }

        } else
        {
            const rt_uint16_t * send_ptr = message->send_buf;
            rt_uint16_t * recv_ptr = message->recv_buf;
            
            while (length--)
            {
                rt_uint16_t data = ~0;
                
                if(send_ptr != RT_NULL)
                {
                    data = *send_ptr++;
                }
                
                /* send data once */
                while (__HAL_SPI_GET_FLAG(spi_handle, SPI_FLAG_TXE) == RESET);
                *(volatile rt_uint16_t *)(&spi_handle->Instance->DR) = data;

                /* receive data once */
#if defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32F7)
                SET_BIT(spi_handle->Instance->CR2, SPI_RXFIFO_THRESHOLD_HF);
#endif
                while (__HAL_SPI_GET_FLAG(spi_handle, SPI_FLAG_RXNE) == RESET);
                data = *(volatile rt_uint16_t *)(&spi_handle->Instance->DR);
                
                if(recv_ptr != RT_NULL)
                {
                    *recv_ptr++ = data;
                }
            }
        }
    }

    /* Wait until Busy flag is reset before disabling SPI */
    while (__HAL_SPI_GET_FLAG(spi_handle, SPI_FLAG_BSY) == SET);

    if (message->cs_release)
    {
        HAL_GPIO_WritePin(cs->GPIOx, cs->GPIO_Pin, GPIO_PIN_SET);
    }

    return message->length;
}

static rt_err_t spi_configure(struct rt_spi_device *device,
                              struct rt_spi_configuration *configuration)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    struct stm32_spi *spi_drv =  rt_container_of(device->bus, struct stm32_spi, spi_bus);
    spi_drv->cfg = configuration;

    return stm32_spi_init(spi_drv, configuration);
}

static const struct rt_spi_ops stm_spi_ops =
{
    .configure = spi_configure,
    .xfer = spixfer,
};

static int rt_hw_spi_bus_init(void)
{
    rt_err_t result;
    for (int i = 0; i < sizeof(spi_config) / sizeof(spi_config[0]); i++)
    {
        spi_bus_obj[i].config = &spi_config[i];
        spi_bus_obj[i].spi_bus.parent.user_data = &spi_config[i];
        spi_bus_obj[i].handle.Instance = spi_config[i].Instance;

#ifdef BSP_SPI_USING_DMA
        /* Configure the DMA handler for Transmission process */
        spi_bus_obj[i].dma.handle_tx.Instance = spi_config[i].dma_tx.Instance;
#if defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7)
        spi_bus_obj[i].dma.handle_tx.Init.Channel = spi_config[i].dma_tx.channel;
#elif defined(SOC_SERIES_STM32L4)
        spi_bus_obj[i].dma.handle_tx.Init.Request = spi_config[i].dma_tx.request;
#endif
        spi_bus_obj[i].dma.handle_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        spi_bus_obj[i].dma.handle_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        spi_bus_obj[i].dma.handle_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        spi_bus_obj[i].dma.handle_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        spi_bus_obj[i].dma.handle_tx.Init.Mode                = DMA_NORMAL;
        spi_bus_obj[i].dma.handle_tx.Init.Priority            = DMA_PRIORITY_LOW;
#if defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7)
        spi_bus_obj[i].dma.handle_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        spi_bus_obj[i].dma.handle_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        spi_bus_obj[i].dma.handle_tx.Init.MemBurst            = DMA_MBURST_INC4;
        spi_bus_obj[i].dma.handle_tx.Init.PeriphBurst         = DMA_PBURST_INC4;
#endif

        spi_bus_obj[i].dma.handle_rx.Instance = spi_config[i].dma_rx.Instance;
#if defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7)
        spi_bus_obj[i].dma.handle_rx.Init.Channel = spi_config[i].dma_rx.channel;
#elif defined(SOC_SERIES_STM32L4)
        spi_bus_obj[i].dma.handle_rx.Init.Request = spi_config[i].dma_rx.request;
#endif
        spi_bus_obj[i].dma.handle_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        spi_bus_obj[i].dma.handle_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        spi_bus_obj[i].dma.handle_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        spi_bus_obj[i].dma.handle_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        spi_bus_obj[i].dma.handle_rx.Init.Mode                = DMA_NORMAL;
        spi_bus_obj[i].dma.handle_rx.Init.Priority            = DMA_PRIORITY_HIGH;
#if defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7)
        spi_bus_obj[i].dma.handle_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        spi_bus_obj[i].dma.handle_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        spi_bus_obj[i].dma.handle_rx.Init.MemBurst            = DMA_MBURST_INC4;
        spi_bus_obj[i].dma.handle_rx.Init.PeriphBurst         = DMA_PBURST_INC4;
#endif
        {
            rt_uint32_t tmpreg = 0x00U;
#if defined(SOC_SERIES_STM32F1)
            /* enable DMA clock && Delay after an RCC peripheral clock enabling*/
            SET_BIT(RCC->AHBENR, spi_config[i].dma_rx.dma_rcc);
            tmpreg = READ_BIT(RCC->AHBENR, spi_config[i].dma_rx.dma_rcc);
#elif defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32L4) 
            SET_BIT(RCC->AHB1ENR, spi_config[i].dma_rx.dma_rcc);
            /* Delay after an RCC peripheral clock enabling */
            tmpreg = READ_BIT(RCC->AHB1ENR, spi_config[i].dma_rx.dma_rcc);
#endif
            UNUSED(tmpreg); /* To avoid compiler warnings */
        }

        LOG_D("%s DMA clock init done", spi_config[i].bus_name);
#endif /* BSP_SPI_USING_DMA */

        result = rt_spi_bus_register(&spi_bus_obj[i].spi_bus, spi_config[i].bus_name, &stm_spi_ops);
        RT_ASSERT(result == RT_EOK);

        LOG_D("%s bus init done", spi_config[i].bus_name);
    }

    return result;
}

/**
  * Attach the spi device to SPI bus, this function must be used after initialization.
  */
rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, GPIO_TypeDef* cs_gpiox, uint16_t cs_gpio_pin)
{
    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    rt_err_t result;
    struct rt_spi_device *spi_device;
    struct stm32_hw_spi_cs *cs_pin;

    /* initialize the cs pin && select the slave*/
    GPIO_InitTypeDef GPIO_Initure;
    GPIO_Initure.Pin = cs_gpio_pin;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(cs_gpiox, &GPIO_Initure);
    HAL_GPIO_WritePin(cs_gpiox, cs_gpio_pin, GPIO_PIN_SET);

    /* attach the device to spi bus*/
    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    RT_ASSERT(spi_device != RT_NULL);
    cs_pin = (struct stm32_hw_spi_cs *)rt_malloc(sizeof(struct stm32_hw_spi_cs));
    RT_ASSERT(cs_pin != RT_NULL);
    cs_pin->GPIOx = cs_gpiox;
    cs_pin->GPIO_Pin = cs_gpio_pin;
    result = rt_spi_bus_attach_device(spi_device, device_name, bus_name, (void *)cs_pin);

    if (result != RT_EOK)
    {
        LOG_E("%s attach to %s faild, %d\n", device_name, bus_name, result);
    }

    RT_ASSERT(result == RT_EOK);

    LOG_D("%s attach to %s done", device_name, bus_name);

    return result;
}

#if defined(BSP_USING_SPI1) && defined(BSP_SPI_USING_DMA)
/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
void SPI1_DMA_RX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI1_INDEX].dma.handle_rx);

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
void SPI1_DMA_TX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI1_INDEX].dma.handle_tx);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* defined(BSP_USING_SPI1) && defined(BSP_SPI_USING_DMA) */

#if defined(BSP_USING_SPI2) && defined(BSP_SPI_USING_DMA)
/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
void SPI2_DMA_RX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI2_INDEX].dma.handle_rx);

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
void SPI2_DMA_TX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI2_INDEX].dma.handle_tx);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* defined(BSP_USING_SPI2) && defined(BSP_SPI_USING_DMA) */

#if defined(BSP_USING_SPI3) && defined(BSP_SPI_USING_DMA)
/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
void SPI3_DMA_RX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI3_INDEX].dma.handle_rx);

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
void SPI3_DMA_TX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI3_INDEX].dma.handle_tx);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* defined(BSP_USING_SPI3) && defined(BSP_SPI_USING_DMA) */


#if defined(BSP_USING_SPI4) && defined(BSP_SPI_USING_DMA)
/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
void SPI4_DMA_RX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI4_INDEX].dma.handle_rx);

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
void SPI4_DMA_TX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI4_INDEX].dma.handle_tx);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* defined(BSP_USING_SPI4) && defined(BSP_SPI_USING_DMA) */

#if defined(BSP_USING_SPI5) && defined(BSP_SPI_USING_DMA)
/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
void SPI5_DMA_RX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI5_INDEX].dma.handle_rx);

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
void SPI5_DMA_TX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI5_INDEX].dma.handle_tx);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* defined(BSP_USING_SPI5) && defined(BSP_SPI_USING_DMA) */

#if defined(BSP_USING_SPI6) && defined(BSP_SPI_USING_DMA)
/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
void SPI6_DMA_RX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI6_INDEX].dma.handle_rx);

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
void SPI6_DMA_TX_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_DMA_IRQHandler(&spi_bus_obj[SPI6_INDEX].dma.handle_tx);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* defined(BSP_USING_SPI6) && defined(BSP_SPI_USING_DMA) */

int rt_hw_spi_init(void)
{
    return rt_hw_spi_bus_init();
}
INIT_BOARD_EXPORT(rt_hw_spi_init);

#endif /* BSP_USING_SPI1 || BSP_USING_SPI2 || BSP_USING_SPI3 || BSP_USING_SPI4 || BSP_USING_SPI5 */
#endif /* RT_USING_SPI */
