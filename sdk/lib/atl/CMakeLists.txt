
add_library(atl_classes INTERFACE)
if(DBG)
    #target_compile_definitions(atl_classes INTERFACE _DEBUG) # HACK & FIXME: CORE-17505
endif()

target_include_directories(atl_classes INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_definitions(atl_classes INTERFACE
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<BOOL:$<TARGET_PROPERTY:WITH_CXX_EXCEPTIONS>>>>:_ATL_NO_EXCEPTIONS>")

target_link_libraries(atl_classes INTERFACE pseh)
