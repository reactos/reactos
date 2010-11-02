
add_definitions(
    -D_DLL -D__USE_CRTIMP
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(msgsm32.acm SHARED msgsm32.c msgsm32.def)

if(NOT MSVC)
    set_source_files_properties(msgsm32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(msgsm32.acm 0)
set_target_properties(msgsm32.acm PROPERTIES SUFFIX "")

target_link_libraries(msgsm32.acm wine)

add_importlibs(msgsm32.acm winmm user32 msvcrt kernel32 ntdll)
add_dependencies(msgsm32.acm psdk buildno_header)
add_cab_target(msgsm32.acm 1)
