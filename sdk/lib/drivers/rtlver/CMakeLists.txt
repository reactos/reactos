add_definitions(-DUNICODE -D_UNICODE -D_NTOSKRNL_ -D_NTSYSTEM_)

list(APPEND SOURCE
    rtl.c)

add_library(rtlver ${SOURCE})
add_dependencies(rtlver bugcodes xdk)
