
if(ENABLE_HELP)
    add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/common.css DESTINATION reactos/Help/res FOR bootcd)
    add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/common.js DESTINATION reactos/Help/res FOR bootcd)
    add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/roslogo-32x32.png DESTINATION reactos/Help/res FOR bootcd)
    add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/banner-bg.png DESTINATION reactos/Help/res FOR bootcd)
endif()
