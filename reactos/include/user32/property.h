
typedef struct tagPROPERTY
{
    struct tagPROPERTY *next;     /* Next property in window list */
    HANDLE            handle;   /* User's data */
    ATOM              atom;
    HANDLE	      app;
} PROPERTY;


typedef struct tagPROPVALUE
{
    HANDLE	      hwnd;
    HANDLE            handle;  
    ATOM              atom;
} PROPVALUE;

HANDLE PROPERTY_FindProp( HWND hwnd, ATOM Atom );
WINBOOL PROPERTY_SetProp( HANDLE hwnd, ATOM atom,HANDLE hData);
HANDLE PROPERTY_RemoveProp(  HANDLE hwnd , ATOM atom );
void PROPERTY_RemoveWindowProps( HANDLE hwnd  );
WINBOOL PROPERTY_EnumPropEx(HWND hwnd, PROPVALUE **pv , int maxsize, int *size );