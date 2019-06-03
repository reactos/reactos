
# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

list(APPEND SOURCE
    cards.c
    cards.rc)

add_library(cards MODULE ${SOURCE})
spec2def(cards cards.dll cards.spec)

set_module_type(cards win32dll UNICODE)
add_importlibs(cards user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET cards DESTINATION reactos/system32 FOR all)
