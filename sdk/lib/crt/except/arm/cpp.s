/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           MSVC wrappers for C++ functions
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

    #undef _MSVCRT_
    MACRO
    START_VTABLE $ShortName, $CxxName
        LCLS RttiName
        LCLS VtblName
        LCLS DtorName
        LCLS CxxLabel
CxxLabel    SETS "|$CxxName|"
RttiName    SETS "|$ShortName._rtti|"
VtblName    SETS "|":CC:"$ShortName._vtable|"
DtorName    SETS "|":CC:"$ShortName._vector_dtor|"
        EXTERN $RttiName
        DCD $RttiName
        EXPORT $VtblName
$VtblName
        EXPORT $CxxLabel
$CxxLabel
        EXTERN $DtorName
        DCD $DtorName
    MEND

    MACRO
    DEFINE_EXCEPTION_VTABLE $ShortName, $CxxName
        START_VTABLE $ShortName, $CxxName
        EXTERN exception_what
        DCD exception_what
    MEND

    START_VTABLE type_info, __dummyname_type_info
    DEFINE_EXCEPTION_VTABLE exception, ??_7exception@@6B@
    DEFINE_EXCEPTION_VTABLE bad_typeid, ??_7bad_typeid@@6B@
    DEFINE_EXCEPTION_VTABLE bad_cast, ??_7bad_cast@@6B@
    DEFINE_EXCEPTION_VTABLE __non_rtti_object, ??_7__non_rtti_object@@6B@

    GBLS FuncName

    //EXTERN operator_delete
    //__ExportName ??3@YAXPAX@Z
    //b operator_delete

    //EXTERN operator_new
    //__ExportName ??_U@YAPAXI@Z
    //b operator_new

    END
/* EOF */
