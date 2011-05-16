
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
    dropzone.cpp)

add_library(cardlib ${SOURCE})
add_dependencies(cardlib psdk)
