
add_definitions(-D_NTOSKRNL_)
add_library(csq csq.c)
add_dependencies(csq bugcodes xdk)
