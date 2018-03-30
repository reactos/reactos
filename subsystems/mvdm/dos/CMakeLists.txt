
include_directories(${REACTOS_SOURCE_DIR}/subsystems/mvdm/dos)
add_asm16_bin(command ${CMAKE_CURRENT_BINARY_DIR}/command.com 0x0100 command.S)
add_cd_file(TARGET command FILE $<TARGET_PROPERTY:command,BINARY_PATH> DESTINATION reactos/system32 FOR all)
