add_definitions(-DUNICODE -D_UNICODE -DRDBSS_TRACKER)

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/mup
                    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/copysup)

if(MSVC)
    # error C4101: 'BreakpointsSave' : unreferenced local variable in Release-Configuration
    replace_compile_flags("/we4101" " ")
endif()

list(APPEND SOURCE
    rdbss.c)

add_library(rdbsslib ${SOURCE})
add_dependencies(rdbsslib bugcodes xdk)
