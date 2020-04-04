
function(add_d3dx9_target __version)
    set(module d3dx9_${__version})

    spec2def(${module}.dll ${module}.spec ADD_IMPORTLIB)

    list(APPEND SOURCE
        ../d3dx9_36/animation.c
        ../d3dx9_36/core.c
        ../d3dx9_36/effect.c
        ../d3dx9_36/font.c
        ../d3dx9_36/line.c
        ../d3dx9_36/main.c
        ../d3dx9_36/math.c
        ../d3dx9_36/mesh.c
        ../d3dx9_36/preshader.c
        ../d3dx9_36/render.c
        ../d3dx9_36/shader.c
        ../d3dx9_36/skin.c
        ../d3dx9_36/sprite.c
        ../d3dx9_36/surface.c
        ../d3dx9_36/texture.c
        ../d3dx9_36/util.c
        ../d3dx9_36/volume.c
        ../d3dx9_36/xfile.c
        ../d3dx9_36/precomp.h)

    add_library(${module} MODULE
        ${SOURCE}
        ../d3dx9_36/guid.c
        version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${module}_stubs.c
        ${CMAKE_CURRENT_BINARY_DIR}/${module}.def)
    
    add_definitions(-D__ROS_LONG64__)
    set_module_type(${module} win32dll)
    add_dependencies(${module} d3d_idl_headers)
    target_link_libraries(${module} dxguid wine)
    add_importlibs(${module} d3dcompiler_43 d3dxof d3dwine user32 ole32 gdi32 msvcrt kernel32 ntdll)
    add_delay_importlibs(${module} windowscodecs)
    add_pch(${module} ../d3dx9_36/precomp.h SOURCE)
    add_cd_file(TARGET ${module} DESTINATION reactos/system32 FOR all)
    
    target_compile_definitions(${module} PRIVATE -DD3DX_SDK_VERSION=${__version} -D__WINESRC__ -Dcopysignf=_copysignf)
    target_include_directories(${module} PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
endfunction()
