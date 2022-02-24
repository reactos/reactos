
add_definitions(-DUSER_MODE)
include_directories(${REACTOS_SOURCE_DIR}/drivers/storage/ide/uniata)
add_executable(atactl atactl.cpp atactl.rc)
set_module_type(atactl win32cui)
add_importlibs(atactl advapi32 msvcrt kernel32 ntdll)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 9))
    target_compile_options(atactl PRIVATE -Wno-format)
endif()

add_cd_file(TARGET atactl DESTINATION reactos/system32 FOR all)
