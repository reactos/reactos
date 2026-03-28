
# This exists purely for syncing code that is compiled as an ANSI exe but depends on
# Vista+ functionality, Eventually we won't need to maintain a 0x502 build and this
# will be removed.

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

list(APPEND SOURCE
   sync_hacks.c)

add_library(ansi_sync_hacks STATIC ${SOURCE})
add_dependencies(ansi_sync_hacks psdk)
