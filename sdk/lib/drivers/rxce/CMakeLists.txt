add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup)

list(APPEND SOURCE
    rxce.c)

add_library(rxce ${SOURCE})
set_ntddi_defines(rxce win2003)
target_link_libraries(rxce PRIVATE pseh)
add_dependencies(rxce bugcodes xdk)
