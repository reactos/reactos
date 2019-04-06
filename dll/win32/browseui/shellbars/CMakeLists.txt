
PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_ATL_NO_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    CBandSite.cpp
    CBandSiteMenu.cpp
    CBaseBar.cpp
    CISFBand.cpp
    CSHEnumClassesOfCategories.cpp
    shellbars.h)

add_library(shellbars ${SOURCE})

if(NOT MSVC)
    add_target_compile_flags(shellbars "-Wno-unused-but-set-variable")
endif()

add_pch(shellbars shellbars.h SOURCE)
add_dependencies(shellbars xdk)
