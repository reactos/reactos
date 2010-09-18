


add_definitions(-DUNICODE -D_UNICODE)

# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

spec2def(cards ${CMAKE_CURRENT_SOURCE_DIR}/cards.spec ${CMAKE_CURRENT_BINARY_DIR}/cards.def)

add_library(cards SHARED ${CMAKE_CURRENT_SOURCE_DIR}/cards.c ${CMAKE_CURRENT_SOURCE_DIR}/cards.rc)

set_target_properties(cards PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(cards
    ${CMAKE_CURRENT_BINARY_DIR}/cards.def
    -luser32
    -lgdi32)

add_dependencies(cards cards_def psdk)
