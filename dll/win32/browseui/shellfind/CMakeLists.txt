
PROJECT(SHELL)

add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    CSearchBar.cpp
    CFindFolder.cpp
    shellfind.h)

add_library(shellfind OBJECT ${SOURCE})
target_link_libraries(shellfind PRIVATE atl_classes)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(shellfind PRIVATE -Wno-unused-but-set-variable)
endif()

add_pch(shellfind shellfind.h SOURCE)
add_dependencies(shellfind psdk)
