
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_definitions(
    -D__ROS_LONG64__
    -D_USE_MATH_DEFINES
    -Dwcsicmp=_wcsicmp
    -Dfmin=min
    -Dfmax=max
    -Dfminf=min
    -Dfmaxf=max
)

spec2def(gdiplus.dll gdiplus.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    brush.c
    customlinecap.c
    font.c
    gdiplus.c
    graphics.c
    graphicspath.c
    image.c
    imageattributes.c
    matrix.c
    metafile.c
    pathiterator.c
    pen.c
    region.c
    stringformat.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/gdiplus_stubs.c)

add_library(gdiplus MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    gdiplus.rc
    ${CMAKE_CURRENT_BINARY_DIR}/gdiplus.def)

if(MSVC)
    target_compile_options(gdiplus PRIVATE
        /wd4146 # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
        /wd4267 # Disable warning C4267: 'return': conversion from 'size_t' to 'UINT', possible loss of data
        /wd4305 # Disable warning C4305: '=': truncation from 'double' to 'REAL'
        /wd4334 # Disable warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    )
else()
    target_compile_options(gdiplus PRIVATE -Wno-incompatible-pointer-types)
endif()

set_module_type(gdiplus win32dll)
target_link_libraries(gdiplus wine)
add_delay_importlibs(gdiplus windowscodecs)
add_importlibs(gdiplus mlang msvcrt shlwapi oleaut32 ole32 user32 gdi32 kernel32 kernel32_vista ntdll ntdll_vista)

add_cd_file(TARGET gdiplus DESTINATION reactos/system32 FOR all)

add_cd_file(TARGET gdiplus DESTINATION reactos/winsxs/${WINARCH}_microsoft.windows.gdiplus_6595b64144ccf1df_1.1.7601.23038_none_deadbeef FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/${WINARCH}_microsoft.windows.gdiplus_6595b64144ccf1df_1.1.7601.23038_none_deadbeef.manifest DESTINATION reactos/winsxs/manifests FOR all)

add_cd_file(TARGET gdiplus DESTINATION reactos/winsxs/${WINARCH}_microsoft.windows.gdiplus_6595b64144ccf1df_1.0.14393.0_none_deadbeef FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/${WINARCH}_microsoft.windows.gdiplus_6595b64144ccf1df_1.0.14393.0_none_deadbeef.manifest DESTINATION reactos/winsxs/manifests FOR all)
set_wine_module_FIXME(gdiplus) # CORE-5743: No ARRAY_SIZE macro
