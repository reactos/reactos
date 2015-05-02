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

    MACRO
    DEFINE_ALIAS $FuncName, $Target
        IMPORT $Target
        NESTED_ENTRY $FuncName
        b $Target
        NESTED_END $FuncName
    MEND

    DEFINE_ALIAS ??0exception@@QAE@ABQBD@Z, MSVCRT_exception_ctor
    DEFINE_ALIAS ??0exception@@QAE@ABQBDH@Z, MSVCRT_exception_ctor_noalloc
    DEFINE_ALIAS ??0exception@@QAE@ABV0@@Z, MSVCRT_exception_copy_ctor
    DEFINE_ALIAS ??0exception@@QAE@XZ, MSVCRT_exception_default_ctor
    DEFINE_ALIAS ??1exception@@UAE@XZ, MSVCRT_exception_dtor
    DEFINE_ALIAS ??4exception@@QAEAAV0@ABV0@@Z, MSVCRT_exception_opequals
    DEFINE_ALIAS ??_Eexception@@UAEPAXI@Z, MSVCRT_exception_vector_dtor
    DEFINE_ALIAS ??_Gexception@@UAEPAXI@Z, MSVCRT_exception_scalar_dtor
    DEFINE_ALIAS ?what@exception@@UBEPBDXZ, MSVCRT_what_exception
    DEFINE_ALIAS ??0bad_typeid@@QAE@ABV0@@Z, MSVCRT_bad_typeid_copy_ctor
    DEFINE_ALIAS ??0bad_typeid@@QAE@PBD@Z, MSVCRT_bad_typeid_ctor
    DEFINE_ALIAS ??_Fbad_typeid@@QAEXXZ, MSVCRT_bad_typeid_default_ctor
    DEFINE_ALIAS ??1bad_typeid@@UAE@XZ, MSVCRT_bad_typeid_dtor
    DEFINE_ALIAS ??4bad_typeid@@QAEAAV0@ABV0@@Z, MSVCRT_bad_typeid_opequals
    DEFINE_ALIAS ??_Ebad_typeid@@UAEPAXI@Z, MSVCRT_bad_typeid_vector_dtor
    DEFINE_ALIAS ??_Gbad_typeid@@UAEPAXI@Z, MSVCRT_bad_typeid_scalar_dtor
    DEFINE_ALIAS ??0__non_rtti_object@@QAE@ABV0@@Z, MSVCRT___non_rtti_object_copy_ctor
    DEFINE_ALIAS ??0__non_rtti_object@@QAE@PBD@Z, MSVCRT___non_rtti_object_ctor
    DEFINE_ALIAS ??1__non_rtti_object@@UAE@XZ, MSVCRT___non_rtti_object_dtor
    DEFINE_ALIAS ??4__non_rtti_object@@QAEAAV0@ABV0@@Z, MSVCRT___non_rtti_object_opequals
    DEFINE_ALIAS ??_E__non_rtti_object@@UAEPAXI@Z, MSVCRT___non_rtti_object_vector_dtor
    DEFINE_ALIAS ??_G__non_rtti_object@@UAEPAXI@Z, MSVCRT___non_rtti_object_scalar_dtor
    DEFINE_ALIAS ??0bad_cast@@AAE@PBQBD@Z, MSVCRT_bad_cast_ctor
    DEFINE_ALIAS ??0bad_cast@@QAE@ABQBD@Z, MSVCRT_bad_cast_ctor
    DEFINE_ALIAS ??0bad_cast@@QAE@ABV0@@Z, MSVCRT_bad_cast_copy_ctor
    DEFINE_ALIAS ??0bad_cast@@QAE@PBD@Z, MSVCRT_bad_cast_ctor_charptr
    DEFINE_ALIAS ??_Fbad_cast@@QAEXXZ, MSVCRT_bad_cast_default_ctor
    DEFINE_ALIAS ??1bad_cast@@UAE@XZ, MSVCRT_bad_cast_dtor
    DEFINE_ALIAS ??4bad_cast@@QAEAAV0@ABV0@@Z, MSVCRT_bad_cast_opequals
    DEFINE_ALIAS ??_Ebad_cast@@UAEPAXI@Z, MSVCRT_bad_cast_vector_dtor
    DEFINE_ALIAS ??_Gbad_cast@@UAEPAXI@Z, MSVCRT_bad_cast_scalar_dtor
    DEFINE_ALIAS ??8type_info@@QBEHABV0@@Z, MSVCRT_type_info_opequals_equals
    DEFINE_ALIAS ??9type_info@@QBEHABV0@@Z, MSVCRT_type_info_opnot_equals
    DEFINE_ALIAS ?before@type_info@@QBEHABV1@@Z, MSVCRT_type_info_before
    DEFINE_ALIAS ??1type_info@@UAE@XZ, MSVCRT_type_info_dtor
    DEFINE_ALIAS ?name@type_info@@QBEPBDXZ, MSVCRT_type_info_name
    DEFINE_ALIAS ?raw_name@type_info@@QBEPBDXZ, MSVCRT_type_info_raw_name

    DEFINE_ALIAS ??_V@YAXPAX@Z, MSVCRT_operator_delete
    DEFINE_ALIAS ??2@YAPAXI@Z, MSVCRT_operator_new
    DEFINE_ALIAS ?_query_new_handler@@YAP6AHI@ZXZ, MSVCRT__query_new_handler
    DEFINE_ALIAS ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z, MSVCRT__set_new_handler
    DEFINE_ALIAS ?set_new_handler@@YAP6AXXZP6AXXZ@Z, MSVCRT_set_new_handler
    DEFINE_ALIAS ?_query_new_mode@@YAHXZ, MSVCRT__query_new_mode
    DEFINE_ALIAS ?_set_new_mode@@YAHH@Z, MSVCRT__set_new_mode
    DEFINE_ALIAS ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z, MSVCRT__set_se_translator
    DEFINE_ALIAS ?set_terminate@@YAP6AXXZP6AXXZ@Z, MSVCRT_set_terminate
    DEFINE_ALIAS ?set_unexpected@@YAP6AXXZP6AXXZ@Z, MSVCRT_set_unexpected
    DEFINE_ALIAS ?terminate@@YAXXZ, MSVCRT_terminate
    DEFINE_ALIAS ?unexpected@@YAXXZ, MSVCRT_unexpected

    #undef _MSVCRT_
    MACRO
    START_VTABLE $ShortName, $CxxName
        LCLS RttiName
        LCLS VtblName
        LCLS DtorName
        LCLS CxxLabel
CxxLabel    SETS "|$CxxName|"
RttiName    SETS "|$ShortName._rtti|"
VtblName    SETS "|MSVCRT_":CC:"$ShortName._vtable|"
DtorName    SETS "|MSVCRT_":CC:"$ShortName._vector_dtor|"
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
        EXTERN MSVCRT_what_exception
        DCD MSVCRT_what_exception
    MEND

    START_VTABLE type_info, __dummyname_type_info
    DEFINE_EXCEPTION_VTABLE exception, ??_7exception@@6B@
    DEFINE_EXCEPTION_VTABLE bad_typeid, ??_7bad_typeid@@6B@
    DEFINE_EXCEPTION_VTABLE bad_cast, ??_7bad_cast@@6B@
    DEFINE_EXCEPTION_VTABLE __non_rtti_object, ??_7__non_rtti_object@@6B@

    GBLS FuncName

    EXTERN MSVCRT_operator_delete
    __ExportName ??3@YAXPAX@Z
    b MSVCRT_operator_delete

    EXTERN MSVCRT_operator_new
    __ExportName ??_U@YAPAXI@Z
    b MSVCRT_operator_new

    END
/* EOF */
