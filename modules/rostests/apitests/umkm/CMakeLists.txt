
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND SOURCE
    SystemCall.c
    precomp.h)

if(ARCH STREQUAL "i386")
    add_asm_files(umkm_apitest_asm
        i386/SystemCall_asm.s
    )
elseif(ARCH STREQUAL "amd64")
    add_asm_files(umkm_apitest_asm
        amd64/SystemCall_asm.s
    )
endif()

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(umkm_apitest
    ${SOURCE}
    ${umkm_apitest_asm}
    ${PCH_SKIP_SOURCE})

target_link_libraries(umkm_apitest wine uuid ${PSEH_LIB})
set_module_type(umkm_apitest win32cui)
add_importlibs(umkm_apitest msvcrt kernel32 ntdll)
add_pch(umkm_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_dependencies(umkm_apitest load_notifications)

add_rostests_file(TARGET umkm_apitest)

