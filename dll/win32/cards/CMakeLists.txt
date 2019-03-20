
# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

spec2def(cards.dll cards.spec)

list(APPEND SOURCE
    cards.c
    cards.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cards.def)

add_library(cards MODULE ${SOURCE})
set_module_type(cards win32dll UNICODE)
add_importlibs(cards user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET cards DESTINATION reactos/system32 FOR all)
