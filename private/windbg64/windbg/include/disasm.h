/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/*************************************************************************
**                                  **
**              DISASM.H                **
**                                  **
**  Description:                            **
**  This file contains the prototypes and common declarations   **
**  for the disassembler window                 **
**                                  **
*************************************************************************/

LRESULT CALLBACK DisasmEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL DisasmGetAddrFromLine(LPADDR lpaddr, DWORD iLine, PBOOL pbSegAlwaysZero = NULL);

void PASCAL ViewDisasm(LPADDR, int);


#define     DISASM_NONE 0
#define     DISASM_OTHER    2
#define     DISASM_BRKPT    3

#define     disasmForce 0x01
#define     disasmHighlight 0x02
#define     disasmDown  0x04
#define     disasmLine  0x08
#define     disasmUp    0x10
#define     disasmPage  0x40
#define     disasmPC    0x80
#define     disasmRefresh 0x100

#define     disasmDownLine  (disasmDown | disasmLine)
#define     disasmDownPage  (disasmDown | disasmPage)
#define     disasmUpLine    (disasmUp   | disasmLine)
#define     disasmUpPage    (disasmUp   | disasmPage)


#define     DISASM_PC   (disasmForce|disasmPC)
