#include <stm32f10x.h>
#include "adc_test.h"
#include "i2c_test.h"
#include "system_stm32f10x.h"

static uint8_t buff[10u] = {0};

int main(void) {
    bool init = false;
    uint8_t count;
    uint32_t reg;
    
    #ifdef VECT_TAB_SRAM
      SCB->VTOR = SRAM_BASE;// | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM. */
    #else
      SCB->VTOR = FLASH_BASE;// | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH. */
    #endif 
    SystemInit();

    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    reg = GPIOC->CRH;
    reg &= ~((uint32_t)GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    reg |= GPIO_CRH_MODE13; // 11: Output mode, max speed 50 MHz
    GPIOC->CRH = reg;
    for(;;) {
        // bit banding pre PC13 ODR registra je adresa 0x422201B4 = 0x42000000 + (0x1100c*32)+(0xD*4)
        // adresa bitu BS13 pre PORTC je: 0x42220234 = 0x42000000 + (0x11010*32)+(13*4)
        // adresa bitu BR13 pre PORTC je: 0x42220274 = 0x42000000 + (0x11010*32)+(29*4)
        GPIOC->BSRR = (GPIOC->ODR & GPIO_ODR_ODR13) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
        adc_handleTask();
        i2c_handleTask();
        if(init == false) {
            init = true;
            count = 10u;
            while(count) {
                if(i2c_read(0xA0u, 0u, buff, 2u)) {//sizeof(buff))) {
                    count--;
                }
            }
        }
    }
    return 0;
}
/*
int main(void) {
    for(;;) {
        
    }
    return 0;
}*/
