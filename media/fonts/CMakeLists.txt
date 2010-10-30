file(GLOB NLS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.ttf")
foreach(item ${NLS_FILES})
    add_cab("${CMAKE_CURRENT_SOURCE_DIR}/${item}" 3)
endforeach(item)