#include "adc_test.h"
#include "stm32f10x.h"

/*
PC15 0V - OUTPUT
PA0 296.4mV
PA1 594.4
PA2 891.5
PA3 1.189V
PA4 1.486
PA5 1.783
PA6 2.079
PA7 2.377
PB0 2.676
PB1 2.974
PB10 3.272V - Vdd
*/

enum adcState_t {
    ADC_STATE_NOT_INIT,
    ADC_STATE_INIT
};

static enum adcState_t adcState = ADC_STATE_NOT_INIT;

void adc_init(void) {
    uint32_t reg;

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->APB2ENR |= ((uint32_t)RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN);
    reg = GPIOC->CRH;
    reg &= ~((uint32_t)GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
    reg |= ((uint32_t)GPIO_CRH_MODE15_1 | GPIO_CRH_CNF15_0); // 10: Output mode, max speed 2 MHz; 01: General purpose output Open-drain
    GPIOC->CRH = reg;
    GPIOC->BSRR = GPIO_BSRR_BR15;

    // Analog pins PA0 - PA7, PB0, PB1
    GPIOA->CRL = 0u;            // Analog mode for PORTA0 - PORTA7
    GPIOB->CRL &= ~((uint32_t)GPIO_CRL_MODE0 | GPIO_CRL_MODE1 | GPIO_CRL_CNF0 | GPIO_CRL_CNF1); // Analog mode for PORTB0 and PORTB1

    // PB10 - VDD
    reg = GPIOB->CRH;
    reg &= ~((uint32_t)GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
    reg |= GPIO_CRH_MODE10_1;   // 10: Output mode, max speed 2 MHz; 00: General purpose output push-pull
    GPIOB->CRH = reg;
    GPIOB->BSRR = GPIO_BSRR_BS10; 
}

void adc_handleTask(void) {
    switch(adcState) {
        case ADC_STATE_INIT:
            break;
        case ADC_STATE_NOT_INIT:
            adc_init();
            adcState = ADC_STATE_INIT;
            break;
        default:
            break;
    }
}
