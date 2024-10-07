
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
        ../d3dx9_36/txc_compress_dxtn.c
        ../d3dx9_36/txc_fetch_dxtn.c
        ../d3dx9_36/util.c
        ../d3dx9_36/volume.c
        ../d3dx9_36/xfile.c)

    list(APPEND PCH_SKIP_SOURCE
        ../d3dx9_36/guid.c
        ${CMAKE_CURRENT_BINARY_DIR}/${module}_stubs.c)

    add_library(${module} MODULE
        ${SOURCE}
        ${PCH_SKIP_SOURCE}
        version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${module}.def)

    add_definitions(-D__ROS_LONG64__)
    set_module_type(${module} win32dll)
    add_dependencies(${module} d3d_idl_headers)
    target_link_libraries(${module} dxguid wine oldnames)
    add_importlibs(${module} d3dcompiler_43 d3dxof usp10 user32 ole32 gdi32 msvcrt kernel32 ntdll)
    add_delay_importlibs(${module} windowscodecs)
    add_pch(${module} ../d3dx9_36/precomp.h "${PCH_SKIP_SOURCE}")
    add_cd_file(TARGET ${module} DESTINATION reactos/system32 FOR all)

    target_compile_definitions(${module} PRIVATE D3DX_SDK_VERSION=${__version} __WINESRC__ copysignf=_copysignf)
endfunction()
