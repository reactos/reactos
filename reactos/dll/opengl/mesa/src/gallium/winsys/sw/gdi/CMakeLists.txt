
add_definitions(-DWIN32_LEAN_AND_MEAN)

add_library(gallium_ws_gdi STATIC EXCLUDE_FROM_ALL gdi_sw_winsys.c)
