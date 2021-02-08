#include "i2c_test.h"
#include "stm32f10x.h"
#include "RTE_Device.h"

//#define I2C_DMA_ENABLE

enum i2cState_t {
    I2C_STATE_NOT_INIT,
    I2C_STATE_INIT,
    I2C_STATE_READ_START
};

enum i2cInterruptState_t {
    I2C_INT_WRITE_ADDRESS_1,
    I2C_INT_WRITE_ADDRESS_2,
    I2C_INT_READ_ADDRESS,
    I2C_INT_READ_ADDRESS_1,
    I2C_INT_READ_DATA,
    I2C_INT_WRITE_ADDRESS_END
};

struct i2c_type_t {
    uint8_t deviceAddress;
    uint16_t address;
    uint8_t *buff;
    uint8_t lenght;
    enum i2cInterruptState_t transmitState;
    #if !defined(I2C_DMA_ENABLE)
    uint16_t i2cCountReceived;
    #endif
};

static struct i2c_type_t i2cSettings;
static enum i2cState_t i2cState = I2C_STATE_NOT_INIT;

#define LED1_PORT   B
#define LED1_PIN    6
#define LED2_PORT   B
#define LED2_PIN    6
#define LED3_PORT   B
#define LED3_PIN    6
#define LED4_PORT   B
#define LED4_PIN    5
#define LED5_PORT   B
#define LED5_PIN    4
#define LED6_PORT   B
#define LED6_PIN    3
#define LED7_PORT   A
#define LED7_PIN    15
#define LED8_PORT   A
#define LED8_PIN    12

#if (LED1_PIN < 8)  
#define LED1_LH     L
#else
#define LED1_LH     H
#endif
#if (LED2_PIN < 8)  
#define LED2_LH     L
#else
#define LED2_LH     H
#endif
#if (LED3_PIN < 8)  
#define LED3_LH     L
#else
#define LED3_LH     H
#endif
#if (LED4_PIN < 8)  
#define LED4_LH     L
#else
#define LED4_LH     H
#endif
#if (LED5_PIN < 8)  
#define LED5_LH     L
#else
#define LED5_LH     H
#endif
#if (LED6_PIN < 8)  
#define LED6_LH     L
#else
#define LED6_LH     H
#endif
#if (LED7_PIN < 8)  
#define LED7_LH     L
#else
#define LED7_LH     H
#endif
#if (LED8_PIN < 8)  
#define LED8_LH     L
#else
#define LED8_LH     H
#endif

//GPIOA->BSRR = (GPIOA->ODR & GPIO_ODR_ODR0) ? GPIO_BSRR_BR0 : GPIO_BSRR_BS0;
#define _led_toggle(port, pin)   GPIO##port->BSRR = (GPIO##port->ODR & GPIO_ODR_ODR##pin) ? GPIO_BSRR_BR##pin : GPIO_BSRR_BS##pin;
#define  led_toggle(port, pin)   _led_toggle(port, pin)
static inline void led1_toggle(void) { led_toggle(LED1_PORT, LED1_PIN) };
static inline void led2_toggle(void) { led_toggle(LED2_PORT, LED2_PIN) };
static inline void led3_toggle(void) { led_toggle(LED3_PORT, LED3_PIN) };
static inline void led4_toggle(void) { led_toggle(LED4_PORT, LED4_PIN) };
static inline void led5_toggle(void) { led_toggle(LED5_PORT, LED5_PIN) };
static inline void led6_toggle(void) { led_toggle(LED6_PORT, LED6_PIN) };
static inline void led7_toggle(void) { led_toggle(LED7_PORT, LED7_PIN) };
static inline void led8_toggle(void) { led_toggle(LED8_PORT, LED8_PIN) };

#define __ledInit(port, pin, lh)   GPIO##port->BSRR |= GPIO_BSRR_BR##pin;                                   \
                                   reg = GPIO##port->CR##lh;                                                \
                                   reg &= ~((uint32_t)GPIO_CR##lh##_CNF##pin | GPIO_CR##lh##_MODE##pin);    \
                                   reg |= GPIO_CR##lh##_MODE##pin;                                          \
                                   GPIO##port->CR##lh = reg;
#define  _ledInit(port, pin, lh)    __ledInit(port, pin, lh)
static inline void led_init(void) {
    uint32_t reg;
    RCC->APB2ENR |= ((uint32_t)RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN);
    //GPIOA->BSRR |= GPIO_BSRR_BR0;
    //uint32_t reg = GPIOA->CRH;
    //reg &= ~((uint32_t)GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    //reg |= GPIO_CRL_MODE0;
    //GPIOA->CRH = reg;
    _ledInit(LED1_PORT, LED1_PIN, LED1_LH);
    _ledInit(LED2_PORT, LED2_PIN, LED2_LH);
    _ledInit(LED3_PORT, LED3_PIN, LED3_LH);
    _ledInit(LED4_PORT, LED4_PIN, LED4_LH);
    _ledInit(LED5_PORT, LED5_PIN, LED5_LH);
    _ledInit(LED6_PORT, LED6_PIN, LED6_LH);
    _ledInit(LED7_PORT, LED7_PIN, LED7_LH);
    _ledInit(LED8_PORT, LED8_PIN, LED8_LH);
    led1_toggle(); led2_toggle();
    led1_toggle(); led2_toggle();
}
/*
    PB8 - I2C1_SCL
    PB9 - I2C1_SDA
*/
void i2c_init_mapping(void) {
    uint32_t reg;

    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->MAPR |= AFIO_MAPR_I2C1_REMAP;
    RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
    
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    reg = GPIOB->CRH;
    reg &= ~((uint32_t)GPIO_CRH_MODE8 | GPIO_CRH_CNF8 | GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
    reg |= ((uint32_t)GPIO_CRH_MODE8_1 | GPIO_CRH_CNF8); // 10: Output mode, max speed 2 MHz; 11: Alternate function output Open-drain - PB8 - I2C1_SCL
    reg |= ((uint32_t)GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9); // 10: Output mode, max speed 2 MHz; 11: Alternate function output Open-drain - PB9 - I2C1_SDA
    reg |= (GPIO_CRH_MODE8 | GPIO_CRH_MODE9);
    GPIOB->CRH = reg;
}

void i2c_init(void) {
    bool reInit;
    uint8_t countError = 10u;
    led_init();
    led3_toggle();
    led4_toggle();
    led5_toggle();
    led6_toggle();
    led7_toggle();
    led8_toggle();
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    //I2C1->CR2 = RTE_PCLK1 / 1000000u;
    do {
        reInit = false;
        countError--;
        I2C1->CR1 = 0;
        I2C1->CR2 = 8u;
        I2C1->CCR = 0x28u;
        I2C1->TRISE = 9u;
        I2C1->CR2 |= I2C_CR2_ITERREN;
        if(I2C1->SR2 & I2C_SR2_BUSY) {
            reInit = true;
            I2C1->CR1 = I2C_CR1_SWRST;
            I2C1->CR1 = (I2C_CR1_START | I2C_CR1_STOP | I2C_CR1_PE);
            while(I2C1->CR1 & (I2C_CR1_STOP | I2C_CR1_START));
        }
        I2C1->CR1 = 0;
    } while(reInit && (countError != 0u));

    #if defined(I2C_DMA_ENABLE)
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel7->CCR = 0u;
    DMA1->ISR |= DMA_ISR_TCIF7;                 // enable transfer complete DMA interrupt
    DMA1_Channel7->CPAR = I2C1_BASE + 0x10u;    // 0x10 is offcet of i2c DR register
    DMA1_Channel7->CCR = ((uint32_t) DMA_CCR1_MINC | DMA_CCR1_TCIE);
    #endif
    RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;

    NVIC_EnableIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

bool i2c_read(uint8_t deviceAddress, uint16_t address, uint8_t *buff, uint16_t length) {
    bool ret = false;
    if(i2cState == I2C_STATE_INIT) {
        I2C1->CR1 = I2C_CR1_PE;
        do {
            if(I2C1->SR2 & I2C_SR2_BUSY) {
                i2c_init();
                I2C1->CR1 = I2C_CR1_PE;
            }
        } while(I2C1->SR2 & I2C_SR2_BUSY);
        #if defined(I2C_DMA_ENABLE)
        RCC->AHBENR |= RCC_AHBENR_DMA1EN;
        DMA1_Channel7->CNDTR = length;
        DMA1_Channel7->CMAR = (uint32_t)buff;
        DMA1_Channel7->CCR &= ~DMA_CCR1_DIR;    // DMA - data from peripheral
        RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
        #endif

        i2cSettings.deviceAddress = deviceAddress;
        i2cSettings.address = address;
        i2cSettings.buff = buff;
        i2cSettings.lenght = length;
        i2cSettings.transmitState = I2C_INT_READ_ADDRESS;
        i2cState = I2C_STATE_READ_START;
        #if !defined(I2C_DMA_ENABLE)
        i2cSettings.i2cCountReceived = 0u;
        #endif
        ret = true;

        I2C1->CR2 |= I2C_CR2_ITEVTEN;   // enable interrupt
        I2C1->CR1 |= I2C_CR1_START;

    }
    return ret;
}

void i2c_handleTask(void) {
    switch(i2cState) {
        case I2C_STATE_INIT:
            break;
        case I2C_STATE_NOT_INIT:
            i2c_init_mapping();
            i2c_init();
            i2cState = I2C_STATE_INIT;
            break;
        default:
            break;
    }
}

__irq void I2C1_ER_IRQHandler(void) {
    uint32_t status = I2C1->SR1;
    if(status & (I2C_SR1_AF | I2C_SR1_BERR))  {
        I2C1->SR1 &= ~(uint32_t)(I2C_SR1_AF | I2C_SR1_BERR);
        I2C1->CR1 |= I2C_CR1_STOP;
        I2C1->CR2 &= ~I2C_CR2_ITEVTEN;
    } else {
        i2c_init();
    }
    RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
    i2cState = I2C_STATE_INIT;
}

#if defined(I2C_DMA_ENABLE)
__irq void DMA1_Channel7_IRQHandler(void) {                                                                     // 7.
    DMA1->IFCR |= DMA_IFCR_CTCIF7;      // clear interrupt flag
    I2C1->CR2 &= ~((uint32_t) I2C_CR2_LAST | I2C_CR2_DMAEN); // disable i2c request for DMA in 
    DMA1_Channel7->CCR &= ~DMA_CCR1_EN; // disable DMA
    I2C1->CR1 |= I2C_CR1_STOP;
    RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
    i2cState = I2C_STATE_INIT;
    I2C1->CR1 = 0u;
}
#endif

__irq void I2C1_EV_IRQHandler(void) {
    uint32_t status = I2C1->SR1;
    if(status & I2C_SR1_SB) {
        if(i2cSettings.transmitState == I2C_INT_READ_ADDRESS) {
            I2C1->DR = i2cSettings.deviceAddress & 0xFEu; // write address for read                             // 1.
        } else {
            RCC->AHBENR |= RCC_AHBENR_DMA1EN;                                                                   // 5.
            #if defined(I2C_DMA_ENABLE)
            DMA1_Channel7->CCR |= DMA_CCR1_EN;                           // enable DMA                                                                     6.
            I2C1->CR2 |= ((uint32_t) I2C_CR2_DMAEN | I2C_CR2_LAST);      // enable i2c request for DMA
            #endif
            I2C1->CR1 |= I2C_CR1_ACK;
            I2C1->DR = i2cSettings.deviceAddress | 0x01u; // read after write address and restart
        }
    } else if(status & I2C_SR1_ADDR){
        (void)I2C1->SR2; // clear ADDR status bit
        if(RCC->AHBENR & RCC_AHBENR_DMA1EN) {
            #if defined(I2C_DMA_ENABLE)
            I2C1->CR2 &= ~I2C_CR2_ITEVTEN;             // disable i2c interrupt                                 // 6.
            #else
            i2cSettings.transmitState = I2C_INT_READ_DATA;
            #endif
        } else {                                                                                                // 2.
            I2C1->DR = (uint8_t)i2cSettings.address >> 8u; // send hight address 0xUUxx - UU vysovy byte 16-bit adresy, xx- nizka byte
        }
    } else if(status & I2C_SR1_BTF) {
        // if(i2cSettings.transmitState == I2C_INT_READ_DATA) {
        if(status & I2C_SR1_RXNE) {
            #if !defined(I2C_DMA_ENABLE)
            if((i2cSettings.lenght - i2cSettings.i2cCountReceived) == 1u) {
                RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
                i2cState = I2C_STATE_INIT;
                I2C1->CR2 &= ~I2C_CR2_ITEVTEN;
                I2C1->CR1 = 0u;
            } else {
                i2cSettings.buff[i2cSettings.i2cCountReceived++] = I2C1->DR;
                if((i2cSettings.lenght - i2cSettings.i2cCountReceived) == 1u) {
                    status = I2C1->CR1 & ~I2C_CR1_ACK;
                    status |= I2C_CR1_STOP;
                    I2C1->CR1 = status;
                }
            }
            #endif
        } else {
            if(i2cSettings.transmitState == I2C_INT_READ_ADDRESS) {
                I2C1->DR = (uint8_t)i2cSettings.address; //                                                     // 3.
            } else if(i2cSettings.transmitState == I2C_INT_READ_ADDRESS_1) {
                I2C1->CR1 |= ((uint32_t) I2C_CR1_START | I2C_CR1_STOP);  // restart                             // 4.
            }
            i2cSettings.transmitState++;
        }
    }
}
