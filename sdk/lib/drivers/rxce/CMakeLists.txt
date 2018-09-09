# Starting with NT6, RXCE got totally rewritten to change the way it works.
# Re-definition useful only when forcing ReactOS to build as NT6.
remove_definitions(-D_WIN32_WINNT=0x600)
add_definitions(-D_WIN32_WINNT=0x502)

add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup)

list(APPEND SOURCE
    rxce.c)

add_library(rxce ${SOURCE})
add_dependencies(rxce bugcodes xdk)
