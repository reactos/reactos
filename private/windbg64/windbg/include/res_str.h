/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Res_str.h

Abstract:

    This module contains the ids for loadable resource strings.

Author:

    David J. Gilman (davegi) 21-Apr-92

Environment:

    Win32, User Mode

--*/

#if ! defined( _RES_STR_ )
#define _RES_STR_


// BUGBUG - I'll take these out as soon as the QCQP button class is removed.
// -kcarlos
//Toolbar : Definitions for button's states
#define STATE_NORMAL    0
#define STATE_PUSHED    1
#define STATE_GRAYED    2
#define STATE_ON        3


#ifdef RESOURCES
#define RES_STR(a, b, c) b, c
STRINGTABLE
BEGIN
#else

enum _RESOURCEIDS {
#define RES_STR(a, b, c) a = b,
#endif

//
// Error Messages
//

#define ERR_Reserved0                       0
#define ERR_Reserved1                       1
#define ERR_Reserved2                       2
RES_STR(ERR_File_Disk_Full,                 1020, "There is not enough space on disk to save '%s' file")
RES_STR(ERR_Assertion_Failed,               1021, "Assertion Failed")
RES_STR(ERR_Init_Application,               1022, "Windows Debugger cannot be initialized")
RES_STR(ERR_File_Name_Too_Long,             1023, "'%s' is too long for a filename")
RES_STR(ERR_Internal_Error,                 1024, "Internal Error.  Please contact Microsoft Technical Support (%s)")
RES_STR(ERR_File_Not_Found,                 1025, "The file '%s' cannot be found")
RES_STR(ERR_File_Open,                      1026, "The file '%s' cannot be opened")
RES_STR(ERR_File_Create,                    1027, "The file '%s' cannot be created")
RES_STR(ERR_File_Read,                      1028, "The file '%s' cannot be read")
RES_STR(ERR_File_Write,                     1029, "The file '%s' cannot be written")
RES_STR(ERR_TL_Cannot_Be_Overridden,        1030, "The Transport Layer cannot be overridden when debugging the kernel or a crash dump")
RES_STR(ERR_TL_Both_Overrides,              1031, "-n and -N are mutually exclusive, and both switches have been ignored. Using the default Transport Layer.")
RES_STR(ERR_TL_Cmd_Line_Opt_N,              1032, "-N is only supported by WindbgRM. For Windbg use the <.opt DllTl> from within the command window.")
RES_STR(ERR_DumpInvalidFile,                1033, "Invalid dump file. Please run DUMPCHK.EXE.")
RES_STR(ERR_DumpWrongPlatform,              1034, "Cross-platform debugging of dump files is not supported. Please run DUMPCHK.EXE.")
RES_STR(ERR_File_Not_Open,                  1035, "The file is not open")
RES_STR(ERR_Ini_File_Read,                  1036, "The options file cannot be read")
RES_STR(ERR_Ini_File_Write,                 1037, "The options file cannot be written")
RES_STR(ERR_No_Remote_Pipe_Name,            1038, "The -s option requires a pipe name")
RES_STR(ERR_No_TL_Selected,                 1039, "No transport layer has been selected.")
RES_STR(ERR_Transport_Doesnt_Exist,         1040, "The transport layer [%s] does not exist. Please select a valid transport layer.")
RES_STR(ERR_Unable_To_Initialize_WorkSpaces,1041, "Unable to initialize the workspaces")
RES_STR(ERR_Please_Select_A_TL,             1042, "Please select a transport layer.")

RES_STR(ERR_Too_Many_Opened_Documents,      1045, "The maximum number of open windows has been reached")
RES_STR(ERR_Too_Many_Opened_Views,          1046, "The maximum number of open windows has been reached")
RES_STR(ERR_Cannot_Allocate_Memory,         1047, "Memory cannot be allocated")
RES_STR(ERR_File_Already_Loaded,            1048, "File '%s' already loaded")
RES_STR(ERR_Not_A_Text_File,                1049, "'%s' is not a text file")



RES_STR(ERR_Change_Directory,               1055, "The current directory cannot be changed to '%s'")
RES_STR(ERR_Change_Drive,                   1056,"")



RES_STR(ERR_Invalid_Command_Line_File,      1070, "The command line filename '%s' is invalid")
RES_STR(ERR_Invalid_Command_Line,           1071, "Syntax error in command line")


RES_STR(ERR_File_Is_ReadOnly,               1075, "File is read only")


RES_STR(ERR_Clipboard_Overflow,             1077, "There is not enough room to paste from the Clipboard")
RES_STR(ERR_Cant_Load,                      1078, "Cannot load '%s'.")
RES_STR(ERR_Line_Too_Long,                  1079, "This line contains the maximum number of characters allowed")
RES_STR(ERR_Modified_Document_Corrupted,    1080, "This file has been corrupted.  Please save it with a new name, then exit and restart Windows Debugger")
RES_STR(ERR_Document_Corrupted,             1081, "This file has been corrupted.  Please exit and restart Windows Debugger")
RES_STR(ERR_UndoRedoBufferTooSmall,         1082, "There is not enough space in the Undo buffer to save this action.  Would you like to continue?")



RES_STR(ERR_Truncate_Line,                  1086, "Line #%u is too long and will be truncated.  Do you want to continue?")
RES_STR(ERR_Truncate_Doc,                   1087, "There are too many lines in this file.  The view will be truncated")
RES_STR(ERR_Too_Many_Lines,                 1088, "This file contains the maximum number of lines allowed")
RES_STR(ERR_String_Not_Found,               1089, "The string '%s' was not found")


RES_STR(ERR_No_DLL_Caller,                  1092, "No Debuggee specified")


RES_STR(ERR_Cannot_Load_DLL,                1094, "Cannot load the DLL '%s'\n%s")
RES_STR(ERR_DLL_Symbol_Unspecified,         1095, "Symbol handler DLL unspecified")
RES_STR(ERR_DLL_Expr_Unspecified,           1096, "Expression evaluator DLL unspecified")
RES_STR(ERR_DLL_Transport_Unspecified,      1097, "Transport layer DLL unspecified")
RES_STR(ERR_DLL_Exec_Unspecified,           1098, "Execution model DLL unspecified")
RES_STR(ERR_No_RegExp_Match,                1099, "The regular expression '%s' was not found")
RES_STR(ERR_Close_When_Debugging,           1100, "Debuggee still running. Do you still want to exit?")
RES_STR(ERR_Duplicate_File_Name,            1101, "You cannot use the name of an open file.")
RES_STR(ERR_RegExpr_Invalid,                1102, "'%s' is not a valid regular expression (invalid parameter).")
RES_STR(ERR_RegExpr_Undef,                  1103, "'%s' is not a valid regular expression (undefined operator).")
RES_STR(ERR_Tabs_OutOfRange,                1104, "'Tab Stops' must be between %i and %i.")


RES_STR(ERR_Cannot_Quit,                    1107, "Cannot exit Windows Debugger.")

RES_STR(ERR_Cannot_Create_Dlg,              1108, "Cannot create the dialog for the following reason:\n%s")


RES_STR(ERR_Lost_Default_Font,              1203,"The default font '%s' has been removed from the system. It has been replaced by the font '%s'.")
RES_STR(ERR_Lost_Font,                      1204,"One or more fonts used are not available.  The Default Font will be substitued for them.")
RES_STR(ERR_Tab_Too_Big,                    1205,"The new Tab Stop setting makes line %i of file '%s' too long. Enter a smaller value.")
RES_STR(ERR_Memory_Is_Low,                  1206,"The available memory is low. Close other applications or exit from Windows Debugger.")
RES_STR(ERR_Memory_Is_Low_2,                1207,"The available memory is low. Please exit.")
RES_STR(ERR_Invalid_Debugger_Dll,           1209,"Debugger DLL '%s' is invalid")
RES_STR(ERR_Initializing_Debugger,          1210,"Debugger DLL '%s' cannot be initialized")

RES_STR(ERR_AddrExpr_Invalid,               1211,"Invalid or unevaluatable address expression")
RES_STR(ERR_RangeExpr_Invalid,              1212,"Invalid or unevaluatable range expression")


RES_STR(ERR_Breakpoint_Not_Exist,           1214,"There is no breakpoint #%d")
RES_STR(ERR_Breakpoint_Already_Used,        1215,"There is already a breakpoint #%d")
RES_STR(ERR_Breakpoint_Not_Instantiated,    1216,"Breakpoint not instantiated")
RES_STR(ERR_Debuggee_Not_Alive,             1217,"Debuggee is not alive")
RES_STR(ERR_Process_Not_Exist,              1218,"Process does not exist")
RES_STR(ERR_Exception_Invalid,              1219,"Invalid exception")
RES_STR(ERR_Register_Invalid,               1220,"Invalid register")
RES_STR(ERR_Command_Error,                  1221,"Command Error")
RES_STR(ERR_No_Breakpoints,                 1222,"There are no breakpoints set")
RES_STR(ERR_No_Threads,                     1223,"No threads exist")
RES_STR(ERR_Edit_Failed,                    1224,"Unable to modify memory")
RES_STR(ERR_Expr_Invalid,                   1225,"Invalid or unevaluatable expression")
RES_STR(ERR_Radix_Invalid,                  1226,"Radix value must be 8, 10, or 16")
RES_STR(ERR_String_Invalid,                 1227,"Badly formed string")
RES_STR(ERR_Bad_Count,                      1228,"Bad count - use a positive decimal number")
RES_STR(ERR_Bad_Assembly,                   1229,"Cannot assemble input")
RES_STR(ERR_Cant_Go_Exception,              1230,"Stopped at exception - use the gh or the gn command")
RES_STR(ERR_Already_Running,                1231,"Thread is already running")
RES_STR(ERR_Cant_Step_Frozen,               1232,"Cannot step through a frozen thread - only F5 is allowed")
RES_STR(ERR_DbgState,                       1233,"Debugger is busy...")
RES_STR(ERR_Cant_Start_Proc,                1234,"Unable to start debuggee!")
RES_STR(ERR_Cant_Cont_Exception,            1235,"INTERNAL ERROR: Exception failed to continue")
RES_STR(ERR_Not_At_Exception,               1236,"Thread is not at an exception")
RES_STR(ERR_Thread_Wild_Invalid,            1237,"Thread wildcard is invalid here")
RES_STR(ERR_Process_Wild_Invalid,           1238,"Process wildcard is invalid here")
RES_STR(ERR_Cant_Freeze,                    1239,"INTERNAL ERROR: Cannot freeze thread")
RES_STR(ERR_Cant_Thaw,                      1240,"INTERNAL ERROR: Cannot thaw thread")
RES_STR(ERR_Stop_B4_Restart,                1241,"Cannot restart while running")


RES_STR(ERR_Thread_Exited,                  1243,"Thread has exited")
RES_STR(ERR_Cant_Continue_Rip,              1244,"INTERNAL ERROR: Cannot continue from RIP")
RES_STR(ERR_Error_Level_Invalid,            1245,"Valid error levels are 0-3")
RES_STR(ERR_Thread_Is_Frozen,               1246,"Thread is frozen")
RES_STR(ERR_Thread_Not_Frozen,              1247,"Thread is not frozen")
RES_STR(ERR_Cant_Step_Rip,                  1248,"Only F5 is allowed at RIP")


RES_STR(ERR_Cant_Assign_To_Reg,             1250,"You cannot change that register")
RES_STR(ERR_Simple_Go_Frozen,               1251,"Only F5 is allowed on frozen thread")
RES_STR(ERR_Cant_Run_Frozen_Proc,           1252,"Cannot run frozen process")
RES_STR(ERR_Go_Failed,                      1253,"Go command failed")
RES_STR(ERR_Read_Failed_At,                 1254,"Memory read failed at %s")
RES_STR(ERR_Write_Failed_At,                1255,"Memory write failed at %s")
RES_STR(ERR_No_Thread_With_Wildproc,        1256,"Cannot specify thread with process wildcard")
RES_STR(ERR_Invalid_Option,                 1257,"Invalid debug option")
RES_STR(ERR_True_False,                     1258,"Valid values are ON and OFF")
RES_STR(ERR_Bad_Context,                    1259,"Bad context string")
RES_STR(ERR_Process_Cant_Go,                1260,"Cannot complete go command on any threads in that process")
RES_STR(ERR_Debuggee_Starting,              1261,"Wait, debuggee is loading.")
RES_STR(ERR_Invalid_Process_Id,             1262,"Invalid process ID %ld")
RES_STR(ERR_Attach_Failed,                  1263,"Attach failed")
RES_STR(ERR_Detach_Failed,                  1264,"Detach failed")
RES_STR(ERR_Kill_Failed,                    1265,"Kill failed")
RES_STR(ERR_Debuggee_Not_Loaded,            1266,"No debuggee is loaded")
RES_STR(ERR_No_ExceptionList,               1267,"Cannot obtain exception list")


RES_STR(ERR_Not_Unique_Shortname,           1269,"Short name '%s' duplicates an existing name, and must be unique.")




RES_STR(ERR_NoSymbols,                      1272,"No Symbolic Info for Debuggee")


RES_STR(ERR_Not_Windbg_DLL,                 1274,"'%s' is not a valid WinDbg DLL")
RES_STR(ERR_Wrong_DLL_Type,                 1275,"The DLL '%s' is type '%2.2s', type '%2.2s' was expected")
RES_STR(ERR_Wrong_DLL_Version,              1276,"DLL '%s' is release type %d, version %d.  %d, %d was expected")
RES_STR(ERR_Goto_Line,                      1277,"Cannot understand line to go to")
RES_STR(ERR_Function_Locate,                1278,"Cannot locate function or address")


RES_STR(ERR_Dll_Key_Missing,                1280,"Windows Debugger tried to load %s, which is not listed\r\nas a %s DLL.")
RES_STR(ERR_File_Deleted,                   1281,"%s No longer exists on disk, use click Save from the File menu to restore it")
RES_STR(ERR_Empty_Shortname,                1282,"You must enter a short name")
RES_STR(ERR_Exception_Unknown,              1283,"Exception 0x%08lx unknown")
RES_STR(ERR_NoCodeForFileLine,              1284,"Code not found, breakpoint not set")
RES_STR(ERR_Breakpoint_Not_Set,             1285,"Breakpoint not set")
RES_STR(ERR_Wrong_Remote_DLL_Version,       1286,"Remote transport DLL connecting to '%s' is the wrong version")


RES_STR(ERR_Cannot_Connect,                 1288,"Transport DLL '%s' cannot connect to remote computer")
RES_STR(ERR_Cant_Open_Com_Port,             1289,"Remote transport cannot open comm port '%s'")
RES_STR(ERR_Bad_Com_Parameters,             1390,"Remote transport cannot set comm parameters '%s'")
RES_STR(ERR_Bad_Pipe_Server,                1391,"Remote transport cannot find named pipe server '%s'")
RES_STR(ERR_Bad_Pipe_Name,                  1392,"Remote transport cannot find named pipe '%s'")
RES_STR(ERR_NoModulesFound,                 1393,"No modules found")
RES_STR(ERR_ModuleNotFound,                 1394,"Module '%s' not found")
RES_STR(ERR_BP_Edited,                      1395,"Breakpoint was edited but was not added.\r\nAre you sure you want to leave without adding it?")
RES_STR(ERR_Expclass_No_Members,            1396,"Expanded class with no members ")
RES_STR(ERR_No_Funcs_In_Watch,              1397,"Function calls not supported in watch window ")
RES_STR(ERR_Unable_To_Complete_Gountil,     1398,"Unable to complete Go-Until command")
RES_STR(ERR_Start_Failed,                   1399,"Start failed")


// Memory win strings

RES_STR(ERR_Expression_Not_Parsable,        1400,"Cannot parse expression")
RES_STR(ERR_Expression_Not_Bindable,        1401,"Cannot bind expression")
RES_STR(ERR_Memory_Context,                 1402,"Memory object out of context")


RES_STR(ERR_Cant_Modify_BP_While_Running,   1410,"Debuggee must be stopped before breakpoints can be modified.")
RES_STR(ERR_Invalid_Crashdump_File,         1411,"Cannot initialize %s for crash dump analysis")


RES_STR(ERR_Entries_Cant_be_blank,          1412,"Neither path entry can be blank.")

// Error installing windbg as postmortem debugger
RES_STR(ERR_Inst_Postmortem_Debugger,       1413,"An error occurred while trying to install Windows Debugger as the postmortem \
debugger. Please make sure [AEDebug] key exists in the registry and your account has permission to modify it.")

// Attach to a process
RES_STR(ERR_Invalid_Process_Name,           1414,"Invalid Process Name %s")

// Error string used when validating a series of path separated by ';'
RES_STR(ERR_Path_Too_Long,                  1415,"The maximum length for a path is %d characters.\n Please truncate the following path:\n\n%s")
RES_STR(ERR_Pipes_Not_Supported_On_Win9x,   1416,"The transport layer <%s> is currently in use, but it uses named pipes, which are not supported on Microsoft Windows 95/98. Windbg may not run correctly. Please select a transport layer that does not use %s.")


RES_STR(ERR_Must_Supply_WrkSpc_Name,        1417,"Please specify a name for this work space.")
RES_STR(ERR_Deleting_Registry_Key,          1418,"Unable to delete the registry key.")
RES_STR(ERR_Not_YetImplemented,             1419,"Not yet implemented.")



//
// System Strings
//

RES_STR(SYS_Main_wTitle,                    2000,"Windows Debugger:" VER_PRODUCTVERSION_STR " " )
RES_STR(SYS_Main_wClass,                    2001,"QcQpClass")
RES_STR(SYS_Child_wClass,                   2002,"QcQpChildClass")

#if !defined( NEW_WINDOWING_CODE )
RES_STR(SYS_Cmd_wClass,                     2003,"CmdClass")
RES_STR(SYS_Float_wClass,                   2004,"FloatClass")
RES_STR(SYS_Memory_wClass,                  2005,"MemoryClass")
RES_STR(SYS_Edit_wClass,                    2006,"EditClass")
RES_STR(SYS_Cpu_wClass,                     2007,"CpuClass")
RES_STR(SYS_Watch_wClass,                   2008,"WatchClass")
RES_STR(SYS_Locals_wClass,                  2009,"LocalsClass")
RES_STR(SYS_Disasm_wClass,                  2010,"DisasmClass")
#else
RES_STR(SYS_NewCmd_wClass,                  2003,"NewCmdClass")
RES_STR(SYS_NewFloat_wClass,                2004,"NewFloatClass")
RES_STR(SYS_NewMemory_wClass,               2005,"NewMemoryClass")
RES_STR(SYS_NewEdit_wClass,                 2006,"NewEditClass")
RES_STR(SYS_NewCpu_wClass,                  2007,"NewCpuClass")
RES_STR(SYS_NewWatch_wClass,                2008,"NewWatchClass")
RES_STR(SYS_NewLocals_wClass,               2009,"NewLocalsClass")
RES_STR(SYS_NewCalls_wClass,                2010,"NewCallsClass")
RES_STR(SYS_NewDocs_wClass,                 2011,"NewDocsClass")
RES_STR(SYS_NewDisasm_wClass,               2012,"NewDisasmClass")
#endif

RES_STR(SYS_QCQPCtrl_wClass,                2013,"QCQPCtrlClass")

RES_STR(SYS_Driver_FileExt,                 2014,"DRV")
RES_STR(SYS_Help_FileExt,                   2015,"HLP")
RES_STR(SYS_Bad_Sym_Col_Hdr1,               2016,"Symbol File")
RES_STR(SYS_Bad_Sym_Col_Hdr2,               2017,"Load Message")
RES_STR(SYS_Bad_Sym_Col_Hdr3,               2018,"Extended Information")
RES_STR(SYS_Image_Col_Hdr1,                 2019,"Image File")
RES_STR(SYS_Image_Col_Hdr2,                 2020,"Time Stamp")
RES_STR(SYS_Does_Not_Exist_Create,          2021,"'%s' does not exist.")
RES_STR(SYS_Desc_For_TL_Created_From_Cmd_Wnd,2022,"TL created from within the command window")
RES_STR(SYS_New_TL_Settings,                2023,"New TL [%s] parameters set: %s %s")




RES_STR(SYS_StatusClear,                    2024," ")
RES_STR(SYS_CpuWin_Title,                   2025,"&Registers")
RES_STR(SYS_WatchWin_Title,                 2026,"&Watch")
RES_STR(SYS_LocalsWin_Title,                2027,"&Locals")
RES_STR(SYS_DisasmWin_Title,                2028,"&Disassembly")
RES_STR(SYS_CmdWin_Title,                   2029,"C&ommand")
RES_STR(SYS_FloatWin_Title,                 2030,"&Floating Point")
RES_STR(SYS_MemoryWin_Title,                2031,"Memory")
RES_STR(SYS_Nb_Of_Occurrences_Replaced,     2032,"%i occurrence(s) have been replaced")
RES_STR(SYS_File_Changed,                   2033,"Another application has changed the file '%s'.  Do you want to reload it?")
RES_STR(SYS_Alt,                            2034,"Alt")
RES_STR(SYS_Shift,                          2035,"Shift")
RES_STR(SYS_Save_Changes_To,                2036,"Would you like to save the changes made in '%s'?")
RES_STR(SYS_My_String,                      2037,"%s")

RES_STR(SYS_Warning,                        2038,"Warning")

RES_STR(SYS_New_Workspace_Prompt,           2039,"Current values in your workspace are about to be replaced by the default values. Do you wish to continue?")







RES_STR(SYS_Free_Memory,                    2043,"Cannot free memory")
RES_STR(SYS_Lock_Memory,                    2044,"Cannot lock memory")








RES_STR(SYS_Untitled_File,                  2049,"UNTITLED %d")


RES_STR(SYS_Allocate_Memory,                2051, "")
RES_STR(SYS_RedoBufferOverflow,             2052,"Redo Buffer Overflow")
RES_STR(SYS_RegExpr_StackOverflow,          2053, "")
RES_STR(SYS_RegExpr_CompileAction,          2054, "")
RES_STR(SYS_RegExpr_EstimateAction,         2055, "")




RES_STR(SYS_Done,                           2058,"Done")
RES_STR(SYS_Quick_wClass,                   2059,"QuickClass")
RES_STR(SYS_Untitled_Workspace,             2060,"Untitled")
RES_STR(SYS_Calls_wClass,                   2061,"CallsClass")
RES_STR(SYS_CallsWin_Title,                 2062,"&Calls")
RES_STR(SYS_TRANSPORT_LAYER_COL_HDR1,       2063,"Name")
RES_STR(SYS_TRANSPORT_LAYER_COL_HDR2,       2064,"Description")
RES_STR(SYS_TRANSPORT_LAYER_COL_HDR3,       2065,"DLL")
RES_STR(SYS_TRANSPORT_LAYER_COL_HDR4,       2066,"Params")








RES_STR(SYS_SRC_ROOT_MAPPING_COL_HDR1,      2071,"Source")
RES_STR(SYS_SRC_ROOT_MAPPING_COL_HDR2,      2072,"Mapped To")
//
// Debugger strings
//

RES_STR(DBG_Success_Inst_Postmortem_Debugger, 2200, "WinDbg has been installed as the default postmortem application debugger")
 
 








RES_STR(DBG_Checksum_Mismatch,              2205,"sym 0x%08X img 0x%08X ")
RES_STR(DBG_Timestamp_Mismatch,             2206,"sym 0x%08X img 0x%08X ")
 
 
 
 
RES_STR(DBG_Exception1_Occurred,            2207,"First chance exception %08lx (%s) occurred")
RES_STR(DBG_Exception2_Occurred,            2208,"Second chance exception %08lx (%s) occurred")
RES_STR(DBG_Thread_Stopped,                 2209,"Thread stopped.")
RES_STR(DBG_Go_When_Frozen,                 2210,"(Thread is still frozen)")
RES_STR(DBG_Notify_Break_Levels,            2211,"Notify %d, break %d")




RES_STR(DBG_Bytes_Copied,                   2214,"%u of %u bytes copied")
RES_STR(DBG_Symbol_Not_Found,               2215,"No symbol found")
RES_STR(DBG_Not_Exact_Fill,                 2216,"Range not divisible by object size -\r\nthe last byte will not be filled")
RES_STR(DBG_Not_Exact_Fills,                2217,"Range not divisible by object size -\r\nlast %d bytes will not be filled")
RES_STR(DBG_Attach_Running,                 2218,"Process attached is running")
RES_STR(DBG_Attach_Stopped,                 2219,"Process attached is stopped")
RES_STR(DBG_Bad_DLL_YESNO,                  2220,"Invalid debugger DLL '%s'. Do you want to fix it now?")
RES_STR(DBG_Deleting_DLL,                   2221,"\
Deleting %s DLL configuration '%s'.\r\n\
If a workspace refers to this configuration, you\r\n\
you need to correct it when that workspace is next\r\n\
loaded. Press OK to delete, Cancel to keep it.")
RES_STR(DBG_Losing_Command_Line,            2222,"Canceling command line '%s'")
RES_STR(DBG_Attach_Deadlock,                2223,"\
The debuggee is deadlocked during startup or attaching\r\n\
to a crashed process.  This is probably because the\r\n\
crash occurred in a DllMain function.  You will be able\r\n\
to examine variables, the stack, etc., but the debuggee\r\n\
will probably not be able to step or run.")
// BUGBUG - Dead code
// RES_STR(DBG_DebugInfo,                      2224, "The file '%s'\r\n%s\r\n\r\nUse this file anyway?")
RES_STR(DBG_Hard_Coded_Breakpoint,          2225, "Hard coded breakpoint hit")
RES_STR(DBG_At_Entry_Point,                 2226, "Stopped at program entry point")
RES_STR(DBG_Default_Exception_Text,         2227, "Debugger will %s first chance exceptions not listed.\r\n")
RES_STR(DBG_Default_Exception_Ignore,       2228, "ignore")
RES_STR(DBG_Default_Exception_Stop,         2229, "stop for")
RES_STR(DBG_Default_Exception_Notify,       2230, "announce and continue")
RES_STR(DBG_Restart_With_New_Args,          2231, "Arguments have changed... Restart now with new arguments?")

//
// Status Bar Messages
//


RES_STR(STA_Program_Opened,                 6100,"Opened the program %s")
RES_STR(STA_Undo,                           6160,"Undoing the action #%i")
RES_STR(STA_Redo,                           6170,"Redoing the action #%i")
RES_STR(STA_End_Of_Undo,                    6180,"Undo is complete")
RES_STR(STA_End_Of_Redo,                    6190,"Redo is complete")
RES_STR(STA_Open_MRU_File,                  6200,"Open this file")
RES_STR(STA_Open_MRU_Project,               6210,"Open this project")
RES_STR(STA_Open_MDI_Window,                6220,"Activate the window ")
RES_STR(STA_Find_Hit_EOF,                   6240,"End of file")
RES_STR(STA_Loading_Workspace,              6241,"Loading workspace...")
RES_STR(STA_Saving_Workspace,               6242,"Saving workspace...")
RES_STR(STA_Empty,                          6243," ")

//
// Resource Strings
//




//
// File-box title strings
//

RES_STR(DLG_Open_Filebox_Title,             3200,"Open Source File")
RES_STR(DLG_SaveAs_Filebox_Title,           3201,"Save As")
RES_STR(DLG_Merge_Filebox_Title,            3202,"Merge")
RES_STR(DLG_Browse_Filebox_Title,           3203,"Browse For File ")
RES_STR(DLG_Browse_DbugDll_Title,           3204,"Browse For DLL ")
RES_STR(DLG_Browse_For_Symbols_Title,       3205,"Open Symbol File For ")
RES_STR(DLG_Browse_LogFile_Title,           3206,"Browse For Log File")
RES_STR(DLG_Browse_Executable_Title,        3207,"Open Executable")
RES_STR(DLG_Browse_CrashDump_Title,         3208,"Open Crash Dump")


//
// Version string
//

RES_STR(DLG_Version,                        3210,"%d.%02d.%03d %s")
RES_STR(DLG_VersionIni,                     3211,"0021")

//
// Dialog Boxes Strings
//
        // Dont insert
        // From here
        // These ID numbers
        // must be separated
        // by one and sorted
        // ascending
        // Up to there

// BUGBUG - Dead code
// RES_STR(DLG_Cols_SourceWindow,              4001,"Source Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_WatchWindow,               4002,"Watch Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_LocalsWindow,              4003,"Locals Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_CpuWindow,                 4004,"Registers Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_DisasmWindow,              4005,"Disassembler Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_CommandWindow,             4006,"Command Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_FloatWindow,               4007,"Floating Point Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_MemoryWindow,              4008,"Memory Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_CallsWindow,               4009,"Calls Window")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_BreakpointLine,            4010,"Breakpoint Line")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_CurrentLine,               4011,"Current Line")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_TaggedLine,                4012,"Tagged Line")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_Selection,                 4013,"Text Selection")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_Keyword,                   4014,"Keyword")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_Identifier,                4015,"Identifier")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_Comment,                   4016,"Comment")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_Number,                    4017,"Number")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_Real,                      4018,"Real")
// BUGBUG - Dead code
// RES_STR(DLG_Cols_String,                    4019,"String")
RES_STR(DLG_Cols_Freeze,                    4020,"Free&ze")
RES_STR(DLG_Cols_Thaw,                      4021,"Tha&w")

    // Editor : Do not change the order without forcasting consequences
    // there is a link between the first several colors and the type of
    // the document in document structure

#define DLG_Cols_First  0
// BUGBUG - Dead code
// RES_STR(Cols_SourceWindow,                  0, "")
// BUGBUG - Dead code
// RES_STR(Cols_DummyWindow,                   1, "")
// BUGBUG - Dead code
// RES_STR(Cols_WatchWindow,                   2, "")
// BUGBUG - Dead code
// RES_STR(Cols_LocalsWindow,                  3, "")
// BUGBUG - Dead code
// RES_STR(Cols_CpuWindow,                     4, "")
// BUGBUG - Dead code
// RES_STR(Cols_DisasmWindow,                  5, "")
// BUGBUG - Dead code
// RES_STR(Cols_CommandWindow,                 6, "")
// BUGBUG - Dead code
// RES_STR(Cols_FloatWindow,                   7, "")
// BUGBUG - Dead code
// RES_STR(Cols_MemoryWindow,                  8, "")
// BUGBUG - Dead code
// RES_STR(Cols_CallsWindow,                   9, "")
RES_STR(Cols_BreakpointLine,                10,"")
RES_STR(Cols_CurrentLine,                   11, "")
RES_STR(Cols_CurrentBreak,                  12, "")
RES_STR(Cols_UnInstantiatedBreakpoint,      13, "")
RES_STR(Cols_TaggedLine,                    14, "")
RES_STR(Cols_Selection,                     15, "")
RES_STR(Cols_Keyword,                       16, "")
RES_STR(Cols_Identifier,                    17, "")
RES_STR(Cols_Comment,                       18, "")
RES_STR(Cols_Number,                        19, "")
RES_STR(Cols_Real,                          20, "")
RES_STR(Cols_String,                        21, "")
// BUGBUG - Dead code
// RES_STR(Cols_ActiveEdit,                    22, "")
// BUGBUG - Dead code
// RES_STR(Cols_ChangeHistory,                 23, "")
#define DLG_Cols_Last                       23

// BUGBUG - Dead code
// RES_STR(DLG_Cols_Sample_Text,               4030, " -- Sample Text -- ")


RES_STR(DLG_CloseText1,                     4060,"Save state information for program %s")
RES_STR(DLG_CloseText2,                     4061,"(Current workspace: %s) ?")
RES_STR(DLG_CannotUnload,                   4062,"Unable to unload current program")
RES_STR(DLG_NoSuchWorkSpace,                4063,"Specified workspace does not exist, loading Common.")
RES_STR(DLG_Deleting_DLL_Title,             4064,"Confirm deletion of Transport Layer")
// BUGBUG - Dead code
// RES_STR(DLG_LoadedDefault,                  4065,"Program has no workspace, loaded the Common workspace.")
RES_STR(DLG_WorkSpaceSaved,                 4066,"Workspace saved.")
RES_STR(DLG_WorkSpaceNotSaved,              4067,"Could not save workspace.")
RES_STR(DLG_DefaultSaved,                   4068,"Common Workspace saved.")
RES_STR(DLG_DefaultNotSaved,                4069,"Could not save Common Workspace.")
RES_STR(DLG_NoMsg,                          4080,"Must select a message")
RES_STR(DLG_AddPathToSymSearchPath,         4081,"Would you like to add <%s> to the symbols search path?")
// BUGBUG - Dead code
// RES_STR(DLG_AlreadyAdded,                   4090,"DLL already in list, use MODIFY to change settings")
RES_STR(DLG_ResolveBpCaption,               4091,"Ambiguous Breakpoint")
RES_STR(DLG_ResolveFuncCaption,             4092,"Ambiguous Expression")
RES_STR(DLG_DefaultDoesNotExist,            4093,"Default workspace not found, using Common instead")
RES_STR(DLG_ResolveFSCaption,               4094,"Ambiguous Source File Name")
RES_STR(DLG_AskToSaveWorkspace,             4095,"Save current workspace?")

//
// Environment text strings
//

RES_STR(ENV_Include_Var_Name,               7010, "")
RES_STR(ENV_Library_Var_Name,               7020, "")

//
// Dos Exec Errors (Keep the resource # in accordance with error #)
//

// BUGBUG - Dead code
// RES_STR(DOS_Err_0,                          8000,"There is not enough memory to start the application")
// BUGBUG - Dead code
// RES_STR(DOS_Err_2,                          8002,"Cannot find the application")
// BUGBUG - Dead code
// RES_STR(DOS_Err_3,                          8003,"Cannot find the application on the specified path")
// BUGBUG - Dead code
// RES_STR(DOS_Err_5,                          8005,"Cannot dynamically link to a task")
// BUGBUG - Dead code
// RES_STR(DOS_Err_6,                          8006,"Library requires separate data segments for each task")
// BUGBUG - Dead code
// RES_STR(DOS_Err_10,                         8010,"This is the incorrect Windows version for this application")
// BUGBUG - Dead code
// RES_STR(DOS_Err_11,                         8011,"This is an invalid .EXE file (Non-Windows or error in image)")
// BUGBUG - Dead code
// RES_STR(DOS_Err_12,                         8012,"Cannot run an OS/2 application")
// BUGBUG - Dead code
// RES_STR(DOS_Err_13,                         8013,"Cannot run a DOS 4.0 application")
// BUGBUG - Dead code
// RES_STR(DOS_Err_14,                         8014,"The application type is unknown")
// BUGBUG - Dead code
// RES_STR(DOS_Err_15,                         8015,"Cannot run an Old Windows .EXE in protected mode")
// BUGBUG - Dead code
// RES_STR(DOS_Err_16,                         8016,"Cannot run a second instance of an application containing multiple, writeable data segments")
// BUGBUG - Dead code
// RES_STR(DOS_Err_17,                         8017,"Cannot run a second instance of this application in large-frame EMS mode")
// BUGBUG - Dead code
// RES_STR(DOS_Err_18,                         8018,"Cannot run a protected-mode application in real mode")

//
// Debug Menu option strings
// Set Breakpoint Action listbox - do not change
// numbering without changing the BREAKPOINTACTIONS
// enumeration
//

#define DBG_Brk_Start_Actions               9001
RES_STR(DBG_Brk_At_Loc,                     9001,"Break at location")
RES_STR(DBG_Brk_At_Loc_Expr_True,           9002,"Break at location if expression is true")
RES_STR(DBG_Brk_At_Loc_Expr_Chgd,           9003,"Break at location if memory changes")
RES_STR(DBG_Brk_Expr_True,                  9004,"Break if expression is true")
RES_STR(DBG_Brk_Expr_Chgd,                  9005,"Break if memory changes")
RES_STR(DBG_Brk_At_WndProc,                 9006,"Break at window proc")
RES_STR(DBG_Brk_At_WndProc_Expr_True,       9007,"Break at window proc if expression is true")
RES_STR(DBG_Brk_At_WndProc_Expr_Chgd,       9008,"Break at window proc if memory changes")
RES_STR(DBG_Brk_At_WndProc_Msg_Recvd,       9009,"Break at window proc if message received")
#define DBG_Brk_End_Actions                 9009

//
// Message strings for the message selection combobox
//

// BUGBUG - Dead code
// #define DBG_Msgs_Start                      9020
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_1,                      DBG_Msgs_Start, "WM_ACTIVATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_2,                      DBG_Msgs_Start+1,"WM_ACTIVATEAPP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_3,                      DBG_Msgs_Start+2,"WM_ASKCBFORMATNAME")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_4,                      DBG_Msgs_Start+3,"WM_CANCELMODE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_5,                      DBG_Msgs_Start+4,"WM_CHANGECBCHAIN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_6,                      DBG_Msgs_Start+5,"WM_CHAR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_7,                      DBG_Msgs_Start+6,"WM_CHARTOITEM")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_8,                      DBG_Msgs_Start+7,"WM_CHILDACTIVATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_9,                      DBG_Msgs_Start+8,"WM_CLEAR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_10,                     DBG_Msgs_Start+9,"WM_CLOSE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_11,                     DBG_Msgs_Start+10,"WM_COMMAND")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_12,                     DBG_Msgs_Start+11,"WM_COMPACTING")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_13,                     DBG_Msgs_Start+12,"WM_COMPAREITEM")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_14,                     DBG_Msgs_Start+13,"WM_COPY")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_15,                     DBG_Msgs_Start+14,"WM_CREATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_16,                     DBG_Msgs_Start+15,"WM_CTLCOLOR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_17,                     DBG_Msgs_Start+16,"WM_CUT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_18,                     DBG_Msgs_Start+17,"WM_DEADCHAR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_19,                     DBG_Msgs_Start+18,"WM_DELETEITEM")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_20,                     DBG_Msgs_Start+19,"WM_DESTROY")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_21,                     DBG_Msgs_Start+20,"WM_DESTROYCLIPBOARD")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_22,                     DBG_Msgs_Start+21,"WM_DEVMODECHANGE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_23,                     DBG_Msgs_Start+22,"WM_DRAWCLIPBOARD")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_24,                     DBG_Msgs_Start+23,"WM_DRAWITEM")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_25,                     DBG_Msgs_Start+24,"WM_ENABLE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_26,                     DBG_Msgs_Start+25,"WM_ENDSESSION")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_27,                     DBG_Msgs_Start+26,"WM_ENTERIDLE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_28,                     DBG_Msgs_Start+27,"WM_ERASEBKGND")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_29,                     DBG_Msgs_Start+28,"WM_FONTCHANGE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_30,                     DBG_Msgs_Start+29,"WM_GETDLGCODE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_31,                     DBG_Msgs_Start+30,"WM_GETFONT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_32,                     DBG_Msgs_Start+31,"WM_GETMINMAXINFO")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_33,                     DBG_Msgs_Start+32,"WM_GETTEXT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_34,                     DBG_Msgs_Start+33,"WM_GETTEXTLENGTH")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_35,                     DBG_Msgs_Start+34,"WM_HSCROLL")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_36,                     DBG_Msgs_Start+35,"WM_HSCROLLCLIPBOARD")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_37,                     DBG_Msgs_Start+36,"WM_ICONERASEBKGND")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_38,                     DBG_Msgs_Start+37,"WM_INITDIALOG")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_39,                     DBG_Msgs_Start+38,"WM_INITMENU")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_40,                     DBG_Msgs_Start+39,"WM_INITMENUPOPUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_41,                     DBG_Msgs_Start+40,"WM_KEYDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_42,                     DBG_Msgs_Start+41,"WM_KEYFIRST")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_43,                     DBG_Msgs_Start+42,"WM_KEYLAST")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_44,                     DBG_Msgs_Start+43,"WM_KEYUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_45,                     DBG_Msgs_Start+44,"WM_KILLFOCUS")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_46,                     DBG_Msgs_Start+45,"WM_LBUTTONDBLCLK")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_47,                     DBG_Msgs_Start+46,"WM_LBUTTONDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_48,                     DBG_Msgs_Start+47,"WM_LBUTTONUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_49,                     DBG_Msgs_Start+48,"WM_MBUTTONDBLCLK")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_50,                     DBG_Msgs_Start+49,"WM_MBUTTONDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_51,                     DBG_Msgs_Start+50,"WM_MBUTTONUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_52,                     DBG_Msgs_Start+51,"WM_MDIACTIVATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_53,                     DBG_Msgs_Start+52,"WM_MDICASCADE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_54,                     DBG_Msgs_Start+53,"WM_MDICREATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_55,                     DBG_Msgs_Start+54,"WM_MDIDESTROY")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_56,                     DBG_Msgs_Start+55,"WM_MDIGETACTIVE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_57,                     DBG_Msgs_Start+56,"WM_MDIICONARRANGE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_58,                     DBG_Msgs_Start+57,"WM_MDIMAXIMIZE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_59,                     DBG_Msgs_Start+58,"WM_MDINEXT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_60,                     DBG_Msgs_Start+59,"WM_MDIRESTORE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_61,                     DBG_Msgs_Start+60,"WM_MDISETMENU")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_62,                     DBG_Msgs_Start+61,"WM_MDITILE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_63,                     DBG_Msgs_Start+62,"WM_MEASUREITEM")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_64,                     DBG_Msgs_Start+63,"WM_MENUCHAR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_65,                     DBG_Msgs_Start+64,"WM_MENUSELECT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_66,                     DBG_Msgs_Start+65,"WM_MOUSEACTIVATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_67,                     DBG_Msgs_Start+66,"WM_MOUSEFIRST")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_68,                     DBG_Msgs_Start+67,"WM_MOUSELAST")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_69,                     DBG_Msgs_Start+68,"WM_MOUSEMOVE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_70,                     DBG_Msgs_Start+69,"WM_MOVE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_71,                     DBG_Msgs_Start+70,"WM_NCACTIVATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_72,                     DBG_Msgs_Start+71,"WM_NCCALCSIZE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_73,                     DBG_Msgs_Start+72,"WM_NCCREATE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_74,                     DBG_Msgs_Start+73,"WM_NCDESTROY")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_75,                     DBG_Msgs_Start+74,"WM_NCHITTEST")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_76,                     DBG_Msgs_Start+75,"WM_NCLBUTTONDBLCLK")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_77,                     DBG_Msgs_Start+76,"WM_NCLBUTTONDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_78,                     DBG_Msgs_Start+77,"WM_NCLBUTTONUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_79,                     DBG_Msgs_Start+78,"WM_NCMBUTTONDBLCLK")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_80,                     DBG_Msgs_Start+79,"WM_NCMBUTTONDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_81,                     DBG_Msgs_Start+80,"WM_NCMBUTTONUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_82,                     DBG_Msgs_Start+81,"WM_NCMOUSEMOVE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_83,                     DBG_Msgs_Start+82,"WM_NCPAINT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_84,                     DBG_Msgs_Start+83,"WM_NCRBUTTONDBLCLK")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_85,                     DBG_Msgs_Start+84,"WM_NCRBUTTONDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_86,                     DBG_Msgs_Start+85,"WM_NCRBUTTONUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_87,                     DBG_Msgs_Start+86,"WM_NEXTDLGCTL")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_88,                     DBG_Msgs_Start+87,"WM_NULL")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_89,                     DBG_Msgs_Start+88,"WM_PAINT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_90,                     DBG_Msgs_Start+89,"WM_PAINTCLIPBOARD")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_91,                     DBG_Msgs_Start+90,"WM_PAINTICON")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_92,                     DBG_Msgs_Start+91,"WM_PALETTECHANGED")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_93,                     DBG_Msgs_Start+92,"WM_PALETTEISCHANGING")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_94,                     DBG_Msgs_Start+93,"WM_PARENTNOTIFY")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_95,                     DBG_Msgs_Start+94,"WM_PASTE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_96,                     DBG_Msgs_Start+95,"WM_QUERYDRAGICON")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_97,                     DBG_Msgs_Start+96,"WM_QUERYENDSESSION")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_98,                     DBG_Msgs_Start+97,"WM_QUERYNEWPALETTE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_99,                     DBG_Msgs_Start+98,"WM_QUERYOPEN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_100,                    DBG_Msgs_Start+99,"WM_QUEUESYNC")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_101,                    DBG_Msgs_Start+100,"WM_QUIT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_102,                    DBG_Msgs_Start+101,"WM_RBUTTONDBLCLK")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_103,                    DBG_Msgs_Start+102,"WM_RBUTTONDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_104,                    DBG_Msgs_Start+103,"WM_RBUTTONUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_105,                    DBG_Msgs_Start+104,"WM_RENDERALLFORMATS")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_106,                    DBG_Msgs_Start+105,"WM_RENDERFORMAT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_107,                    DBG_Msgs_Start+106,"WM_SETCURSOR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_108,                    DBG_Msgs_Start+107,"WM_SETFOCUS")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_109,                    DBG_Msgs_Start+108,"WM_SETFONT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_110,                    DBG_Msgs_Start+109,"WM_SETREDRAW")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_111,                    DBG_Msgs_Start+110,"WM_SETTEXT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_112,                    DBG_Msgs_Start+111,"WM_SHOWWINDOW")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_113,                    DBG_Msgs_Start+112,"WM_SIZE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_114,                    DBG_Msgs_Start+113,"WM_SIZECLIPBOARD")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_115,                    DBG_Msgs_Start+114,"WM_SPOOLERSTATUS")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_116,                    DBG_Msgs_Start+115,"WM_SYSCHAR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_117,                    DBG_Msgs_Start+116,"WM_SYSCOLORCHANGE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_118,                    DBG_Msgs_Start+117,"WM_SYSCOMMAND")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_119,                    DBG_Msgs_Start+118,"WM_SYSDEADCHAR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_120,                    DBG_Msgs_Start+119,"WM_SYSKEYDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_121,                    DBG_Msgs_Start+120,"WM_SYSKEYUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_122,                    DBG_Msgs_Start+121,"WM_TIMECHANGE")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_123,                    DBG_Msgs_Start+122,"WM_TIMER")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_124,                    DBG_Msgs_Start+123,"WM_UNDO")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_125,                    DBG_Msgs_Start+124,"WM_VKEYTOITEM")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_126,                    DBG_Msgs_Start+125,"WM_VSCROLL")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_127,                    DBG_Msgs_Start+126,"WM_VSCROLLCLIPBOARD")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_128,                    DBG_Msgs_Start+127,"WM_WININICHANGE")
// add messages for IME
//RES_STR(DBG_Msgs_WM_129,                    DBG_Msgs_Start+128,"WM_CONVERTREQUESTEX")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_129,                    DBG_Msgs_Start+128,"WM_IME_CHAR")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_130,                    DBG_Msgs_Start+129,"WM_IME_COMPOSITION")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_131,                    DBG_Msgs_Start+130,"WM_IME_COMPOSITIONFULL")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_132,                    DBG_Msgs_Start+131,"WM_IME_CONTROL")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_133,                    DBG_Msgs_Start+132,"WM_IME_ENDCOMPOSITION")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_134,                    DBG_Msgs_Start+133,"WM_IME_KEYDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_135,                    DBG_Msgs_Start+134,"WM_IME_KEYLAST")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_136,                    DBG_Msgs_Start+135,"WM_IME_KEYUP")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_137,                    DBG_Msgs_Start+136,"WM_IME_NOTIFY")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_138,                    DBG_Msgs_Start+137,"WM_IME_SELECT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_139,                    DBG_Msgs_Start+138,"WM_IME_SETCONTEXT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_140,                    DBG_Msgs_Start+139,"WM_IME_STARTCOMPOSITION")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_141,                    DBG_Msgs_Start+140,"WM_IME_REPORT")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_142,                    DBG_Msgs_Start+141,"WM_IMEKEYDOWN")
// BUGBUG - Dead code
// RES_STR(DBG_Msgs_WM_143,                    DBG_Msgs_Start+142,"WM_IMEKEYUP")
// BUGBUG - Dead code
// #define DBG_Msgs_End                        DBG_Msgs_WM_143

//
// definitions QCQP user control string IDs stored in string table
// each controls must be together and must use the states offset.
//

RES_STR(IDS_CTRL_TRACENORMAL,               10100 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_TRACEPUSHED,               10100 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_TRACEGRAYED,               10100 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_STEPNORMAL,                10110 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_STEPPUSHED,                10110 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_STEPGRAYED,                10110 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_BREAKNORMAL,               10120 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_BREAKPUSHED,               10120 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_BREAKGRAYED,               10120 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_GONORMAL,                  10130 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_GOPUSHED,                  10130 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_GOGRAYED,                  10130 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_HALTNORMAL,                10140 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_HALTPUSHED,                10140 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_HALTGRAYED,                10140 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_QWATCHNORMAL,              10150 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_QWATCHPUSHED,              10150 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_QWATCHGRAYED,              10150 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_SMODENORMAL,               10160 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_SMODEPUSHED,               10160 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_SMODEGRAYED,               10160 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_AMODENORMAL,               10170 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_AMODEPUSHED,               10170 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_AMODEGRAYED,               10170 + STATE_GRAYED, "")

RES_STR(IDS_CTRL_FORMATNORMAL,              10180 + STATE_NORMAL, "")
RES_STR(IDS_CTRL_FORMATPUSHED,              10180 + STATE_PUSHED, "")
RES_STR(IDS_CTRL_FORMATGRAYED,              10180 + STATE_GRAYED, "")


//
// Definitions for status line messages
//
// The '\t' is used to center the text in the status bar rectangle
RES_STR(STS_MESSAGE_OVERTYPE,               10201,"\tOVR")
RES_STR(STS_MESSAGE_CAPSLOCK,               10203,"\tCAPS")
RES_STR(STS_MESSAGE_NUMLOCK,                10204,"\tNUM")
RES_STR(STS_MESSAGE_LINE,                   10205,"\tLn")
RES_STR(STS_MESSAGE_COLUMN,                 10206,"Col")
// BUGBUG - Dead code
// RES_STR(STS_MESSAGE_SRC,                    10207,"\tSRC")
RES_STR(STS_MESSAGE_CURPROCID,              10208,"\tProc")
RES_STR(STS_MESSAGE_CURTHRDID,              10209,"\tThrd")
RES_STR(STS_MESSAGE_ASM,                    10210,"\tASM")

//)
// Title bar strings)
//)

// BUGBUG - Dead code
// RES_STR(TBR_Mode_Work,                      10300," [edit]")
RES_STR(TBR_Mode_Run,                       10310," [run]")
RES_STR(TBR_Mode_Break,                     10320," [break]")

//)
// Files types description)
//)

//RES_STR(TYP_File_SOURCE,                    11011,"C/C++ Source Files (*.c, *.cxx)")
//RES_STR(TYP_File_INCLUDE,                   11012,"C/C++ Include Files (*.h)")
//RES_STR(TYP_File_ASMSRC,                    11013,"ASM Source Files (*.asm, *.s)")

RES_STR(TYP_File_SOURCE,                    11011,"C/C++ Src (*.c;*.cpp;*.cxx)")
RES_STR(TYP_File_INCLUDE,                   11012,"C/C++ Inc (*.h)")
RES_STR(TYP_File_ASMSRC,                    11013,"ASM   Src (*.asm;*.s)")
RES_STR(TYP_File_INC,                       11180,"User  Inc  (*.inc)")
RES_STR(TYP_File_RC,                        11100,"Resource  (*.rc)")
RES_STR(TYP_File_DLG,                       11110,"Dialog      (*.dlg)")
RES_STR(TYP_File_DEF,                       11090,"Definition (*.def)")
RES_STR(TYP_File_MAK,                       11060,"Project    (*.mak)")
RES_STR(TYP_File_All_SYMS,                  11261,"All Symbols (*.pdb;*.dbg;*.sym;*.exe;*.dll)")
RES_STR(TYP_File_Cust_SYMS,                 11262,"Symbols")
RES_STR(TYP_File_DLL,                       11220,"Dynamic Library (*.dll)")
RES_STR(TYP_File_ALL,                       11270,"All Files (*.*)")

// BUGBUG - Dead code
// RES_STR(TYP_File_C,                         11010,"C Source (*.c)")
// BUGBUG - Dead code
// RES_STR(TYP_File_CPP,                       11020,"C++ Source (*.cpp)")
// BUGBUG - Dead code
// RES_STR(TYP_File_CXX,                       11030,"C++ Source (*.cxx)")
// BUGBUG - Dead code
// RES_STR(TYP_File_H,                         11040,"C Header (*.h)")
// BUGBUG - Dead code
// RES_STR(TYP_File_NOEXT,                     11050,"No Extension (*.)")
// BUGBUG - Dead code
// RES_STR(TYP_File_OBJ,                       11080,"Object Code (*.obj)")
RES_STR(TYP_File_EXE,                       11200,"Executable (*.exe)")
// BUGBUG - Dead code
// RES_STR(TYP_File_COM,                       11210,"Command (*.com)")
RES_STR(TYP_File_ASM,                       11250,"Asm Files (*.asm)")
// BUGBUG - Dead code
// RES_STR(TYP_File_MIPSASM,                   11260,"Mips Asm Files (*.s)")
RES_STR(TYP_File_EXE_COM,                   11280,"Executable (*.exe;*.com)")
RES_STR(TYP_File_LOG,                       11281,"Log Files (*.log)")
RES_STR(TYP_File_DUMP,                      11282,"Crash Dump Files (*.dmp)")

RES_STR(DEF_Ext_C,                          11300,"*.C")
RES_STR(DEF_Ext_SOURCE,                     11301,"*.C;*.CPP;*.CXX")
RES_STR(DEF_Ext_CPP,                        11310,"*.CPP")
RES_STR(DEF_Ext_CXX,                        11320,"*.CXX")
RES_STR(DEF_Ext_INCLUDE,                    11321,"*.H;*.HPP;*.HXX")
RES_STR(DEF_Ext_H,                          11330,"*.H")
// BUGBUG - Dead code
// RES_STR(DEF_Ext_NOEXT,                      11340,"*.")
RES_STR(DEF_Ext_MAK,                        11350,"*.MAK")
// BUGBUG - Dead code
// RES_STR(DEF_Ext_OBJ,                        11370,"*.OBJ")
RES_STR(DEF_Ext_DEF,                        11380,"*.DEF")
RES_STR(DEF_Ext_RC,                         11390,"*.RC")
RES_STR(DEF_Ext_DLG,                        11400,"*.DLG")
RES_STR(DEF_Ext_INC,                        11470,"*.INC")
RES_STR(DEF_Ext_EXE,                        11490,"*.EXE")
RES_STR(DEF_Ext_COM,                        11500,"*.COM")
RES_STR(DEF_Ext_DLL,                        11510,"*.DLL")
RES_STR(DEF_Ext_ASMSRC,                     11511,"*.ASM;*.S")
RES_STR(DEF_Ext_ASM,                        11540,"*.ASM")
// BUGBUG - Dead code
// RES_STR(DEF_Ext_MIPSASM,                    11550,"*.S")
RES_STR(DEF_Ext_All_SYMS,                   11551,"*.pdb;*.dbg;*.sym;*.exe;*.dll")
RES_STR(DEF_Ext_Cust_SYMS,                  11552,".pdb\0.dbg\0.sym\0")
RES_STR(DEF_Ext_ALL,                        11560,"*.*")
RES_STR(DEF_Ext_EXE_COM,                    11570,"*.exe;*.com")
RES_STR(DEF_Ext_LOG,                        11571,"*.log")
RES_STR(DEF_Ext_DUMP,                       11572,"*.dmp")


// Toolbar strings
RES_STR(TBR_FILE_OPEN,                      12000,"Open source file (Ctrl+O)")
RES_STR(TBR_FILE_SAVE_WORKSPACE,            12001,"Save workspace (Ctrl+S)")

RES_STR(TBR_EDIT_CUT,                       12002,"Cut (Ctrl+X)")
RES_STR(TBR_EDIT_COPY,                      12003,"Copy (Ctrl+C)")
RES_STR(TBR_EDIT_PASTE,                     12004,"Paste (Ctrl+V)")

RES_STR(TBR_DEBUG_GO,                       12005,"Go (F5)")
RES_STR(TBR_DEBUG_RESTART,                  12006,"Restart (Ctrl+Shift+F5)")
RES_STR(TBR_DEBUG_STOPDEBUGGING,            12007,"Stop debugging (Shift+F5)")
RES_STR(TBR_DEBUG_BREAK,                    12008,"Break (Ctrl+Break)")

RES_STR(TBR_DEBUG_STEPINTO,                 12009,"Step into (F11 or F8)")
RES_STR(TBR_DEBUG_STEPOVER,                 12010,"Step over (F10)")
RES_STR(TBR_DEBUG_RUNTOCURSOR,              12011,"Run to cursor (Ctrl+F10 or F7)")

RES_STR(TBR_EDIT_BREAKPOINTS,               12012,"Insert or remove breakpoint (F9)")
RES_STR(TBR_DEBUG_QUICKWATCH,               12013,"Quick watch (Shift+F9)")

RES_STR(TBR_VIEW_COMMAND,                   12014,"Command (Alt+1)")
RES_STR(TBR_VIEW_WATCH,                     12015,"Watch (Alt+2)")
RES_STR(TBR_VIEW_LOCALS,                    12018,"Locals (Alt+3)")
RES_STR(TBR_VIEW_REGISTERS,                 12019,"Registers (Alt+4)")
RES_STR(TBR_VIEW_MEMORY,                    12017,"Memory window (Alt+5)")
RES_STR(TBR_VIEW_CALLSTACK,                 12016,"Call stack (Alt+6)")
RES_STR(TBR_VIEW_DISASM,                    12020,"Disassembly (Alt+7)")
RES_STR(TBR_VIEW_FLOAT,                     12021,"Float (Alt+8)")

RES_STR(TBR_DEBUG_SOURCE_MODE_ON,           12022,"Source mode on")
RES_STR(TBR_DEBUG_SOURCE_MODE_OFF,          12023,"Source mode off")

RES_STR(TBR_VIEW_FONT,                      12024,"Font")

RES_STR(TBR_EDIT_PROPERTIES,                12025,"Properties")

RES_STR(TBR_VIEW_OPTIONS,                   12026,"Options")


RES_STR(IDS_PROCESS_EXCLUSION_LIST,         12100,"smss.exe\0System.exe\0Terminate with two zeros. Not case sensitive.\0")


#ifdef RESOURCES
    //
    // Menu Items Status bar help
    //

    IDM_FILE,                           "File operations"
    IDM_FILE_OPEN,                      "Open a source file"
    IDM_FILE_CLOSE,                     "Close active window"

    IDM_FILE_OPEN_EXECUTABLE,           "Open an executable to debug"
    IDM_FILE_OPEN_CRASH_DUMP,           "Open a crash dump to debug"
    //IDM_FILE_START_KERNEL_DEBUGGING,    "Start kernel debugging"

    IDM_FILE_NEW_WORKSPACE,             "Create a new workspace"
    IDM_FILE_SAVE_WORKSPACE,            "Save the current workspace"
    IDM_FILE_SAVEAS_WORKSPACE,          "Save the current workspace with a new name"
    IDM_FILE_MANAGE_WORKSPACE,          "Delete, create, copy, edit workspaces"
    IDM_FILE_SAVE_AS_WINDOW_LAYOUTS     "Save the current window layout with a new name"
    IDM_FILE_MANAGE_WINDOW_LAYOUTS      "Open, delete, create, copy, edit window layouts"
    IDM_FILE_EXIT,                      "Exit Windows Debugger"

    //
    // Edit
    //
    IDM_EDIT,                           "Edit operations"
    IDM_EDIT_CUT,                       "Move the selected text to the clipboard"
    IDM_EDIT_COPY,                      "Copy the selected text to the clipboard"
    IDM_EDIT_PASTE,                     "Paste the clipboard text at the insertion point"
    IDM_EDIT_FIND,                      "Find some text"
    IDM_EDIT_REPLACE,                   "Find some text and replace it"
    IDM_EDIT_GOTO_LINE,                 "Move to a specified line number"
    IDM_EDIT_GOTO_ADDRESS,              "Move to the specified address"
    IDM_EDIT_BREAKPOINTS,               "Edit program breakpoints"
    IDM_EDIT_PROPERTIES,                "Edit properties for Memory, Watch, Locals, and Call Stack windows"

    //
    // View
    //
    IDM_VIEW,                           "File navigation, status and toolbars"
    
    IDM_VIEW_COMMAND,                   "Open the command window"
    IDM_VIEW_WATCH,                     "Open the watch window"
    IDM_VIEW_CALLSTACK,                 "Open a call stack window"
    IDM_VIEW_MEMORY,                    "Open a memory window"
    IDM_VIEW_LOCALS,                    "Open the locals window"
    IDM_VIEW_REGISTERS,                 "Open the registers window"
    IDM_VIEW_DISASM,                    "Open the disassembly window"
    IDM_VIEW_FLOAT,                     "Open the floating point window"

    IDM_VIEW_TOGGLETAG,                 "Toggle a tag for the current line"
    IDM_VIEW_NEXTTAG,                   "Move to the next tagged line"
    IDM_VIEW_PREVIOUSTAG,               "Move to the previous tagged line"
    IDM_VIEW_CLEARALLTAGS,              "Clear all tags in the active file"

    IDM_VIEW_TOOLBAR,                   "Toggle the toolbar on or off"
    IDM_VIEW_STATUS,                    "Toggle the status bar on or off"

    IDM_VIEW_FONT,                      "View or edit the font for the current window"
    IDM_VIEW_COLORS,                    "View or edit the current color selection"

    IDM_VIEW_OPTIONS,                   "View program options"

    //
    // Debug
    //
    IDM_DEBUG,                          "Debug operatons"
    IDM_DEBUG_GO,                       "Run the Program"
    IDM_DEBUG_RESTART,                  "Restart the Program"
    IDM_DEBUG_STOPDEBUGGING,            "Stop debugging the current program"
    IDM_DEBUG_BREAK,                    "Halt the current program"

    IDM_DEBUG_STEPINTO,                 "Trace into the next statement"
    IDM_DEBUG_STEPOVER,                 "Step over the next statement"
    IDM_DEBUG_RUNTOCURSOR,              "Run the program to the line containing the cursor"

    IDM_DEBUG_QUICKWATCH,               "Quick watch and modify variables or expressions"

    IDM_DEBUG_SOURCE_MODE,              "Source window"
    
    IDM_DEBUG_EXCEPTIONS,               "Manage exception handling"
    IDM_DEBUG_SET_THREAD,               "View or set the current thread"
    IDM_DEBUG_SET_PROCESS,              "View or set the current process"
    
    IDM_DEBUG_ATTACH,                   "Attach to a running process"
    
    IDM_DEBUG_GO_UNHANDLED,             "Do not handle the exception, but continue running"
    IDM_DEBUG_GO_HANDLED,               "Handle the exception and continue running"

    //
    // Window
    //
    IDM_WINDOW,                         "Window arrangement and selection"
    IDM_WINDOW_NEWWINDOW,               "Duplicate the active window"
    IDM_WINDOW_CASCADE,                 "Arrange the windows in a cascaded view"
    IDM_WINDOW_TILE_HORZ,               "Tiles the windows so that they are wide rather than tall"
    IDM_WINDOW_TILE_VERT,               "Tiles the windows so that they are tall rather than wide"
    IDM_WINDOW_ARRANGE,                 "Arrange the windows"
    IDM_WINDOW_ARRANGE_ICONS,           "Arrange the icons"
    IDM_WINDOW_SOURCE_OVERLAY,          "Overlay the source windows"

    //
    // Help
    //
    IDM_HELP,                           "Help contents and searches"
    IDM_HELP_CONTENTS,                  "Open the help table of contents"
    IDM_HELP_SEARCH,                    "Open the help search dialog"
    IDM_HELP_ABOUT,                     "About Windows Debugger"




    IDS_SOURCE_WINDOW,                  "Source window"
    IDS_DUMMY_WINDOW,                   "Dummy"              //do NOT Remove!!GWK
    IDS_WATCH_WINDOW,                   "Watch window"
    IDS_LOCALS_WINDOW,                  "Locals window"
    IDS_CPU_WINDOW,                     "Registers window"
    IDS_DISASSEMBLER_WINDOW,            "Disassembler window"
    IDS_COMMAND_WINDOW,                 "Command window"
    IDS_FLOAT_WINDOW,                   "Floating-point window"
    IDS_MEMORY_WINDOW,                  "Memory window"
    IDS_CALLS_WINDOW,                   "Calls window"
    IDS_BREAKPOINT_LINE,                "Breakpoint line"
    IDS_CURRENT_LINE,                   "Current line"
    IDS_CURRENTBREAK_LINE,              "Current line, with breakpoint"
    IDS_UNINSTANTIATEDBREAK,            "Uninstantiated breakpoint"
    IDS_TAGGED_LINE,                    "Tagged line"
    IDS_TEXT_SELECTION,                 "Text selection"
    IDS_KEYWORD,                        "Keyword"
    IDS_IDENTIFIER,                     "Identifier"
    IDS_COMMENT,                        "Comment"
    IDS_NUMBER,                         "Number"
    IDS_REAL,                           "Real"
    IDS_STRING,                         "String"
    IDS_ACTIVEEDIT,                     "Active edit"
    IDS_CHANGEHISTORY,                  "Change history"


    IDS_SELECT_ALL,                     "Select All"
    IDS_CLEAR_ALL,                      "Clear All"
#endif

#ifdef RESOURCES
END
#else
    IDS_REMOVE_WARNING
};
#endif


#endif // _RES_STR_
