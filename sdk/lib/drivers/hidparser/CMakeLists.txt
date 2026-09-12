
add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    context.c
    hidparser.c
    parser.c
    api.c
    parser.h
    hidp.c
    hidp.h)

add_library(hidparser_um STATIC ${SOURCE})
add_importlibs(hidparser_um ntdll)
add_dependencies(hidparser_um bugcodes xdk)
target_include_directories(hidparser_um INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hidparser_km STATIC ${SOURCE})
add_importlibs(hidparser_km ntoskrnl)
add_dependencies(hidparser_km bugcodes xdk)
target_include_directories(hidparser_km INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
