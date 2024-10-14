

list(APPEND SOURCE
    RtlIntSafe.c
)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        i386/RtlCaptureContext.c
        i386/RtlUnwind.c
    )
    list(APPEND ASM_SOURCE
        i386/RtlUnwind-asm.s
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        amd64/RtlCaptureContext-asm.s
    )
    list(APPEND SOURCE
        amd64/RtlCaptureContext.c
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE
        # To be filled
    )
endif()

add_asm_files(rtl_test_asm ${ASM_SOURCE})

add_library(rtl_test_lib
    ${SOURCE}
    ${rtl_test_asm}
)

target_compile_definitions(rtl_test_lib PRIVATE _RTL_TEST _NTSYSTEM_)

add_dependencies(rtl_test_lib asm)

if(NOT MSVC)
    set_source_files_properties(RtlGetFullPathName_UstrEx.c PROPERTIES COMPILE_OPTIONS "-Wno-format")
endif()

# RTL tests with static linkage (called unittest, so it won't run in rosautotest)
add_executable(rtl_unittest
    testlist.c
    ldrstubs.c)
target_compile_definitions(rtl_unittest PRIVATE _RTL_TEST _NTSYSTEM_)
target_link_libraries(rtl_unittest rtl_test_lib rtl rtl_um rtl_vista wine uuid ${PSEH_LIB})
set_module_type(rtl_unittest win32cui)
add_importlibs(rtl_unittest msvcrt advapi32 kernel32 ntdll)
target_compile_definitions(rtl_unittest PRIVATE KMT_USER_MODE NTDDI_VERSION=NTDDI_WS03SP1)

add_rostests_file(TARGET rtl_unittest)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(rtl_unittest PRIVATE -Wno-format)
endif()
