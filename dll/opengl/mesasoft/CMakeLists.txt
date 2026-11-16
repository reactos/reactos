# Mesa 24.0.9 softpipe WGL ICD staged by Build-MesaSoft.ps1 into bin/.
# Source: downloads/mesa/mesa-src (https://gitlab.freedesktop.org/mesa/mesa.git, MIT).

set(_MESASOFT_BIN ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(_MESASOFT_REQUIRED
    mesasoft.dll
    libglapi.dll
    libz-1.dll
    libwinpthread-1.dll)

foreach(_dll ${_MESASOFT_REQUIRED})
    if(NOT EXISTS ${_MESASOFT_BIN}/${_dll})
        message(FATAL_ERROR
            "Missing ${_MESASOFT_BIN}/${_dll}. "
            "Run Build-MesaSoft.ps1 from the VerOS checkout root first.")
    endif()
    add_cd_file(FILE ${_MESASOFT_BIN}/${_dll} DESTINATION VerOS/system32 FOR all)
endforeach()

add_registry_inf(mesasoft_reg.inf)
