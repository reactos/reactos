//--------------------------------------------------------------------
// EMF.H
//
// This contains a list of all the EM functions.
// 
// There are multiple users of this file.  To use it, you must
// define a DECL_EMF macro to extract the pieces of information that
// you are interested in from this file.  For example, if you want
// to define the emf enumeration, you could write the following:
//
//		#define DECL_EMF(name)	emf ## name,
//
//		typedef enum {
//			#include "emf.h"
//		} EMF;
//
//		#undef DECL_EMF
//--------------------------------------------------------------------

DECL_EMF(DebugPacket)

DECL_EMF(RegisterDBF)
DECL_EMF(Init)
DECL_EMF(GetModel)
DECL_EMF(UnInit)
DECL_EMF(Detach)
DECL_EMF(Attach)
DECL_EMF(GetInfo)
DECL_EMF(Setup)
DECL_EMF(Connect)
DECL_EMF(Disconnect)

DECL_EMF(CreateHpid)
DECL_EMF(DestroyHpid)
DECL_EMF(DestroyHtid)

DECL_EMF(SetMulti)
DECL_EMF(Debugger)

DECL_EMF(SpawnOrphan)
DECL_EMF(ProgramLoad)
DECL_EMF(DebugActive)
DECL_EMF(SetPath)
DECL_EMF(ProgramFree)

DECL_EMF(ThreadStatus)
DECL_EMF(ProcessStatus)
DECL_EMF(FreezeThread)
DECL_EMF(SetThreadPriority)

DECL_EMF(GetExceptionState)
DECL_EMF(SetExceptionState)

DECL_EMF(GetModuleNameFromAddress)
DECL_EMF(GetModuleList)

DECL_EMF(Go)
DECL_EMF(SingleStep)
DECL_EMF(RangeStep)
DECL_EMF(ReturnStep)
DECL_EMF(Stop)

DECL_EMF(BreakPoint)

DECL_EMF(SetupExecute)
DECL_EMF(StartExecute)
DECL_EMF(CleanUpExecute)

DECL_EMF(GetAddr)
DECL_EMF(SetAddr)
DECL_EMF(FixupAddr)
DECL_EMF(UnFixupAddr)
DECL_EMF(SetEmi)
DECL_EMF(RegisterEmi)
DECL_EMF(UnRegisterEmi)
DECL_EMF(CompareAddrs)
DECL_EMF(GetObjLength)
DECL_EMF(GetMemoryInfo)
DECL_EMF(GetFunctionInfo)

DECL_EMF(ReadMemory)
DECL_EMF(WriteMemory)

DECL_EMF(GetRegStruct)
DECL_EMF(GetFlagStruct)
DECL_EMF(GetReg)
DECL_EMF(SetReg)
DECL_EMF(GetFlag)
DECL_EMF(SetFlag)
DECL_EMF(SaveRegs)
DECL_EMF(RestoreRegs)

DECL_EMF(Unassemble)
DECL_EMF(GetPrevInst)
DECL_EMF(Assemble)

DECL_EMF(GetFrame)

DECL_EMF(Metric)

DECL_EMF(GetMessageMap)
DECL_EMF(GetMessageMaskMap)

DECL_EMF(InfoReply)
DECL_EMF(Continue)

DECL_EMF(ReadFile)
DECL_EMF(WriteFile)

DECL_EMF(ShowDebuggee)
DECL_EMF(GetTaskList)
DECL_EMF(SystemService)
DECL_EMF(SetDebugMode)

DECL_EMF(Dbc)

DECL_EMF(GetTimeStamp)

DECL_EMF(NewSymbolsLoaded)

DECL_EMF(KernelLoaded)

DECL_EMF(Max)
