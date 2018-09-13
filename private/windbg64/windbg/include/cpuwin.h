/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/

HWND GetCpuHWND(void);
HWND GetFloatHWND(void);
LRESULT CALLBACK CPUEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
**  This structure is used to describe all of the information needed
**  to keep track of where registers are in a window and what registers
**  are in a window
*/

typedef struct tagREGINFO REGINFO;
typedef struct tagREGINFO *PREGINFO;

struct tagREGINFO {
    char FAR *  lpsz;       /* Name to display      */
    char FAR *  pszValueC;  /* Current Value Displayed */
    char FAR *  pszValueP;  /* Last Value Displayed */
    int         x;          /* X-Pos            */
    int         y;          /* Y-Pos            */
    int         cch;        /* # characters displayed   */
    UINT        hReg;       /* Register handle      */
    UINT        hFlag;      /* Flag handle          */
    UINT        cbits;      /* Count of bits in value   */
    UINT        offValue;   /* Offset of value field    */
    UINT        type;       /* fmtFloat or fmtUInt      */
    BOOL        fChanged;   /* if data has changed since last step */
};
