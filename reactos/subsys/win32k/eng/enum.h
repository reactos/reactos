#define ENUM_RECT_LIMIT   50

typedef struct _RECT_ENUM
{
  ULONG c;
  RECTL arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;
