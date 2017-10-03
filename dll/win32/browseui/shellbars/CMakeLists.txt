PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

add_definitions(-DUNICODE -D_UNICODE)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    CBandSite.cpp
    CBandSiteMenu.cpp
    CBaseBar.cpp
    CSHEnumClassesOfCategories.cpp)

add_library(shellbars ${SOURCE})
add_dependencies(shellbars xdk)

if(NOT MSVC)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(shellbars "-Wno-unused-but-set-variable")
    endif()
endif()
