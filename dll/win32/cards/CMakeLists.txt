
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

spec2def(cards ${CMAKE_CURRENT_SOURCE_DIR}/cards.spec ${CMAKE_CURRENT_BINARY_DIR}/cards.def)

add_library(cards SHARED ${CMAKE_CURRENT_SOURCE_DIR}/cards.c ${CMAKE_CURRENT_SOURCE_DIR}/cards.rc)

set_target_properties(cards PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(cards
    ${CMAKE_CURRENT_BINARY_DIR}/cards.def
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    ${REACTOS_SOURCE_DIR}/dll/win32/gdi32/libgdi32.a)

add_dependencies(cards cards_def psdk)
