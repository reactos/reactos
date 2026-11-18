# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        sdk/cmake/VerosBuildenv.cmake
# PURPOSE:     Locate the buildenv tools directory for nested or side-by-side layouts
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
# PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>

# Nested:   <buildenv>/source  +  <buildenv>/tools
# Side-by-side: <ws>/source  +  <ws>/buildenv/tools

get_filename_component(_VEROS_WORKSPACE "${REACTOS_SOURCE_DIR}" DIRECTORY)
if(EXISTS "${_VEROS_WORKSPACE}/buildenv/tools/rust/cargo/bin/cargo.exe")
    set(VEROS_CHECKOUT "${_VEROS_WORKSPACE}/buildenv")
elseif(EXISTS "${_VEROS_WORKSPACE}/tools/rust/cargo/bin/cargo.exe")
    set(VEROS_CHECKOUT "${_VEROS_WORKSPACE}")
else()
    set(VEROS_CHECKOUT "${_VEROS_WORKSPACE}")
endif()
unset(_VEROS_WORKSPACE)
