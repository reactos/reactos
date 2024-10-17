# Disable unknown-pragmas warning
foreach(lang C CXX ASM)
  set(CMAKE_${lang}_FLAGS_DEBUG "-Wno-unknown-pragmas")
  set(CMAKE_${lang}_FLAGS_MINSIZEREL "-Wno-unknown-pragmas -Os -DNDEBUG")
  set(CMAKE_${lang}_FLAGS_RELEASE "-Wno-unknown-pragmas")
  set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO "-Wno-unknown-pragmas -O2 -g -DNDEBUG")
  set(CMAKE_${lang}_IMPLICIT_LINK_LIBRARIES " ")
  set(CMAKE_${lang}_IMPLICIT_LINK_DIRECTORIES " ")
endforeach()
