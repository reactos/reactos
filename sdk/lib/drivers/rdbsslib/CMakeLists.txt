add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup
                    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/copysup)

list(APPEND SOURCE
    rdbss.c)

add_library(rdbsslib ${SOURCE})
set_ntddi_defines(rdbsslib win2003)
target_link_libraries(rdbsslib PRIVATE pseh)
add_dependencies(rdbsslib bugcodes xdk)
