
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/msgsm32.acm.spec --filename msgsm32.acm
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def PROPERTIES GENERATED TRUE)
add_custom_target(msgsm32.acm_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def)

add_library(msgsm32.acm SHARED msgsm32.c)

set_target_properties(msgsm32.acm PROPERTIES LINK_FLAGS "-Wl,-entry,0" SUFFIX "")

target_link_libraries(msgsm32.acm
    ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.acm.def
    wine
    -lwinmm
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    mingw_common
    ${REACTOS_SOURCE_DIR}/dll/win32/msvcrt/libmsvcrt.a)

add_dependencies(msgsm32.acm msgsm32.acm_def psdk)
