cmake_minimum_required(VERSION 3.25)
project(USBCDC VERSION 1.0)


enable_language(ASM C)
set(CMAKE_C_COMPILER	"arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER	"arm-none-eabi-gcc")
set(CMAKE_LINKER		"arm-none-eabi-ld")
set(OBJCOPY				"arm-none-eabi-objcopy")
set(OBJDUMP				"arm-none-eabi-objdump")
set(SIZE				"arm-none-eabi-size")

# The paths to the libopencm3 files
set(hal_path			${CMAKE_SOURCE_DIR}/../libopencm3)
set(hal_src_stm32		${hal_path}/lib/stm32)
set(hal_src_stm32_cmn 	${hal_src_stm32}/common)
set(hal_src_stm32_f1 	${hal_src_stm32}/f1)
set(hal_src_usb			${hal_path}/lib/usb)

#The paths to the FreeRTOS
set(rtos_path		${CMAKE_SOURCE_DIR}/../FreeRTOS/FreeRTOS)
set(rtos_sources	${rtos_path}/Source)
set(rtos_portable	${rtos_sources}/portable/GCC/ARM_CM3)
set(rtos_meman		${rtos_sources}/portable/MemMang)

add_compile_definitions(
	STM32F103xB STM32F1
)

#-mapcs-frame -msoft-float
# -fno-hosted -fno-builtin
add_compile_options(
	-Wall -Os -std=gnu17
	-mcpu=cortex-m3
	# -mthumb
	# -mtune=cortex-m3 
	-fdata-sections -ffunction-sections
	# -specs=nosys.specs
	# -specs=nano.specs
	# -mapcs-frame
	# -msoft-float
	# -fno-hosted
	# -fno-builtin
)

add_link_options(
	-mcpu=cortex-m3
	-mthumb
	"LINKER:--gc-sections"	# used in conjunction with '-fdata-sections -ffunction-sections'
	"LINKER:-T,${CMAKE_SOURCE_DIR}/src/STM32F103XB_FLASH.ld"
	"LINKER:-Map=map.log"
)


add_subdirectory(src)