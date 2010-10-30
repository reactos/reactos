
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(intl ${CMAKE_CURRENT_SOURCE_DIR}/intl.spec ${CMAKE_CURRENT_BINARY_DIR}/intl.def)

add_library(intl SHARED
    currency.c
    date.c
    generalp.c
    intl.c
    inplocale.c
    numbers.c
    time.c
    misc.c
    kblayouts.c
    languages.c
    advanced.c
    sort.c
    intl.rc)

set_module_type(intl cpl)

target_link_libraries(intl ${CMAKE_CURRENT_BINARY_DIR}/intl.def)

add_importlibs(intl
    msvcrt
    user32
    comctl32
    advapi32
    setupapi
    shell32
    kernel32)

add_dependencies(intl intl_def)
add_cab_target(intl 1)
