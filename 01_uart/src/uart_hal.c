#include <stdint.h>
#include "configs.h"

void
set_72MHz_clock(void)
{
	RCC_CR |= RCC_CR_HSEON;

	// wait for oscillator becomes ready
	while (!(RCC_CR & RCC_CR_HSERDY)) { volatile int32_t i = 0; i++; }

	// Set the AHB prescale factor
	RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_HPRE) | (RCC_CFGR_HPRE_NODIV << RCC_CFGR_HPRE_SHIFT);

	// The APB1 clock frequency must not exceed 36MHz.
	RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PPRE1) | (RCC_CFGR_PPRE_DIV2 << RCC_CFGR_PPRE1_SHIFT);

	// RCC Set the APB2 Prescale Factor.
	RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PPRE2) | (RCC_CFGR_PPRE_NODIV << RCC_CFGR_PPRE2_SHIFT);

	// The ADC's have a common clock prescale setting.
	RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE) | (RCC_CFGR_ADCPRE_DIV8 << RCC_CFGR_ADCPRE_SHIFT);

	// RCC Set the USB Prescale Factor.
	RCC_CFGR &= ~RCC_CFGR_USBPRE;

	// Set wait state for FLASH
	{
		uint32_t reg32;

		reg32 = FLASH_ACR;
		reg32 &= ~(FLASH_ACR_LATENCY_MASK << FLASH_ACR_LATENCY_SHIFT);
		reg32 |= (2 << FLASH_ACR_LATENCY_SHIFT);
		FLASH_ACR = reg32;
	}

	// RCC Set the PLL Multiplication Factor.
	RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PLLMUL) |
			(RCC_CFGR_PLLMUL_PLL_CLK_MUL9 << RCC_CFGR_PLLMUL_SHIFT);
	
	// RCC Set the PLL Clock Source.
	RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PLLSRC) | (RCC_CFGR_PLLSRC_HSE_CLK << 16);
}