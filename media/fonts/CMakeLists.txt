file(GLOB NLS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.ttf")
foreach(item ${NLS_FILES})
    add_livecd("${CMAKE_CURRENT_SOURCE_DIR}/${item}" reactos/fonts ${item})
endforeach(item)