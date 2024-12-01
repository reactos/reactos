
function(add_host_tool _tool)
    add_executable(${_tool} ${ARGN})
    set_target_properties(${_tool} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TOOLS_FOLDER})
endfunction()

function(add_host_module _module)
    add_library(${_module} MODULE ${ARGN})
    set_target_properties(${_module} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${TOOLS_FOLDER})
endfunction()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -DHAVE_IO_H=1)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/EHsc>")

    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    add_compile_options("/wd4267")
endif()

add_host_tool(bin2c bin2c.c)
add_host_tool(gendib gendib/gendib.c)
add_host_tool(geninc geninc/geninc.c)
add_host_tool(mkshelllink mkshelllink/mkshelllink.c)
add_host_tool(obj2bin obj2bin/obj2bin.c)
target_link_libraries(obj2bin PRIVATE host_includes)

add_host_tool(spec2def spec2def/spec2def.c)
add_host_tool(utf16le utf16le/utf16le.cpp)

add_subdirectory(asmpp)
add_subdirectory(cabman)
add_subdirectory(fatten)
add_subdirectory(hhpcomp)
add_subdirectory(hpp)
add_subdirectory(isohybrid)
add_subdirectory(kbdtool)
add_subdirectory(log2lines)
add_subdirectory(mkhive)
add_subdirectory(mkisofs)
add_subdirectory(rsym)
add_subdirectory(txt2nls)
add_subdirectory(unicode)
add_subdirectory(widl)
add_subdirectory(wpp)
add_subdirectory(xml2sdb)

if ((ARCH STREQUAL "amd64") AND (TARGET_COMPILER_ID STREQUAL "GNU"))
    add_subdirectory(gcc_plugin_seh)
endif()

if (TARGET_BUILD_TYPE)
    # Silence warning about unused variable
endif()

if(NOT MSVC)
    add_host_tool(pefixup pefixup.c)
    if (ARCH STREQUAL "amd64" OR ARCH STREQUAL "arm64")
        target_compile_definitions(pefixup PRIVATE _TARGET_PE64)
    endif()
    target_link_libraries(pefixup PRIVATE host_includes)
endif()
