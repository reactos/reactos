add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup)

list(APPEND SOURCE
    rxce.c)

add_library(rxce ${SOURCE})
if(NOT MSVC)
target_link_libraries(rxce ntoskrnl memcmp)
else()
target_link_libraries(rxce memcmp)
endif()
add_dependencies(rxce bugcodes xdk)
