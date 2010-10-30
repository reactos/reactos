file(GLOB NLS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.inf")
foreach(item ${NLS_FILES})
    add_cab("${CMAKE_CURRENT_SOURCE_DIR}/${item}" 6)
endforeach(item)

add_livecd(${CMAKE_CURRENT_SOURCE_DIR}/syssetup.inf.trunk reactos/inf syssetup.inf)