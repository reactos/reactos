

list(APPEND SOURCE
    cardbitmaps.cpp
    cardbutton.cpp
    cardcolor.cpp
    cardcount.cpp
    cardlib.cpp
    cardregion.cpp
    cardrgndraw.cpp
    cardrgnmouse.cpp
    cardstack.cpp
    cardwindow.cpp
    dropzone.cpp
    cardlib.h)

add_library(cardlib STATIC ${SOURCE})
target_link_libraries(cardlib PRIVATE cpprt)
target_include_directories(cardlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cardlib PRIVATE -Wno-unused-but-set-variable)
endif()

add_dependencies(cardlib psdk)
add_pch(cardlib cardlib.h SOURCE)
