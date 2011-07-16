if(NOT MSVC)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a
        COMMAND ${MINGW_PREFIX}dlltool --def ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def --kill-at --output-lib ${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a
        COMMAND ${MINGW_PREFIX}ar -rc ${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a)

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a PROPERTIES GENERATED TRUE)

    add_custom_target(oldnames ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/liboldnames.a)
endif()
