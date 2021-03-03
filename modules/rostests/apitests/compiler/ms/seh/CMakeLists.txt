
foreach(num RANGE 1 9)
   list(APPEND SOURCE seh000${num}.c)
   set_source_files_properties(seh000${num}.c PROPERTIES COMPILE_DEFINITIONS
       "main=seh000${num};test=test_${num};Echo=Echo_${num};dojump=dojump_${num};AccessViolation=AccessViolation_${num};rtlRaiseExceptin=rtlRaiseExceptin_${num};rtlRaiseException=rtlRaiseException_${num};rtlRaiseStatus=rtlRaiseStatus_${num}")
endforeach()

foreach(num RANGE 10 58)
   list(APPEND SOURCE seh00${num}.c)
   set_source_files_properties(seh00${num}.c PROPERTIES COMPILE_DEFINITIONS
       "main=seh00${num};test=test_${num};Echo=Echo_${num};dojump=dojump_${num};AccessViolation=AccessViolation_${num};rtlRaiseExceptin=rtlRaiseExceptin_${num};rtlRaiseException=rtlRaiseException_${num};rtlRaiseStatus=rtlRaiseStatus_${num}")
endforeach()

add_library(ms_seh_test ${SOURCE})
add_dependencies(ms_seh_test psdk)

if(MSVC)
    # error C4189: 'Index2': local variable is initialized but not referenced
    # warning C4197: 'volatile LONG': top-level volatile in cast is ignored
    # warning C4532: 'break': jump out of __finally block has undefined behavior during termination handling
    remove_target_compile_option(ms_seh_test "/we4189")
    target_compile_options(ms_seh_test PRIVATE /wd4189 /wd4197 /wd4532)
else()
    target_compile_options(ms_seh_test
        PRIVATE -Wno-format -Wno-implicit-function-declaration -Wno-unused-label -Wno-unused-variable)
endif()
