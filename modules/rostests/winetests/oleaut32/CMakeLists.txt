
add_definitions(
    -D__WINESRC__
    -DWINETEST_USE_DBGSTR_LONGLONG
    -D_USE_MATH_DEFINES
)

add_idl_Headers(oleaut32_idlheaders test_reg.idl test_tlb.idl tmarshal.idl test_simple.idl)
generate_idl_iids(test_reg.idl test_tlb.idl tmarshal.idl test_simple.idl)

list(APPEND SOURCE
    dispatch.c
    olefont.c
    olepicture.c
    safearray.c
    tmarshal.c
    typelib.c
    usrmarshal.c
    varformat.c
    vartest.c
    vartype.c
    ${CMAKE_CURRENT_BINARY_DIR}/test_reg_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/test_tlb_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/tmarshal_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/test_simple_i.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(oleaut32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    tmarshal.rc)

add_idl_reg_scripts(oleaut32_winetest regtypelib test_reg.idl test_tlb.idl tmarshal.idl test_simple.idl)
target_link_libraries(oleaut32_winetest uuid)
set_module_type(oleaut32_winetest win32cui)
add_importlibs(oleaut32_winetest oleaut32 ole32 rpcrt4 user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(oleaut32_winetest ntdll)
    # warning C4477: 'sprintf' : format string '%u' requires an argument of type 'unsigned int', but variadic argument 2 has type 'const char *'
    target_compile_options(oleaut32_winetest PRIVATE /wd4477)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(oleaut32_winetest PRIVATE -Wno-discarded-qualifiers)
endif()

add_dependencies(oleaut32_winetest stdole2 oleaut32_idlheaders)
add_pch(oleaut32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET oleaut32_winetest)
