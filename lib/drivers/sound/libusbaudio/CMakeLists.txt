
add_definitions(
    -DUNICODE -D_UNICODE
    -DNDEBUG=1)

list(APPEND SOURCE
    libusbaudio.c
    parser.c
    format.c)

add_library(libusbaudio ${SOURCE})
add_dependencies(libusbaudio bugcodes xdk)
