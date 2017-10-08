/*
 *	PE symbol dumper
 *
 *	symdump.c
 *
 *	Copyright (c) 2008 Timo Kreuzer <timo <dot> kreuzer <at> reactos <dot> org>
 *
 *	This program is released under the terms of the GNU GPL.
 *
 * TODO:
 * - fix GDILoObjType
 * - fix UDTKind1
 * - include the correct headers for some stuff
 * - fix unions like LARGE_INTEGER
 */

#include <stdio.h>
#define _WINVER 0x501
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS 0x00000800
#include <windows.h>
#include <shlwapi.h>
#include <dbghelp.h>

HANDLE hCurrentProcess;
BOOL g_bShowPos = 0;

#define MAX_SYMBOL_NAME         1024

#define CV_CALL_NEAR_C 0x00
#define CV_CALL_FAR_C 0x01
#define CV_CALL_NEAR_PASCAL 0x02
#define CV_CALL_FAR_PASCAL 0x03
#define CV_CALL_NEAR_FAST 0x04
#define CV_CALL_FAR_FAST 0x05
#define CV_CALL_SKIPPED 0x06
#define CV_CALL_NEAR_STD 0x07
#define CV_CALL_FAR_STD 0x08
#define CV_CALL_NEAR_SYS 0x09
#define CV_CALL_FAR_SYS 0x0a
#define CV_CALL_THISCALL 0x0b
#define CV_CALL_MIPSCALL 0x0c
#define CV_CALL_GENERIC 0x0d
#define CV_CALL_ALPHACALL 0x0e
#define CV_CALL_PPCCALL 0x0f
#define CV_CALL_SHCALL 0x10
#define CV_CALL_ARMCALL 0x11
#define CV_CALL_AM33CALL 0x12
#define CV_CALL_TRICALL 0x13
#define CV_CALL_SH5CALL 0x14
#define CV_CALL_M32RCALL 0x15

enum SymTagEnum
{
	SymTagNull,
	SymTagExe,
	SymTagCompiland,
	SymTagCompilandDetails,
	SymTagCompilandEnv,
	SymTagFunction,
	SymTagBlock,
	SymTagData,
	SymTagAnnotation,
	SymTagLabel,
	SymTagPublicSymbol,
	SymTagUDT,
	SymTagEnum,
	SymTagFunctionType,
	SymTagPointerType,
	SymTagArrayType,
	SymTagBaseType,
	SymTagTypedef,
	SymTagBaseClass,
	SymTagFriend,
	SymTagFunctionArgType,
	SymTagFuncDebugStart,
	SymTagFuncDebugEnd,
	SymTagUsingNamespace,
	SymTagVTableShape,
	SymTagVTable,
	SymTagCustom,
	SymTagThunk,
	SymTagCustomType,
	SymTagManagedType,
	SymTagDimension,
	SymTagMax
};

enum
{
	UDTKind_Struct = 0,
	UDTKind_Class = 1, /* ? */
	UDTKind_Union = 2,
};

enum BasicType
{
	btNoType = 0,
	btVoid = 1,
	btChar = 2,
	btWChar = 3,
	btInt = 6,
	btUInt = 7,
	btFloat = 8,
	btBCD = 9,
	btBool = 10,
	btLong = 13,
	btULong = 14,
	btCurrency = 25,
	btDate = 26,
	btVariant = 27,
	btComplex = 28,
	btBit = 29,
	btBSTR = 30,
	btHresult = 31
};

typedef struct
{
	HANDLE hProcess;
	DWORD64 dwModuleBase;
	LPSTR pszSymbolName;
	BOOL bType;
} ENUMINFO, *PENUMINFO;

VOID DumpType(DWORD dwTypeIndex, PENUMINFO pei, INT indent, BOOL bMembers);

CHAR *SymTagString[] =
{
	"SymTagNull",
	"SymTagExe",
	"SymTagCompiland",
	"SymTagCompilandDetails",
	"SymTagCompilandEnv",
	"SymTagFunction",
	"SymTagBlock",
	"SymTagData",
	"SymTagAnnotation",
	"SymTagLabel",
	"SymTagPublicSymbol",
	"SymTagUDT",
	"SymTagEnum",
	"SymTagFunctionType",
	"SymTagPointerType",
	"SymTagArrayType",
	"SymTagBaseType",
	"SymTagTypedef",
	"SymTagBaseClass",
	"SymTagFriend",
	"SymTagFunctionArgType",
	"SymTagFuncDebugStart",
	"SymTagFuncDebugEnd",
	"SymTagUsingNamespace",
	"SymTagVTableShape",
	"SymTagVTable",
	"SymTagCustom",
	"SymTagThunk",
	"SymTagCustomType",
	"SymTagManagedType",
	"SymTagDimension",
	"SymTagMax"
};

void
IndentPrint(INT ind)
{
	INT i;
	for (i = 0; i < ind; i++)
	{
		printf("  ");
	}
}

#define printfi \
	IndentPrint(indent); printf

VOID
PrintUsage()
{
	printf("Syntax:\n\n");
	printf("dumpsym <file> [-sp=<symbolpath>] [-p] [<symname>]\n\n");
	printf("<file>           The PE file you want to dump the symbols of\n");
	printf("-sp=<symbolpath> Path to your symbol files.\n");
	printf("                 Default is MS symbol server.\n");
	printf("-p               Enable struct positions.\n");
	printf("<symname>        A name of a Symbol, you want to dump\n");
	printf("                 Default is all symbols.\n");
	printf("\n");
}

BOOL InitDbgHelp(HANDLE hProcess, LPSTR pszSymbolPath)
{
	if (!SymInitialize(hProcess, 0, FALSE))
		return FALSE;

	SymSetOptions(SymGetOptions() | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);
	SymSetOptions(SymGetOptions() & (~SYMOPT_DEFERRED_LOADS));
	SymSetSearchPath(hProcess, pszSymbolPath);
	return TRUE;
}

VOID
DumpBaseType(DWORD dwTypeIndex, PENUMINFO pei, INT indent)
{
	ULONG64 ulSize;
	DWORD dwBaseType;

	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwTypeIndex, TI_GET_LENGTH, &ulSize);
	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwTypeIndex, TI_GET_BASETYPE, &dwBaseType);

	switch (dwBaseType)
	{
	case btVoid:
		printfi("VOID");
		return;
	case btChar:
		printfi("CHAR");
		return;
	case btWChar:
		printfi("WCHAR");
		return;
	case btInt:
		switch (ulSize)
		{
		case 1:
			printfi("CHAR");
			return;
		case 2:
			printfi("SHORT");
			return;
		case 4:
			printfi("INT");
			return;
		case 8:
			printfi("INT64");
			return;
		default:
			printfi("INT%ld", (ULONG)ulSize * 8);
			return;
		}
	case btUInt:
		switch (ulSize)
		{
		case 1:
			printfi("UCHAR");
			return;
		case 2:
			printfi("USHORT");
			return;
		case 4:
			printfi("UINT");
			return;
		case 8:
			printfi("UINT64");
			return;
		default:
			printfi("UINT%ld", (ULONG)ulSize * 8);
			return;
		}
	case btFloat:
		switch (ulSize)
		{
		case 4:
			printfi("FLOAT");
			return;
		case 8:
			printfi("DOUBLE");
			return;
		default:
			printfi("FLOAT%ld", (ULONG)ulSize * 8);
			return;
		}
	case btBCD:
		printfi("BCD%ld", (ULONG)ulSize * 8);
		return;
	case btBool:
		switch (ulSize)
		{
		case 1:
			printfi("BOOLEAN");
			return;
		case 4:
			printfi("BOOL");
			return;
		default:
			printfi("BOOL%ld", (ULONG)ulSize * 8);
			return;
		}
	case btLong:
		switch (ulSize)
		{
		case 1:
			printfi("CHAR");
			return;
		case 2:
			printfi("SHORT");
			return;
		case 4:
			printfi("LONG");
			return;
		case 8:
			printfi("LONGLONG");
			return;
		default:
			printfi("LONG%ld", (ULONG)ulSize * 8);
			return;
		}
	case btULong:
		switch (ulSize)
		{
		case 1:
			printfi("UCHAR");
			return;
		case 2:
			printfi("USHORT");
			return;
		case 4:
			printfi("ULONG");
			return;
		case 8:
			printfi("ULONGLONG");
			return;
		default:
			printfi("ULONG%ld", (ULONG)ulSize * 8);
			return;
		}
	case btCurrency:
	case btDate:
	case btVariant:
	case btComplex:
	case btBit:
	case btBSTR:
		printfi("UNSUP_%ld_%ld", dwBaseType, (ULONG)ulSize);
		return;
	case btHresult:
		if (ulSize == 4)
		{
			printfi("HRESULT");
			return;
		}
		printfi("HRESULT%ld", (ULONG)ulSize);
		return;
	}

	printfi("UNKNBASETYPE");
}

VOID
DumpArray(DWORD dwTypeIndex, PENUMINFO pei, INT indent)
{
	DWORD dwTypeId;

	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwTypeIndex, TI_GET_TYPE, &dwTypeId);
	DumpType(dwTypeId, pei, indent, FALSE);
}

VOID
DumpPointer(DWORD dwTypeIndex, PENUMINFO pei, INT indent)
{
	DWORD dwRefTypeId;
	DWORD dwTag = 0;
	ULONG64 ulSize;
	DWORD dwBaseType;

	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwTypeIndex, TI_GET_TYPE, &dwRefTypeId);
	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwRefTypeId, TI_GET_BASETYPE, &dwBaseType);
	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwRefTypeId, TI_GET_LENGTH, &ulSize);
	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwRefTypeId, TI_GET_SYMTAG, &dwTag);

	if (dwTag == SymTagFunctionType)
	{
		printfi("PPROC");
		return;
	}

	switch (dwBaseType)
	{
	case btVoid:
		switch (ulSize)
		{
		case 0:
			printfi("PVOID");
			return;
		}
		break;

	case btChar:
		switch (ulSize)
		{
		case 1:
			printfi("PCHAR");
			return;
		}
		break;
	case btWChar:
		switch (ulSize)
		{
		case 2:
			printfi("PWCHAR");
			return;
		}
		break;
	case btInt:
		switch (ulSize)
		{
		case 4:
			printfi("PINT");
			return;
		}
		break;
	case btUInt:
		switch (ulSize)
		{
		case 4:
			printfi("PUINT");
			return;
		}
		break;
	case btFloat:
		switch (ulSize)
		{
		case 4:
			printfi("PFLOAT");
			return;
		case 8:
			printfi("PDOUBLE");
			return;
		}
		break;
	case btBCD:
		break;
	case btBool:
		switch (ulSize)
		{
		case 1:
			printfi("PBOOLEAN");
			return;
		case 4:
			printfi("PBOOL");
			return;
		}
		break;
	case btLong:
		switch (ulSize)
		{
		case 4:
			printfi("PLONG");
			return;
		case 8:
			printfi("PLONGLONG");
			return;
		}
		break;
	case btULong:
		switch (ulSize)
		{
		case 4:
			printfi("PULONG");
			return;
		case 8:
			printfi("PULONGLONG");
			return;
		}
		break;
	case btCurrency:
	case btDate:
	case btVariant:
	case btComplex:
	case btBit:
	case btBSTR:
	case btHresult:
		break;
	}

	DumpType(dwRefTypeId, pei, indent, FALSE);
	printf("*");
}

VOID
PrintVariant(VARIANT *v)
{
//      printf("<vt%d>", v->n1.n2.vt);
	switch (v->n1.n2.vt)
	{
	case VT_I1:
		printf("%d", (INT)v->n1.n2.n3.cVal);
		break;
	case VT_UI1:
		printf("0x%x", (UINT)v->n1.n2.n3.cVal);
		break;
	case VT_I2:
		printf("%d", (UINT)v->n1.n2.n3.iVal);
		break;
	case VT_UI2:
		printf("0x%x", (UINT)v->n1.n2.n3.iVal);
		break;
	case VT_INT:
	case VT_I4:
		printf("%d", (UINT)v->n1.n2.n3.lVal);
		break;
	case VT_UINT:
	case VT_UI4:
		printf("0x%x", (UINT)v->n1.n2.n3.lVal);
		break;
	}
}

BOOL
IsUnnamed(WCHAR *pszName)
{
	if ((StrStrW(pszName, L"__unnamed") != NULL) ||
	        (StrStrW(pszName, L"<unnamed-tag>") != NULL))
	{
		return TRUE;
	}
	return FALSE;
}

VOID
DumpEnum(DWORD dwTypeIndex, PENUMINFO pei, INT indent, BOOL bMembers)
{
	DWORD64 dwModuleBase = pei->dwModuleBase;
	HANDLE hProcess = pei->hProcess;
	INT i;
	DWORD dwUDTKind;
	WCHAR *pszName, *pszNameX;
	struct
	{
		TI_FINDCHILDREN_PARAMS tfp;
		ULONG TypeIds[200];
	} tfpex;
	VARIANT v;

	SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_GET_SYMNAME, &pszNameX);
	SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_GET_UDTKIND, &dwUDTKind);
	pszName = pszNameX;
	if (IsUnnamed(pszName))
	{
		if (bMembers)
		{
			LocalFree(pszNameX);
			return;
		}
		bMembers = TRUE;
		pszName = L"";
	}
	printfi("enum %ls", pszName);
	LocalFree(pszNameX);

	if (bMembers)
	{
		printf(" /* %03x */", 0);
		printfi("\n{\n");

		/* Get the children */
		SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_GET_CHILDRENCOUNT, &tfpex.tfp.Count);

		tfpex.tfp.Start = 0;
		SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_FINDCHILDREN, &tfpex.tfp);

		for (i = 0; i < tfpex.tfp.Count; i++)
		{
			pszName = L"";

			SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i], TI_GET_SYMNAME, &pszName);
			SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i], TI_GET_VALUE, &v);

			indent++;
			printfi("%ls = ", pszName);
			PrintVariant(&v);
			printf(",\n");
			indent--;

			LocalFree(pszName);
		}
		printfi("}");
	}
}

VOID
DumpUDT(DWORD dwTypeIndex, PENUMINFO pei, INT indent, BOOL bMembers)
{
	DWORD64 dwModuleBase = pei->dwModuleBase;
	HANDLE hProcess = pei->hProcess;
	INT i;
	DWORD dwUDTKind;
	WCHAR *pszName, *pszNameX;
	struct
	{
		TI_FINDCHILDREN_PARAMS tfp;
		ULONG TypeIds[200];
	} tfpex;

	DWORD dwDataKind;
	DWORD dwTypeId;
	DWORD dwCount;
	WCHAR *pszTypeName;

	SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_GET_SYMNAME, &pszNameX);
	SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_GET_UDTKIND, &dwUDTKind);

	pszName = pszNameX;
	if (IsUnnamed(pszName))
	{
		if (bMembers)
		{
			LocalFree(pszNameX);
			return;
		}
		bMembers = TRUE;
		pszName = L"";
	}
	if (dwUDTKind == UDTKind_Struct)
	{
		printfi("struct %ls", pszName);
	}
	else if (dwUDTKind == UDTKind_Union)
	{
		printfi("union %ls", pszName);
	}
	else
	{
		printfi("UTDKind%ld %ls", dwUDTKind, pszName);
	}
	LocalFree(pszNameX);

	if (bMembers)
	{
		ULONG64 ulLength;

		printf("\n");
		printfi("{\n");

		/* Get the children */
		SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_GET_CHILDRENCOUNT, &tfpex.tfp.Count);

		tfpex.tfp.Start = 0;
		SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_FINDCHILDREN, &tfpex.tfp);

		for (i = 0; i < tfpex.tfp.Count; i++)
		{
			DWORD dwChildTag;
			DWORD dwOffset;

			pszName = L"";
			pszTypeName = L"";

			SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i], TI_GET_SYMNAME, &pszName);
			SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i], TI_GET_DATAKIND, &dwDataKind);
			SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i], TI_GET_TYPE, &dwTypeId);
			SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i], TI_GET_OFFSET, &dwOffset);
			SymGetTypeInfo(hProcess, dwModuleBase, dwTypeId, TI_GET_SYMTAG, &dwChildTag);
			SymGetTypeInfo(hProcess, dwModuleBase, dwTypeId, TI_GET_LENGTH, &ulLength);

			printf(" /* %03lx */", dwOffset);
			DumpType(dwTypeId, pei, indent + 1, FALSE);
			printf(" %ls", pszName);
			if (dwChildTag == SymTagArrayType)
			{
				SymGetTypeInfo(hProcess, dwModuleBase, dwTypeId, TI_GET_COUNT, &dwCount);
				printf("[%ld]", dwCount);
			}
			else
			{
				DWORD dwCurrentBitPos;
				DWORD dwNextBitPos;

				SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i], TI_GET_BITPOSITION, &dwCurrentBitPos);
				if (i < tfpex.tfp.Count - 1)
				{
					SymGetTypeInfo(hProcess, dwModuleBase, tfpex.tfp.ChildId[i+1], TI_GET_BITPOSITION, &dwNextBitPos);
				}
				else
				{
					dwNextBitPos = 0;
				}

				if (dwNextBitPos == 0 && dwCurrentBitPos != 0)
				{
					dwNextBitPos = ulLength * 8;
				}

				if (dwNextBitPos != dwCurrentBitPos)
				{
					printf(":%ld", dwNextBitPos - dwCurrentBitPos);
				}
			}
			printf(";\n");
			LocalFree(pszName);
		}
		printfi("}");
	}
}

VOID
DumpType(DWORD dwTypeIndex, PENUMINFO pei, INT indent, BOOL bMembers)
{
	HANDLE hProcess = pei->hProcess;
	DWORD64 dwModuleBase = pei->dwModuleBase;
	DWORD dwTag = 0;

	SymGetTypeInfo(hProcess, dwModuleBase, dwTypeIndex, TI_GET_SYMTAG, &dwTag);

	switch (dwTag)
	{
	case SymTagEnum:
		DumpEnum(dwTypeIndex, pei, indent, bMembers);
		break;

	case SymTagUDT:
		DumpUDT(dwTypeIndex, pei, indent, bMembers);
		break;

	case SymTagPointerType:
		DumpPointer(dwTypeIndex, pei, indent);
		break;

	case SymTagBaseType:
		DumpBaseType(dwTypeIndex, pei, indent);
		break;

	case SymTagArrayType:
		DumpArray(dwTypeIndex, pei, indent);
		break;

	case SymTagFunctionType:
		printfi("function");
		break;

	default:
		printfi("typeTag%ld", dwTag);
		break;
	}

}


VOID
DumpCV(DWORD dwTypeIndex, PENUMINFO pei)
{
	DWORD cv = 0x20;

	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, dwTypeIndex, TI_GET_CALLING_CONVENTION, &cv);
	switch (cv)
	{
	case CV_CALL_NEAR_C:
		printf("CDECL");
		return;
	case CV_CALL_FAR_C:
		printf("FAR CDECL");
		return;
	case CV_CALL_NEAR_PASCAL:
		printf("PASCAL");
		return;
	case CV_CALL_FAR_PASCAL:
		printf("FAR PASCAL");
		return;
	case CV_CALL_NEAR_FAST:
		printf("FASTCALL");
		return;
	case CV_CALL_FAR_FAST:
		printf("FAR FASTCALL");
		return;
	case CV_CALL_SKIPPED:
		printf("SKIPPED");
		return;
	case CV_CALL_NEAR_STD:
		printf("STDCALL");
		return;
	case CV_CALL_FAR_STD:
		printf("FAR STDCALL");
		return;
	case CV_CALL_NEAR_SYS:
	case CV_CALL_FAR_SYS:
	case CV_CALL_THISCALL:
		printf("THISCALL");
		return;
	case CV_CALL_MIPSCALL:
		printf("MIPSCALL");
		return;
	case CV_CALL_GENERIC:
	case CV_CALL_ALPHACALL:
	case CV_CALL_PPCCALL:
	case CV_CALL_SHCALL:
	case CV_CALL_ARMCALL:
	case CV_CALL_AM33CALL:
	case CV_CALL_TRICALL:
	case CV_CALL_SH5CALL:
	case CV_CALL_M32RCALL:
	default:
		printf("UNKNOWNCV");
	}

}

BOOL CALLBACK
EnumParamsProc(
    PSYMBOL_INFO pSymInfo,
    ULONG SymbolSize,
    PVOID UserContext)
{
	printf("x, ");
	(*(INT*)UserContext)++;
	return TRUE;
}

VOID
DumpParams(PSYMBOL_INFO pSymInfo, PENUMINFO pei)
{
	IMAGEHLP_STACK_FRAME sf;
	BOOL bRet;
	INT NumLocals = 0; // the number of local variables found

	sf.InstructionOffset = pSymInfo->Address;

	printf("(");
	bRet = SymSetContext(pei->hProcess, &sf, 0);

	if (!bRet)
	{
		printf("\nError: SymSetContext() failed. Error code: %lu \n", GetLastError());
		return;
	}
	printf("Address == 0x%x, ReturnOffset = 0x%x", (UINT)pSymInfo->Address, (UINT)sf.ReturnOffset);

	// Enumerate local variables

	bRet = SymEnumSymbols(pei->hProcess, 0, 0, EnumParamsProc, &NumLocals);

	if (!bRet)
	{
//              printf("Error: SymEnumSymbols() failed. Error code: %lu \n", GetLastError());
		printf("?)");
		return;
	}

	if (NumLocals == 0)
	{
//              printf("The function does not have parameters and local variables.\n");
		printf("void)");
	}

	printf(")");
}

VOID
DumpFunction(PSYMBOL_INFO pSymInfo, PENUMINFO pei)
{
	DWORD dwTypeId;

//printf("Name=%s, Size=%ld, TypeId=0x%ld\n", pSymInfo->Name, pSymInfo->Size, pSymInfo->TypeIndex);

	SymGetTypeInfo(pei->hProcess, pei->dwModuleBase, pSymInfo->TypeIndex, TI_GET_TYPEID, &dwTypeId);

//      DumpCV(pSymInfo->TypeIndex, pei);
//      printf("\n");
//      DumpType(pSymInfo->TypeIndex, pei, 0, FALSE);
	printf("%s", pSymInfo->Name);
	DumpParams(pSymInfo, pei);
}

BOOL CALLBACK
EnumSymbolsProc(
    PSYMBOL_INFO pSymInfo,
    ULONG SymbolSize,
    PVOID UserContext)
{
	PENUMINFO pei = (PENUMINFO)UserContext;

	if ((pei->pszSymbolName == NULL) ||
	        (strstr(pSymInfo->Name, pei->pszSymbolName) != 0))
	{
		if (pei->bType)
		{
			DumpType(pSymInfo->TypeIndex, pei, 0, TRUE);
			printf("\n\n");
		}
		else
		{
#if defined(__GNUC__) && \
	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ < 40400)
		    printf("Symbol: %s, TypeIndex=%ld, Flags=%lx, Value=0x%llx\n",
#else
		    printf("Symbol: %s, TypeIndex=%ld, Flags=%lx, Value=0x%I64x\n",
#endif
		        pSymInfo->Name, pSymInfo->TypeIndex, pSymInfo->Flags, pSymInfo->Value);
			//if (pSymInfo->Flags & SYMFLAG_FUNCTION)
			{
//                              DumpFunction(pSymInfo, pei);
//                              printf("\n\n");
			}
		}
	}
	return TRUE;
}

int main(int argc, char* argv[])
{
	HANDLE hProcess;
	CHAR szFullFileName[MAX_PATH+1];
	DWORD64 dwModuleBase;
	BOOL bRet;
	LPSTR pszSymbolPath, pszSymbolName;
	INT i;
	ENUMINFO enuminfo;

	printf("PE symbol dumper\n");
	printf("Copyright (c) Timo Kreuzer 2008\n\n");

	if (argc < 2)
	{
		PrintUsage();
		return 0;
	}

	/* Get the full path name of the PE file from first argument */
	GetFullPathName(argv[1], MAX_PATH, szFullFileName, NULL);

	/* Default Symbol Name (all) */
	pszSymbolName = NULL;

	/* Default to ms symbol server */
	pszSymbolPath = "srv**symbols*http://msdl.microsoft.com/download/symbols";

	/* Check other command line arguments */
	for (i = 2; i < argc; i++)
	{
		if (*argv[i] == '-')
		{
			if (strncmp(argv[i], "-sp=", 4) == 0)
			{
				pszSymbolPath = argv[i] + 4;
			}
			else if (strcmp(argv[i], "-p") == 0)
			{
				g_bShowPos = 1;
			}
			else
			{
				printf("Invalid argument: %s\n", argv[i]);
				PrintUsage();
				return 0;
			}
		}
		else
		{
			pszSymbolName = argv[i];
		}
	}

	hProcess = GetCurrentProcess();

	printf("Trying to get symbols from: %s\n", pszSymbolPath);

	if (!InitDbgHelp(hProcess, pszSymbolPath))
	{
		printf("SymInitialize() failed\n");
		goto cleanup;
	}

	printf("Loading symbols for %s, please wait...\n", szFullFileName);
	dwModuleBase = SymLoadModule64(hProcess, 0, szFullFileName, 0, 0, 0);
	if (dwModuleBase == 0)
	{
		printf("SymLoadModule64() failed: %ld\n", GetLastError());
		goto cleanup;
	}

	printf("\nSymbols:\n");
	enuminfo.hProcess = hProcess;
	enuminfo.pszSymbolName = pszSymbolName;
	enuminfo.bType = FALSE;
	SetLastError(ERROR_SUCCESS);
	bRet = SymEnumSymbols(hProcess, dwModuleBase, NULL, EnumSymbolsProc, &enuminfo);
	if (!bRet)
	{
		printf("SymEnumSymbols failed: %ld\n", GetLastError());
	}

	printf("\nTypes:\n");
	enuminfo.bType = TRUE;
	enuminfo.dwModuleBase = dwModuleBase;
	SetLastError(ERROR_SUCCESS);
	bRet = SymEnumTypes(hProcess, dwModuleBase, EnumSymbolsProc, &enuminfo);
	if (!bRet)
	{
		printf("SymEnumTypes failed: %ld\n", GetLastError());
	}

cleanup:

	return 0;
}
