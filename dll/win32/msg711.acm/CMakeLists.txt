
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/msg711.acm.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/msg711.acm.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/msg711.acm.spec --filename msg711.acm
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/msg711.acm.def PROPERTIES GENERATED TRUE)
add_custom_target(msg711.acm_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msg711.acm.def)

add_library(msg711.acm SHARED msg711.c)

set_target_properties(msg711.acm PROPERTIES LINK_FLAGS "-Wl,-entry,0" SUFFIX "")

target_link_libraries(msg711.acm
    ${CMAKE_CURRENT_BINARY_DIR}/msg711.acm.def
    wine
    -lwinmm
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    mingw_common
    ${REACTOS_SOURCE_DIR}/dll/win32/msvcrt/libmsvcrt.a)

add_dependencies(msg711.acm msg711.acm_def psdk)
