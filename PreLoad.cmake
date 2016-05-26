
# small trick to get the real source directory at this stage
STRING(REPLACE "/PreLoad.cmake" "" REACTOS_HOME_DIR ${CMAKE_CURRENT_LIST_FILE})

#message("/PreLoad.cmake ... ${REACTOS_HOME_DIR}")

SET(CMAKE_MODULE_PATH "${REACTOS_HOME_DIR}/sdk/cmake" CACHE INTERNAL "")

#message("CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}")

