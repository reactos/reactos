
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_typelib(oleaut32_typelibs test_reg.idl test_tlb.idl tmarshal.idl)
add_dependencies(oleaut32_typelibs stdole2)

add_idl_Headers(oleaut32_idlheaders test_reg.idl tmarshal.idl)
generate_idl_iids(tmarshal.idl)

list(APPEND SOURCE
    dispatch.c
    olefont.c
    olepicture.c
    safearray.c
    testlist.c
    tmarshal.c
    tmarshal.rc
    typelib.c
    usrmarshal.c
    varformat.c
    vartest.c
    vartype.c
    ${CMAKE_CURRENT_BINARY_DIR}/tmarshal_i.c)

add_executable(oleaut32_winetest ${SOURCE})
target_link_libraries(oleaut32_winetest uuid wine)
set_module_type(oleaut32_winetest win32cui)
add_importlibs(oleaut32_winetest oleaut32 ole32 rpcrt4 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(oleaut32_winetest oleaut32_typelibs oleaut32_idlheaders)
add_cd_file(TARGET oleaut32_winetest DESTINATION reactos/bin FOR all)
