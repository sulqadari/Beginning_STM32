add_executable(${PROJECT_NAME}.elf
	main.c
	usbcdc.c
	startup_stm32f103xb.s
	${rtos_sources}/tasks.c
	${rtos_sources}/list.c
	${rtos_portable}/port.c
	${rtos_sources}/portable/MemMang/heap_4.c
	${rtos_sources}/queue.c
	${hal_src_usb}/usb.c
	${hal_src_usb}/usb_control.c
	${hal_src_usb}/usb_standard.c
	${hal_src_stm32}/st_usbfs_v1.c
	${hal_src_stm32_cmn}/st_usbfs_core.c
	${hal_src_stm32_f1}/rcc.c
	${hal_src_stm32_cmn}/rcc_common_all.c
	${hal_src_stm32_f1}/gpio.c
	${hal_src_stm32_cmn}/flash_common_all.c
)

target_include_directories(${PROJECT_NAME}.elf
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${PROJECT_SOURCE_DIR}/includes
		${rtos_sources}/include
		${rtos_portable}
		${hal_path}/include
)

add_custom_command(TARGET ${PROJECT_NAME}.elf
	POST_BUILD
	COMMAND ${OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.bin
	BYPRODUCTS USBCDC.bin
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
