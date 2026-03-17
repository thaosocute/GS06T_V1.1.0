#include "pcf8575.h"
#include "stm32l4xx_hal_i2c.h"

/**
 * @brief bit operate definition
 */
#define BITBAND(addr, bitnum)    ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2)) 
#define MEM_ADDR(addr)           *((uint32_t *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))

/**
 * @brief iic gpio operate definition
 */
#define GPIOB_ODR_Addr    (GPIOB_BASE + 0x14)
#define GPIOB_IDR_Addr    (GPIOB_BASE + 0x10)
#define PBout(n)          BIT_ADDR(GPIOB_ODR_Addr, n)
#define PBin(n)           BIT_ADDR(GPIOB_IDR_Addr, n)
#define SDA_IN()          {GPIOB->MODER &= ~(3 << (9 * 2)); GPIOB->MODER |= 0 << 9 * 2;}
#define SDA_OUT()         {GPIOB->MODER &= ~(3 << (9 * 2)); GPIOB->MODER |= 1 << 9 * 2;}
#define IIC_SCL           PBout(8)
#define IIC_SDA           PBout(9)
#define READ_SDA          PBin(9)

static volatile uint32_t gs_fac_us = 0;
static uint16_t pcf8575_state = 0x0000;
static uint8_t write_mode_cmd = 0x42;

static void delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t told;
    uint32_t tnow;
    uint32_t tcnt;
    uint32_t reload;
    
    /* set the used param */
    tcnt = 0;
    reload = SysTick->LOAD;
    ticks = us * gs_fac_us;
    told = SysTick->VAL;
    
    /* delay */
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else 
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

void pcf8575_i2c_start(void)
{
    SDA_OUT();
    IIC_SDA = 1;
    IIC_SCL = 1;
    delay_us(4);
    IIC_SDA = 0;
    delay_us(4);
    IIC_SCL = 0;
}

void pcf8575_i2c_stop(void)
{
    SDA_OUT();
    IIC_SCL = 0;
    IIC_SDA = 0;
    delay_us(4);
    IIC_SCL = 1;
    delay_us(4);
    IIC_SDA = 1;
    delay_us(4);
}

uint8_t pcf8575_i2c_wait_ack(void)
{
    uint8_t ucErrTime = 0;
    SDA_IN();
    IIC_SDA = 1;
    delay_us(1);
    IIC_SCL = 1;
    delay_us(1);
    while (READ_SDA)
    {
        ucErrTime++;
        if (ucErrTime > 250)
        {
            pcf8575_i2c_stop();
            return 1;
        }
    }
    IIC_SCL = 0;
    return 0;
}

void pcf8575_write_pin(I2C_HandleTypeDef *hi2c, uint8_t pin, uint8_t level)
{
    uint8_t data[2];
    if (pin < 0 || pin > 15)
    {
        return;                                                         /* check pin */
    } else if (pin < 8)
    {
        if (level)
        {
            pcf8575_state |= (1 << (pin + 8));                                       /* set bit */
        } else
        {
            pcf8575_state &= ~(1 << (pin + 8));                                      /* clear bit */
        }
    } else
    {
        if (level)
        {
            pcf8575_state |= (1 << (pin - 8));                                       /* set bit */
        } else
        {
            pcf8575_state &= ~(1 << (pin - 8));                                      /* clear bit */
        }
    }
    data[0] = (pcf8575_state >> 8) & 0xFF;                                              /* set buffer 0 */
    data[1] = (pcf8575_state >> 0) & 0xFF;   
    pcf8575_i2c_start();
    pcf8575_i2c_wait_ack();
    HAL_I2C_Master_Transmit(hi2c, 0x42, &write_mode_cmd, 1, HAL_MAX_DELAY);
    pcf8575_i2c_wait_ack();
    HAL_I2C_Master_Transmit(hi2c, 0x42, data, 2, HAL_MAX_DELAY);
    pcf8575_i2c_wait_ack();
    pcf8575_i2c_stop();
}

void pcf8575_reset_state(I2C_HandleTypeDef *hi2c)
{
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = 0x00;
    pcf8575_state = 0x0000;
    pcf8575_i2c_start();
    pcf8575_i2c_wait_ack();
    HAL_I2C_Master_Transmit(hi2c, 0x42, &write_mode_cmd, 1, HAL_MAX_DELAY);
    pcf8575_i2c_wait_ack();
    HAL_I2C_Master_Transmit(hi2c, 0x42, data, 2, HAL_MAX_DELAY);
    pcf8575_i2c_wait_ack();
    pcf8575_i2c_stop();
}

