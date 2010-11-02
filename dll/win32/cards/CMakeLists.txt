
set_unicode()

# Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN
add_definitions(-DCARDSTYLE_DEFAULT)

add_library(cards SHARED cards.c cards.rc cards.def)

if(NOT MSVC)
    set_source_files_properties(cards.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(cards win32dll)

add_importlibs(cards user32 gdi32)

add_cab_target(cards 1)
