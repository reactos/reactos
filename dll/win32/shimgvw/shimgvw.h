#include "resource.h"

#define TB_IMAGE_WIDTH  16
#define TB_IMAGE_HEIGHT 16

extern HINSTANCE hInstance;

typedef struct
{
    BOOL Maximized;
    INT Left;
    INT Top;
    INT Right;
    INT Bottom;
} SHIMGVW_SETTINGS;

typedef struct SHIMGVW_FILENODE_INTERNAL
{
    WCHAR FileName[MAX_PATH];
    struct SHIMGVW_FILENODE_INTERNAL *Prev;
    struct SHIMGVW_FILENODE_INTERNAL *Next;
} SHIMGVW_FILENODE;
