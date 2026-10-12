# Implements the WS03 RDBSS interface until it is ported to the Vista one
remove_definitions(-D_WIN32_WINNT=0x600 -DNTDDI_VERSION=0x06000200)
add_definitions(-D_WIN32_WINNT=0x502 -DNTDDI_VERSION=0x05020400)

add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup
                    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/copysup)

list(APPEND SOURCE
    rdbss.c)

add_library(rdbsslib ${SOURCE})
target_link_libraries(rdbsslib PRIVATE pseh)
add_dependencies(rdbsslib bugcodes xdk)
