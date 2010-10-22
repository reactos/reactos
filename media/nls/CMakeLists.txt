add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/l_intl.nls reactos l_intl.nls)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/c_437.nls reactos c_437.nls)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/c_1252.nls reactos c_1252.nls)

file(GLOB NLS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.nls")
foreach(item ${NLS_FILES})
    add_livecd("${CMAKE_CURRENT_SOURCE_DIR}/${item}" reactos/system32 ${item})
endforeach(item)