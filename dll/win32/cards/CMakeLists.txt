
set_unicode()

# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

spec2def(cards.dll cards.spec)

add_library(cards SHARED cards.c cards.rc ${CMAKE_CURRENT_BINARY_DIR}/cards.def)

set_module_type(cards win32dll)

add_importlibs(cards user32 gdi32)

add_cab_target(cards 1)
