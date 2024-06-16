#ifndef UART_HAL_H
#define UART_HAL_H

#define MMIO32(addr)			(*(volatile uint32_t *)(addr))


#define RCC_BASE				(PERIPH_BASE_AHB + 0x09000)

#define RCC_CR					MMIO32(RCC_BASE + 0x00)
#define RCC_CR_HSEON			(1 << 16)
#define RCC_CR_HSERDY			(1 << 17)

#define RCC_CFGR				MMIO32(RCC_BASE + 0x04)
#define RCC_CFGR_HPRE_SHIFT		4
#define RCC_CFGR_HPRE			(0xF << RCC_CFGR_HPRE_SHIFT)
#define RCC_CFGR_HPRE_NODIV		0x0
#define RCC_CFGR_PPRE1_SHIFT	8
#define RCC_CFGR_PPRE1			(7 << RCC_CFGR_PPRE1_SHIFT)
#define RCC_CFGR_PPRE_DIV2		0x4
#define RCC_CFGR_PPRE2_SHIFT	11
#define RCC_CFGR_PPRE2			(7 << RCC_CFGR_PPRE2_SHIFT)
#define RCC_CFGR_PPRE_NODIV		0x0
#define RCC_CFGR_ADCPRE_SHIFT	14
#define RCC_CFGR_ADCPRE			(3 << RCC_CFGR_ADCPRE_SHIFT)
#define RCC_CFGR_ADCPRE_DIV8	0x3
#define RCC_CFGR_USBPRE			(1 << 22)

#define RCC_CFGR_PLLMUL_PLL_CLK_MUL9	0x7
#define RCC_CFGR_PLLMUL_SHIFT			18
#define RCC_CFGR_PLLMUL					(0xF << RCC_CFGR_PLLMUL_SHIFT)
#define RCC_CFGR_PLLSRC					(1 << 16)
#define RCC_CFGR_PLLSRC_HSE_CLK			0x1

#define PERIPH_BASE						(0x40000000U)
#define PERIPH_BASE_AHB					(PERIPH_BASE + 0x18000)


#define FLASH_MEM_INTERFACE_BASE		(PERIPH_BASE_AHB + 0x0a000)
#define FLASH_ACR						MMIO32(FLASH_MEM_INTERFACE_BASE + 0x00)
#define FLASH_ACR_LATENCY_SHIFT			0
#define FLASH_ACR_LATENCY_MASK			7

#endif // !UART_HAL_H