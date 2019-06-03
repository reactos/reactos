
add_definitions(
    -D__WINESRC__
    -DDIRECT3D_VERSION=0x0900)

if(MSVC)
    # error C4133: 'function': incompatible types - from 'D3D_CBUFFER_TYPE *' to 'DWORD *'
    replace_compile_flags("/we4133" " ")
endif()

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    asmparser.c
    blob.c
    bytecodewriter.c
    compiler.c
    main.c
    reflection.c
    utils.c
    precomp.h)

add_library(d3dcompiler_43 MODULE
    ${SOURCE}
    asmshader.tab.c
    asmshader.yy.c
    hlsl.tab.c
    hlsl.yy.c
    version.rc)
spec2def(d3dcompiler_43 d3dcompiler_43.dll d3dcompiler_43.spec ADD_IMPORTLIB)

# some files have been generated with relative file paths...
set_source_files_properties(
    asmshader.tab.c
    asmshader.yy.c
    hlsl.tab.c
    hlsl.yy.c
    PROPERTIES COMPILE_FLAGS "-UREACTOS_SOURCE_DIR -DREACTOS_SOURCE_DIR=\"\\\".\\\"\"")

set_module_type(d3dcompiler_43 win32dll)
target_link_libraries(d3dcompiler_43 dx10guid uuid wine wpp)
add_importlibs(d3dcompiler_43 msvcrt kernel32 ntdll)
add_dependencies(d3dcompiler_43 d3d_idl_headers)
add_pch(d3dcompiler_43 precomp.h SOURCE)
add_cd_file(TARGET d3dcompiler_43 DESTINATION reactos/system32 FOR all)
