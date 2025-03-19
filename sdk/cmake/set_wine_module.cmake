
function(set_wine_module TARGET)
    target_include_directories(${TARGET} SYSTEM BEFORE PRIVATE
        ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/wine)
    target_compile_definitions(${TARGET} PRIVATE __WINESRC__) # FIXME: CORE-5743: Delete this line
endfunction()
