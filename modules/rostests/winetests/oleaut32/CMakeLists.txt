
add_definitions(
    -D__WINESRC__
    -DUSE_WINE_TODOS
    -DWINETEST_USE_DBGSTR_LONGLONG
    -D_USE_MATH_DEFINES
    -Dstrcasecmp=_stricmp
)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_typelib(test_reg.idl test_tlb.idl tmarshal.idl)
add_idl_Headers(oleaut32_idlheaders test_reg.idl test_tlb.idl tmarshal.idl)
generate_idl_iids(test_reg.idl test_tlb.idl tmarshal.idl)

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
    ${CMAKE_CURRENT_BINARY_DIR}/tmarshal_i.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

set(tmarshal.rc_DEPS
    ${CMAKE_CURRENT_BINARY_DIR}/test_reg.tlb
    ${CMAKE_CURRENT_BINARY_DIR}/test_tlb.tlb
    ${CMAKE_CURRENT_BINARY_DIR}/tmarshal.tlb)

set_source_files_properties(tmarshal.rc PROPERTIES OBJECT_DEPENDS "${tmarshal.rc_DEPS}")

add_executable(oleaut32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    tmarshal.rc)

target_link_libraries(oleaut32_winetest uuid)
set_module_type(oleaut32_winetest win32cui)
add_importlibs(oleaut32_winetest oleaut32 ole32 rpcrt4 user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(oleaut32_winetest ntdll)
endif()

add_dependencies(oleaut32_winetest stdole2 oleaut32_idlheaders)
add_pch(oleaut32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET oleaut32_winetest)
