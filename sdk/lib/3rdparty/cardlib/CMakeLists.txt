
set_cpp()

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

add_library(cardlib ${SOURCE})

if(NOT MSVC)
    target_compile_options(cardlib PRIVATE "-Wno-unused-but-set-variable")
endif()

add_dependencies(cardlib psdk)
add_pch(cardlib cardlib.h SOURCE)
