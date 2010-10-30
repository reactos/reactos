
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/msgsm32.acm.spec --filename msgsm32.acm
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def PROPERTIES GENERATED TRUE)
add_custom_target(msgsm32.acm_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def)

add_library(msgsm32.acm SHARED msgsm32.c)

set_entrypoint(msgsm32.acm 0)
set_target_properties(msgsm32.acm PROPERTIES SUFFIX "")

target_link_libraries(msgsm32.acm
    ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def
    wine)

add_importlibs(msgsm32.acm winmm user32 msvcrt kernel32 ntdll)
add_dependencies(msgsm32.acm psdk buildno_header)
add_cab_target(msgsm32.acm 1)
