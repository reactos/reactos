#define SIZE_OF_NT_SIGNATURE    sizeof (DWORD)

/* global macros to define header offsets into file */
/* offset to PE file signature                     */
#define NTSIGNATURE(a) ((LPVOID)((BYTE *)a           +  \
            ((PIMAGE_DOS_HEADER)a)->e_lfanew))

/* DOS header identifies the NT PEFile signature dword
   the PEFILE header exists just after that dword          */
#define PEFHDROFFSET(a) ((LPVOID)((BYTE *)a          +  \
             ((PIMAGE_DOS_HEADER)a)->e_lfanew    +  \
             SIZE_OF_NT_SIGNATURE))

/* PE optional header is immediately after PEFile header       */
#define OPTHDROFFSET(a) ((LPVOID)((BYTE *)a          +  \
             ((PIMAGE_DOS_HEADER)a)->e_lfanew    +  \
             SIZE_OF_NT_SIGNATURE            +  \
             sizeof (IMAGE_FILE_HEADER)))

/* section headers are immediately after PE optional header    */
#define SECHDROFFSET(a) ((LPVOID) IMAGE_FIRST_SECTION(NTSIGNATURE(a)))

/* global prototypes for functions in pefile.c */
/* PE file header info */
DWORD   WINAPI ImageFileType (LPVOID);
BOOL    WINAPI GetPEFileHeader (LPVOID, PIMAGE_FILE_HEADER);

LPVOID  WINAPI GetImageBase (LPVOID);
LPVOID  WINAPI ImageDirectoryOffset (LPVOID, DWORD);

/* PE section header info */
int WINAPI GetSectionNames (LPVOID, HANDLE, char **);
BOOL    WINAPI GetSectionHdrByName (LPVOID, PIMAGE_SECTION_HEADER, char *);

/* export section info */
int WINAPI GetExportFunctionNames (LPVOID);
