add_definitions(-DUNICODE -D_UNICODE -D__NTOSKRNL__ -D_NTOSKRNL_ -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup)

list(APPEND SOURCE
    rdbss.c)

add_library(rdbsslib ${SOURCE})
target_link_libraries(rdbsslib rxce)
add_dependencies(rdbsslib rxce bugcodes xdk)
