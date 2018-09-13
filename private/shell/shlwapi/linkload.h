//
// table format used to declare export tables
//

typedef struct
{
    LPCSTR pName;                        // name (lowercase - ansi)
    FARPROC pEntry;
} EXPORTTABLE, * LPEXPORTTABLE;

typedef struct
{
    DWORD dwOrdinal;                    // ordinal number
    FARPROC pEntry;                     
} ORDINALTABLE, * LPORDINALTABLE;

typedef struct
{
    LPCSTR pName;                       // module name (lowercase)
    const INT* pcOrdinalTable;          // -> array size of ordinal table
    const LPORDINALTABLE pOrdinalTable;
    const INT* pcExportTable;           // -> array size of export table
    const LPEXPORTTABLE pExportTable;
} MODULETABLE, * LPMODULETABLE;


//
// macros to make referencing tables easier
//

#define MODULETABLEDECL(module)                            \
        extern INT g_c##module##OrdinalTable;              \
        extern ORDINALTABLE g_##module##OrdinalTable;      \
        extern INT g_c##module##ExportTable;               \
        extern EXPORTTABLE g_##module##ExportTable;        

#define MODULETABLEREF(name, module)                             \
        { ##name##,                                              \
          &g_c##module##OrdinalTable, &g_##module##OrdinalTable, \
          &g_c##module##ExportTable, &g_##module##ExportTable    \
        }

