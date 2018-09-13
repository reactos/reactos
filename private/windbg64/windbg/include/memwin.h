/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/

#define MAX_CHUNK_TOREAD 4096 // maximum chunk of memory to read at one go


LRESULT
CALLBACK
MemoryEditProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void ViewMem(int view, BOOL fVoidCache);


extern char   memText[MAX_MSG_TXT]; //the selected text for memory dlg


struct memItem {
    char    iStart;
    char    cch;
    char    iFmt;
};


struct memWinDesc {
    int     iFormat;
    ATOM    atmAddress;
    BOOL    fLive;
    BOOL    fHaveAddr;
    BOOL    fBadRead;               // dis we really read mem or just ??
    char FAR *  lpbBytes;
    struct memItem FAR * lpMi;
    UINT    cMi;
    BOOL    fEdit;
    BOOL    fFill;
    UINT    cPerLine;
    ADDR    addr;
    ADDR    orig_addr;
    ADDR    old_addr;
    char    szAddress[MAX_MSG_TXT]; //the mem address expression in ascii
    UINT    cbRead;
};

extern struct memWinDesc    MemWinDesc[MAX_VIEWS];
extern struct memWinDesc    TempMemWinDesc;

/*
**  Define the set of memory formats
*/

enum {
    MW_ASCII = 0,
    MW_BYTE,
    MW_SHORT,
    MW_SHORT_HEX,
    MW_SHORT_UNSIGNED,
    MW_LONG,
    MW_LONG_HEX,
    MW_LONG_UNSIGNED,
    MW_QUAD,
    MW_QUAD_HEX,
    MW_QUAD_UNSIGNED,
    MW_REAL,
    MW_REAL_LONG,
    MW_REAL_TEN
};

#define MEM_FORMATS {\
            1,  /* ASCII */ \
            1,  /* BYTE  */ \
            2,  /* SHORT */ \
            2,  /* SHORT_HEX */ \
            2,  /* SHORT_UNSIGNED */ \
            4,  /* LONG */ \
            4,  /* LONG_HEX */ \
            4,  /* LONG_UNSIGNED */ \
            8,  /* QUAD */ \
            8,  /* QUAD_HEX */ \
            8,  /* QUAD_UNSIGNED */ \
            4,  /* REAL */ \
            8,  /* REAL_LONG */ \
           10,  /* REAL_TEN */ \
           16   /*  */ \
} 
