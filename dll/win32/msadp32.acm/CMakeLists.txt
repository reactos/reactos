
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/msadp32.acm.spec --filename msadp32.acm
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def PROPERTIES GENERATED TRUE)
add_custom_target(msadp32.acm_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def)

add_library(msadp32.acm SHARED msadp32.c)

set_target_properties(msadp32.acm PROPERTIES LINK_FLAGS "-Wl,-entry,0" SUFFIX "")

target_link_libraries(msadp32.acm
    ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def
    wine
    -lwinmm
    -luser32
    mingw_common
    -lmsvcrt)

add_dependencies(msadp32.acm msadp32.acm_def psdk)
