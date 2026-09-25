#******************************** File Header ******************************\
# Licensed to the .NET Foundation under one or more agreements.
# The .NET Foundation licenses this file to you under the MIT license.
# See the LICENSE file in the project root for more information.
#
# SOURCES file containing definitions for locations of library files used
# by milcore.dll and milcoretest.dll
#
#***************************************************************************/

SOURCES_USED=$(SOURCES_USED) $(WGX_PATH)\core\dll\dllsources.inc

#
#   These libs will be included TARGETLIBS in wgxcore.dll
#   WGX test apps should link these in as well
#
WGXCORE_IMPORTLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib                                   \
    $(SDK_LIB_PATH)\winmm.lib                                   \
    $(SDK_LIB_PATH)\user32.lib                                     \
    $(WINDOWS_LIB_PATH)\gdi32p.lib                                 \
    $(SDK_LIB_PATH)\ole32.lib                                      \
    $(SDK_LIB_PATH)\oleaut32.lib                                      \
    $(SDK_LIB_PATH)\ntdll.lib                                      \
    $(SDK_LIB_PATH)\uuid.lib                                       \
    $(SDK_LIB_PATH)\advapi32.lib                                   \
    $(SDK_LIB_PATH)\version.lib                                    \
    $(SDK_LIB_PATH)\rpcrt4.lib                                     \
    $(SDK_LIB_PATH)\WindowsCodecs.lib                              \
    $(SDK_LIB_PATH)\evr.lib                                        \
    $(SDK_LIB_PATH)\Strmbase.lib                                   \
    $(SDK_LIB_PATH)\psapi.lib                                      \
    

#
#   Link to stuff which is exported by our DLL.
#   All the .def dependencies go here.
#
WGXCORE_LINKLIBS=\
    $(WGX_CORE_OBJ_PATH)\api\$(O)\api.lib                           \
    $(WGX_CORE_OBJ_PATH)\uce\$(O)\uce.lib                           \
    $(WGX_CORE_OBJ_PATH)\resources\$(O)\milresource.lib             \
    $(WGX_CORE_OBJ_PATH)\common\core\$(O)\common.lib                \
    $(WGX_CORE_OBJ_PATH)\control\util\$(O)\gcutil.lib               \
    $(WGX_CORE_OBJ_PATH)\av\$(O)\av.lib                             \
    
    
    
#
#   Link to all the objects which are included internally.
#
#   NOTE: Milcore used to include d3dx9bbt.lib, i.e. the BBTed version
#   of d3d9x.lib. 
#

WGXCORE_TARGETLIBS=\
    $(SDK_LIB_PATH)\d3dx9.lib                                       \
    $(WGX_DEBUG_LIB)                                                \
    $(WGX_UTIL_LIB)                                                 \
    $(WGX_CORE_OBJ_PATH)\targets\$(O)\targets.lib                   \
    $(WGX_CORE_OBJ_PATH)\sw\$(O)\sw.lib                             \
    $(WGX_CORE_OBJ_PATH)\hw\$(O)\hw.lib                             \
    $(WGX_CORE_OBJ_PATH)\hw\$(O)\hw.res                             \
    $(WGX_CORE_OBJ_PATH)\meta\$(O)\meta.lib                         \
    $(WGX_OBJ_PATH)\common\scanop\$(O)\scanop.lib                      \
    $(WGX_OBJ_PATH)\common\shared\$(O)\shared.lib                      \
    $(WGX_CORE_OBJ_PATH)\av\$(O)\av.lib                             \
    $(WGX_CORE_OBJ_PATH)\guid\internal\$(O)\InternalGUIDs.lib       \
    $(WGX_CORE_OBJ_PATH)\glyph\$(O)\glyph.lib                       \
    $(WGX_CORE_OBJ_PATH)\log\etwtrace\$(O)\etwtrace.lib             \
    $(WGX_PATH)\external\lib\DWrite.lib					   \
