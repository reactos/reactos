
function(add_d3dx9_target VERSION)
    set(_target d3dx9_${VERSION})
    set(_srcdir ${CMAKE_CURRENT_SOURCE_DIR}/../d3dx9_36)

    spec2def(${_target}.dll ${_target}.spec ADD_IMPORTLIB)

    set(_source
        ${_srcdir}/animation.c
        ${_srcdir}/core.c
        ${_srcdir}/effect.c
        ${_srcdir}/font.c
        ${_srcdir}/line.c
        ${_srcdir}/main.c
        ${_srcdir}/math.c
        ${_srcdir}/mesh.c
        ${_srcdir}/preshader.c
        ${_srcdir}/render.c
        ${_srcdir}/shader.c
        ${_srcdir}/skin.c
        ${_srcdir}/sprite.c
        ${_srcdir}/surface.c
        ${_srcdir}/texture.c
        ${_srcdir}/txc_compress_dxtn.c
        ${_srcdir}/txc_fetch_dxtn.c
        ${_srcdir}/util.c
        ${_srcdir}/volume.c
        ${_srcdir}/xfile.c)

    add_library(${_target} MODULE
        ${_source}
        version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}_stubs.c
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}.def)

    target_compile_definitions(${_target} PRIVATE
        __WINESRC__
        __ROS_LONG64__
        # math.c wants M_PI, which <math.h> only exposes on request here.
        _USE_MATH_DEFINES
        D3DX_SDK_VERSION=${VERSION})

    target_include_directories(${_target} BEFORE PRIVATE
        ${REACTOS_SOURCE_DIR}/sdk/include/wine
        ${REACTOS_BINARY_DIR}/sdk/include/wine
        ${_srcdir}
        ${CMAKE_CURRENT_BINARY_DIR})

    if(MSVC)
        # effect.c's SET_D3D_STATE forwards __VA_ARGS__ from one variadic macro
        # into another.
        target_compile_options(${_target} PRIVATE /Zc:preprocessor)
    endif()

    set_module_type(${_target} win32dll)
    target_link_libraries(${_target} wine dxguid uuid oldnames)
    add_importlibs(${_target}
        d3d9 d3dcompiler_47 d3dxof ole32 gdi32 user32 usp10
        kernel32_vista msvcrt kernel32 ntdll)
    add_delay_importlibs(${_target} windowscodecs)
    add_dependencies(${_target} wineheaders d3d_idl_headers)
    add_cd_file(TARGET ${_target} DESTINATION reactos/system32 FOR all)
endfunction()
