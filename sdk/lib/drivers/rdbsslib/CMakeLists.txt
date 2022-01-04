add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup
                    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/copysup)

list(APPEND SOURCE
    rdbss.c)

add_library(rdbsslib ${SOURCE})
target_link_libraries(rdbsslib PRIVATE pseh)
add_dependencies(rdbsslib bugcodes xdk)
