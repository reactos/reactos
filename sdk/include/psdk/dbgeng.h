/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "objbase.h"

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(IID_IDebugInputCallbacks,     0x9f50e42c, 0xf136, 0x499e, 0x9a, 0x97, 0x73, 0x03, 0x6c, 0x94, 0xed, 0x2d);
DEFINE_GUID(IID_IDebugOutputCallbacks,    0x4bf58045, 0xd654, 0x4c40, 0xb0, 0xaf, 0x68, 0x30, 0x90, 0xf3, 0x56, 0xdc);
DEFINE_GUID(IID_IDebugOutputCallbacksWide,0x4c7fd663, 0xc394, 0x4e26, 0x8e, 0xf1, 0x34, 0xad, 0x5e, 0xd3, 0x76, 0x4c);
DEFINE_GUID(IID_IDebugEventCallbacks,     0x337be28b, 0x5036, 0x4d72, 0xb6, 0xbf, 0xc4, 0x5f, 0xbb, 0x9f, 0x2e, 0xaa);
DEFINE_GUID(IID_IDebugEventCallbacksWide, 0x0690e046, 0x9c23, 0x45ac, 0xa0, 0x4f, 0x98, 0x7a, 0xc2, 0x9a, 0xd0, 0xd3);
DEFINE_GUID(IID_IDebugEventContextCallbacks, 0x61a4905b, 0x23f9, 0x4247, 0xb3, 0xc5, 0x53, 0xd0, 0x87, 0x52, 0x9a, 0xb7);
DEFINE_GUID(IID_IDebugBreakpoint,         0x5bd9d474, 0x5975, 0x423a, 0xb8, 0x8b, 0x65, 0xa8, 0xe7, 0x11, 0x0e, 0x65);
DEFINE_GUID(IID_IDebugBreakpoint2,        0x1b278d20, 0x79f2, 0x426e, 0xa3, 0xf9, 0xc1, 0xdd, 0xf3, 0x75, 0xd4, 0x8e);
DEFINE_GUID(IID_IDebugClient,             0x27fe5639, 0x8407, 0x4f47, 0x83, 0x64, 0xee, 0x11, 0x8f, 0xb0, 0x8a, 0xc8);
DEFINE_GUID(IID_IDebugClient2,            0xedbed635, 0x372e, 0x4dab, 0xbb, 0xfe, 0xed, 0x0d, 0x2f, 0x63, 0xbe, 0x81);
DEFINE_GUID(IID_IDebugClient3,            0xdd492d7f, 0x71b8, 0x4ad6, 0xa8, 0xdc, 0x1c, 0x88, 0x74, 0x79, 0xff, 0x91);
DEFINE_GUID(IID_IDebugClient4,            0xca83c3de, 0x5089, 0x4cf8, 0x93, 0xc8, 0xd8, 0x92, 0x38, 0x7f, 0x2a, 0x5e);
DEFINE_GUID(IID_IDebugClient5,            0xe3acb9d7, 0x7ec2, 0x4f0c, 0xa0, 0xda, 0xe8, 0x1e, 0x0c, 0xbb, 0xe6, 0x28);
DEFINE_GUID(IID_IDebugClient6,            0xfd28b4c5, 0xc498, 0x4686, 0xa2, 0x8e, 0x62, 0xca, 0xd2, 0x15, 0x4e, 0xb3);
DEFINE_GUID(IID_IDebugClient7,            0x13586be3, 0x542e, 0x481e, 0xb1, 0xf2, 0x84, 0x97, 0xba, 0x74, 0xf9, 0xa9);
DEFINE_GUID(IID_IDebugDataSpaces,         0x88f7dfab, 0x3ea7, 0x4c3a, 0xae, 0xfb, 0xc4, 0xe8, 0x10, 0x61, 0x73, 0xaa);
DEFINE_GUID(IID_IDebugDataSpaces2,        0x7a5e852f, 0x96e9, 0x468f, 0xac, 0x1b, 0x0b, 0x3a, 0xdd, 0xc4, 0xa0, 0x49);
DEFINE_GUID(IID_IDebugSymbols,            0x8c31e98c, 0x983a, 0x48a5, 0x90, 0x16, 0x6f, 0xe5, 0xd6, 0x67, 0xa9, 0x50);
DEFINE_GUID(IID_IDebugSymbols2,           0x3a707211, 0xafdd, 0x4495, 0xad, 0x4f, 0x56, 0xfe, 0xcd, 0xf8, 0x16, 0x3f);
DEFINE_GUID(IID_IDebugSymbols3,           0xf02fbecc, 0x50ac, 0x4f36, 0x9a, 0xd9, 0xc9, 0x75, 0xe8, 0xf3, 0x2f, 0xf8);
DEFINE_GUID(IID_IDebugControl,            0x5182e668, 0x105e, 0x416e, 0xad, 0x92, 0x24, 0xef, 0x80, 0x04, 0x24, 0xba);
DEFINE_GUID(IID_IDebugControl2,           0xd4366723, 0x44df, 0x4bed, 0x8c, 0x7e, 0x4c, 0x05, 0x42, 0x4f, 0x45, 0x88);
DEFINE_GUID(IID_IDebugControl3,           0x7df74a86, 0xb03f, 0x407f, 0x90, 0xab, 0xa2, 0x0d, 0xad, 0xce, 0xad, 0x08);
DEFINE_GUID(IID_IDebugControl4,           0x94e60ce9, 0x9b41, 0x4b19, 0x9f, 0xc0, 0x6d, 0x9e, 0xb3, 0x52, 0x72, 0xb3);
DEFINE_GUID(IID_IDebugAdvanced,           0xf2df5f53, 0x071f, 0x47bd, 0x9d, 0xe6, 0x57, 0x34, 0xc3, 0xfe, 0xd6, 0x89);
DEFINE_GUID(IID_IDebugAdvanced2,          0x716d14c9, 0x119b, 0x4ba5, 0xaf, 0x1f, 0x08, 0x90, 0xe6, 0x72, 0x41, 0x6a);
DEFINE_GUID(IID_IDebugAdvanced3,          0xcba4abb4, 0x84c4, 0x444d, 0x87, 0xca, 0xa0, 0x4e, 0x13, 0x28, 0x67, 0x39);
DEFINE_GUID(IID_IDebugSystemObjects,      0x6b86fe2c, 0x2c4f, 0x4f0c, 0x9d, 0xa2, 0x17, 0x43, 0x11, 0xac, 0xc3, 0x27);
DEFINE_GUID(IID_IDebugSystemObjects2,     0x0ae9f5ff, 0x1852, 0x4679, 0xb0, 0x55, 0x49, 0x4b, 0xee, 0x64, 0x07, 0xee);
DEFINE_GUID(IID_IDebugSystemObjects3,     0xe9676e2f, 0xe286, 0x4ea3, 0xb0, 0xf9, 0xdf, 0xe5, 0xd9, 0xfc, 0x33, 0x0e);

/* Engine options */
#define DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION         0x00000001
#define DEBUG_ENGOPT_IGNORE_EXTENSION_VERSIONS      0x00000002
#define DEBUG_ENGOPT_ALLOW_NETWORK_PATHS            0x00000004
#define DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS         0x00000008
#define DEBUG_ENGOPT_IGNORE_LOADER_EXCEPTIONS       0x00000010
#define DEBUG_ENGOPT_INITIAL_BREAK                  0x00000020
#define DEBUG_ENGOPT_INITIAL_MODULE_BREAK           0x00000040
#define DEBUG_ENGOPT_FINAL_BREAK                    0x00000080
#define DEBUG_ENGOPT_NO_EXECUTE_REPEAT              0x00000100
#define DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION    0x00000200
#define DEBUG_ENGOPT_ALLOW_READ_ONLY_BREAKPOINTS    0x00000400
#define DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS        0x00000800
#define DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS        0x00001000
#define DEBUG_ENGOPT_KD_QUIET_MODE                  0x00002000
#define DEBUG_ENGOPT_DISABLE_MANAGED_SUPPORT        0x00004000
#define DEBUG_ENGOPT_DISABLE_MODULE_SYMBOL_LOAD     0x00008000
#define DEBUG_ENGOPT_DISABLE_EXECUTION_COMMANDS     0x00010000
#define DEBUG_ENGOPT_DISALLOW_IMAGE_FILE_MAPPING    0x00020000
#define DEBUG_ENGOPT_PREFER_DML                     0x00040000
#define DEBUG_ENGOPT_DISABLESQM                     0x00080000
#define DEBUG_ENGOPT_DISABLE_STEPLINES_OPTIONS      0x00200000
#define DEBUG_ENGOPT_DEBUGGING_SENSITIVE_DATA       0x00400000
#define DEBUG_ENGOPT_ALL                            0x004fffff
#define DEBUG_ENGOPT_NETWORK_PATHS (DEBUG_ENGOPT_ALLOW_NETWORK_PATHS | DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)

/* AttachProcess() flags */
#define DEBUG_ATTACH_DEFAULT                        0x00000000
#define DEBUG_ATTACH_NONINVASIVE                    0x00000001
#define DEBUG_ATTACH_EXISTING                       0x00000002
#define DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND         0x00000004
#define DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK      0x00000008
#define DEBUG_ATTACH_INVASIVE_RESUME_PROCESS        0x00000010
#define DEBUG_ATTACH_NONINVASIVE_ALLOW_PARTIAL      0x00000020

/* EndSession() flags */
#define DEBUG_END_PASSIVE                           0x00000000
#define DEBUG_END_ACTIVE_TERMINATE                  0x00000001
#define DEBUG_END_ACTIVE_DETACH                     0x00000002
#define DEBUG_END_REENTRANT                         0x00000003
#define DEBUG_END_DISCONNECT                        0x00000004

/* ChangeEngineState() flags */
#define DEBUG_CES_CURRENT_THREAD                    0x00000001
#define DEBUG_CES_EFFECTIVE_PROCESSOR               0x00000002
#define DEBUG_CES_BREAKPOINTS                       0x00000004
#define DEBUG_CES_CODE_LEVEL                        0x00000008
#define DEBUG_CES_EXECUTION_STATUS                  0x00000010
#define DEBUG_CES_ENGINE_OPTIONS                    0x00000020
#define DEBUG_CES_LOG_FILE                          0x00000040
#define DEBUG_CES_RADIX                             0x00000080
#define DEBUG_CES_EVENT_FILTERS                     0x00000100
#define DEBUG_CES_PROCESS_OPTIONS                   0x00000200
#define DEBUG_CES_EXTENSIONS                        0x00000400
#define DEBUG_CES_SYSTEMS                           0x00000800
#define DEBUG_CES_ASSEMBLY_OPTIONS                  0x00001000
#define DEBUG_CES_EXPRESSION_SYNTAX                 0x00002000
#define DEBUG_CES_TEXT_REPLACEMENTS                 0x00004000
#define DEBUG_CES_ALL                               0xffffffff

#define DEBUG_STATUS_NO_CHANGE                               0
#define DEBUG_STATUS_GO                                      1
#define DEBUG_STATUS_GO_HANDLED                              2
#define DEBUG_STATUS_GO_NOT_HANDLED                          3
#define DEBUG_STATUS_STEP_OVER                               4
#define DEBUG_STATUS_STEP_INTO                               5
#define DEBUG_STATUS_BREAK                                   6
#define DEBUG_STATUS_NO_DEBUGGEE                             7
#define DEBUG_STATUS_STEP_BRANCH                             8
#define DEBUG_STATUS_IGNORE_EVENT                            9
#define DEBUG_STATUS_RESTART_REQUESTED                      10
#define DEBUG_STATUS_REVERSE_GO                             11
#define DEBUG_STATUS_REVERSE_STEP_BRANCH                    12
#define DEBUG_STATUS_REVERSE_STEP_OVER                      13
#define DEBUG_STATUS_REVERSE_STEP_INTO                      14
#define DEBUG_STATUS_OUT_OF_SYNC                            15
#define DEBUG_STATUS_WAIT_INPUT                             16
#define DEBUG_STATUS_TIMEOUT                                17
#define DEBUG_STATUS_MASK                                 0x1f

/* ChangeSymbolState() flags */
#define DEBUG_CSS_LOADS                             0x00000001
#define DEBUG_CSS_UNLOADS                           0x00000002
#define DEBUG_CSS_SCOPE                             0x00000004
#define DEBUG_CSS_PATHS                             0x00000008
#define DEBUG_CSS_SYMBOL_OPTIONS                    0x00000010
#define DEBUG_CSS_TYPE_OPTIONS                      0x00000020
#define DEBUG_CSS_COLLAPSE_CHILDREN                 0x00000040
#define DEBUG_CSS_ALL                               0xffffffff

/* SessionStatus() flags */
#define DEBUG_SESSION_ACTIVE                        0x00000000
#define DEBUG_SESSION_END_SESSION_ACTIVE_TERMINATE  0x00000001
#define DEBUG_SESSION_END_SESSION_ACTIVE_DETACH     0x00000002
#define DEBUG_SESSION_END_SESSION_PASSIVE           0x00000003
#define DEBUG_SESSION_END                           0x00000004
#define DEBUG_SESSION_REBOOT                        0x00000005
#define DEBUG_SESSION_HIBERNATE                     0x00000006
#define DEBUG_SESSION_FAILURE                       0x00000007

/* ChangeDebuggeeState() flags */
#define DEBUG_CDS_REGISTERS                         0x00000001
#define DEBUG_CDS_DATA                              0x00000002
#define DEBUG_CDS_REFRESH                           0x00000004
#define DEBUG_CDS_ALL                               0xffffffff

#define DEBUG_CDS_REFRESH_EVALUATE                           1
#define DEBUG_CDS_REFRESH_EXECUTE                            2
#define DEBUG_CDS_REFRESH_EXECUTECOMMANDFILE                 3
#define DEBUG_CDS_REFRESH_ADDBREAKPOINT                      4
#define DEBUG_CDS_REFRESH_REMOVEBREAKPOINT                   5
#define DEBUG_CDS_REFRESH_WRITEVIRTUAL                       6
#define DEBUG_CDS_REFRESH_WRITEVIRTUALUNCACHED               7
#define DEBUG_CDS_REFRESH_WRITEPHYSICAL                      8
#define DEBUG_CDS_REFRESH_WRITEPHYSICAL2                     9
#define DEBUG_CDS_REFRESH_SETVALUE                          10
#define DEBUG_CDS_REFRESH_SETVALUE2                         11
#define DEBUG_CDS_REFRESH_SETSCOPE                          12
#define DEBUG_CDS_REFRESH_SETSCOPEFRAMEBYINDEX              13
#define DEBUG_CDS_REFRESH_SETSCOPEFROMJITDEBUGINFO          14
#define DEBUG_CDS_REFRESH_SETSCOPEFROMSTOREDEVENT           15
#define DEBUG_CDS_REFRESH_INLINESTEP                        16
#define DEBUG_CDS_REFRESH_INLINESTEP_PSEUDO                 17

/* GetModuleNameString() indices */
#define DEBUG_MODNAME_IMAGE                                  0
#define DEBUG_MODNAME_MODULE                                 1
#define DEBUG_MODNAME_LOADED_IMAGE                           2
#define DEBUG_MODNAME_SYMBOL_FILE                            3
#define DEBUG_MODNAME_MAPPED_IMAGE                           4

#define DEBUG_CLASS_UNINITIALIZED                            0
#define DEBUG_CLASS_KERNEL                                   1
#define DEBUG_CLASS_USER_WINDOWS                             2
#define DEBUG_CLASS_IMAGE_FILE                               3

#define DEBUG_DUMP_SMALL                                  1024
#define DEBUG_DUMP_DEFAULT                                1025
#define DEBUG_DUMP_FULL                                   1026
#define DEBUG_DUMP_IMAGE_FILE                             1027
#define DEBUG_DUMP_TRACE_LOG                              1028
#define DEBUG_DUMP_WINDOWS_CE                             1029
#define DEBUG_DUMP_ACTIVE                                 1030

#define DEBUG_USER_WINDOWS_PROCESS          0
#define DEBUG_USER_WINDOWS_PROCESS_SERVER   1
#define DEBUG_USER_WINDOWS_IDNA             2
#define DEBUG_USER_WINDOWS_REPT             3
#define DEBUG_USER_WINDOWS_SMALL_DUMP       DEBUG_DUMP_SMALL
#define DEBUG_USER_WINDOWS_DUMP             DEBUG_DUMP_DEFAULT
#define DEBUG_USER_WINDOWS_DUMP_WINDOWS_CE  DEBUG_DUMP_WINDOWS_CE

#define DEBUG_INVALID_OFFSET                      ((ULONG64)-1)
#define DEBUG_ANY_ID                                0xffffffff

#ifndef __IDebugClient_FWD_DEFINED__
#define __IDebugClient_FWD_DEFINED__
typedef interface IDebugClient IDebugClient;
#ifdef __cplusplus
interface IDebugClient;
#endif /* __cplusplus */
#endif


typedef struct _DEBUG_MODULE_PARAMETERS
{
    ULONG64 Base;
    ULONG Size;
    ULONG TimeDateStamp;
    ULONG Checksum;
    ULONG Flags;
    ULONG SymbolType;
    ULONG ImageNameSize;
    ULONG ModuleNameSize;
    ULONG LoadedImageNameSize;
    ULONG SymbolFileNameSize;
    ULONG MappedImageNameSize;
    ULONG64 Reserved[2];
} DEBUG_MODULE_PARAMETERS, *PDEBUG_MODULE_PARAMETERS;

typedef struct _DEBUG_SYMBOL_PARAMETERS
{
    ULONG64 Module;
    ULONG   TypeId;
    ULONG   ParentSymbol;
    ULONG   SubElements;
    ULONG   Flags;
    ULONG64 Reserved;
} DEBUG_SYMBOL_PARAMETERS, *PDEBUG_SYMBOL_PARAMETERS;

typedef struct _DEBUG_STACK_FRAME
{
    ULONG64 InstructionOffset;
    ULONG64 ReturnOffset;
    ULONG64 FrameOffset;
    ULONG64 StackOffset;
    ULONG64 FuncTableEntry;
    ULONG64 Params[4];
    ULONG64 Reserved[6];
    BOOL Virtual;
    ULONG FrameNumber;
} DEBUG_STACK_FRAME, *PDEBUG_STACK_FRAME;

typedef struct _DEBUG_VALUE
{
    union
    {
        UCHAR I8;
        USHORT I16;
        ULONG I32;
        struct
        {
            ULONG64 I64;
            BOOL Nat;
        };
        float F32;
        double F64;
        UCHAR F80Bytes[10];
        UCHAR F82Bytes[11];
        UCHAR F128Bytes[16];
        UCHAR VI8[16];
        USHORT VI16[8];
        ULONG VI32[4];
        ULONG64 VI64[2];
        float VF32[4];
        double VF64[2];
        struct
        {
            ULONG LowPart;
            ULONG HighPart;
        } I64Parts32;
        struct
        {
            ULONG64 LowPart;
            LONG64 HighPart;
        } F128Parts64;
        UCHAR RawBytes[24];
    };
    ULONG TailOfRawBytes;
    ULONG Type;
} DEBUG_VALUE, *PDEBUG_VALUE;

typedef struct _DEBUG_BREAKPOINT_PARAMETERS
{
    ULONG64 Offset;
    ULONG Id;
    ULONG BreakType;
    ULONG ProcType;
    ULONG Flags;
    ULONG DataSize;
    ULONG DataAccessType;
    ULONG PassCount;
    ULONG CurrentPassCount;
    ULONG MatchThread;
    ULONG CommandSize;
    ULONG OffsetExpressionSize;
} DEBUG_BREAKPOINT_PARAMETERS, *PDEBUG_BREAKPOINT_PARAMETERS;

typedef struct _WINDBG_EXTENSION_APIS32 *PWINDBG_EXTENSION_APIS32;
typedef struct _WINDBG_EXTENSION_APIS64 *PWINDBG_EXTENSION_APIS64;
typedef struct _MEMORY_BASIC_INFORMATION64 *PMEMORY_BASIC_INFORMATION64;

typedef struct _DEBUG_SPECIFIC_FILTER_PARAMETERS
{
    ULONG ExecutionOption;
    ULONG ContinueOption;
    ULONG TextSize;
    ULONG CommandSize;
    ULONG ArgumentSize;
} DEBUG_SPECIFIC_FILTER_PARAMETERS, *PDEBUG_SPECIFIC_FILTER_PARAMETERS;

typedef struct _DEBUG_EXCEPTION_FILTER_PARAMETERS
{
    ULONG ExecutionOption;
    ULONG ContinueOption;
    ULONG TextSize;
    ULONG CommandSize;
    ULONG SecondCommandSize;
    ULONG ExceptionCode;
} DEBUG_EXCEPTION_FILTER_PARAMETERS, *PDEBUG_EXCEPTION_FILTER_PARAMETERS;

typedef struct _DEBUG_MODULE_AND_ID
{
    ULONG64 ModuleBase;
    ULONG64 Id;
} DEBUG_MODULE_AND_ID, *PDEBUG_MODULE_AND_ID;

typedef struct _DEBUG_OFFSET_REGION
{
    ULONG64 Base;
    ULONG64 Size;
} DEBUG_OFFSET_REGION, *PDEBUG_OFFSET_REGION;

typedef struct _DEBUG_SYMBOL_ENTRY
{
    ULONG64 ModuleBase;
    ULONG64 Offset;
    ULONG64 Id;
    ULONG64 Arg64;
    ULONG Size;
    ULONG Flags;
    ULONG TypeId;
    ULONG NameSize;
    ULONG Token;
    ULONG Tag;
    ULONG Arg32;
    ULONG Reserved;
} DEBUG_SYMBOL_ENTRY, *PDEBUG_SYMBOL_ENTRY;

typedef struct _DEBUG_SYMBOL_SOURCE_ENTRY
{
    ULONG64 ModuleBase;
    ULONG64 Offset;
    ULONG64 FileNameId;
    ULONG64 EngineInternal;
    ULONG Size;
    ULONG Flags;
    ULONG FileNameSize;
    ULONG StartLine;
    ULONG EndLine;
    ULONG StartColumn;
    ULONG EndColumn;
    ULONG Reserved;
} DEBUG_SYMBOL_SOURCE_ENTRY, *PDEBUG_SYMBOL_SOURCE_ENTRY;

#define INTERFACE IDebugBreakpoint
DECLARE_INTERFACE_(IDebugBreakpoint, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugBreakpoint */
    STDMETHOD(GetId)(THIS_ PULONG id) PURE;
    STDMETHOD(GetType)(THIS_ PULONG breaktype, PULONG proc) PURE;
    STDMETHOD(GetAdder)(THIS_ IDebugClient **adder) PURE;
    STDMETHOD(GetFlags)(THIS_ PULONG flags) PURE;
    STDMETHOD(AddFlags)(THIS_ ULONG flags) PURE;
    STDMETHOD(RemoveFlags)(THIS_ ULONG flags) PURE;
    STDMETHOD(SetFlags)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetOffset)(THIS_ PULONG64 offset) PURE;
    STDMETHOD(SetOffset)(THIS_ ULONG64 offset) PURE;
    STDMETHOD(GetDataParameters)(THIS_ PULONG size, PULONG access) PURE;
    STDMETHOD(SetDataParameters)(THIS_ ULONG size, ULONG access) PURE;
    STDMETHOD(GetPassCount)(THIS_ PULONG count) PURE;
    STDMETHOD(SetPassCount)(THIS_ ULONG count) PURE;
    STDMETHOD(GetCurrentPassCount)(THIS_ PULONG count) PURE;
    STDMETHOD(GetMatchThreadId)(THIS_ PULONG id) PURE;
    STDMETHOD(SetMatchThreadId)(THIS_ ULONG thread) PURE;
    STDMETHOD(GetCommand)(THIS_ PSTR buffer, ULONG buf_size, PULONG command) PURE;
    STDMETHOD(SetCommand)(THIS_ PCSTR command) PURE;
    STDMETHOD(GetOffsetExpression)(THIS_ PSTR buffer, ULONG buf_size, PULONG expression) PURE;
    STDMETHOD(SetOffsetExpression)(THIS_ PCSTR expression) PURE;
    STDMETHOD(GetParameters)(THIS_ PDEBUG_BREAKPOINT_PARAMETERS params) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugBreakpoint2
DECLARE_INTERFACE_(IDebugBreakpoint2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugBreakpoint */
    STDMETHOD(GetId)(THIS_ PULONG id) PURE;
    STDMETHOD(GetType)(THIS_ PULONG breaktype, PULONG proc) PURE;
    STDMETHOD(GetAdder)(THIS_ IDebugClient **adder) PURE;
    STDMETHOD(GetFlags)(THIS_ PULONG flags) PURE;
    STDMETHOD(AddFlags)(THIS_ ULONG flags) PURE;
    STDMETHOD(RemoveFlags)(THIS_ ULONG flags) PURE;
    STDMETHOD(SetFlags)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetOffset)(THIS_ PULONG64 offset) PURE;
    STDMETHOD(SetOffset)(THIS_ ULONG64 offset) PURE;
    STDMETHOD(GetDataParameters)(THIS_ PULONG size, PULONG access) PURE;
    STDMETHOD(SetDataParameters)(THIS_ ULONG size, ULONG access) PURE;
    STDMETHOD(GetPassCount)(THIS_ PULONG count) PURE;
    STDMETHOD(SetPassCount)(THIS_ ULONG count) PURE;
    STDMETHOD(GetCurrentPassCount)(THIS_ PULONG count) PURE;
    STDMETHOD(GetMatchThreadId)(THIS_ PULONG id) PURE;
    STDMETHOD(SetMatchThreadId)(THIS_ ULONG thread) PURE;
    STDMETHOD(GetCommand)(THIS_ PSTR buffer, ULONG buf_size, PULONG command) PURE;
    STDMETHOD(SetCommand)(THIS_ PCSTR command) PURE;
    STDMETHOD(GetOffsetExpression)(THIS_ PSTR buffer, ULONG buf_size, PULONG expression) PURE;
    STDMETHOD(SetOffsetExpression)(THIS_ PCSTR expression) PURE;
    STDMETHOD(GetParameters)(THIS_ PDEBUG_BREAKPOINT_PARAMETERS params) PURE;
    /* IDebugBreakpoint2. */
    STDMETHOD(GetCommandWide)(THIS_ PWSTR buffer, ULONG buf_size, PULONG command) PURE;
    STDMETHOD(SetCommandWide)(THIS_ PCWSTR command) PURE;
    STDMETHOD(GetOffsetExpressionWide)(THIS_ PWSTR buffer, ULONG buf_size, PULONG expression) PURE;
    STDMETHOD(SetOffsetExpressionWide)(THIS_ PCWSTR expression) PURE;
};
#undef INTERFACE


#define INTERFACE IDebugSymbolGroup
DECLARE_INTERFACE_(IDebugSymbolGroup, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSymbolGroup */
    STDMETHOD(GetNumberSymbols)(THIS_ PULONG number) PURE;
    STDMETHOD(AddSymbol)(THIS_ PCSTR name, PULONG index) PURE;
    STDMETHOD(RemoveSymbolByName)(THIS_ PCSTR name) PURE;
    STDMETHOD(RemoveSymbolByIndex)(THIS_ ULONG index) PURE;
    STDMETHOD(GetSymbolName)(THIS_ ULONG index, PSTR buffer, ULONG buf_size, PULONG name_size) PURE;
    STDMETHOD(GetSymbolParameters)(THIS_ ULONG start, ULONG count, PDEBUG_SYMBOL_PARAMETERS params) PURE;
    STDMETHOD(ExpandSymbol)(THIS_ ULONG index, BOOL expand) PURE;
    STDMETHOD(OutputSymbols)(THIS_ ULONG control, ULONG flags, ULONG start, ULONG count) PURE;
    STDMETHOD(WriteSymbol)(THIS_ ULONG index, PCSTR value) PURE;
    STDMETHOD(OutputAsType)(THIS_ ULONG index, PCSTR type) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugSymbolGroup2
DECLARE_INTERFACE_(IDebugSymbolGroup2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSymbolGroup */
    STDMETHOD(GetNumberSymbols)(THIS_ PULONG number) PURE;
    STDMETHOD(AddSymbol)(THIS_ PCSTR name, PULONG index) PURE;
    STDMETHOD(RemoveSymbolByName)(THIS_ PCSTR name) PURE;
    STDMETHOD(RemoveSymbolByIndex)(THIS_ ULONG index) PURE;
    STDMETHOD(GetSymbolName)(THIS_ ULONG index, PSTR buffer, ULONG buf_size, PULONG name_size) PURE;
    STDMETHOD(GetSymbolParameters)(THIS_ ULONG start, ULONG count, PDEBUG_SYMBOL_PARAMETERS params) PURE;
    STDMETHOD(ExpandSymbol)(THIS_ ULONG index, BOOL expand) PURE;
    STDMETHOD(OutputSymbols)(THIS_ ULONG control, ULONG flags, ULONG start, ULONG count) PURE;
    STDMETHOD(WriteSymbol)(THIS_ ULONG index, PCSTR value) PURE;
    STDMETHOD(OutputAsType)(THIS_ ULONG index, PCSTR type) PURE;
    /* IDebugSymbolGroup2 */
    STDMETHOD(AddSymbolWide)(THIS_ PCWSTR name, PULONG index) PURE;
    STDMETHOD(RemoveSymbolByNameWide)(THIS_ PCWSTR name) PURE;
    STDMETHOD(GetSymbolNameWide)(THIS_ ULONG index, PWSTR buffer, ULONG buf_size, PULONG size) PURE;
    STDMETHOD(WriteSymbolWide)(THIS_ ULONG index, PCWSTR value) PURE;
    STDMETHOD(OutputAsTypeWide)(THIS_ ULONG index, PCWSTR type) PURE;
    STDMETHOD(GetSymbolTypeName)(THIS_ ULONG index, PSTR buffer, ULONG buf_size, PULONG size) PURE;
    STDMETHOD(GetSymbolTypeNameWide)(THIS_ ULONG index, PWSTR buffer, ULONG buf_size, PULONG size) PURE;
    STDMETHOD(GetSymbolSize)(THIS_ ULONG index, PULONG size) PURE;
    STDMETHOD(GetSymbolOffset)(THIS_ ULONG index, PULONG64 offset) PURE;
    STDMETHOD(GetSymbolRegister)(THIS_ ULONG index, PULONG reg) PURE;
    STDMETHOD(GetSymbolValueText)(THIS_ ULONG index, PSTR buffer, ULONG buf_size, PULONG size) PURE;
    STDMETHOD(GetSymbolValueTextWide)(THIS_ ULONG index, PWSTR buffer, ULONG buf_size, PULONG size) PURE;
    STDMETHOD(GetSymbolEntryInformation)(THIS_ ULONG index, PDEBUG_SYMBOL_ENTRY entry) PURE;
};
#undef INTERFACE

typedef IDebugBreakpoint *PDEBUG_BREAKPOINT;
typedef IDebugBreakpoint2 *PDEBUG_BREAKPOINT2;
typedef IDebugSymbolGroup2 *PDEBUG_SYMBOL_GROUP2;

#define INTERFACE IDebugInputCallbacks
DECLARE_INTERFACE_(IDebugInputCallbacks, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugInputCallbacks */
    STDMETHOD(StartInput)(THIS_ ULONG buffer_size) PURE;
    STDMETHOD(EndInput)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugOutputCallbacks
DECLARE_INTERFACE_(IDebugOutputCallbacks, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugOutputCallbacks */
    STDMETHOD(Output)(THIS_ ULONG mask, const char *text) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugOutputCallbacksWide
DECLARE_INTERFACE_(IDebugOutputCallbacksWide, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugOutputCallbacksWide */
    STDMETHOD(Output)(THIS_ ULONG mask, const WCHAR *text) PURE;
};
#undef INTERFACE

#ifdef WINE_NO_UNICODE_MACROS
#undef CreateProcess
#endif

#define INTERFACE IDebugEventCallbacks
DECLARE_INTERFACE_(IDebugEventCallbacks, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugEventCallbacks */
    STDMETHOD(GetInterestMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(Breakpoint)(THIS_ PDEBUG_BREAKPOINT breakpoint) PURE;
    STDMETHOD(Exception)(THIS_ EXCEPTION_RECORD64 *exception, ULONG first_chance) PURE;
    STDMETHOD(CreateThread)(THIS_ ULONG64 handle, ULONG64 data_offset, ULONG64 start_offset) PURE;
    STDMETHOD(ExitThread)(THIS_ ULONG exit_code) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 image_handle, ULONG64 handle, ULONG64 base_offset, ULONG module_size,
            const char *module_name, const char *image_name, ULONG checksum, ULONG timedatestamp,
            ULONG64 initial_thread_handle, ULONG64 thread_data_offset, ULONG64 start_offset) PURE;
    STDMETHOD(ExitProcess)(THIS_ ULONG exit_code) PURE;
    STDMETHOD(LoadModule)(THIS_ ULONG64 image_handle, ULONG64 base_offset, ULONG module_size,  const char *module_name,
            const char *image_name, ULONG checksum, ULONG timedatestamp) PURE;
    STDMETHOD(UnloadModule)(THIS_ const char *image_basename, ULONG64 base_offset) PURE;
    STDMETHOD(SystemError)(THIS_ ULONG error, ULONG level) PURE;
    STDMETHOD(SessionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(ChangeDebuggeeState)(THIS_ ULONG flags, ULONG64 argument) PURE;
    STDMETHOD(ChangeEngineState)(THIS_ ULONG flags, ULONG64 argument) PURE;
    STDMETHOD(ChangeSymbolState)(THIS_ ULONG flags, ULONG64 argument) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugEventCallbacksWide
DECLARE_INTERFACE_(IDebugEventCallbacksWide, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDebugEventCallbacksWide */
    STDMETHOD(GetInterestMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(Breakpoint)(THIS_ PDEBUG_BREAKPOINT breakpoint) PURE;
    STDMETHOD(Exception)(THIS_ EXCEPTION_RECORD64 *exception, ULONG first_chance) PURE;
    STDMETHOD(CreateThread)(THIS_ ULONG64 handle, ULONG64 data_offset, ULONG64 start_offset) PURE;
    STDMETHOD(ExitThread)(THIS_ ULONG exit_code) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 image_handle, ULONG64 handle, ULONG64 base_offset, ULONG module_size,
            const WCHAR *module_name, const WCHAR *image_name, ULONG checksum, ULONG timedatestamp,
            ULONG64 initial_thread_handle, ULONG64 thread_data_offset, ULONG64 start_offset) PURE;
    STDMETHOD(ExitProcess)(THIS_ ULONG exit_code) PURE;
    STDMETHOD(LoadModule)(THIS_ ULONG64 image_handle, ULONG64 base_offset, ULONG module_size, const WCHAR *module_name,
            const WCHAR *image_name, ULONG checksum, ULONG timedatestamp) PURE;
    STDMETHOD(UnloadModule)(THIS_ const WCHAR *image_basename, ULONG64 base_offset) PURE;
    STDMETHOD(SystemError)(THIS_ ULONG error, ULONG level) PURE;
    STDMETHOD(SessionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(ChangeDebuggeeState)(THIS_ ULONG flags, ULONG64 argument) PURE;
    STDMETHOD(ChangeEngineState)(THIS_ ULONG flags, ULONG64 argument) PURE;
    STDMETHOD(ChangeSymbolState)(THIS_ ULONG flags, ULONG64 argument) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugEventContextCallbacks
DECLARE_INTERFACE_(IDebugEventContextCallbacks, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDebugEventContextCallbacks */
    STDMETHOD(GetInterestMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(Breakpoint)(THIS_ IDebugBreakpoint2 *bp, void *context, ULONG size) PURE;
    STDMETHOD(Exception)(THIS_ EXCEPTION_RECORD64 *exception, ULONG firstchance, void *context, ULONG size) PURE;
    STDMETHOD(CreateThread)(THIS_ ULONG64 handle, ULONG64 data_offset, ULONG64 start_offset, void *context, ULONG size) PURE;
    STDMETHOD(ExitThread)(THIS_ ULONG exitcode, void *context, ULONG size) PURE;

    STDMETHOD(CreateProcess)(THIS_ ULONG64 image_handle, ULONG64 handle, ULONG64 baseoffset, ULONG module_size,
                const WCHAR *module_name, const WCHAR image_name, ULONG checksum, ULONG timestamp, ULONG64 initial,
                ULONG64 thread_offset, ULONG64 start_offset, void *context, ULONG size) PURE;
    STDMETHOD(ExitProcess)(THIS_ ULONG exitcode, void *context, ULONG size) PURE;
    STDMETHOD(LoadModule)(THIS_ ULONG64 image_handle, ULONG64 baseoffset, ULONG module_size, const WCHAR *module_name,
                const WCHAR *image_name, ULONG checksum, ULONG timestamp, void *context, ULONG size) PURE;
    STDMETHOD(UnloadModule)(THIS_ const WCHAR *image_name, ULONG64 offset, void *context, ULONG size) PURE;
    STDMETHOD(SystemError)(THIS_ ULONG error, ULONG level, void *context, ULONG size) PURE;
    STDMETHOD(SessionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(ChangeDebuggeeState)(THIS_ ULONG flags, ULONG64 argument, void *context, ULONG size) PURE;
    STDMETHOD(ChangeEngineState)(THIS_ ULONG flags, ULONG64 argument, void *context, ULONG size) PURE;
    STDMETHOD(ChangeSymbolState)(THIS_ ULONG flags, ULONG64 argument) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient
DECLARE_INTERFACE_(IDebugClient, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient2
DECLARE_INTERFACE_(IDebugClient2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
    /* IDebugClient2 */
    STDMETHOD(WriteDumpFile2)(THIS_ const char *dumpfile, ULONG qualifier, ULONG flags, const char *comment) PURE;
    STDMETHOD(AddDumpInformationFile)(THIS_ const char *infofile, ULONG type) PURE;
    STDMETHOD(EndProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(WaitForProcessServerEnd)(THIS_ ULONG timeout) PURE;
    STDMETHOD(IsKernelDebuggerEnabled)(THIS) PURE;
    STDMETHOD(TerminateCurrentProcess)(THIS) PURE;
    STDMETHOD(DetachCurrentProcess)(THIS) PURE;
    STDMETHOD(AbandonCurrentProcess)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient3
DECLARE_INTERFACE_(IDebugClient3, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
    /* IDebugClient2 */
    STDMETHOD(WriteDumpFile2)(THIS_ const char *dumpfile, ULONG qualifier, ULONG flags, const char *comment) PURE;
    STDMETHOD(AddDumpInformationFile)(THIS_ const char *infofile, ULONG type) PURE;
    STDMETHOD(EndProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(WaitForProcessServerEnd)(THIS_ ULONG timeout) PURE;
    STDMETHOD(IsKernelDebuggerEnabled)(THIS) PURE;
    STDMETHOD(TerminateCurrentProcess)(THIS) PURE;
    STDMETHOD(DetachCurrentProcess)(THIS) PURE;
    STDMETHOD(AbandonCurrentProcess)(THIS) PURE;
    /* IDebugClient3 */
    STDMETHOD(GetRunningProcessSystemIdByExecutableNameWide)(THIS_ ULONG64 server, const WCHAR *exename, ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescriptionWide)(THIS_ ULONG64 server, ULONG id, ULONG flags, WCHAR *exename, ULONG size,
                ULONG *actualsize, WCHAR *description, ULONG desc_size, ULONG *actual_descr_size) PURE;
    STDMETHOD(CreateProcessWide)(THIS_ ULONG64 server, WCHAR *CommandLine, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttachWide)(THIS_ ULONG64 server, WCHAR *commandline, ULONG flags,
                ULONG processid,ULONG attachflags) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient4
DECLARE_INTERFACE_(IDebugClient4, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
    /* IDebugClient2 */
    STDMETHOD(WriteDumpFile2)(THIS_ const char *dumpfile, ULONG qualifier, ULONG flags, const char *comment) PURE;
    STDMETHOD(AddDumpInformationFile)(THIS_ const char *infofile, ULONG type) PURE;
    STDMETHOD(EndProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(WaitForProcessServerEnd)(THIS_ ULONG timeout) PURE;
    STDMETHOD(IsKernelDebuggerEnabled)(THIS) PURE;
    STDMETHOD(TerminateCurrentProcess)(THIS) PURE;
    STDMETHOD(DetachCurrentProcess)(THIS) PURE;
    STDMETHOD(AbandonCurrentProcess)(THIS) PURE;
    /* IDebugClient3 */
    STDMETHOD(GetRunningProcessSystemIdByExecutableNameWide)(THIS_ ULONG64 server, const WCHAR *exename, ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescriptionWide)(THIS_ ULONG64 server, ULONG id, ULONG flags, WCHAR *exename, ULONG size,
                ULONG *actualsize, WCHAR *description, ULONG desc_size, ULONG *actual_descr_size) PURE;
    STDMETHOD(CreateProcessWide)(THIS_ ULONG64 server, WCHAR *CommandLine, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttachWide)(THIS_ ULONG64 server, WCHAR *commandline, ULONG flags,
                ULONG processid,ULONG attachflags) PURE;
    /* IDebugClient4 */
    STDMETHOD(OpenDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle) PURE;
    STDMETHOD(WriteDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle, ULONG qualifier, ULONG Flags, const WCHAR *comment) PURE;
    STDMETHOD(AddDumpInformationFileWide)(THIS_ const WCHAR *filename,ULONG64 handle, ULONG type) PURE;
    STDMETHOD(GetNumberDumpFiles)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetDumpFile)(THIS_ ULONG index, char *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle, ULONG *type) PURE;
    STDMETHOD(GetDumpFileWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle,ULONG *type) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient5
DECLARE_INTERFACE_(IDebugClient5, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
    /* IDebugClient2 */
    STDMETHOD(WriteDumpFile2)(THIS_ const char *dumpfile, ULONG qualifier, ULONG flags, const char *comment) PURE;
    STDMETHOD(AddDumpInformationFile)(THIS_ const char *infofile, ULONG type) PURE;
    STDMETHOD(EndProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(WaitForProcessServerEnd)(THIS_ ULONG timeout) PURE;
    STDMETHOD(IsKernelDebuggerEnabled)(THIS) PURE;
    STDMETHOD(TerminateCurrentProcess)(THIS) PURE;
    STDMETHOD(DetachCurrentProcess)(THIS) PURE;
    STDMETHOD(AbandonCurrentProcess)(THIS) PURE;
    /* IDebugClient3 */
    STDMETHOD(GetRunningProcessSystemIdByExecutableNameWide)(THIS_ ULONG64 server, const WCHAR *exename, ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescriptionWide)(THIS_ ULONG64 server, ULONG id, ULONG flags, WCHAR *exename, ULONG size,
                ULONG *actualsize, WCHAR *description, ULONG desc_size, ULONG *actual_descr_size) PURE;
    STDMETHOD(CreateProcessWide)(THIS_ ULONG64 server, WCHAR *CommandLine, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttachWide)(THIS_ ULONG64 server, WCHAR *commandline, ULONG flags,
                ULONG processid,ULONG attachflags) PURE;
    /* IDebugClient4 */
    STDMETHOD(OpenDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle) PURE;
    STDMETHOD(WriteDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle, ULONG qualifier, ULONG Flags, const WCHAR *comment) PURE;
    STDMETHOD(AddDumpInformationFileWide)(THIS_ const WCHAR *filename,ULONG64 handle, ULONG type) PURE;
    STDMETHOD(GetNumberDumpFiles)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetDumpFile)(THIS_ ULONG index, char *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle, ULONG *type) PURE;
    STDMETHOD(GetDumpFileWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle,ULONG *type) PURE;
    /* IDebugClient5 */
    STDMETHOD(AttachKernelWide)(THIS_ ULONG flags, const WCHAR *options) PURE;
    STDMETHOD(GetKernelConnectionOptionsWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetKernelConnectionOptionsWide)(THIS_ const WCHAR *options) PURE;
    STDMETHOD(StartProcessServerWide)(THIS_ ULONG flags, const WCHAR *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServerWide)(THIS_ const WCHAR *options, ULONG64 *server) PURE;
    STDMETHOD(StartServerWide)(THIS_ const WCHAR *options) PURE;
    STDMETHOD(OutputServersWide)(THIS_ ULONG control, const WCHAR *machine, ULONG flags) PURE;
    STDMETHOD(GetOutputCallbacksWide)(THIS_ IDebugOutputCallbacksWide **callbacks) PURE;
    STDMETHOD(SetOutputCallbacksWide)(THIS_ IDebugOutputCallbacksWide *callbacks) PURE;
    STDMETHOD(GetOutputLinePrefixWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetOutputLinePrefixWide)(THIS_ const WCHAR *prefix) PURE;
    STDMETHOD(GetIdentityWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *identity) PURE;
    STDMETHOD(OutputIdentityWide)(THIS_ ULONG control, ULONG flags, const WCHAR *format) PURE;
    STDMETHOD(GetEventCallbacksWide)(THIS_ IDebugEventCallbacksWide **callbacks) PURE;
    STDMETHOD(SetEventCallbacksWide)(THIS_ IDebugEventCallbacksWide *callbacks) PURE;
    STDMETHOD(CreateProcess2)(THIS_ ULONG64 server, char *command, void *options, ULONG buf_size,
                const char *initial, const char *environment) PURE;
    STDMETHOD(CreateProcess2Wide)(THIS_ ULONG64 server, WCHAR *command, void *options, ULONG size,
                const WCHAR *initial, const WCHAR *environment) PURE;
    STDMETHOD(CreateProcessAndAttach2)(THIS_ ULONG64 server, char *command, void *options, ULONG buf_size,
                const char *initial, const char *environment, ULONG processid, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach2Wide)(THIS_ ULONG64 server, WCHAR *command, void *buffer, ULONG buf_size,
                const WCHAR *initial, const WCHAR *environment, ULONG processid, ULONG flags) PURE;
    STDMETHOD(PushOutputLinePrefix)(THIS_ const char *prefix, ULONG64 *handle) PURE;
    STDMETHOD(PushOutputLinePrefixWide)(THIS_ const WCHAR *prefix, ULONG64 *handle) PURE;
    STDMETHOD(PopOutputLinePrefix)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetNumberInputCallbacks)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberOutputCallbacks)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberEventCallbacks)(THIS_ ULONG flags, ULONG *count) PURE;
    STDMETHOD(GetQuitLockString)(THIS_ char *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetQuitLockString)(THIS_ char *string) PURE;
    STDMETHOD(GetQuitLockStringWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetQuitLockStringWide)(THIS_ const WCHAR *string) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient6
DECLARE_INTERFACE_(IDebugClient6, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
    /* IDebugClient2 */
    STDMETHOD(WriteDumpFile2)(THIS_ const char *dumpfile, ULONG qualifier, ULONG flags, const char *comment) PURE;
    STDMETHOD(AddDumpInformationFile)(THIS_ const char *infofile, ULONG type) PURE;
    STDMETHOD(EndProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(WaitForProcessServerEnd)(THIS_ ULONG timeout) PURE;
    STDMETHOD(IsKernelDebuggerEnabled)(THIS) PURE;
    STDMETHOD(TerminateCurrentProcess)(THIS) PURE;
    STDMETHOD(DetachCurrentProcess)(THIS) PURE;
    STDMETHOD(AbandonCurrentProcess)(THIS) PURE;
    /* IDebugClient3 */
    STDMETHOD(GetRunningProcessSystemIdByExecutableNameWide)(THIS_ ULONG64 server, const WCHAR *exename, ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescriptionWide)(THIS_ ULONG64 server, ULONG id, ULONG flags, WCHAR *exename, ULONG size,
                ULONG *actualsize, WCHAR *description, ULONG desc_size, ULONG *actual_descr_size) PURE;
    STDMETHOD(CreateProcessWide)(THIS_ ULONG64 server, WCHAR *CommandLine, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttachWide)(THIS_ ULONG64 server, WCHAR *commandline, ULONG flags,
                ULONG processid,ULONG attachflags) PURE;
    /* IDebugClient4 */
    STDMETHOD(OpenDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle) PURE;
    STDMETHOD(WriteDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle, ULONG qualifier, ULONG Flags, const WCHAR *comment) PURE;
    STDMETHOD(AddDumpInformationFileWide)(THIS_ const WCHAR *filename,ULONG64 handle, ULONG type) PURE;
    STDMETHOD(GetNumberDumpFiles)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetDumpFile)(THIS_ ULONG index, char *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle, ULONG *type) PURE;
    STDMETHOD(GetDumpFileWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle,ULONG *type) PURE;
    /* IDebugClient5 */
    STDMETHOD(AttachKernelWide)(THIS_ ULONG flags, const WCHAR *options) PURE;
    STDMETHOD(GetKernelConnectionOptionsWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetKernelConnectionOptionsWide)(THIS_ const WCHAR *options) PURE;
    STDMETHOD(StartProcessServerWide)(THIS_ ULONG flags, const WCHAR *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServerWide)(THIS_ const WCHAR *options, ULONG64 *server) PURE;
    STDMETHOD(StartServerWide)(THIS_ const WCHAR *options) PURE;
    STDMETHOD(OutputServersWide)(THIS_ ULONG control, const WCHAR *machine, ULONG flags) PURE;
    STDMETHOD(GetOutputCallbacksWide)(THIS_ IDebugOutputCallbacksWide **callbacks) PURE;
    STDMETHOD(SetOutputCallbacksWide)(THIS_ IDebugOutputCallbacksWide *callbacks) PURE;
    STDMETHOD(GetOutputLinePrefixWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetOutputLinePrefixWide)(THIS_ const WCHAR *prefix) PURE;
    STDMETHOD(GetIdentityWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *identity) PURE;
    STDMETHOD(OutputIdentityWide)(THIS_ ULONG control, ULONG flags, const WCHAR *format) PURE;
    STDMETHOD(GetEventCallbacksWide)(THIS_ IDebugEventCallbacksWide **callbacks) PURE;
    STDMETHOD(SetEventCallbacksWide)(THIS_ IDebugEventCallbacksWide *callbacks) PURE;
    STDMETHOD(CreateProcess2)(THIS_ ULONG64 server, char *command, void *options, ULONG buf_size,
                const char *initial, const char *environment) PURE;
    STDMETHOD(CreateProcess2Wide)(THIS_ ULONG64 server, WCHAR *command, void *options, ULONG size,
                const WCHAR *initial, const WCHAR *environment) PURE;
    STDMETHOD(CreateProcessAndAttach2)(THIS_ ULONG64 server, char *command, void *options, ULONG buf_size,
                const char *initial, const char *environment, ULONG processid, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach2Wide)(THIS_ ULONG64 server, WCHAR *command, void *buffer, ULONG buf_size,
                const WCHAR *initial, const WCHAR *environment, ULONG processid, ULONG flags) PURE;
    STDMETHOD(PushOutputLinePrefix)(THIS_ const char *prefix, ULONG64 *handle) PURE;
    STDMETHOD(PushOutputLinePrefixWide)(THIS_ const WCHAR *prefix, ULONG64 *handle) PURE;
    STDMETHOD(PopOutputLinePrefix)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetNumberInputCallbacks)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberOutputCallbacks)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberEventCallbacks)(THIS_ ULONG flags, ULONG *count) PURE;
    STDMETHOD(GetQuitLockString)(THIS_ char *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetQuitLockString)(THIS_ char *string) PURE;
    STDMETHOD(GetQuitLockStringWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetQuitLockStringWide)(THIS_ const WCHAR *string) PURE;
    /* IDebugClient6 */
    STDMETHOD(SetEventContextCallbacks)(THIS_ IDebugEventContextCallbacks *callbacks) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugClient7
DECLARE_INTERFACE_(IDebugClient7, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugClient */
    STDMETHOD(AttachKernel)(THIS_ ULONG flags, const char *options) PURE;
    STDMETHOD(GetKernelConnectionOptions)(THIS_ char *buffer, ULONG buffer_size, ULONG *options_size) PURE;
    STDMETHOD(SetKernelConnectionOptions)(THIS_ const char *options) PURE;
    STDMETHOD(StartProcessServer)(THIS_ ULONG flags, const char *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServer)(THIS_ const char *remote_options, ULONG64 *server) PURE;
    STDMETHOD(DisconnectProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(GetRunningProcessSystemIds)(THIS_ ULONG64 server, ULONG *ids, ULONG count, ULONG *actual_count) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(THIS_ ULONG64 server, const char *exe_name,
            ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescription)(THIS_ ULONG64 server, ULONG systemid, ULONG flags, char *exe_name,
            ULONG exe_name_size, ULONG *actual_exe_name_size, char *description, ULONG description_size,
            ULONG *actual_description_size) PURE;
    STDMETHOD(AttachProcess)(THIS_ ULONG64 server, ULONG pid, ULONG flags) PURE;
    STDMETHOD(CreateProcess)(THIS_ ULONG64 server, char *cmdline, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach)(THIS_ ULONG64 server, char *cmdline, ULONG create_flags,
            ULONG pid, ULONG attach_flags) PURE;
    STDMETHOD(GetProcessOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetProcessOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(OpenDumpFile)(THIS_ const char *filename) PURE;
    STDMETHOD(WriteDumpFile)(THIS_ const char *filename, ULONG qualifier) PURE;
    STDMETHOD(ConnectSession)(THIS_ ULONG flags, ULONG history_limit) PURE;
    STDMETHOD(StartServer)(THIS_ const char *options) PURE;
    STDMETHOD(OutputServers)(THIS_ ULONG output_control, const char *machine, ULONG flags) PURE;
    STDMETHOD(TerminateProcesses)(THIS) PURE;
    STDMETHOD(DetachProcesses)(THIS) PURE;
    STDMETHOD(EndSession)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetExitCode)(THIS_ ULONG *code) PURE;
    STDMETHOD(DispatchCallbacks)(THIS_ ULONG timeout) PURE;
    STDMETHOD(ExitDispatch)(THIS_ IDebugClient *client) PURE;
    STDMETHOD(CreateClient)(THIS_ IDebugClient **client) PURE;
    STDMETHOD(GetInputCallbacks)(THIS_ IDebugInputCallbacks **callbacks) PURE;
    STDMETHOD(SetInputCallbacks)(THIS_ IDebugInputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputCallbacks)(THIS_ IDebugOutputCallbacks **callbacks) PURE;
    STDMETHOD(SetOutputCallbacks)(THIS_ IDebugOutputCallbacks *callbacks) PURE;
    STDMETHOD(GetOutputMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetOutputMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetOtherOutputMask)(THIS_ IDebugClient *client, ULONG *mask) PURE;
    STDMETHOD(SetOtherOutputMask)(THIS_ IDebugClient *client, ULONG mask) PURE;
    STDMETHOD(GetOutputWidth)(THIS_ ULONG *columns) PURE;
    STDMETHOD(SetOutputWidth)(THIS_ ULONG columns) PURE;
    STDMETHOD(GetOutputLinePrefix)(THIS_ char *buffer, ULONG buffer_size, ULONG *prefix_size) PURE;
    STDMETHOD(SetOutputLinePrefix)(THIS_ const char *prefix) PURE;
    STDMETHOD(GetIdentity)(THIS_ char *buffer, ULONG buffer_size, ULONG *identity_size) PURE;
    STDMETHOD(OutputIdentity)(THIS_ ULONG output_control, ULONG flags, const char *format) PURE;
    STDMETHOD(GetEventCallbacks)(THIS_ IDebugEventCallbacks **callbacks) PURE;
    STDMETHOD(SetEventCallbacks)(THIS_ IDebugEventCallbacks *callbacks) PURE;
    STDMETHOD(FlushCallbacks)(THIS) PURE;
    /* IDebugClient2 */
    STDMETHOD(WriteDumpFile2)(THIS_ const char *dumpfile, ULONG qualifier, ULONG flags, const char *comment) PURE;
    STDMETHOD(AddDumpInformationFile)(THIS_ const char *infofile, ULONG type) PURE;
    STDMETHOD(EndProcessServer)(THIS_ ULONG64 server) PURE;
    STDMETHOD(WaitForProcessServerEnd)(THIS_ ULONG timeout) PURE;
    STDMETHOD(IsKernelDebuggerEnabled)(THIS) PURE;
    STDMETHOD(TerminateCurrentProcess)(THIS) PURE;
    STDMETHOD(DetachCurrentProcess)(THIS) PURE;
    STDMETHOD(AbandonCurrentProcess)(THIS) PURE;
    /* IDebugClient3 */
    STDMETHOD(GetRunningProcessSystemIdByExecutableNameWide)(THIS_ ULONG64 server, const WCHAR *exename, ULONG flags, ULONG *id) PURE;
    STDMETHOD(GetRunningProcessDescriptionWide)(THIS_ ULONG64 server, ULONG id, ULONG flags, WCHAR *exename, ULONG size,
                ULONG *actualsize, WCHAR *description, ULONG desc_size, ULONG *actual_descr_size) PURE;
    STDMETHOD(CreateProcessWide)(THIS_ ULONG64 server, WCHAR *CommandLine, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttachWide)(THIS_ ULONG64 server, WCHAR *commandline, ULONG flags,
                ULONG processid,ULONG attachflags) PURE;
    /* IDebugClient4 */
    STDMETHOD(OpenDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle) PURE;
    STDMETHOD(WriteDumpFileWide)(THIS_ const WCHAR *filename, ULONG64 handle, ULONG qualifier, ULONG Flags, const WCHAR *comment) PURE;
    STDMETHOD(AddDumpInformationFileWide)(THIS_ const WCHAR *filename,ULONG64 handle, ULONG type) PURE;
    STDMETHOD(GetNumberDumpFiles)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetDumpFile)(THIS_ ULONG index, char *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle, ULONG *type) PURE;
    STDMETHOD(GetDumpFileWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buf_size, ULONG *name_size, ULONG64 *handle,ULONG *type) PURE;
    /* IDebugClient5 */
    STDMETHOD(AttachKernelWide)(THIS_ ULONG flags, const WCHAR *options) PURE;
    STDMETHOD(GetKernelConnectionOptionsWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetKernelConnectionOptionsWide)(THIS_ const WCHAR *options) PURE;
    STDMETHOD(StartProcessServerWide)(THIS_ ULONG flags, const WCHAR *options, void *reserved) PURE;
    STDMETHOD(ConnectProcessServerWide)(THIS_ const WCHAR *options, ULONG64 *server) PURE;
    STDMETHOD(StartServerWide)(THIS_ const WCHAR *options) PURE;
    STDMETHOD(OutputServersWide)(THIS_ ULONG control, const WCHAR *machine, ULONG flags) PURE;
    STDMETHOD(GetOutputCallbacksWide)(THIS_ IDebugOutputCallbacksWide **callbacks) PURE;
    STDMETHOD(SetOutputCallbacksWide)(THIS_ IDebugOutputCallbacksWide *callbacks) PURE;
    STDMETHOD(GetOutputLinePrefixWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetOutputLinePrefixWide)(THIS_ const WCHAR *prefix) PURE;
    STDMETHOD(GetIdentityWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *identity) PURE;
    STDMETHOD(OutputIdentityWide)(THIS_ ULONG control, ULONG flags, const WCHAR *format) PURE;
    STDMETHOD(GetEventCallbacksWide)(THIS_ IDebugEventCallbacksWide **callbacks) PURE;
    STDMETHOD(SetEventCallbacksWide)(THIS_ IDebugEventCallbacksWide *callbacks) PURE;
    STDMETHOD(CreateProcess2)(THIS_ ULONG64 server, char *command, void *options, ULONG buf_size,
                const char *initial, const char *environment) PURE;
    STDMETHOD(CreateProcess2Wide)(THIS_ ULONG64 server, WCHAR *command, void *options, ULONG size,
                const WCHAR *initial, const WCHAR *environment) PURE;
    STDMETHOD(CreateProcessAndAttach2)(THIS_ ULONG64 server, char *command, void *options, ULONG buf_size,
                const char *initial, const char *environment, ULONG processid, ULONG flags) PURE;
    STDMETHOD(CreateProcessAndAttach2Wide)(THIS_ ULONG64 server, WCHAR *command, void *buffer, ULONG buf_size,
                const WCHAR *initial, const WCHAR *environment, ULONG processid, ULONG flags) PURE;
    STDMETHOD(PushOutputLinePrefix)(THIS_ const char *prefix, ULONG64 *handle) PURE;
    STDMETHOD(PushOutputLinePrefixWide)(THIS_ const WCHAR *prefix, ULONG64 *handle) PURE;
    STDMETHOD(PopOutputLinePrefix)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetNumberInputCallbacks)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberOutputCallbacks)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberEventCallbacks)(THIS_ ULONG flags, ULONG *count) PURE;
    STDMETHOD(GetQuitLockString)(THIS_ char *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetQuitLockString)(THIS_ char *string) PURE;
    STDMETHOD(GetQuitLockStringWide)(THIS_ WCHAR *buffer, ULONG buf_size, ULONG *size) PURE;
    STDMETHOD(SetQuitLockStringWide)(THIS_ const WCHAR *string) PURE;
    /* IDebugClient6 */
    STDMETHOD(SetEventContextCallbacks)(THIS_ IDebugEventContextCallbacks *callbacks) PURE;
    /* IDebugClient7 */
    STDMETHOD(SetClientContext)(THIS_ void *context, ULONG size) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugDataSpaces
DECLARE_INTERFACE_(IDebugDataSpaces, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugDataSpaces */
    STDMETHOD(ReadVirtual)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteVirtual)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(SearchVirtual)(THIS_ ULONG64 offset, ULONG64 length, void *pattern, ULONG pattern_size,
            ULONG pattern_granularity, ULONG64 *ret_offset) PURE;
    STDMETHOD(ReadVirtualUncached)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteVirtualUncached)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadPointersVirtual)(THIS_ ULONG count, ULONG64 offset, ULONG64 *pointers) PURE;
    STDMETHOD(WritePointersVirtual)(THIS_ ULONG count, ULONG64 offset, ULONG64 *pointers) PURE;
    STDMETHOD(ReadPhysical)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WritePhysical)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadControl)(THIS_ ULONG processor, ULONG64 offset, void *buffer, ULONG buffer_size,
            ULONG *read_len) PURE;
    STDMETHOD(WriteControl)(THIS_ ULONG processor, ULONG64 offset, void *buffer, ULONG buffer_size,
            ULONG *written) PURE;
    STDMETHOD(ReadIo)(THIS_ ULONG type, ULONG bus_number, ULONG address_space, ULONG64 offset, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteIo)(THIS_ ULONG type, ULONG bus_number, ULONG address_space, ULONG64 offset, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadMsr)(THIS_ ULONG msr, ULONG64 *value) PURE;
    STDMETHOD(WriteMsr)(THIS_ ULONG msr, ULONG64 value) PURE;
    STDMETHOD(ReadBusData)(THIS_ ULONG data_type, ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteBusData)(THIS_ ULONG data_type, ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(CheckLowMemory)(THIS) PURE;
    STDMETHOD(ReadDebuggerData)(THIS_ ULONG index, void *buffer, ULONG buffer_size, ULONG *data_size) PURE;
    STDMETHOD(ReadProcessorSystemData)(THIS_ ULONG processor, ULONG index, void *buffer, ULONG buffer_size,
            ULONG *data_size) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugDataSpaces2
DECLARE_INTERFACE_(IDebugDataSpaces2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugDataSpaces */
    STDMETHOD(ReadVirtual)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteVirtual)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(SearchVirtual)(THIS_ ULONG64 offset, ULONG64 length, void *pattern, ULONG pattern_size,
            ULONG pattern_granularity, ULONG64 *ret_offset) PURE;
    STDMETHOD(ReadVirtualUncached)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteVirtualUncached)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadPointersVirtual)(THIS_ ULONG count, ULONG64 offset, ULONG64 *pointers) PURE;
    STDMETHOD(WritePointersVirtual)(THIS_ ULONG count, ULONG64 offset, ULONG64 *pointers) PURE;
    STDMETHOD(ReadPhysical)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WritePhysical)(THIS_ ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadControl)(THIS_ ULONG processor, ULONG64 offset, void *buffer, ULONG buffer_size,
            ULONG *read_len) PURE;
    STDMETHOD(WriteControl)(THIS_ ULONG processor, ULONG64 offset, void *buffer, ULONG buffer_size,
            ULONG *written) PURE;
    STDMETHOD(ReadIo)(THIS_ ULONG type, ULONG bus_number, ULONG address_space, ULONG64 offset, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteIo)(THIS_ ULONG type, ULONG bus_number, ULONG address_space, ULONG64 offset, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(ReadMsr)(THIS_ ULONG msr, ULONG64 *value) PURE;
    STDMETHOD(WriteMsr)(THIS_ ULONG msr, ULONG64 value) PURE;
    STDMETHOD(ReadBusData)(THIS_ ULONG data_type, ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteBusData)(THIS_ ULONG data_type, ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(CheckLowMemory)(THIS) PURE;
    STDMETHOD(ReadDebuggerData)(THIS_ ULONG index, void *buffer, ULONG buffer_size, ULONG *data_size) PURE;
    STDMETHOD(ReadProcessorSystemData)(THIS_ ULONG processor, ULONG index, void *buffer, ULONG buffer_size,
            ULONG *data_size) PURE;
    /* IDebugDataSpaces2 */
    STDMETHOD(VirtualToPhysical)(THIS_ ULONG64 virt, ULONG64 *physical) PURE;
    STDMETHOD(GetVirtualTranslationPhysicalOffsets)(THIS_ ULONG64 virt, ULONG64 *offsets, ULONG offsets_size,
            ULONG *levels) PURE;
    STDMETHOD(ReadHandleData)(THIS_ ULONG64 handle, ULONG datatype, void *buffer, ULONG buffer_size,
            ULONG *data_size) PURE;
    STDMETHOD(FillVirtual)(THIS_ ULONG64 start, ULONG size, void *pattern, ULONG pattern_size, ULONG *filled) PURE;
    STDMETHOD(FillPhysical)(THIS_ ULONG64 start, ULONG size, void *pattern, ULONG pattern_size, ULONG *filled) PURE;
    STDMETHOD(QueryVirtual)(THIS_ ULONG64 offset, PMEMORY_BASIC_INFORMATION64 info) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugSymbols
DECLARE_INTERFACE_(IDebugSymbols, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSymbols */
    STDMETHOD(GetSymbolOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetNameByOffset)(THIS_ ULONG64 offset, char *buffer, ULONG buffer_size, ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByName)(THIS_ const char *symbol, ULONG64 *offset) PURE;
    STDMETHOD(GetNearNameByOffset)(THIS_ ULONG64 offset, LONG delta, char *buffer, ULONG buffer_size,ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetLineByOffset)(THIS_ ULONG64 offset, ULONG *line, char *buffer, ULONG buffer_size, ULONG *file_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByLine)(THIS_ ULONG line, const char *file, ULONG64 *offset) PURE;
    STDMETHOD(GetNumberModules)(THIS_ ULONG *loaded, ULONG *unloaded) PURE;
    STDMETHOD(GetModuleByIndex)(THIS_ ULONG index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByModuleName)(THIS_ const char *name, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByOffset)(THIS_ ULONG64 offset, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleNames)(THIS_ ULONG index, ULONG64 base, char *image_name, ULONG image_name_buffer_size,
            ULONG *image_name_size, char *module_name, ULONG module_name_buffer_size, ULONG *module_name_size,
            char *loaded_image_name, ULONG loaded_image_name_buffer_size, ULONG *loaded_image_size) PURE;
    STDMETHOD(GetModuleParameters)(THIS_ ULONG count, ULONG64 *bases, ULONG start,
            DEBUG_MODULE_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSymbolModule)(THIS_ const char *symbol, ULONG64 *base) PURE;
    STDMETHOD(GetTypeName)(THIS_ ULONG64 base, ULONG type_id, char *buffer, ULONG buffer_size, ULONG *name_size) PURE;
    STDMETHOD(GetTypeId)(THIS_ ULONG64 module, const char *name, ULONG *type_id) PURE;
    STDMETHOD(GetTypeSize)(THIS_ ULONG64 module, ULONG type_id, ULONG *size) PURE;
    STDMETHOD(GetFieldOffset)(THIS_ ULONG64 base, ULONG type_id, const char *field, ULONG *offset) PURE;
    STDMETHOD(GetSymbolTypeId)(THIS_ const char *symbol, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(GetOffsetTypeId)(THIS_ ULONG64 offset, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(ReadTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataVirtual)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(ReadTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataPhysical)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(GetScope)(THIS_ ULONG64 *instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(SetScope)(THIS_ ULONG64 instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(ResetScope)(THIS) PURE;
    STDMETHOD(GetScopeSymbolGroup)(THIS_ ULONG flags, IDebugSymbolGroup *update, IDebugSymbolGroup **symbols) PURE;
    STDMETHOD(CreateSymbolGroup)(THIS_ IDebugSymbolGroup **group) PURE;
    STDMETHOD(StartSymbolMatch)(THIS_ const char *pattern, ULONG64 *handle) PURE;
    STDMETHOD(GetNextSymbolMatch)(THIS_ ULONG64 handle, char *buffer, ULONG buffer_size, ULONG *match_size,
            ULONG64 *offset) PURE;
    STDMETHOD(EndSymbolMatch)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Reload)(THIS_ const char *path) PURE;
    STDMETHOD(GetSymbolPath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(GetImagePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(GetSourcePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(GetSourcePathElement)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *element_size) PURE;
    STDMETHOD(SetSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(FindSourceFile)(THIS_ ULONG start, const char *file, ULONG flags, ULONG *found_element, char *buffer,
            ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSourceFileLineOffsets)(THIS_ const char *file, ULONG64 *buffer, ULONG buffer_lines,
            ULONG *file_lines) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugSymbols2
DECLARE_INTERFACE_(IDebugSymbols2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSymbols */
    STDMETHOD(GetSymbolOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetNameByOffset)(THIS_ ULONG64 offset, char *buffer, ULONG buffer_size, ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByName)(THIS_ const char *symbol, ULONG64 *offset) PURE;
    STDMETHOD(GetNearNameByOffset)(THIS_ ULONG64 offset, LONG delta, char *buffer, ULONG buffer_size,ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetLineByOffset)(THIS_ ULONG64 offset, ULONG *line, char *buffer, ULONG buffer_size, ULONG *file_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByLine)(THIS_ ULONG line, const char *file, ULONG64 *offset) PURE;
    STDMETHOD(GetNumberModules)(THIS_ ULONG *loaded, ULONG *unloaded) PURE;
    STDMETHOD(GetModuleByIndex)(THIS_ ULONG index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByModuleName)(THIS_ const char *name, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByOffset)(THIS_ ULONG64 offset, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleNames)(THIS_ ULONG index, ULONG64 base, char *image_name, ULONG image_name_buffer_size,
            ULONG *image_name_size, char *module_name, ULONG module_name_buffer_size, ULONG *module_name_size,
            char *loaded_image_name, ULONG loaded_image_name_buffer_size, ULONG *loaded_image_size) PURE;
    STDMETHOD(GetModuleParameters)(THIS_ ULONG count, ULONG64 *bases, ULONG start,
            DEBUG_MODULE_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSymbolModule)(THIS_ const char *symbol, ULONG64 *base) PURE;
    STDMETHOD(GetTypeName)(THIS_ ULONG64 base, ULONG type_id, char *buffer, ULONG buffer_size, ULONG *name_size) PURE;
    STDMETHOD(GetTypeId)(THIS_ ULONG64 module, const char *name, ULONG *type_id) PURE;
    STDMETHOD(GetTypeSize)(THIS_ ULONG64 module, ULONG type_id, ULONG *size) PURE;
    STDMETHOD(GetFieldOffset)(THIS_ ULONG64 base, ULONG type_id, const char *field, ULONG *offset) PURE;
    STDMETHOD(GetSymbolTypeId)(THIS_ const char *symbol, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(GetOffsetTypeId)(THIS_ ULONG64 offset, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(ReadTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataVirtual)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(ReadTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataPhysical)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(GetScope)(THIS_ ULONG64 *instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(SetScope)(THIS_ ULONG64 instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(ResetScope)(THIS) PURE;
    STDMETHOD(GetScopeSymbolGroup)(THIS_ ULONG flags, IDebugSymbolGroup *update, IDebugSymbolGroup **symbols) PURE;
    STDMETHOD(CreateSymbolGroup)(THIS_ IDebugSymbolGroup **group) PURE;
    STDMETHOD(StartSymbolMatch)(THIS_ const char *pattern, ULONG64 *handle) PURE;
    STDMETHOD(GetNextSymbolMatch)(THIS_ ULONG64 handle, char *buffer, ULONG buffer_size, ULONG *match_size,
            ULONG64 *offset) PURE;
    STDMETHOD(EndSymbolMatch)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Reload)(THIS_ const char *path) PURE;
    STDMETHOD(GetSymbolPath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(GetImagePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(GetSourcePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(GetSourcePathElement)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *element_size) PURE;
    STDMETHOD(SetSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(FindSourceFile)(THIS_ ULONG start, const char *file, ULONG flags, ULONG *found_element, char *buffer,
            ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSourceFileLineOffsets)(THIS_ const char *file, ULONG64 *buffer, ULONG buffer_lines,
            ULONG *file_lines) PURE;
    /* IDebugSymbols2 */
    STDMETHOD(GetModuleVersionInformation)(THIS_ ULONG index, ULONG64 base, const char *item, void *buffer,
            ULONG buffer_size, ULONG *info_size) PURE;
    STDMETHOD(GetModuleNameString)(THIS_ ULONG which, ULONG index, ULONG64 base, char *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetConstantName)(THIS_ ULONG64 module, ULONG type_id, ULONG64 value, char *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetFieldName)(THIS_ ULONG64 module, ULONG type_id, ULONG field_index, char *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetTypeOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddTypeOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveTypeOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetTypeOptions)(THIS_ ULONG options) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugSymbols3
DECLARE_INTERFACE_(IDebugSymbols3, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSymbols */
    STDMETHOD(GetSymbolOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetSymbolOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetNameByOffset)(THIS_ ULONG64 offset, char *buffer, ULONG buffer_size, ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByName)(THIS_ const char *symbol, ULONG64 *offset) PURE;
    STDMETHOD(GetNearNameByOffset)(THIS_ ULONG64 offset, LONG delta, char *buffer, ULONG buffer_size,ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetLineByOffset)(THIS_ ULONG64 offset, ULONG *line, char *buffer, ULONG buffer_size, ULONG *file_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByLine)(THIS_ ULONG line, const char *file, ULONG64 *offset) PURE;
    STDMETHOD(GetNumberModules)(THIS_ ULONG *loaded, ULONG *unloaded) PURE;
    STDMETHOD(GetModuleByIndex)(THIS_ ULONG index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByModuleName)(THIS_ const char *name, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleByOffset)(THIS_ ULONG64 offset, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetModuleNames)(THIS_ ULONG index, ULONG64 base, char *image_name, ULONG image_name_buffer_size,
            ULONG *image_name_size, char *module_name, ULONG module_name_buffer_size, ULONG *module_name_size,
            char *loaded_image_name, ULONG loaded_image_name_buffer_size, ULONG *loaded_image_size) PURE;
    STDMETHOD(GetModuleParameters)(THIS_ ULONG count, ULONG64 *bases, ULONG start,
            DEBUG_MODULE_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSymbolModule)(THIS_ const char *symbol, ULONG64 *base) PURE;
    STDMETHOD(GetTypeName)(THIS_ ULONG64 base, ULONG type_id, char *buffer, ULONG buffer_size, ULONG *name_size) PURE;
    STDMETHOD(GetTypeId)(THIS_ ULONG64 module, const char *name, ULONG *type_id) PURE;
    STDMETHOD(GetTypeSize)(THIS_ ULONG64 module, ULONG type_id, ULONG *size) PURE;
    STDMETHOD(GetFieldOffset)(THIS_ ULONG64 base, ULONG type_id, const char *field, ULONG *offset) PURE;
    STDMETHOD(GetSymbolTypeId)(THIS_ const char *symbol, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(GetOffsetTypeId)(THIS_ ULONG64 offset, ULONG *type_id, ULONG64 *base) PURE;
    STDMETHOD(ReadTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataVirtual)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataVirtual)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(ReadTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *read_len) PURE;
    STDMETHOD(WriteTypedDataPhysical)(THIS_ ULONG64 offset, ULONG64 base, ULONG type_id, void *buffer,
            ULONG buffer_size, ULONG *written) PURE;
    STDMETHOD(OutputTypedDataPhysical)(THIS_ ULONG output_control, ULONG64 offset, ULONG64 base, ULONG type_id,
            ULONG flags) PURE;
    STDMETHOD(GetScope)(THIS_ ULONG64 *instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(SetScope)(THIS_ ULONG64 instr_offset, DEBUG_STACK_FRAME *frame, void *scope_context,
            ULONG scope_context_size) PURE;
    STDMETHOD(ResetScope)(THIS) PURE;
    STDMETHOD(GetScopeSymbolGroup)(THIS_ ULONG flags, IDebugSymbolGroup *update, IDebugSymbolGroup **symbols) PURE;
    STDMETHOD(CreateSymbolGroup)(THIS_ IDebugSymbolGroup **group) PURE;
    STDMETHOD(StartSymbolMatch)(THIS_ const char *pattern, ULONG64 *handle) PURE;
    STDMETHOD(GetNextSymbolMatch)(THIS_ ULONG64 handle, char *buffer, ULONG buffer_size, ULONG *match_size,
            ULONG64 *offset) PURE;
    STDMETHOD(EndSymbolMatch)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Reload)(THIS_ const char *path) PURE;
    STDMETHOD(GetSymbolPath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSymbolPath)(THIS_ const char *path) PURE;
    STDMETHOD(GetImagePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendImagePath)(THIS_ const char *path) PURE;
    STDMETHOD(GetSourcePath)(THIS_ char *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(GetSourcePathElement)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *element_size) PURE;
    STDMETHOD(SetSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(AppendSourcePath)(THIS_ const char *path) PURE;
    STDMETHOD(FindSourceFile)(THIS_ ULONG start, const char *file, ULONG flags, ULONG *found_element, char *buffer,
            ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSourceFileLineOffsets)(THIS_ const char *file, ULONG64 *buffer, ULONG buffer_lines,
            ULONG *file_lines) PURE;
    /* IDebugSymbols2 */
    STDMETHOD(GetModuleVersionInformation)(THIS_ ULONG index, ULONG64 base, const char *item, void *buffer,
            ULONG buffer_size, ULONG *info_size) PURE;
    STDMETHOD(GetModuleNameString)(THIS_ ULONG which, ULONG index, ULONG64 base, char *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetConstantName)(THIS_ ULONG64 module, ULONG type_id, ULONG64 value, char *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetFieldName)(THIS_ ULONG64 module, ULONG type_id, ULONG field_index, char *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetTypeOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddTypeOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveTypeOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetTypeOptions)(THIS_ ULONG options) PURE;
    /* IDebugSymbols3 */
    STDMETHOD(GetNameByOffsetWide)(THIS_ ULONG64 offset, WCHAR *buffer, ULONG buffer_size, ULONG *name_size,
            ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByNameWide)(THIS_ const WCHAR *symbol, ULONG64 *offset) PURE;
    STDMETHOD(GetNearNameByOffsetWide)(THIS_ ULONG64 offset, LONG delta, WCHAR *buffer, ULONG buffer_size,
            ULONG *name_size, ULONG64 *displacement) PURE;
    STDMETHOD(GetLineByOffsetWide)(THIS_ ULONG64 offset, ULONG *line, WCHAR *buffer, ULONG buffer_size,
            ULONG *file_size, ULONG64 *displacement) PURE;
    STDMETHOD(GetOffsetByLineWide)(THIS_ ULONG line, const WCHAR *file, ULONG64 *offset) PURE;
    STDMETHOD(GetModuleByModuleNameWide)(THIS_ const WCHAR *name, ULONG start_index, ULONG *index, ULONG64 *base) PURE;
    STDMETHOD(GetSymbolModuleWide)(THIS_ const WCHAR *symbol, ULONG64 *base) PURE;
    STDMETHOD(GetTypeNameWide)(THIS_ ULONG64 module, ULONG type_id, WCHAR *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetTypeIdWide)(THIS_ ULONG64 module, const WCHAR *name, ULONG *type_id) PURE;
    STDMETHOD(GetFieldOffsetWide)(THIS_ ULONG64 module, ULONG type_id, const WCHAR *field, ULONG *offset) PURE;
    STDMETHOD(GetSymbolTypeIdWide)(THIS_ const WCHAR *symbol, ULONG *type_id, ULONG64 *module) PURE;
    STDMETHOD(GetScopeSymbolGroup2)(THIS_ ULONG flags, PDEBUG_SYMBOL_GROUP2 update,
            PDEBUG_SYMBOL_GROUP2 *symbols) PURE;
    STDMETHOD(CreateSymbolGroup2)(THIS_ PDEBUG_SYMBOL_GROUP2 *group) PURE;
    STDMETHOD(StartSymbolMatchWide)(THIS_ const WCHAR *pattern, ULONG64 *handle) PURE;
    STDMETHOD(GetNextSymbolMatchWide)(THIS_ ULONG64 handle, WCHAR *buffer, ULONG buffer_size, ULONG *match_size,
            ULONG64 *offset) PURE;
    STDMETHOD(ReloadWide)(THIS_ const WCHAR *module) PURE;
    STDMETHOD(GetSymbolPathWide)(THIS_ WCHAR *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetSymbolPathWide)(THIS_ const WCHAR *path) PURE;
    STDMETHOD(AppendSymbolPathWide)(THIS_ const WCHAR *addition) PURE;
    STDMETHOD(GetImagePathWide)(THIS_ WCHAR *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(SetImagePathWide)(THIS_ const WCHAR *path) PURE;
    STDMETHOD(AppendImagePathWide)(THIS_ const WCHAR *addition) PURE;
    STDMETHOD(GetSourcePathWide)(THIS_ WCHAR *buffer, ULONG buffer_size, ULONG *path_size) PURE;
    STDMETHOD(GetSourcePathElementWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buffer_size, ULONG *element_size) PURE;
    STDMETHOD(SetSourcePathWide)(THIS_ const WCHAR *path) PURE;
    STDMETHOD(AppendSourcePathWide)(THIS_ const WCHAR *addition) PURE;
    STDMETHOD(FindSourceFileWide)(THIS_ ULONG start_element, const WCHAR *file, ULONG flags, ULONG *found_element,
            WCHAR *buffer, ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSourceFileLineOffsetsWide)(THIS_ const WCHAR *file, ULONG64 *buffer, ULONG buffer_lines,
            ULONG *file_lines) PURE;
    STDMETHOD(GetModuleVersionInformationWide)(THIS_ ULONG index, ULONG64 base, const WCHAR *item, void *buffer,
            ULONG buffer_size, ULONG *version_info_size) PURE;
    STDMETHOD(GetModuleNameStringWide)(THIS_ ULONG which, ULONG index, ULONG64 base, WCHAR *buffer, ULONG buffer_size,
            ULONG *name_size) PURE;
    STDMETHOD(GetConstantNameWide)(THIS_ ULONG64 module, ULONG type_id, ULONG64 value, WCHAR *buffer,
            ULONG buffer_size, ULONG *name_size) PURE;
    STDMETHOD(GetFieldNameWide)(THIS_ ULONG64 module, ULONG type_id, ULONG field_index, WCHAR *buffer,
            ULONG buffer_size, ULONG *name_size) PURE;
    STDMETHOD(IsManagedModule)(THIS_ ULONG index, ULONG64 base) PURE;
    STDMETHOD(GetModuleByModuleName2)(THIS_ const char *name, ULONG start_index, ULONG flags, ULONG *index,
            ULONG64 *base) PURE;
    STDMETHOD(GetModuleByModuleName2Wide)(THIS_ const WCHAR *name, ULONG start_index, ULONG flags, ULONG *index,
            ULONG64 *base) PURE;
    STDMETHOD(GetModuleByOffset2)(THIS_ ULONG64 offset, ULONG start_index, ULONG flags, ULONG *index,
            ULONG64 *base) PURE;
    STDMETHOD(AddSyntheticModule)(THIS_ ULONG64 base, ULONG size, const char *image_path, const char *module_name,
            ULONG flags) PURE;
    STDMETHOD(AddSyntheticModuleWide)(THIS_ ULONG64 base, ULONG size, const WCHAR *image_path,
            const WCHAR *module_name, ULONG flags) PURE;
    STDMETHOD(RemoveSyntheticModule)(THIS_ ULONG64 base) PURE;
    STDMETHOD(GetCurrentScopeFrameIndex)(THIS_ ULONG *index) PURE;
    STDMETHOD(SetScopeFrameByIndex)(THIS_ ULONG index) PURE;
    STDMETHOD(SetScopeFromJitDebugInfo)(THIS_ ULONG output_control, ULONG64 info_offset) PURE;
    STDMETHOD(SetScopeFromStoredEvent)(THIS) PURE;
    STDMETHOD(OutputSymbolByOffset)(THIS_ ULONG output_control, ULONG flags, ULONG64 offset) PURE;
    STDMETHOD(GetFunctionEntryByOffset)(THIS_ ULONG64 offset, ULONG flags, void *buffer, ULONG buffer_size,
            ULONG *needed_size) PURE;
    STDMETHOD(GetFieldTypeAndOffset)(THIS_ ULONG64 module, ULONG container_type_id, const char *field,
            ULONG *field_type_id, ULONG *offset) PURE;
    STDMETHOD(GetFieldTypeAndOffsetWide)(THIS_ ULONG64 module, ULONG container_type_id, const WCHAR *field,
            ULONG *field_type_id, ULONG *offset) PURE;
    STDMETHOD(AddSyntheticSymbol)(THIS_ ULONG64 offset, ULONG size, const char *name, ULONG flags,
            DEBUG_MODULE_AND_ID *id) PURE;
    STDMETHOD(AddSyntheticSymbolWide)(THIS_ ULONG64 offset, ULONG size, const WCHAR *name, ULONG flags,
            DEBUG_MODULE_AND_ID *id) PURE;
    STDMETHOD(RemoveSyntheticSymbol)(THIS_ DEBUG_MODULE_AND_ID *id) PURE;
    STDMETHOD(GetSymbolEntriesByOffset)(THIS_ ULONG64 offset, ULONG flags, DEBUG_MODULE_AND_ID *ids,
            LONG64 *displacements, ULONG count, ULONG *entries) PURE;
    STDMETHOD(GetSymbolEntriesByName)(THIS_ const char *symbol, ULONG flags, DEBUG_MODULE_AND_ID *ids, ULONG count,
            ULONG *entries) PURE;
    STDMETHOD(GetSymbolEntriesByNameWide)(THIS_ const WCHAR *symbol, ULONG flags, DEBUG_MODULE_AND_ID *ids,
            ULONG count, ULONG *entries) PURE;
    STDMETHOD(GetSymbolEntryByToken)(THIS_ ULONG64 base, ULONG token, DEBUG_MODULE_AND_ID *id) PURE;
    STDMETHOD(GetSymbolEntryInformation)(THIS_ DEBUG_MODULE_AND_ID *id, DEBUG_SYMBOL_ENTRY *info) PURE;
    STDMETHOD(GetSymbolEntryString)(THIS_ DEBUG_MODULE_AND_ID *id, ULONG which, char *buffer, ULONG buffer_size,
            ULONG *string_size) PURE;
    STDMETHOD(GetSymbolEntryStringWide)(THIS_ DEBUG_MODULE_AND_ID *id, ULONG which, WCHAR *buffer, ULONG buffer_size,
            ULONG *string_size) PURE;
    STDMETHOD(GetSymbolEntryOffsetRegions)(THIS_ DEBUG_MODULE_AND_ID *id, ULONG flags, DEBUG_OFFSET_REGION *regions,
            ULONG regions_count, ULONG *regions_avail) PURE;
    STDMETHOD(GetSymbolEntryBySymbolEntry)(THIS_ DEBUG_MODULE_AND_ID *from_id, ULONG flags,
            DEBUG_MODULE_AND_ID *to_id) PURE;
    STDMETHOD(GetSourceEntriesByOffset)(THIS_ ULONG64 offset, ULONG flags, DEBUG_SYMBOL_SOURCE_ENTRY *entries,
            ULONG count, ULONG *entries_avail) PURE;
    STDMETHOD(GetSourceEntriesByLine)(THIS_ ULONG line, const char *file, ULONG flags,
            DEBUG_SYMBOL_SOURCE_ENTRY *entries, ULONG count, ULONG *entries_avail) PURE;
    STDMETHOD(GetSourceEntriesByLineWide)(THIS_ ULONG line, const WCHAR *file, ULONG flags,
            DEBUG_SYMBOL_SOURCE_ENTRY *entries, ULONG count, ULONG *entries_avail) PURE;
    STDMETHOD(GetSourceEntryString)(THIS_ DEBUG_SYMBOL_SOURCE_ENTRY *entry, ULONG which, char *buffer,
            ULONG buffer_size, ULONG *string_size) PURE;
    STDMETHOD(GetSourceEntryStringWide)(THIS_ DEBUG_SYMBOL_SOURCE_ENTRY *entry, ULONG which, WCHAR *buffer,
            ULONG buffer_size, ULONG *string_size) PURE;
    STDMETHOD(GetSourceEntryOffsetRegions)(THIS_ DEBUG_SYMBOL_SOURCE_ENTRY *entry, ULONG flags,
            DEBUG_OFFSET_REGION *regions, ULONG count, ULONG *regions_avail) PURE;
    STDMETHOD(GetSourceEntryBySourceEntry)(THIS_ DEBUG_SYMBOL_SOURCE_ENTRY *from_entry, ULONG flags,
            DEBUG_SYMBOL_SOURCE_ENTRY *to_entry) PURE;
};
#undef INTERFACE

#ifdef __ms_va_list

#define INTERFACE IDebugControl
DECLARE_INTERFACE_(IDebugControl, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugControl */
    STDMETHOD(GetInterrupt)(THIS) PURE;
    STDMETHOD(SetInterrupt)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetInterruptTimeout)(THIS_ ULONG *timeout) PURE;
    STDMETHOD(SetInterruptTimeout)(THIS_ ULONG timeout) PURE;
    STDMETHOD(GetLogFile)(THIS_ char *buffer, ULONG buffer_size, ULONG *file_size, BOOL *append) PURE;
    STDMETHOD(OpenLogFile)(THIS_ const char *file, BOOL append) PURE;
    STDMETHOD(CloseLogFile)(THIS) PURE;
    STDMETHOD(GetLogMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetLogMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(Input)(THIS_ char *buffer, ULONG buffer_size, ULONG *input_size) PURE;
    STDMETHOD(ReturnInput)(THIS_ const char *buffer) PURE;
    STDMETHODV(Output)(THIS_ ULONG mask, const char *format, ...) PURE;
    STDMETHOD(OutputVaList)(THIS_ ULONG mask, const char *format, __ms_va_list args) PURE;
    STDMETHODV(ControlledOutput)(THIS_ ULONG output_control, ULONG mask, const char *format, ...) PURE;
    STDMETHOD(ControlledOutputVaList)(THIS_ ULONG output_control, ULONG mask, const char *format,
            __ms_va_list args) PURE;
    STDMETHODV(OutputPrompt)(THIS_ ULONG output_control, const char *format, ...) PURE;
    STDMETHOD(OutputPromptVaList)(THIS_ ULONG output_control, const char *format, __ms_va_list args) PURE;
    STDMETHOD(GetPromptText)(THIS_ char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(OutputCurrentState)(THIS_ ULONG output_control, ULONG flags) PURE;
    STDMETHOD(OutputVersionInformation)(THIS_ ULONG output_control) PURE;
    STDMETHOD(GetNotifyEventHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(SetNotifyEventHandle)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Assemble)(THIS_ ULONG64 offset, const char *code, ULONG64 *end_offset) PURE;
    STDMETHOD(Disassemble)(THIS_ ULONG64 offset, ULONG flags, char *buffer, ULONG buffer_size, ULONG *disassm_size,
            ULONG64 *end_offset) PURE;
    STDMETHOD(GetDisassembleEffectiveOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputDisassembly)(THIS_ ULONG output_control, ULONG64 offset, ULONG flags, ULONG64 *end_offset) PURE;
    STDMETHOD(OutputDisassemblyLines)(THIS_ ULONG output_control, ULONG prev_lines, ULONG total_lines, ULONG64 offset,
            ULONG flags, ULONG *offset_line, ULONG64 *start_offset, ULONG64 *end_offset, ULONG64 *line_offsets) PURE;
    STDMETHOD(GetNearInstruction)(THIS_ ULONG64 offset, LONG delta, ULONG64 *instr_offset) PURE;
    STDMETHOD(GetStackTrace)(THIS_ ULONG64 frame_offset, ULONG64 stack_offset, ULONG64 instr_offset,
            DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG *frames_filled) PURE;
    STDMETHOD(GetReturnOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputStackTrace)(THIS_ ULONG output_control, DEBUG_STACK_FRAME *frames, ULONG frames_size,
            ULONG flags) PURE;
    STDMETHOD(GetDebuggeeType)(THIS_ ULONG *_class, ULONG *qualifier) PURE;
    STDMETHOD(GetActualProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetExecutingProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetNumberProcessors)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSystemVersion)(THIS_ ULONG *platform_id, ULONG *major, ULONG *minor, char *sp_string,
            ULONG sp_string_size, ULONG *sp_string_used, ULONG *sp_number, char *build_string, ULONG build_string_size,
            ULONG *build_string_used) PURE;
    STDMETHOD(GetPageSize)(THIS_ ULONG *size) PURE;
    STDMETHOD(IsPointer64Bit)(THIS) PURE;
    STDMETHOD(ReadBugCheckData)(THIS_ ULONG *code, ULONG64 *arg1, ULONG64 *arg2, ULONG64 *arg3, ULONG64 *arg4) PURE;
    STDMETHOD(GetNumberSupportedProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetProcessorTypeNames)(THIS_ ULONG type, char *full_name, ULONG full_name_buffer_size,
            ULONG *full_name_size, char *abbrev_name, ULONG abbrev_name_buffer_size, ULONG *abbrev_name_size) PURE;
    STDMETHOD(GetEffectiveProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(SetEffectiveProcessorType)(THIS_ ULONG type) PURE;
    STDMETHOD(GetExecutionStatus)(THIS_ ULONG *status) PURE;
    STDMETHOD(SetExecutionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(GetCodeLevel)(THIS_ ULONG *level) PURE;
    STDMETHOD(SetCodeLevel)(THIS_ ULONG level) PURE;
    STDMETHOD(GetEngineOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetSystemErrorControl)(THIS_ ULONG *output_level, ULONG *break_level) PURE;
    STDMETHOD(SetSystemErrorControl)(THIS_ ULONG output_level, ULONG break_level) PURE;
    STDMETHOD(GetTextMacro)(THIS_ ULONG slot, char *buffer, ULONG buffer_size, ULONG *macro_size) PURE;
    STDMETHOD(SetTextMacro)(THIS_ ULONG slot, const char *macro) PURE;
    STDMETHOD(GetRadix)(THIS_ ULONG *radix) PURE;
    STDMETHOD(SetRadix)(THIS_ ULONG radix) PURE;
    STDMETHOD(Evaluate)(THIS_ const char *expression, ULONG desired_type, DEBUG_VALUE *value,
            ULONG *remainder_index) PURE;
    STDMETHOD(CoerceValue)(THIS_ DEBUG_VALUE input, ULONG output_type, DEBUG_VALUE *output) PURE;
    STDMETHOD(CoerceValues)(THIS_ ULONG count, DEBUG_VALUE *input, ULONG *output_types, DEBUG_VALUE *output) PURE;
    STDMETHOD(Execute)(THIS_ ULONG output_control, const char *command, ULONG flags) PURE;
    STDMETHOD(ExecuteCommandFile)(THIS_ ULONG output_control, const char *command_file, ULONG flags) PURE;
    STDMETHOD(GetNumberBreakpoints)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetBreakpointByIndex)(THIS_ ULONG index, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointById)(THIS_ ULONG id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointParameters)(THIS_ ULONG count, ULONG *ids, ULONG start,
            DEBUG_BREAKPOINT_PARAMETERS *parameters) PURE;
    STDMETHOD(AddBreakpoint)(THIS_ ULONG type, ULONG desired_id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(RemoveBreakpoint)(THIS_ IDebugBreakpoint *bp) PURE;
    STDMETHOD(AddExtension)(THIS_ const char *path, ULONG flags, ULONG64 *handle) PURE;
    STDMETHOD(RemoveExtension)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetExtensionByPath)(THIS_ const char *path, ULONG64 *handle) PURE;
    STDMETHOD(CallExtension)(THIS_ ULONG64 handle, const char *function, const char *args) PURE;
    STDMETHOD(GetExtensionFunction)(THIS_ ULONG64 handle, const char *name, void *function) PURE;
    STDMETHOD(GetWindbgExtensionApis32)(THIS_ PWINDBG_EXTENSION_APIS32 api) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(THIS_ PWINDBG_EXTENSION_APIS64 api) PURE;
    STDMETHOD(GetNumberEventFilters)(THIS_ ULONG *specific_events, ULONG *specific_exceptions,
            ULONG *arbitrary_exceptions) PURE;
    STDMETHOD(GetEventFilterText)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(GetEventFilterCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetEventFilterCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(GetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSpecificFilterArgument)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *argument_size) PURE;
    STDMETHOD(SetSpecificFilterArgument)(THIS_ ULONG index, const char *argument) PURE;
    STDMETHOD(GetExceptionFilterParameters)(THIS_ ULONG count, ULONG *codes, ULONG start,
            DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetExceptionFilterParameters)(THIS_ ULONG count, DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetExceptionFilterSecondCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *command_size) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(WaitForEvent)(THIS_ ULONG flags, ULONG timeout) PURE;
    STDMETHOD(GetLastEventInformation)(THIS_ ULONG *type, ULONG *pid, ULONG *tid, void *extra_info,
            ULONG extra_info_size, ULONG *extra_info_used, char *description, ULONG desc_size, ULONG *desc_used) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugControl2
DECLARE_INTERFACE_(IDebugControl2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugControl */
    STDMETHOD(GetInterrupt)(THIS) PURE;
    STDMETHOD(SetInterrupt)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetInterruptTimeout)(THIS_ ULONG *timeout) PURE;
    STDMETHOD(SetInterruptTimeout)(THIS_ ULONG timeout) PURE;
    STDMETHOD(GetLogFile)(THIS_ char *buffer, ULONG buffer_size, ULONG *file_size, BOOL *append) PURE;
    STDMETHOD(OpenLogFile)(THIS_ const char *file, BOOL append) PURE;
    STDMETHOD(CloseLogFile)(THIS) PURE;
    STDMETHOD(GetLogMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetLogMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(Input)(THIS_ char *buffer, ULONG buffer_size, ULONG *input_size) PURE;
    STDMETHOD(ReturnInput)(THIS_ const char *buffer) PURE;
    STDMETHODV(Output)(THIS_ ULONG mask, const char *format, ...) PURE;
    STDMETHOD(OutputVaList)(THIS_ ULONG mask, const char *format, __ms_va_list args) PURE;
    STDMETHODV(ControlledOutput)(THIS_ ULONG output_control, ULONG mask, const char *format, ...) PURE;
    STDMETHOD(ControlledOutputVaList)(THIS_ ULONG output_control, ULONG mask, const char *format,
            __ms_va_list args) PURE;
    STDMETHODV(OutputPrompt)(THIS_ ULONG output_control, const char *format, ...) PURE;
    STDMETHOD(OutputPromptVaList)(THIS_ ULONG output_control, const char *format, __ms_va_list args) PURE;
    STDMETHOD(GetPromptText)(THIS_ char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(OutputCurrentState)(THIS_ ULONG output_control, ULONG flags) PURE;
    STDMETHOD(OutputVersionInformation)(THIS_ ULONG output_control) PURE;
    STDMETHOD(GetNotifyEventHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(SetNotifyEventHandle)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Assemble)(THIS_ ULONG64 offset, const char *code, ULONG64 *end_offset) PURE;
    STDMETHOD(Disassemble)(THIS_ ULONG64 offset, ULONG flags, char *buffer, ULONG buffer_size, ULONG *disassm_size,
            ULONG64 *end_offset) PURE;
    STDMETHOD(GetDisassembleEffectiveOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputDisassembly)(THIS_ ULONG output_control, ULONG64 offset, ULONG flags, ULONG64 *end_offset) PURE;
    STDMETHOD(OutputDisassemblyLines)(THIS_ ULONG output_control, ULONG prev_lines, ULONG total_lines, ULONG64 offset,
            ULONG flags, ULONG *offset_line, ULONG64 *start_offset, ULONG64 *end_offset, ULONG64 *line_offsets) PURE;
    STDMETHOD(GetNearInstruction)(THIS_ ULONG64 offset, LONG delta, ULONG64 *instr_offset) PURE;
    STDMETHOD(GetStackTrace)(THIS_ ULONG64 frame_offset, ULONG64 stack_offset, ULONG64 instr_offset,
            DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG *frames_filled) PURE;
    STDMETHOD(GetReturnOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputStackTrace)(THIS_ ULONG output_control, DEBUG_STACK_FRAME *frames, ULONG frames_size,
            ULONG flags) PURE;
    STDMETHOD(GetDebuggeeType)(THIS_ ULONG *_class, ULONG *qualifier) PURE;
    STDMETHOD(GetActualProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetExecutingProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetNumberProcessors)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSystemVersion)(THIS_ ULONG *platform_id, ULONG *major, ULONG *minor, char *sp_string,
            ULONG sp_string_size, ULONG *sp_string_used, ULONG *sp_number, char *build_string, ULONG build_string_size,
            ULONG *build_string_used) PURE;
    STDMETHOD(GetPageSize)(THIS_ ULONG *size) PURE;
    STDMETHOD(IsPointer64Bit)(THIS) PURE;
    STDMETHOD(ReadBugCheckData)(THIS_ ULONG *code, ULONG64 *arg1, ULONG64 *arg2, ULONG64 *arg3, ULONG64 *arg4) PURE;
    STDMETHOD(GetNumberSupportedProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetProcessorTypeNames)(THIS_ ULONG type, char *full_name, ULONG full_name_buffer_size,
            ULONG *full_name_size, char *abbrev_name, ULONG abbrev_name_buffer_size, ULONG *abbrev_name_size) PURE;
    STDMETHOD(GetEffectiveProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(SetEffectiveProcessorType)(THIS_ ULONG type) PURE;
    STDMETHOD(GetExecutionStatus)(THIS_ ULONG *status) PURE;
    STDMETHOD(SetExecutionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(GetCodeLevel)(THIS_ ULONG *level) PURE;
    STDMETHOD(SetCodeLevel)(THIS_ ULONG level) PURE;
    STDMETHOD(GetEngineOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetSystemErrorControl)(THIS_ ULONG *output_level, ULONG *break_level) PURE;
    STDMETHOD(SetSystemErrorControl)(THIS_ ULONG output_level, ULONG break_level) PURE;
    STDMETHOD(GetTextMacro)(THIS_ ULONG slot, char *buffer, ULONG buffer_size, ULONG *macro_size) PURE;
    STDMETHOD(SetTextMacro)(THIS_ ULONG slot, const char *macro) PURE;
    STDMETHOD(GetRadix)(THIS_ ULONG *radix) PURE;
    STDMETHOD(SetRadix)(THIS_ ULONG radix) PURE;
    STDMETHOD(Evaluate)(THIS_ const char *expression, ULONG desired_type, DEBUG_VALUE *value,
            ULONG *remainder_index) PURE;
    STDMETHOD(CoerceValue)(THIS_ DEBUG_VALUE input, ULONG output_type, DEBUG_VALUE *output) PURE;
    STDMETHOD(CoerceValues)(THIS_ ULONG count, DEBUG_VALUE *input, ULONG *output_types, DEBUG_VALUE *output) PURE;
    STDMETHOD(Execute)(THIS_ ULONG output_control, const char *command, ULONG flags) PURE;
    STDMETHOD(ExecuteCommandFile)(THIS_ ULONG output_control, const char *command_file, ULONG flags) PURE;
    STDMETHOD(GetNumberBreakpoints)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetBreakpointByIndex)(THIS_ ULONG index, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointById)(THIS_ ULONG id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointParameters)(THIS_ ULONG count, ULONG *ids, ULONG start,
            DEBUG_BREAKPOINT_PARAMETERS *parameters) PURE;
    STDMETHOD(AddBreakpoint)(THIS_ ULONG type, ULONG desired_id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(RemoveBreakpoint)(THIS_ IDebugBreakpoint *bp) PURE;
    STDMETHOD(AddExtension)(THIS_ const char *path, ULONG flags, ULONG64 *handle) PURE;
    STDMETHOD(RemoveExtension)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetExtensionByPath)(THIS_ const char *path, ULONG64 *handle) PURE;
    STDMETHOD(CallExtension)(THIS_ ULONG64 handle, const char *function, const char *args) PURE;
    STDMETHOD(GetExtensionFunction)(THIS_ ULONG64 handle, const char *name, void *function) PURE;
    STDMETHOD(GetWindbgExtensionApis32)(THIS_ PWINDBG_EXTENSION_APIS32 api) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(THIS_ PWINDBG_EXTENSION_APIS64 api) PURE;
    STDMETHOD(GetNumberEventFilters)(THIS_ ULONG *specific_events, ULONG *specific_exceptions,
            ULONG *arbitrary_exceptions) PURE;
    STDMETHOD(GetEventFilterText)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(GetEventFilterCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetEventFilterCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(GetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSpecificFilterArgument)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *argument_size) PURE;
    STDMETHOD(SetSpecificFilterArgument)(THIS_ ULONG index, const char *argument) PURE;
    STDMETHOD(GetExceptionFilterParameters)(THIS_ ULONG count, ULONG *codes, ULONG start,
            DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetExceptionFilterParameters)(THIS_ ULONG count, DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetExceptionFilterSecondCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *command_size) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(WaitForEvent)(THIS_ ULONG flags, ULONG timeout) PURE;
    STDMETHOD(GetLastEventInformation)(THIS_ ULONG *type, ULONG *pid, ULONG *tid, void *extra_info,
            ULONG extra_info_size, ULONG *extra_info_used, char *description, ULONG desc_size, ULONG *desc_used) PURE;
    /* IDebugControl2 */
    STDMETHOD(GetCurrentTimeDate)(THIS_ ULONG timedate) PURE;
    STDMETHOD(GetCurrentSystemUpTime)(THIS_ ULONG uptime) PURE;
    STDMETHOD(GetDumpFormatFlags)(THIS_ ULONG *flags) PURE;
    STDMETHOD(GetNumberTextPlacements)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberTextReplacement)(THIS_ const char *src_text, ULONG index, char *src_buffer,
            ULONG src_buffer_size, ULONG *src_size, char *dst_buffer, ULONG dst_buffer_size, ULONG *dst_size) PURE;
    STDMETHOD(SetTextReplacement)(THIS_ const char *src_text, const char *dst_text) PURE;
    STDMETHOD(RemoveTextReplacements)(THIS) PURE;
    STDMETHOD(OutputTextReplacements)(THIS_ ULONG output_control, ULONG flags) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugControl3
DECLARE_INTERFACE_(IDebugControl3, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugControl */
    STDMETHOD(GetInterrupt)(THIS) PURE;
    STDMETHOD(SetInterrupt)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetInterruptTimeout)(THIS_ ULONG *timeout) PURE;
    STDMETHOD(SetInterruptTimeout)(THIS_ ULONG timeout) PURE;
    STDMETHOD(GetLogFile)(THIS_ char *buffer, ULONG buffer_size, ULONG *file_size, BOOL *append) PURE;
    STDMETHOD(OpenLogFile)(THIS_ const char *file, BOOL append) PURE;
    STDMETHOD(CloseLogFile)(THIS) PURE;
    STDMETHOD(GetLogMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetLogMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(Input)(THIS_ char *buffer, ULONG buffer_size, ULONG *input_size) PURE;
    STDMETHOD(ReturnInput)(THIS_ const char *buffer) PURE;
    STDMETHODV(Output)(THIS_ ULONG mask, const char *format, ...) PURE;
    STDMETHOD(OutputVaList)(THIS_ ULONG mask, const char *format, __ms_va_list args) PURE;
    STDMETHODV(ControlledOutput)(THIS_ ULONG output_control, ULONG mask, const char *format, ...) PURE;
    STDMETHOD(ControlledOutputVaList)(THIS_ ULONG output_control, ULONG mask, const char *format,
            __ms_va_list args) PURE;
    STDMETHODV(OutputPrompt)(THIS_ ULONG output_control, const char *format, ...) PURE;
    STDMETHOD(OutputPromptVaList)(THIS_ ULONG output_control, const char *format, __ms_va_list args) PURE;
    STDMETHOD(GetPromptText)(THIS_ char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(OutputCurrentState)(THIS_ ULONG output_control, ULONG flags) PURE;
    STDMETHOD(OutputVersionInformation)(THIS_ ULONG output_control) PURE;
    STDMETHOD(GetNotifyEventHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(SetNotifyEventHandle)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Assemble)(THIS_ ULONG64 offset, const char *code, ULONG64 *end_offset) PURE;
    STDMETHOD(Disassemble)(THIS_ ULONG64 offset, ULONG flags, char *buffer, ULONG buffer_size, ULONG *disassm_size,
            ULONG64 *end_offset) PURE;
    STDMETHOD(GetDisassembleEffectiveOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputDisassembly)(THIS_ ULONG output_control, ULONG64 offset, ULONG flags, ULONG64 *end_offset) PURE;
    STDMETHOD(OutputDisassemblyLines)(THIS_ ULONG output_control, ULONG prev_lines, ULONG total_lines, ULONG64 offset,
            ULONG flags, ULONG *offset_line, ULONG64 *start_offset, ULONG64 *end_offset, ULONG64 *line_offsets) PURE;
    STDMETHOD(GetNearInstruction)(THIS_ ULONG64 offset, LONG delta, ULONG64 *instr_offset) PURE;
    STDMETHOD(GetStackTrace)(THIS_ ULONG64 frame_offset, ULONG64 stack_offset, ULONG64 instr_offset,
            DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG *frames_filled) PURE;
    STDMETHOD(GetReturnOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputStackTrace)(THIS_ ULONG output_control, DEBUG_STACK_FRAME *frames, ULONG frames_size,
            ULONG flags) PURE;
    STDMETHOD(GetDebuggeeType)(THIS_ ULONG *_class, ULONG *qualifier) PURE;
    STDMETHOD(GetActualProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetExecutingProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetNumberProcessors)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSystemVersion)(THIS_ ULONG *platform_id, ULONG *major, ULONG *minor, char *sp_string,
            ULONG sp_string_size, ULONG *sp_string_used, ULONG *sp_number, char *build_string, ULONG build_string_size,
            ULONG *build_string_used) PURE;
    STDMETHOD(GetPageSize)(THIS_ ULONG *size) PURE;
    STDMETHOD(IsPointer64Bit)(THIS) PURE;
    STDMETHOD(ReadBugCheckData)(THIS_ ULONG *code, ULONG64 *arg1, ULONG64 *arg2, ULONG64 *arg3, ULONG64 *arg4) PURE;
    STDMETHOD(GetNumberSupportedProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetProcessorTypeNames)(THIS_ ULONG type, char *full_name, ULONG full_name_buffer_size,
            ULONG *full_name_size, char *abbrev_name, ULONG abbrev_name_buffer_size, ULONG *abbrev_name_size) PURE;
    STDMETHOD(GetEffectiveProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(SetEffectiveProcessorType)(THIS_ ULONG type) PURE;
    STDMETHOD(GetExecutionStatus)(THIS_ ULONG *status) PURE;
    STDMETHOD(SetExecutionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(GetCodeLevel)(THIS_ ULONG *level) PURE;
    STDMETHOD(SetCodeLevel)(THIS_ ULONG level) PURE;
    STDMETHOD(GetEngineOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetSystemErrorControl)(THIS_ ULONG *output_level, ULONG *break_level) PURE;
    STDMETHOD(SetSystemErrorControl)(THIS_ ULONG output_level, ULONG break_level) PURE;
    STDMETHOD(GetTextMacro)(THIS_ ULONG slot, char *buffer, ULONG buffer_size, ULONG *macro_size) PURE;
    STDMETHOD(SetTextMacro)(THIS_ ULONG slot, const char *macro) PURE;
    STDMETHOD(GetRadix)(THIS_ ULONG *radix) PURE;
    STDMETHOD(SetRadix)(THIS_ ULONG radix) PURE;
    STDMETHOD(Evaluate)(THIS_ const char *expression, ULONG desired_type, DEBUG_VALUE *value,
            ULONG *remainder_index) PURE;
    STDMETHOD(CoerceValue)(THIS_ DEBUG_VALUE input, ULONG output_type, DEBUG_VALUE *output) PURE;
    STDMETHOD(CoerceValues)(THIS_ ULONG count, DEBUG_VALUE *input, ULONG *output_types, DEBUG_VALUE *output) PURE;
    STDMETHOD(Execute)(THIS_ ULONG output_control, const char *command, ULONG flags) PURE;
    STDMETHOD(ExecuteCommandFile)(THIS_ ULONG output_control, const char *command_file, ULONG flags) PURE;
    STDMETHOD(GetNumberBreakpoints)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetBreakpointByIndex)(THIS_ ULONG index, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointById)(THIS_ ULONG id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointParameters)(THIS_ ULONG count, ULONG *ids, ULONG start,
            DEBUG_BREAKPOINT_PARAMETERS *parameters) PURE;
    STDMETHOD(AddBreakpoint)(THIS_ ULONG type, ULONG desired_id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(RemoveBreakpoint)(THIS_ IDebugBreakpoint *bp) PURE;
    STDMETHOD(AddExtension)(THIS_ const char *path, ULONG flags, ULONG64 *handle) PURE;
    STDMETHOD(RemoveExtension)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetExtensionByPath)(THIS_ const char *path, ULONG64 *handle) PURE;
    STDMETHOD(CallExtension)(THIS_ ULONG64 handle, const char *function, const char *args) PURE;
    STDMETHOD(GetExtensionFunction)(THIS_ ULONG64 handle, const char *name, void *function) PURE;
    STDMETHOD(GetWindbgExtensionApis32)(THIS_ PWINDBG_EXTENSION_APIS32 api) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(THIS_ PWINDBG_EXTENSION_APIS64 api) PURE;
    STDMETHOD(GetNumberEventFilters)(THIS_ ULONG *specific_events, ULONG *specific_exceptions,
            ULONG *arbitrary_exceptions) PURE;
    STDMETHOD(GetEventFilterText)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(GetEventFilterCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetEventFilterCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(GetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSpecificFilterArgument)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *argument_size) PURE;
    STDMETHOD(SetSpecificFilterArgument)(THIS_ ULONG index, const char *argument) PURE;
    STDMETHOD(GetExceptionFilterParameters)(THIS_ ULONG count, ULONG *codes, ULONG start,
            DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetExceptionFilterParameters)(THIS_ ULONG count, DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetExceptionFilterSecondCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *command_size) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(WaitForEvent)(THIS_ ULONG flags, ULONG timeout) PURE;
    STDMETHOD(GetLastEventInformation)(THIS_ ULONG *type, ULONG *pid, ULONG *tid, void *extra_info,
            ULONG extra_info_size, ULONG *extra_info_used, char *description, ULONG desc_size, ULONG *desc_used) PURE;
    /* IDebugControl2 */
    STDMETHOD(GetCurrentTimeDate)(THIS_ ULONG timedate) PURE;
    STDMETHOD(GetCurrentSystemUpTime)(THIS_ ULONG uptime) PURE;
    STDMETHOD(GetDumpFormatFlags)(THIS_ ULONG *flags) PURE;
    STDMETHOD(GetNumberTextPlacements)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberTextReplacement)(THIS_ const char *src_text, ULONG index, char *src_buffer,
            ULONG src_buffer_size, ULONG *src_size, char *dst_buffer, ULONG dst_buffer_size, ULONG *dst_size) PURE;
    STDMETHOD(SetTextReplacement)(THIS_ const char *src_text, const char *dst_text) PURE;
    STDMETHOD(RemoveTextReplacements)(THIS) PURE;
    STDMETHOD(OutputTextReplacements)(THIS_ ULONG output_control, ULONG flags) PURE;
    /* IDebugControl3 */
    STDMETHOD(GetAssemblyOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddAssemblyOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveAssemblyOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetAssemblyOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetExpressionSyntax)(THIS_ ULONG *flags) PURE;
    STDMETHOD(SetExpressionSyntax)(THIS_ ULONG flags) PURE;
    STDMETHOD(SetExpressionSyntaxByName)(THIS_ const char *name) PURE;
    STDMETHOD(GetNumberExpressionSyntaxes)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetExpressionSyntaxNames)(THIS_ ULONG index, char *fullname, ULONG fullname_buffer_size,
            ULONG *fullname_size, char *abbrevname, ULONG abbrevname_buffer_size, ULONG *abbrevname_size) PURE;
    STDMETHOD(GetNumberEvents)(THIS_ ULONG *events) PURE;
    STDMETHOD(GetEventIndexDescription)(THIS_ ULONG index, ULONG which, char *buffer, ULONG buffer_size,
            ULONG *desc_size) PURE;
    STDMETHOD(GetCurrentEventIndex)(THIS_ ULONG *index) PURE;
    STDMETHOD(SetNextEventIndex)(THIS_ ULONG relation, ULONG value, ULONG *next_index) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugControl4
DECLARE_INTERFACE_(IDebugControl4, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugControl */
    STDMETHOD(GetInterrupt)(THIS) PURE;
    STDMETHOD(SetInterrupt)(THIS_ ULONG flags) PURE;
    STDMETHOD(GetInterruptTimeout)(THIS_ ULONG *timeout) PURE;
    STDMETHOD(SetInterruptTimeout)(THIS_ ULONG timeout) PURE;
    STDMETHOD(GetLogFile)(THIS_ char *buffer, ULONG buffer_size, ULONG *file_size, BOOL *append) PURE;
    STDMETHOD(OpenLogFile)(THIS_ const char *file, BOOL append) PURE;
    STDMETHOD(CloseLogFile)(THIS) PURE;
    STDMETHOD(GetLogMask)(THIS_ ULONG *mask) PURE;
    STDMETHOD(SetLogMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(Input)(THIS_ char *buffer, ULONG buffer_size, ULONG *input_size) PURE;
    STDMETHOD(ReturnInput)(THIS_ const char *buffer) PURE;
    STDMETHODV(Output)(THIS_ ULONG mask, const char *format, ...) PURE;
    STDMETHOD(OutputVaList)(THIS_ ULONG mask, const char *format, __ms_va_list args) PURE;
    STDMETHODV(ControlledOutput)(THIS_ ULONG output_control, ULONG mask, const char *format, ...) PURE;
    STDMETHOD(ControlledOutputVaList)(THIS_ ULONG output_control, ULONG mask, const char *format,
            __ms_va_list args) PURE;
    STDMETHODV(OutputPrompt)(THIS_ ULONG output_control, const char *format, ...) PURE;
    STDMETHOD(OutputPromptVaList)(THIS_ ULONG output_control, const char *format, __ms_va_list args) PURE;
    STDMETHOD(GetPromptText)(THIS_ char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(OutputCurrentState)(THIS_ ULONG output_control, ULONG flags) PURE;
    STDMETHOD(OutputVersionInformation)(THIS_ ULONG output_control) PURE;
    STDMETHOD(GetNotifyEventHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(SetNotifyEventHandle)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(Assemble)(THIS_ ULONG64 offset, const char *code, ULONG64 *end_offset) PURE;
    STDMETHOD(Disassemble)(THIS_ ULONG64 offset, ULONG flags, char *buffer, ULONG buffer_size, ULONG *disassm_size,
            ULONG64 *end_offset) PURE;
    STDMETHOD(GetDisassembleEffectiveOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputDisassembly)(THIS_ ULONG output_control, ULONG64 offset, ULONG flags, ULONG64 *end_offset) PURE;
    STDMETHOD(OutputDisassemblyLines)(THIS_ ULONG output_control, ULONG prev_lines, ULONG total_lines, ULONG64 offset,
            ULONG flags, ULONG *offset_line, ULONG64 *start_offset, ULONG64 *end_offset, ULONG64 *line_offsets) PURE;
    STDMETHOD(GetNearInstruction)(THIS_ ULONG64 offset, LONG delta, ULONG64 *instr_offset) PURE;
    STDMETHOD(GetStackTrace)(THIS_ ULONG64 frame_offset, ULONG64 stack_offset, ULONG64 instr_offset,
            DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG *frames_filled) PURE;
    STDMETHOD(GetReturnOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(OutputStackTrace)(THIS_ ULONG output_control, DEBUG_STACK_FRAME *frames, ULONG frames_size,
            ULONG flags) PURE;
    STDMETHOD(GetDebuggeeType)(THIS_ ULONG *_class, ULONG *qualifier) PURE;
    STDMETHOD(GetActualProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetExecutingProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetNumberProcessors)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSystemVersion)(THIS_ ULONG *platform_id, ULONG *major, ULONG *minor, char *sp_string,
            ULONG sp_string_size, ULONG *sp_string_used, ULONG *sp_number, char *build_string, ULONG build_string_size,
            ULONG *build_string_used) PURE;
    STDMETHOD(GetPageSize)(THIS_ ULONG *size) PURE;
    STDMETHOD(IsPointer64Bit)(THIS) PURE;
    STDMETHOD(ReadBugCheckData)(THIS_ ULONG *code, ULONG64 *arg1, ULONG64 *arg2, ULONG64 *arg3, ULONG64 *arg4) PURE;
    STDMETHOD(GetNumberSupportedProcessorTypes)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(THIS_ ULONG start, ULONG count, ULONG *types) PURE;
    STDMETHOD(GetProcessorTypeNames)(THIS_ ULONG type, char *full_name, ULONG full_name_buffer_size,
            ULONG *full_name_size, char *abbrev_name, ULONG abbrev_name_buffer_size, ULONG *abbrev_name_size) PURE;
    STDMETHOD(GetEffectiveProcessorType)(THIS_ ULONG *type) PURE;
    STDMETHOD(SetEffectiveProcessorType)(THIS_ ULONG type) PURE;
    STDMETHOD(GetExecutionStatus)(THIS_ ULONG *status) PURE;
    STDMETHOD(SetExecutionStatus)(THIS_ ULONG status) PURE;
    STDMETHOD(GetCodeLevel)(THIS_ ULONG *level) PURE;
    STDMETHOD(SetCodeLevel)(THIS_ ULONG level) PURE;
    STDMETHOD(GetEngineOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetEngineOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetSystemErrorControl)(THIS_ ULONG *output_level, ULONG *break_level) PURE;
    STDMETHOD(SetSystemErrorControl)(THIS_ ULONG output_level, ULONG break_level) PURE;
    STDMETHOD(GetTextMacro)(THIS_ ULONG slot, char *buffer, ULONG buffer_size, ULONG *macro_size) PURE;
    STDMETHOD(SetTextMacro)(THIS_ ULONG slot, const char *macro) PURE;
    STDMETHOD(GetRadix)(THIS_ ULONG *radix) PURE;
    STDMETHOD(SetRadix)(THIS_ ULONG radix) PURE;
    STDMETHOD(Evaluate)(THIS_ const char *expression, ULONG desired_type, DEBUG_VALUE *value,
            ULONG *remainder_index) PURE;
    STDMETHOD(CoerceValue)(THIS_ DEBUG_VALUE input, ULONG output_type, DEBUG_VALUE *output) PURE;
    STDMETHOD(CoerceValues)(THIS_ ULONG count, DEBUG_VALUE *input, ULONG *output_types, DEBUG_VALUE *output) PURE;
    STDMETHOD(Execute)(THIS_ ULONG output_control, const char *command, ULONG flags) PURE;
    STDMETHOD(ExecuteCommandFile)(THIS_ ULONG output_control, const char *command_file, ULONG flags) PURE;
    STDMETHOD(GetNumberBreakpoints)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetBreakpointByIndex)(THIS_ ULONG index, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointById)(THIS_ ULONG id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(GetBreakpointParameters)(THIS_ ULONG count, ULONG *ids, ULONG start,
            DEBUG_BREAKPOINT_PARAMETERS *parameters) PURE;
    STDMETHOD(AddBreakpoint)(THIS_ ULONG type, ULONG desired_id, IDebugBreakpoint **bp) PURE;
    STDMETHOD(RemoveBreakpoint)(THIS_ IDebugBreakpoint *bp) PURE;
    STDMETHOD(AddExtension)(THIS_ const char *path, ULONG flags, ULONG64 *handle) PURE;
    STDMETHOD(RemoveExtension)(THIS_ ULONG64 handle) PURE;
    STDMETHOD(GetExtensionByPath)(THIS_ const char *path, ULONG64 *handle) PURE;
    STDMETHOD(CallExtension)(THIS_ ULONG64 handle, const char *function, const char *args) PURE;
    STDMETHOD(GetExtensionFunction)(THIS_ ULONG64 handle, const char *name, void *function) PURE;
    STDMETHOD(GetWindbgExtensionApis32)(THIS_ PWINDBG_EXTENSION_APIS32 api) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(THIS_ PWINDBG_EXTENSION_APIS64 api) PURE;
    STDMETHOD(GetNumberEventFilters)(THIS_ ULONG *specific_events, ULONG *specific_exceptions,
            ULONG *arbitrary_exceptions) PURE;
    STDMETHOD(GetEventFilterText)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(GetEventFilterCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetEventFilterCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(GetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetSpecificFilterParameters)(THIS_ ULONG start, ULONG count,
            DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetSpecificFilterArgument)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *argument_size) PURE;
    STDMETHOD(SetSpecificFilterArgument)(THIS_ ULONG index, const char *argument) PURE;
    STDMETHOD(GetExceptionFilterParameters)(THIS_ ULONG count, ULONG *codes, ULONG start,
            DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(SetExceptionFilterParameters)(THIS_ ULONG count, DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters) PURE;
    STDMETHOD(GetExceptionFilterSecondCommand)(THIS_ ULONG index, char *buffer, ULONG buffer_size,
            ULONG *command_size) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(THIS_ ULONG index, const char *command) PURE;
    STDMETHOD(WaitForEvent)(THIS_ ULONG flags, ULONG timeout) PURE;
    STDMETHOD(GetLastEventInformation)(THIS_ ULONG *type, ULONG *pid, ULONG *tid, void *extra_info,
            ULONG extra_info_size, ULONG *extra_info_used, char *description, ULONG desc_size, ULONG *desc_used) PURE;
    /* IDebugControl2 */
    STDMETHOD(GetCurrentTimeDate)(THIS_ ULONG timedate) PURE;
    STDMETHOD(GetCurrentSystemUpTime)(THIS_ ULONG uptime) PURE;
    STDMETHOD(GetDumpFormatFlags)(THIS_ ULONG *flags) PURE;
    STDMETHOD(GetNumberTextPlacements)(THIS_ ULONG *count) PURE;
    STDMETHOD(GetNumberTextReplacement)(THIS_ const char *src_text, ULONG index, char *src_buffer,
            ULONG src_buffer_size, ULONG *src_size, char *dst_buffer, ULONG dst_buffer_size, ULONG *dst_size) PURE;
    STDMETHOD(SetTextReplacement)(THIS_ const char *src_text, const char *dst_text) PURE;
    STDMETHOD(RemoveTextReplacements)(THIS) PURE;
    STDMETHOD(OutputTextReplacements)(THIS_ ULONG output_control, ULONG flags) PURE;
    /* IDebugControl3 */
    STDMETHOD(GetAssemblyOptions)(THIS_ ULONG *options) PURE;
    STDMETHOD(AddAssemblyOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(RemoveAssemblyOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(SetAssemblyOptions)(THIS_ ULONG options) PURE;
    STDMETHOD(GetExpressionSyntax)(THIS_ ULONG *flags) PURE;
    STDMETHOD(SetExpressionSyntax)(THIS_ ULONG flags) PURE;
    STDMETHOD(SetExpressionSyntaxByName)(THIS_ const char *name) PURE;
    STDMETHOD(GetNumberExpressionSyntaxes)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetExpressionSyntaxNames)(THIS_ ULONG index, char *fullname, ULONG fullname_buffer_size,
            ULONG *fullname_size, char *abbrevname, ULONG abbrevname_buffer_size, ULONG *abbrevname_size) PURE;
    STDMETHOD(GetNumberEvents)(THIS_ ULONG *events) PURE;
    STDMETHOD(GetEventIndexDescription)(THIS_ ULONG index, ULONG which, char *buffer, ULONG buffer_size,
            ULONG *desc_size) PURE;
    STDMETHOD(GetCurrentEventIndex)(THIS_ ULONG *index) PURE;
    STDMETHOD(SetNextEventIndex)(THIS_ ULONG relation, ULONG value, ULONG *next_index) PURE;
    /* IDebugControl4 */
    STDMETHOD(GetLogFileWide)(THIS_ WCHAR *buffer, ULONG buffer_size, ULONG *file_size, BOOL *append) PURE;
    STDMETHOD(OpenLogFileWide)(THIS_ const WCHAR *filename, BOOL append) PURE;
    STDMETHOD(InputWide)(THIS_ WCHAR *buffer, ULONG buffer_size, ULONG *input_size) PURE;
    STDMETHOD(ReturnInputWide)(THIS_ const WCHAR *buffer) PURE;
    STDMETHODV(OutputWide)(THIS_ ULONG mask, const WCHAR *format, ...) PURE;
    STDMETHOD(OutputVaListWide)(THIS_ ULONG mask, const WCHAR *format, __ms_va_list args) PURE;
    STDMETHODV(ControlledOutputWide)(THIS_ ULONG output_control, ULONG mask, const WCHAR *format, ...) PURE;
    STDMETHOD(ControlledOutputVaListWide)(THIS_ ULONG output_control, ULONG mask, const WCHAR *format, __ms_va_list args) PURE;
    STDMETHODV(OutputPromptWide)(THIS_ ULONG output_control, const WCHAR *format, ...) PURE;
    STDMETHOD(OutputPromptVaListWide)(THIS_ ULONG output_control, const WCHAR *format, __ms_va_list args) PURE;
    STDMETHOD(GetPromptTextWide)(THIS_ WCHAR *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(AssembleWide)(THIS_ ULONG64 offset, const WCHAR *instr, ULONG64 *end_offset) PURE;
    STDMETHOD(DisassembleWide)(THIS_ ULONG64 offset, ULONG flags, WCHAR *buffer, ULONG buffer_size, ULONG *size,
            ULONG64 *end_offset) PURE;
    STDMETHOD(GetProcessorTypeNamesWide)(THIS_ ULONG type, WCHAR *fullename, ULONG fullname_buffer_size,
            ULONG *fullname_size, WCHAR *abbrevname, ULONG abbrevname_buffer_size, ULONG *abbrevname_size) PURE;
    STDMETHOD(GetTextMacroWide)(THIS_ ULONG slot, WCHAR *buffer, ULONG buffer_size, ULONG *macro_size) PURE;
    STDMETHOD(SetTextMacroWide)(THIS_ ULONG slot, const WCHAR *macro) PURE;
    STDMETHOD(EvaluateWide)(THIS_ const WCHAR *expression, ULONG desired_type, DEBUG_VALUE *value, ULONG *remainder_index) PURE;
    STDMETHOD(ExecuteWide)(THIS_ ULONG output_control, const WCHAR *command, ULONG flags) PURE;
    STDMETHOD(ExecuteCommandFileWide)(THIS_ ULONG output_control, const WCHAR *commandfile, ULONG flags) PURE;
    STDMETHOD(GetBreakpointByIndex2)(THIS_ ULONG index, PDEBUG_BREAKPOINT2 *bp) PURE;
    STDMETHOD(GetBreakpointById2)(THIS_ ULONG id, PDEBUG_BREAKPOINT2 *bp) PURE;
    STDMETHOD(AddBreakpoint2)(THIS_ ULONG type, ULONG desired_id, PDEBUG_BREAKPOINT2 *bp) PURE;
    STDMETHOD(RemoveBreakpoint2)(THIS_ PDEBUG_BREAKPOINT2 bp) PURE;
    STDMETHOD(AddExtensionWide)(THIS_ const WCHAR *path, ULONG flags, ULONG64 *handle) PURE;
    STDMETHOD(GetExtensionByPathWide)(THIS_ const WCHAR *path, ULONG64 *handle) PURE;
    STDMETHOD(CallExtensionWide)(THIS_ ULONG64 handle, const WCHAR *function, const WCHAR *arguments) PURE;
    STDMETHOD(GetExtensionFunctionWide)(THIS_ ULONG64 handle, const WCHAR *function, FARPROC *func) PURE;
    STDMETHOD(GetEventFilterTextWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buffer_size, ULONG *text_size) PURE;
    STDMETHOD(GetEventFilterCommandWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetEventFilterCommandWide)(THIS_ ULONG index, const WCHAR *command) PURE;
    STDMETHOD(GetSpecificFilterArgumentWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buffer_size, ULONG *argument_size) PURE;
    STDMETHOD(SetSpecificFilterArgumentWide)(THIS_ ULONG index, const WCHAR *argument) PURE;
    STDMETHOD(GetSpecificFilterSecondCommandWide)(THIS_ ULONG index, WCHAR *buffer, ULONG buffer_size, ULONG *command_size) PURE;
    STDMETHOD(SetSpecificFilterSecondCommandWide)(THIS_ ULONG index, const WCHAR *command) PURE;
    STDMETHOD(GetLastEventInformationWide)(THIS_ ULONG *type, ULONG *processid, ULONG *threadid, void *extra_info,
            ULONG extra_info_size, ULONG *extra_info_used, WCHAR *desc, ULONG desc_size, ULONG *desc_used) PURE;
    STDMETHOD(GetTextReplacementWide)(THIS_ const WCHAR *src_text, ULONG index, WCHAR *src_buffer, ULONG src_buffer_size,
            ULONG *src_size, WCHAR *dst_buffer, ULONG dest_buffer_size, ULONG *dst_size) PURE;
    STDMETHOD(SetTextReplacementWide)(THIS_ const WCHAR *src_text, const WCHAR *dst_text) PURE;
    STDMETHOD(SetExpressionSyntaxByNameWide)(THIS_ const WCHAR *abbrevname) PURE;
    STDMETHOD(GetExpressionSyntaxNamesWide)(THIS_ ULONG index, WCHAR *fullname_buffer, ULONG fullname_buffer_size,
            ULONG *fullname_size, WCHAR *abbrevname_buffer, ULONG abbrevname_buffer_size, ULONG *abbrev_size) PURE;
    STDMETHOD(GetEventIndexDescriptionWide)(THIS_ ULONG index, ULONG which, WCHAR *buffer, ULONG buffer_size,
            ULONG *desc_size) PURE;
    STDMETHOD(GetLogFile2)(THIS_ char *buffer, ULONG buffer_size, ULONG *file_size, ULONG *flags) PURE;
    STDMETHOD(OpenLogFile2)(THIS_ const char *filename, ULONG flags) PURE;
    STDMETHOD(GetLogFile2Wide)(THIS_ WCHAR *buffer, ULONG buffer_size, ULONG *file_size, ULONG *flags) PURE;
    STDMETHOD(OpenLogFile2Wide)(THIS_ const WCHAR *filename, ULONG flags) PURE;
    STDMETHOD(GetSystemVersionValues)(THIS_ ULONG *platformid, ULONG *win32major, ULONG *win32minor,
            ULONG *kdmajor, ULONG *kdminor) PURE;
    STDMETHOD(GetSystemVersionString)(THIS_ ULONG which, char *buffer, ULONG buffer_size, ULONG *string_size) PURE;
    STDMETHOD(GetSystemVersionStringWide)(THIS_ ULONG which, WCHAR *buffer, ULONG buffer_size, ULONG *string_size) PURE;
    STDMETHOD(GetContextStackTrace)(THIS_ void *start_context, ULONG start_context_size, PDEBUG_STACK_FRAME frames,
            ULONG frames_size, void *frame_contexts, ULONG frame_contexts_size, ULONG frame_contexts_entry_size,
            ULONG *frames_filled) PURE;
    STDMETHOD(OutputContextStackTrace)(THIS_ ULONG output_control, PDEBUG_STACK_FRAME frames, ULONG frames_size,
            void *frame_contexts, ULONG frame_contexts_size, ULONG frame_contexts_entry_size, ULONG flags) PURE;
    STDMETHOD(GetStoredEventInformation)(THIS_ ULONG *type, ULONG *processid, ULONG *threadid, void *context,
            ULONG context_size, ULONG *context_used, void *extra_info, ULONG extra_info_size, ULONG *extra_info_used) PURE;
    STDMETHOD(GetManagedStatus)(THIS_ ULONG *flags, ULONG which_string, char *string, ULONG string_size, ULONG string_needed) PURE;
    STDMETHOD(GetManagedStatusWide)(THIS_ ULONG *flags, ULONG which_string, WCHAR *string, ULONG string_size,
            ULONG string_needed) PURE;
    STDMETHOD(ResetManagedStatus)(THIS_ ULONG flags) PURE;
};
#undef INTERFACE

#endif  /* __ms_va_list */

#define INTERFACE IDebugAdvanced
DECLARE_INTERFACE_(IDebugAdvanced, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugAdvanced */
    STDMETHOD(GetThreadContext)(THIS_ void *context, ULONG context_size) PURE;
    STDMETHOD(SetThreadContext)(THIS_ void *context, ULONG context_size) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugAdvanced2
DECLARE_INTERFACE_(IDebugAdvanced2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugAdvanced */
    STDMETHOD(GetThreadContext)(THIS_ void *context, ULONG context_size) PURE;
    STDMETHOD(SetThreadContext)(THIS_ void *context, ULONG context_size) PURE;
    /* IDebugAdvanced2 */
    STDMETHOD(Request)(THIS_ ULONG request, void *inbuffer, ULONG inbuffer_size,
            void *outbuffer, ULONG outbuffer_size, ULONG *outsize) PURE;
    STDMETHOD(GetSourceFileInformation)(THIS_ ULONG which, char *sourcefile,
            ULONG64 arg64, ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size) PURE;
    STDMETHOD(FindSourceFileAndToken)(THIS_ ULONG start_element, ULONG64 modaddr, const char *filename,
            ULONG flags, void *filetoken, ULONG filetokensize, ULONG *found_element, char *buffer,
            ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSymbolInformation)(THIS_ ULONG which, ULONG64 arg64, ULONG arg32, void *buffer,
            ULONG buffer_size, ULONG *info_size, char *string_buffer, ULONG string_buffer_size,
            ULONG *string_size) PURE;
    STDMETHOD(GetSystemObjectInformation)(THIS_ ULONG which, ULONG64 arg64, ULONG arg32,
            void *buffer, ULONG buffer_size, ULONG *info_size) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugAdvanced3
DECLARE_INTERFACE_(IDebugAdvanced3, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugAdvanced */
    STDMETHOD(GetThreadContext)(THIS_ void *context, ULONG context_size) PURE;
    STDMETHOD(SetThreadContext)(THIS_ void *context, ULONG context_size) PURE;
    /* IDebugAdvanced2 */
    STDMETHOD(Request)(THIS_ ULONG request, void *inbuffer, ULONG inbuffer_size,
            void *outbuffer, ULONG outbuffer_size, ULONG *outsize) PURE;
    STDMETHOD(GetSourceFileInformation)(THIS_ ULONG which, char *sourcefile,
            ULONG64 arg64, ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size) PURE;
    STDMETHOD(FindSourceFileAndToken)(THIS_ ULONG start_element, ULONG64 modaddr, const char *filename,
            ULONG flags, void *filetoken, ULONG filetokensize, ULONG *found_element, char *buffer,
            ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSymbolInformation)(THIS_ ULONG which, ULONG64 arg64, ULONG arg32, void *buffer,
            ULONG buffer_size, ULONG *info_size, char *string_buffer, ULONG string_buffer_size,
            ULONG *string_size) PURE;
    STDMETHOD(GetSystemObjectInformation)(THIS_ ULONG which, ULONG64 arg64, ULONG arg32,
            void *buffer, ULONG buffer_size, ULONG *info_size) PURE;
    /* IDebugAdvanced3 */
    STDMETHOD(GetSourceFileInformationWide)(THIS_ ULONG which, WCHAR *source_file, ULONG64 arg64,
            ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size) PURE;
    STDMETHOD(FindSourceFileAndTokenWide)(THIS_ ULONG start_element, ULONG64 modaddr, const WCHAR *filename,
            ULONG flags, void *filetoken, ULONG filetokensize, ULONG *found_element, WCHAR *buffer,
            ULONG buffer_size, ULONG *found_size) PURE;
    STDMETHOD(GetSymbolInformationWide)(THIS_ ULONG which, ULONG64 arg64, ULONG arg32, void *buffer,
            ULONG buffer_size, ULONG *info_size, WCHAR *string_buffer, ULONG string_buffer_size,
            ULONG *string_size) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugSystemObjects
DECLARE_INTERFACE_(IDebugSystemObjects, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSystemObjects */
    STDMETHOD(GetEventThread)(THIS_ ULONG *id) PURE;
    STDMETHOD(GetEventProcess)(THIS_ ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadId)(THIS_ ULONG *id) PURE;
    STDMETHOD(SetCurrentThreadId)(THIS_ ULONG id) PURE;
    STDMETHOD(SetCurrentProcessId)(THIS_ ULONG id) PURE;
    STDMETHOD(GetNumberThreads)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetTotalNumberThreads)(THIS_ ULONG *total, ULONG *largest_process) PURE;
    STDMETHOD(GetThreadIdsByIndex)(THIS_ ULONG start, ULONG count, ULONG *ids, ULONG *sysids) PURE;
    STDMETHOD(GetThreadIdByProcessor)(THIS_ ULONG processor, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetThreadIdByDataOffset)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadTeb)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetThreadIdByTeb)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadSystemId)(THIS_ ULONG *sysid) PURE;
    STDMETHOD(GetThreadIdBySystemId)(THIS_ ULONG sysid, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(GetThreadIdByHandle)(THIS_ ULONG64 handle, ULONG *id) PURE;
    STDMETHOD(GetNumberProcesses)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetProcessIdsByIndex)(THIS_ ULONG start, ULONG count, ULONG *ids, ULONG *sysids) PURE;
    STDMETHOD(GetCurrentProcessDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetProcessIdByDataOffset)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessPeb)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetProcessIdByPeb)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessSystemId)(THIS_ ULONG *sysid) PURE;
    STDMETHOD(GetProcessIdBySystemId)(THIS_ ULONG sysid, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(GetProcessIdByHandle)(THIS_ ULONG64 handle, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessExecutableName)(THIS_ char *buffer, ULONG buffer_size, ULONG *exe_size) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugSystemObjects2
DECLARE_INTERFACE_(IDebugSystemObjects2, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSystemObjects */
    STDMETHOD(GetEventThread)(THIS_ ULONG *id) PURE;
    STDMETHOD(GetEventProcess)(THIS_ ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadId)(THIS_ ULONG *id) PURE;
    STDMETHOD(SetCurrentThreadId)(THIS_ ULONG id) PURE;
    STDMETHOD(SetCurrentProcessId)(THIS_ ULONG id) PURE;
    STDMETHOD(GetNumberThreads)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetTotalNumberThreads)(THIS_ ULONG *total, ULONG *largest_process) PURE;
    STDMETHOD(GetThreadIdsByIndex)(THIS_ ULONG start, ULONG count, ULONG *ids, ULONG *sysids) PURE;
    STDMETHOD(GetThreadIdByProcessor)(THIS_ ULONG processor, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetThreadIdByDataOffset)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadTeb)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetThreadIdByTeb)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadSystemId)(THIS_ ULONG *sysid) PURE;
    STDMETHOD(GetThreadIdBySystemId)(THIS_ ULONG sysid, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(GetThreadIdByHandle)(THIS_ ULONG64 handle, ULONG *id) PURE;
    STDMETHOD(GetNumberProcesses)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetProcessIdsByIndex)(THIS_ ULONG start, ULONG count, ULONG *ids, ULONG *sysids) PURE;
    STDMETHOD(GetCurrentProcessDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetProcessIdByDataOffset)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessPeb)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetProcessIdByPeb)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessSystemId)(THIS_ ULONG *sysid) PURE;
    STDMETHOD(GetProcessIdBySystemId)(THIS_ ULONG sysid, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(GetProcessIdByHandle)(THIS_ ULONG64 handle, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessExecutableName)(THIS_ char *buffer, ULONG buffer_size, ULONG *exe_size) PURE;
    /* IDebugSystemObjects2 */
    STDMETHOD(GetCurrentProcessUpTime)(THIS_ ULONG *uptime) PURE;
    STDMETHOD(GetImplicitThreadDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(SetImplicitThreadDataOffset)(THIS_ ULONG64 offset) PURE;
    STDMETHOD(GetImplicitProcessDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(SetImplicitProcessDataOffset)(THIS_ ULONG64 offset) PURE;
};
#undef INTERFACE

#define INTERFACE IDebugSystemObjects3
DECLARE_INTERFACE_(IDebugSystemObjects3, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDebugSystemObjects */
    STDMETHOD(GetEventThread)(THIS_ ULONG *id) PURE;
    STDMETHOD(GetEventProcess)(THIS_ ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadId)(THIS_ ULONG *id) PURE;
    STDMETHOD(SetCurrentThreadId)(THIS_ ULONG id) PURE;
    STDMETHOD(SetCurrentProcessId)(THIS_ ULONG id) PURE;
    STDMETHOD(GetNumberThreads)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetTotalNumberThreads)(THIS_ ULONG *total, ULONG *largest_process) PURE;
    STDMETHOD(GetThreadIdsByIndex)(THIS_ ULONG start, ULONG count, ULONG *ids, ULONG *sysids) PURE;
    STDMETHOD(GetThreadIdByProcessor)(THIS_ ULONG processor, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetThreadIdByDataOffset)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadTeb)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetThreadIdByTeb)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadSystemId)(THIS_ ULONG *sysid) PURE;
    STDMETHOD(GetThreadIdBySystemId)(THIS_ ULONG sysid, ULONG *id) PURE;
    STDMETHOD(GetCurrentThreadHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(GetThreadIdByHandle)(THIS_ ULONG64 handle, ULONG *id) PURE;
    STDMETHOD(GetNumberProcesses)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetProcessIdsByIndex)(THIS_ ULONG start, ULONG count, ULONG *ids, ULONG *sysids) PURE;
    STDMETHOD(GetCurrentProcessDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetProcessIdByDataOffset)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessPeb)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(GetProcessIdByPeb)(THIS_ ULONG64 offset, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessSystemId)(THIS_ ULONG *sysid) PURE;
    STDMETHOD(GetProcessIdBySystemId)(THIS_ ULONG sysid, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessHandle)(THIS_ ULONG64 *handle) PURE;
    STDMETHOD(GetProcessIdByHandle)(THIS_ ULONG64 handle, ULONG *id) PURE;
    STDMETHOD(GetCurrentProcessExecutableName)(THIS_ char *buffer, ULONG buffer_size, ULONG *exe_size) PURE;
    /* IDebugSystemObjects2 */
    STDMETHOD(GetCurrentProcessUpTime)(THIS_ ULONG *uptime) PURE;
    STDMETHOD(GetImplicitThreadDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(SetImplicitThreadDataOffset)(THIS_ ULONG64 offset) PURE;
    STDMETHOD(GetImplicitProcessDataOffset)(THIS_ ULONG64 *offset) PURE;
    STDMETHOD(SetImplicitProcessDataOffset)(THIS_ ULONG64 offset) PURE;
    /* IDebugSystemObjects3 */
    STDMETHOD(GetEventSystem)(THIS_ ULONG *id) PURE;
    STDMETHOD(GetCurrentSystemId)(THIS_ ULONG *id) PURE;
    STDMETHOD(SetCurrentSystemId)(THIS_ ULONG id) PURE;
    STDMETHOD(GetNumberSystems)(THIS_ ULONG *number) PURE;
    STDMETHOD(GetSystemIdsByIndex)(THIS_ ULONG start, ULONG count, ULONG *ids) PURE;
    STDMETHOD(GetTotalNumberThreadsAndProcesses)(THIS_ ULONG *total_threads, ULONG *total_processes,
            ULONG *largest_process_threads, ULONG *largest_system_threads, ULONG *largest_system_processes) PURE;
    STDMETHOD(GetCurrentSystemServer)(THIS_ ULONG64 *server) PURE;
    STDMETHOD(GetSystemByServer)(THIS_ ULONG64 server, ULONG *id) PURE;
    STDMETHOD(GetCurrentSystemServerName)(THIS_ char *buffer, ULONG buffer_size, ULONG *name_size) PURE;
};
#undef INTERFACE

HRESULT WINAPI DebugCreate(REFIID riid, void **out);

#ifdef __cplusplus
}
#endif
