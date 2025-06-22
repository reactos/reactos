
function(add_d3dx10_target __version)
    set(module d3dx10_${__version})

    spec2def(${module}.dll ${module}.spec ADD_IMPORTLIB)

    list(APPEND SOURCE
        ../d3dx10_43/async.c
        ../d3dx10_43/compiler.c
        ../d3dx10_43/font.c
        ../d3dx10_43/mesh.c
        ../d3dx10_43/sprite.c
        ../d3dx10_43/texture.c)

    list(APPEND PCH_SKIP_SOURCE
        ../d3dx10_43/guid.c
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
  #  add_pch(${module} ../d3dx10_36/precomp.h "${PCH_SKIP_SOURCE}")
    add_cd_file(TARGET ${module} DESTINATION reactos/system32 FOR all)

    target_compile_definitions(${module} PRIVATE D3DX_SDK_VERSION=${__version} __WINESRC__ copysignf=_copysignf)
endfunction()
