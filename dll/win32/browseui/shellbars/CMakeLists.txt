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
    CSHEnumClassesOfCategories.cpp)

add_library(shellbars ${SOURCE})
add_dependencies(shellbars xdk)

target_link_libraries(shellbars atlnew)

if(NOT MSVC)
    add_target_compile_flags(shellbars "-Wno-unused-but-set-variable")
endif()
