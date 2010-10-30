
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/msacm32.drv.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/msacm32.drv.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/msacm32.drv.spec --filename msacm32.drv
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/msacm32.drv.def PROPERTIES GENERATED TRUE)
add_custom_target(msacm32.drv_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msacm32.drv.def)

add_library(msacm32.drv SHARED wavemap.c wavemap.rc)

set_entrypoint(msacm32.drv 0)
set_target_properties(msacm32.drv PROPERTIES SUFFIX "")

target_link_libraries(msacm32.drv
    ${CMAKE_CURRENT_BINARY_DIR}/msacm32.drv.def
    wine)

add_importlibs(msacm32.drv msacm32 user32 winmm advapi32 kernel32 ntdll)
add_dependencies(msacm32.drv psdk buildno_header)
add_cab_target(msacm32.drv 1)