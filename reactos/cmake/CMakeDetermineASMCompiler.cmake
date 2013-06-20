
#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

if(CMAKE_VERSION VERSION_LESS 2.8.10)

# determine the compiler to use for ASM programs

IF(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER)
  # prefer the environment variable ASM
  IF($ENV{ASM${ASM_DIALECT}} MATCHES ".+")
    SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT "$ENV{ASM${ASM_DIALECT}}")
  ENDIF($ENV{ASM${ASM_DIALECT}} MATCHES ".+")

  # finally list compilers to try
  IF("ASM${ASM_DIALECT}" STREQUAL "ASM") # the generic assembler support

    IF(CMAKE_ASM_COMPILER_INIT)
      SET(CMAKE_ASM_COMPILER_LIST ${CMAKE_ASM_COMPILER_INIT})
    ELSE(CMAKE_ASM_COMPILER_INIT)

      IF(CMAKE_C_COMPILER)
        SET(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "The ASM compiler")
        SET(CMAKE_ASM_COMPILER_ID "${CMAKE_C_COMPILER_ID}")
      ELSEIF(CMAKE_CXX_COMPILER)
        SET(CMAKE_ASM_COMPILER "${CMAKE_CXX_COMPILER}" CACHE FILEPATH "The ASM compiler")
        SET(CMAKE_ASM_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
      ELSE(CMAKE_CXX_COMPILER)
        # List all default C and CXX compilers
        SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST ${_CMAKE_TOOLCHAIN_PREFIX}gcc ${_CMAKE_TOOLCHAIN_PREFIX}cc cl bcc xlc
                                                  ${_CMAKE_TOOLCHAIN_PREFIX}c++ ${_CMAKE_TOOLCHAIN_PREFIX}g++ CC aCC cl bcc xlC)
      ENDIF(CMAKE_C_COMPILER)

    ENDIF(CMAKE_ASM_COMPILER_INIT)


  ELSE("ASM${ASM_DIALECT}" STREQUAL "ASM") # some specific assembler "dialect"

    IF(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT)
      SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST ${CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT})
    ELSE(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT)
      MESSAGE(FATAL_ERROR "CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT must be preset !")
    ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT)

  ENDIF("ASM${ASM_DIALECT}" STREQUAL "ASM")


  # Find the compiler.
  IF (_CMAKE_USER_CXX_COMPILER_PATH OR _CMAKE_USER_C_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_ASM${ASM_DIALECT}_COMPILER NAMES ${CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST} PATHS ${_CMAKE_USER_C_COMPILER_PATH} ${_CMAKE_USER_CXX_COMPILER_PATH} DOC "Assembler" NO_DEFAULT_PATH)
  ENDIF (_CMAKE_USER_CXX_COMPILER_PATH OR _CMAKE_USER_C_COMPILER_PATH)
  FIND_PROGRAM(CMAKE_ASM${ASM_DIALECT}_COMPILER NAMES ${CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST} PATHS ${_CMAKE_TOOLCHAIN_LOCATION} DOC "Assembler")

ELSE(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER)

  # we only get here if CMAKE_ASM${ASM_DIALECT}_COMPILER was specified using -D or a pre-made CMakeCache.txt
  # (e.g. via ctest) or set in CMAKE_TOOLCHAIN_FILE
  #
  # if a compiler was specified by the user but without path,
  # now try to find it with the full path
  # if it is found, force it into the cache,
  # if not, don't overwrite the setting (which was given by the user) with "NOTFOUND"
  GET_FILENAME_COMPONENT(_CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" PATH)
  IF(NOT _CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH NAMES ${CMAKE_ASM${ASM_DIALECT}_COMPILER})
    MARK_AS_ADVANCED(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
    IF(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
      SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH} CACHE FILEPATH "Assembler" FORCE)
    ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
  ENDIF(NOT _CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH)
ENDIF(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER)
MARK_AS_ADVANCED(CMAKE_ASM${ASM_DIALECT}_COMPILER)

IF (NOT _CMAKE_TOOLCHAIN_LOCATION)
  GET_FILENAME_COMPONENT(_CMAKE_TOOLCHAIN_LOCATION "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" PATH)
ENDIF (NOT _CMAKE_TOOLCHAIN_LOCATION)


IF(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER_ID)

  # Table of per-vendor compiler id flags with expected output.
  LIST(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS GNU )
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_GNU "--version")
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_GNU "(GNU assembler)|(GCC)|(Free Software Foundation)")

  LIST(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS HP )
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_HP "-V")
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_HP "HP C")

  LIST(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS Intel )
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_Intel "--version")
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_Intel "(ICC)")

  LIST(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS SunPro )
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_SunPro "-V")
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_SunPro "Sun C")

  LIST(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS XL )
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_XL "-qversion")
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_XL "XL C")

  LIST(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS TI_DSP )
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_TI_DSP "-h")
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_TI_DSP "Texas Instruments")
  
  LIST(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS VISUAL)
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_VISUAL "/?")
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_VISUAL "Microsoft Corporation")

  INCLUDE(CMakeDetermineCompilerId)
  CMAKE_DETERMINE_COMPILER_ID_VENDOR(ASM${ASM_DIALECT})

ENDIF()

IF(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID)
  MESSAGE(STATUS "The ASM${ASM_DIALECT} compiler identification is ${CMAKE_ASM${ASM_DIALECT}_COMPILER_ID}")
ELSE(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID)
  MESSAGE(STATUS "The ASM${ASM_DIALECT} compiler identification is unknown")
ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID)



# If we have a gas/as cross compiler, they have usually some prefix, like
# e.g. powerpc-linux-gas, arm-elf-gas or i586-mingw32msvc-gas , optionally
# with a 3-component version number at the end
# The other tools of the toolchain usually have the same prefix
# NAME_WE cannot be used since then this test will fail for names lile
# "arm-unknown-nto-qnx6.3.0-gas.exe", where BASENAME would be
# "arm-unknown-nto-qnx6" instead of the correct "arm-unknown-nto-qnx6.3.0-"
IF (NOT _CMAKE_TOOLCHAIN_PREFIX)
  GET_FILENAME_COMPONENT(COMPILER_BASENAME "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" NAME)
  IF (COMPILER_BASENAME MATCHES "^(.+-)g?as(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
    SET(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  ENDIF (COMPILER_BASENAME MATCHES "^(.+-)g?as(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
ENDIF (NOT _CMAKE_TOOLCHAIN_PREFIX)

# Now try the C compiler regexp:
IF (NOT _CMAKE_TOOLCHAIN_PREFIX)
  IF (COMPILER_BASENAME MATCHES "^(.+-)g?cc(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
    SET(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  ENDIF (COMPILER_BASENAME MATCHES "^(.+-)g?cc(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
ENDIF (NOT _CMAKE_TOOLCHAIN_PREFIX)

# Finally try the CXX compiler regexp:
IF (NOT _CMAKE_TOOLCHAIN_PREFIX)
  IF (COMPILER_BASENAME MATCHES "^(.+-)[gc]\\+\\+(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
    SET(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  ENDIF (COMPILER_BASENAME MATCHES "^(.+-)[gc]\\+\\+(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
ENDIF (NOT _CMAKE_TOOLCHAIN_PREFIX)


INCLUDE(CMakeFindBinUtils)

SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_ENV_VAR "ASM${ASM_DIALECT}")

IF(CMAKE_ASM${ASM_DIALECT}_COMPILER)
  MESSAGE(STATUS "Found assembler: ${CMAKE_ASM${ASM_DIALECT}_COMPILER}")
ELSE(CMAKE_ASM${ASM_DIALECT}_COMPILER)
  MESSAGE(STATUS "Didn't find assembler")
ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER)


SET(_CMAKE_ASM_COMPILER "${CMAKE_ASM${ASM_DIALECT}_COMPILER}")
SET(_CMAKE_ASM_COMPILER_ID "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ID}")
SET(_CMAKE_ASM_COMPILER_ARG1 "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1}")
SET(_CMAKE_ASM_COMPILER_ENV_VAR "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ENV_VAR}")

# configure variables set in this file for fast reload later on
CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeASMCompiler.cmake.in
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeASM${ASM_DIALECT}Compiler.cmake IMMEDIATE @ONLY)

SET(_CMAKE_ASM_COMPILER)
SET(_CMAKE_ASM_COMPILER_ARG1)
SET(_CMAKE_ASM_COMPILER_ENV_VAR)
    
else()

# determine the compiler to use for ASM programs

include(${CMAKE_ROOT}/Modules/CMakeDetermineCompiler.cmake)

if(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER)
  # prefer the environment variable ASM
  if($ENV{ASM${ASM_DIALECT}} MATCHES ".+")
    set(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT "$ENV{ASM${ASM_DIALECT}}")
  endif()

  # finally list compilers to try
  if("ASM${ASM_DIALECT}" STREQUAL "ASM") # the generic assembler support
    if(NOT CMAKE_ASM_COMPILER_INIT)
      if(CMAKE_C_COMPILER)
        set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "The ASM compiler")
        set(CMAKE_ASM_COMPILER_ID "${CMAKE_C_COMPILER_ID}")
      elseif(CMAKE_CXX_COMPILER)
        set(CMAKE_ASM_COMPILER "${CMAKE_CXX_COMPILER}" CACHE FILEPATH "The ASM compiler")
        set(CMAKE_ASM_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
      else()
        # List all default C and CXX compilers
        set(CMAKE_ASM${ASM_DIALECT}_COMPILER_LIST
             ${_CMAKE_TOOLCHAIN_PREFIX}cc  ${_CMAKE_TOOLCHAIN_PREFIX}gcc cl bcc xlc
          CC ${_CMAKE_TOOLCHAIN_PREFIX}c++ ${_CMAKE_TOOLCHAIN_PREFIX}g++ aCC cl bcc xlC)
      endif()
    endif()
  else() # some specific assembler "dialect"
    if(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT)
      message(FATAL_ERROR "CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT must be preset !")
    endif()
  endif()

  # Find the compiler.
  _cmake_find_compiler(ASM${ASM_DIALECT})

else()

  # we only get here if CMAKE_ASM${ASM_DIALECT}_COMPILER was specified using -D or a pre-made CMakeCache.txt
  # (e.g. via ctest) or set in CMAKE_TOOLCHAIN_FILE
  #
  # if a compiler was specified by the user but without path,
  # now try to find it with the full path
  # if it is found, force it into the cache,
  # if not, don't overwrite the setting (which was given by the user) with "NOTFOUND"
  get_filename_component(_CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" PATH)
  if(NOT _CMAKE_USER_ASM${ASM_DIALECT}_COMPILER_PATH)
    find_program(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH NAMES ${CMAKE_ASM${ASM_DIALECT}_COMPILER})
    mark_as_advanced(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
    if(CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH)
      set(CMAKE_ASM${ASM_DIALECT}_COMPILER ${CMAKE_ASM${ASM_DIALECT}_COMPILER_WITH_PATH} CACHE FILEPATH "Assembler" FORCE)
    endif()
  endif()
endif()
mark_as_advanced(CMAKE_ASM${ASM_DIALECT}_COMPILER)

if (NOT _CMAKE_TOOLCHAIN_LOCATION)
  get_filename_component(_CMAKE_TOOLCHAIN_LOCATION "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" PATH)
endif ()


if(NOT CMAKE_ASM${ASM_DIALECT}_COMPILER_ID)

  # Table of per-vendor compiler id flags with expected output.
  list(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS GNU )
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_GNU "--version")
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_GNU "(GNU assembler)|(GCC)|(Free Software Foundation)")

  list(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS HP )
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_HP "-V")
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_HP "HP C")

  list(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS Intel )
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_Intel "--version")
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_Intel "(ICC)")

  list(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS SunPro )
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_SunPro "-V")
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_SunPro "Sun C")

  list(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS XL )
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_XL "-qversion")
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_XL "XL C")

  list(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS MSVC )
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_MSVC "/?")
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_MSVC "Microsoft")

  list(APPEND CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDORS TI_DSP )
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_FLAGS_TI_DSP "-h")
  set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID_VENDOR_REGEX_TI_DSP "Texas Instruments")

  include(CMakeDetermineCompilerId)
  CMAKE_DETERMINE_COMPILER_ID_VENDOR(ASM${ASM_DIALECT})

endif()

if(CMAKE_ASM${ASM_DIALECT}_COMPILER_ID)
  message(STATUS "The ASM${ASM_DIALECT} compiler identification is ${CMAKE_ASM${ASM_DIALECT}_COMPILER_ID}")
else()
  message(STATUS "The ASM${ASM_DIALECT} compiler identification is unknown")
endif()



# If we have a gas/as cross compiler, they have usually some prefix, like
# e.g. powerpc-linux-gas, arm-elf-gas or i586-mingw32msvc-gas , optionally
# with a 3-component version number at the end
# The other tools of the toolchain usually have the same prefix
# NAME_WE cannot be used since then this test will fail for names lile
# "arm-unknown-nto-qnx6.3.0-gas.exe", where BASENAME would be
# "arm-unknown-nto-qnx6" instead of the correct "arm-unknown-nto-qnx6.3.0-"
if (NOT _CMAKE_TOOLCHAIN_PREFIX)
  get_filename_component(COMPILER_BASENAME "${CMAKE_ASM${ASM_DIALECT}_COMPILER}" NAME)
  if (COMPILER_BASENAME MATCHES "^(.+-)g?as(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
    set(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  endif ()
endif ()

# Now try the C compiler regexp:
if (NOT _CMAKE_TOOLCHAIN_PREFIX)
  if (COMPILER_BASENAME MATCHES "^(.+-)g?cc(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
    set(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  endif ()
endif ()

# Finally try the CXX compiler regexp:
if (NOT _CMAKE_TOOLCHAIN_PREFIX)
  if (COMPILER_BASENAME MATCHES "^(.+-)[gc]\\+\\+(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
    set(_CMAKE_TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
  endif ()
endif ()


include(CMakeFindBinUtils)

set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ENV_VAR "ASM${ASM_DIALECT}")

if(CMAKE_ASM${ASM_DIALECT}_COMPILER)
  message(STATUS "Found assembler: ${CMAKE_ASM${ASM_DIALECT}_COMPILER}")
else()
  message(STATUS "Didn't find assembler")
endif()


set(_CMAKE_ASM_COMPILER "${CMAKE_ASM${ASM_DIALECT}_COMPILER}")
set(_CMAKE_ASM_COMPILER_ID "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ID}")
set(_CMAKE_ASM_COMPILER_ARG1 "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1}")
set(_CMAKE_ASM_COMPILER_ENV_VAR "${CMAKE_ASM${ASM_DIALECT}_COMPILER_ENV_VAR}")

# configure variables set in this file for fast reload later on
configure_file(${CMAKE_ROOT}/Modules/CMakeASMCompiler.cmake.in
  ${CMAKE_PLATFORM_INFO_DIR}/CMakeASM${ASM_DIALECT}Compiler.cmake IMMEDIATE @ONLY)

set(_CMAKE_ASM_COMPILER)
set(_CMAKE_ASM_COMPILER_ARG1)
set(_CMAKE_ASM_COMPILER_ENV_VAR)
    
endif()

