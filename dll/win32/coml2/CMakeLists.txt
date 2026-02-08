
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/wine)
spec2def(coml2.dll coml2.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    memlockbytes.c
    stg_prop.c
    storage32.c
    ${CMAKE_CURRENT_BINARY_DIR}/coml2_stubs.c
)

add_library(coml2 MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/coml2.def)
set_module_type(coml2 win32dll)
if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    # Disable warning C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
    target_compile_options(coml2 PRIVATE /wd4090 /wd4312)
endif()
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(coml2 PRIVATE -Wno-discarded-qualifiers)
endif()
target_link_libraries(coml2 wine uuid pseh)
add_importlibs(coml2 advapi32 ole32 oleaut32 user32 gdi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET coml2 DESTINATION reactos/system32 FOR all)
set_wine_module(coml2)
