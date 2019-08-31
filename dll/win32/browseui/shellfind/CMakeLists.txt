
PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_ATL_NO_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    CSearchBar.cpp
    CFindFolder.cpp
    shellfind.h)

add_library(shellfind ${SOURCE})

if(NOT MSVC)
    add_target_compile_flags(shellfind "-Wno-unused-but-set-variable")
endif()

add_pch(shellfind shellfind.h SOURCE)
add_dependencies(shellfind psdk)
