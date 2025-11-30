remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

list(APPEND SOURCE d3dx10_43_main.c)
include(d3dx10.cmake)

add_d3dx10_target(43)
