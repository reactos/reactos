
set_unicode()

# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

add_library(cards SHARED cards.c cards.rc)

set_pdef_file(cards cards.pdef)
add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/cards.def)

set_module_type(cards win32dll)

add_importlibs(cards user32 gdi32)

add_cab_target(cards 1)
