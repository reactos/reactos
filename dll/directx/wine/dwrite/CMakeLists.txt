
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)

spec2def(dwrite.dll dwrite.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    shapers/arabic_table.c
    shapers/arabic.c
    analyzer.c
    bidi.c
    bracket.c
    direction.c
    font.c
    format.c
    freetype.c
    gdiinterop.c
    layout.c
    linebreak.c
    main.c
    mirror.c
    opentype.c
    reactos.c
    scripts.c
    shape.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwrite_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwrite.def)

add_library(dwrite SHARED ${SOURCE})
set_module_type(dwrite win32dll)
target_link_libraries(dwrite wine uuid oldnames freetype)
add_dependencies(dwrite psdk)
set_module_type(dwrite win32dll ENTRYPOINT 0)
add_importlibs(dwrite shell32 shlwapi oleaut32 ole32 combase user32 user32_vista gdi32 ucrtbase advapi32 advapi32_vista kernel32 kernel32_vista ntdll)
add_cd_file(TARGET dwrite DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(dwrite) # CORE-5743: No ARRAY_SIZE macro
