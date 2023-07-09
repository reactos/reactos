#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

PrintUsage() {
    printf("test case for bug #78891 - reported by Chris Mah, April 15th 1997\n");
    printf("Uses WriteConsoleInput() to inject keystrokes for \"ESC y\"\n");

    printf("WritEscY attempts to inject \"ESC y\" into the console input stream\n");
    printf("It does this by constructing KeyEvents with scancode and VK values for\n");
    printf("the Escape and Y key obtained via VkKeyScan() and MapVirtualKey().\n");
    printf("Usage:  writescy [-x]\n");
    printf("  -x :  use VK_ESCAPE literally\n");
    printf("        (default uses VkKeyScanEx(0x1b) to get the VK)\n");
}

int _cdecl main(int argc, char *argv[]) {
    DWORD dwNumWrites;
    INPUT_RECORD inputESC_Y[6];
    BOOL retval;
    int iArg;
    BOOL fLiteral = FALSE;

    /*
     * Compute the flags
     */
    for (iArg=1; iArg<argc; iArg++) {
        if ((argv[iArg][0] == '-') || (argv[iArg][0] == '/')) {
            switch (argv[iArg][1]) {
            case 'x':
                fLiteral = TRUE;
                break;

            default:
                printf("ERROR: Invalid flag %c\n", argv[iArg][1]);

            case '?':
                PrintUsage();
                return 0;
            }
        } else {
            PrintUsage();
            return 0;
        }
    }

    printf("VkKeyScan(0x1b) returns 0x%02x\n", VkKeyScan(0x1b));
    printf("MapVirtualKey(VkKeyScan(0x1b),0) returns 0x%02x\n", MapVirtualKey(VkKeyScan(0x1b),0));

    printf("VK_ESCAPE is 0x%02x\n", VK_ESCAPE);
    printf("MapVirtualKey(VK_ESCAPE,0) returns 0x%02x\n", MapVirtualKey(VK_ESCAPE,0));
    printf("========================================================\n\n");

  if (!fLiteral) {
    printf("Using VkKeyScan(0x1b): ");
    inputESC_Y[0].EventType = KEY_EVENT;
    inputESC_Y[0].Event.KeyEvent.bKeyDown = TRUE;
    inputESC_Y[0].Event.KeyEvent.wRepeatCount = 1;
    inputESC_Y[0].Event.KeyEvent.wVirtualKeyCode = VkKeyScan(27);
    inputESC_Y[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VkKeyScan(27),0);
    inputESC_Y[0].Event.KeyEvent.uChar.AsciiChar = 27;
    inputESC_Y[0].Event.KeyEvent.dwControlKeyState = 0;

    inputESC_Y[1].EventType = KEY_EVENT;
    inputESC_Y[1].Event.KeyEvent.bKeyDown = FALSE;
    inputESC_Y[1].Event.KeyEvent.wRepeatCount = 1;
    inputESC_Y[1].Event.KeyEvent.wVirtualKeyCode = VkKeyScan(27);
    inputESC_Y[1].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VkKeyScan(27),0);
    inputESC_Y[1].Event.KeyEvent.uChar.AsciiChar = 27;
    inputESC_Y[1].Event.KeyEvent.dwControlKeyState = 0;

    inputESC_Y[2].EventType = KEY_EVENT;
    inputESC_Y[2].Event.KeyEvent.bKeyDown = TRUE;
    inputESC_Y[2].Event.KeyEvent.wRepeatCount = 1;
    inputESC_Y[2].Event.KeyEvent.wVirtualKeyCode = VkKeyScan('y');
    inputESC_Y[2].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VkKeyScan('y'),0);
    inputESC_Y[2].Event.KeyEvent.uChar.AsciiChar = 'y';
    inputESC_Y[2].Event.KeyEvent.dwControlKeyState = 0;

    inputESC_Y[3].EventType = KEY_EVENT;
    inputESC_Y[3].Event.KeyEvent.bKeyDown = FALSE;
    inputESC_Y[3].Event.KeyEvent.wRepeatCount = 1;
    inputESC_Y[3].Event.KeyEvent.wVirtualKeyCode = VkKeyScan('y');
    inputESC_Y[3].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VkKeyScan('y'),0);
    inputESC_Y[3].Event.KeyEvent.uChar.AsciiChar = 'y';
    inputESC_Y[3].Event.KeyEvent.dwControlKeyState = 0;

    retval = WriteConsoleInput( GetStdHandle(STD_INPUT_HANDLE), &inputESC_Y[0], 4, &dwNumWrites );

    printf("done: value returned by WriteConsoleOutput was (BOOL)%lx\n", retval);
    return 0;
  }

  printf("Using VK_ESCAPE: ");
  inputESC_Y[0].EventType = KEY_EVENT;
  inputESC_Y[0].Event.KeyEvent.bKeyDown = TRUE;
  inputESC_Y[0].Event.KeyEvent.wRepeatCount = 1;
  inputESC_Y[0].Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE;
  inputESC_Y[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_ESCAPE,0);
  inputESC_Y[0].Event.KeyEvent.uChar.AsciiChar = 27;
  inputESC_Y[0].Event.KeyEvent.dwControlKeyState = 0;

  inputESC_Y[1].EventType = KEY_EVENT;
  inputESC_Y[1].Event.KeyEvent.bKeyDown = FALSE;
  inputESC_Y[1].Event.KeyEvent.wRepeatCount = 1;
  inputESC_Y[1].Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE;
  inputESC_Y[1].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_ESCAPE,0);
  inputESC_Y[1].Event.KeyEvent.uChar.AsciiChar = 27;
  inputESC_Y[1].Event.KeyEvent.dwControlKeyState = 0;

  inputESC_Y[2].EventType = KEY_EVENT;
  inputESC_Y[2].Event.KeyEvent.bKeyDown = TRUE;
  inputESC_Y[2].Event.KeyEvent.wRepeatCount = 1;
  inputESC_Y[2].Event.KeyEvent.wVirtualKeyCode = VkKeyScan('y');
  inputESC_Y[2].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VkKeyScan('y'),0);
  inputESC_Y[2].Event.KeyEvent.uChar.AsciiChar = 'y';
  inputESC_Y[2].Event.KeyEvent.dwControlKeyState = 0;

  inputESC_Y[3].EventType = KEY_EVENT;
  inputESC_Y[3].Event.KeyEvent.bKeyDown = FALSE;
  inputESC_Y[3].Event.KeyEvent.wRepeatCount = 1;
  inputESC_Y[3].Event.KeyEvent.wVirtualKeyCode = VkKeyScan('y');
  inputESC_Y[3].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VkKeyScan('y'),0);
  inputESC_Y[3].Event.KeyEvent.uChar.AsciiChar = 'y';
  inputESC_Y[3].Event.KeyEvent.dwControlKeyState = 0;

  retval = WriteConsoleInput( GetStdHandle(STD_INPUT_HANDLE), &inputESC_Y[0], 4, &dwNumWrites );

  printf("done: value returned by WriteConsoleOutput was (BOOL)%lx\n", retval);
}
