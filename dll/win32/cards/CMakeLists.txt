
set_unicode()

# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

spec2def(cards ${CMAKE_CURRENT_SOURCE_DIR}/cards.spec ${CMAKE_CURRENT_BINARY_DIR}/cards.def)

add_library(cards SHARED cards.c cards.rc)

set_module_type(cards win32dll)

target_link_libraries(cards ${CMAKE_CURRENT_BINARY_DIR}/cards.def)

add_importlibs(cards user32 gdi32)
add_dependencies(cards cards_def)
add_cab_target(cards 1)
