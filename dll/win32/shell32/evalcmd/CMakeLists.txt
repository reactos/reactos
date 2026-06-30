
list(APPEND SOURCE
    evalcmd.cpp
    elements.cpp)

add_library(evalcmd STATIC ${SOURCE})
add_dependencies(evalcmd psdk)
target_include_directories(evalcmd INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(evalcmd PRIVATE wine uuid pathcch_static cppstl)
target_compile_definitions(evalcmd PRIVATE STATIC_PATHCCH)

if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    target_compile_definitions(evalcmd PRIVATE _SHELL32_)
else()
    target_compile_definitions(evalcmd PRIVATE _SHLWAPI_)
endif()
