add_executable(${PROJECT_NAME}.elf
	main.c
	startup_stm32f103xb.s
	${rtos_sources}/tasks.c
	${rtos_sources}/list.c
	${rtos_portable}/port.c
	${rtos_sources}/portable/MemMang/heap_4.c
	${rtos_sources}/queue.c
	${hal_src_stm32_cmn}/usart_common_all.c
	${hal_src_stm32_cmn}/usart_common_f124.c
	${hal_src_stm32_f1}/rcc.c
	${hal_src_stm32_cmn}/rcc_common_all.c
	${hal_src_stm32_f1}/gpio.c
	${hal_src_stm32_cmn}/gpio_common_all.c
	${hal_src_stm32_cmn}/flash_common_all.c

	# ${PROJECT_SOURCE_DIR}/../BookSources/rtos/libwwg/include/uartlib.h
	# ${PROJECT_SOURCE_DIR}/../BookSources/rtos/libwwg/include/miniprintf.h
	# ${PROJECT_SOURCE_DIR}/../BookSources/rtos/libwwg/include/getline.h

	# ${PROJECT_SOURCE_DIR}/../BookSources/rtos/libwwg/src/uartlib.c
	# ${PROJECT_SOURCE_DIR}/../BookSources/rtos/libwwg/src/miniprintf.c
	# ${PROJECT_SOURCE_DIR}/../BookSources/rtos/libwwg/src/getline.c
)

target_include_directories(${PROJECT_NAME}.elf
	PUBLIC
		${PROJECT_SOURCE_DIR}/includes
		${PROJECT_SOURCE_DIR}/../BookSources/rtos/libwwg/include
		${rtos_sources}/include
		${rtos_portable}
		${hal_path}/include
		${hal_path}/include/libopencm3/stm32/common
		${hal_path}/include/libopencm3/stm32/f1
)

add_custom_command(TARGET ${PROJECT_NAME}.elf
	POST_BUILD
	COMMAND ${OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.bin
	BYPRODUCTS SeOS.bin
)

add_custom_command(TARGET ${PROJECT_NAME}.elf
	POST_BUILD
	COMMAND ${OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
	BYPRODUCTS ${PROJECT_NAME}.hex
)

add_custom_command(TARGET ${PROJECT_NAME}.elf
	POST_BUILD
	COMMAND ${SIZE} ${PROJECT_NAME}.elf
)
