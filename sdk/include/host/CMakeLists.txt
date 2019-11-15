
include(CheckIncludeFile)
include(CheckTypeSize)

# check for <sys/types.h>
CHECK_INCLUDE_FILE("sys/types.h" HAVE_SYS_TYPES_H)

# check for pid_t definition
if (HAVE_SYS_TYPES_H)
    set(CMAKE_EXTRA_INCLUDE_FILES "sys/types.h")
    #this sets HAVE_PID_T
    CHECK_TYPE_SIZE("pid_t" PID_T)
    unset(CMAKE_EXTRA_INCLUDE_FILES)
endif()

configure_file(config.h.in config.h @ONLY)

add_library(host_includes INTERFACE)
target_include_directories(host_includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
