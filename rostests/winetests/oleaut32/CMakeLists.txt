
add_definitions(-D__ROS_LONG64__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_typelib(test_reg.idl test_tlb.idl tmarshal.idl)
add_idl_Headers(oleaut32_idlheaders test_reg.idl tmarshal.idl)
generate_idl_iids(tmarshal.idl test_reg.idl)

list(APPEND SOURCE
    dispatch.c
    olefont.c
    olepicture.c
    safearray.c
    testlist.c
    tmarshal.c
    typelib.c
    usrmarshal.c
    varformat.c
    vartest.c
    vartype.c
    ${CMAKE_CURRENT_BINARY_DIR}/test_reg_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/tmarshal_i.c)

add_executable(oleaut32_winetest ${SOURCE} tmarshal.rc)
target_link_libraries(oleaut32_winetest uuid wine)
set_module_type(oleaut32_winetest win32cui)
add_importlibs(oleaut32_winetest oleaut32 ole32 rpcrt4 user32 gdi32 advapi32 msvcrt kernel32 ntdll)

SET(tmarshal.rc_DEPS
${REACTOS_BINARY_DIR}/include/psdk/stdole2.tlb
${CMAKE_CURRENT_BINARY_DIR}/test_reg.tlb
${CMAKE_CURRENT_BINARY_DIR}/test_tlb.tlb
${CMAKE_CURRENT_BINARY_DIR}/tmarshal.tlb)
set_source_files_properties(tmarshal.rc PROPERTIES OBJECT_DEPENDS "${tmarshal.rc_DEPS}")

add_dependencies(oleaut32_winetest stdole2 oleaut32_typelibs oleaut32_idlheaders )
add_cd_file(TARGET oleaut32_winetest DESTINATION reactos/bin FOR all)
