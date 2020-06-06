
set_cpp()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

list(APPEND SOURCE
    hdaudbus.cpp
    fdo.cpp
    pdo.cpp
    businterface.cpp)

add_library(hdaudbus MODULE ${SOURCE})
set_module_type(hdaudbus kernelmodedriver)
target_link_libraries(hdaudbus libcntpr ${PSEH_LIB})
add_importlibs(hdaudbus ntoskrnl hal)
add_cd_file(TARGET hdaudbus DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(hdaudbus hdaudbus.inf)
