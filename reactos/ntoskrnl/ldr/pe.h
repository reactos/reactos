
#ifndef PWORD
typedef WORD *PWORD;
#endif

#ifndef PDWORD
typedef DWORD *PDWORD;
#endif

#define IMAGE_DOS_MAGIC  0x54ad
#define IMAGE_NT_MAGIC   0x00004550

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_SIZEOF_FILE_HEADER             20

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x14c   // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x162   // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x166   // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x168   // MIPS little-endian
#define IMAGE_FILE_MACHINE_ALPHA             0x184   // Alpha_AXP
#define IMAGE_FILE_MACHINE_POWERPC           0x1F0   // IBM PowerPC Little-Endian


//
// Directory format.
//

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

//
// Optional header format.
//

typedef struct _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;

    //
    // NT additional fields.
    //

    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;


typedef struct _IMAGE_NT_HEADERS {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;


// Directory Entries

#define IMAGE_DIRECTORY_ENTRY_EXPORT         0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT         1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE       2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION      3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY       4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC      5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG          6   // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_COPYRIGHT      7   // Description String
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR      8   // Machine Value (MIPS GP)
#define IMAGE_DIRECTORY_ENTRY_TLS            9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG   10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT  11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT           12   // Import Address Table

//
// Section header format.
//

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40


//
// Export Format
//

typedef struct _IMAGE_EXPORT_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Name;
    DWORD   Base;
    DWORD   NumberOfFunctions;
    DWORD   NumberOfNames;
    PDWORD  *AddressOfFunctions;
    PDWORD  *AddressOfNames;
    PWORD   *AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

//
// Import Format
//

typedef struct _IMAGE_IMPORT_BY_NAME {
    WORD    Hint;
    BYTE    Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

#define IMAGE_ORDINAL_FLAG 0x80000000
#define IMAGE_ORDINAL(Ordinal) (Ordinal & 0xffff)


//
// Resource Format.
//

//
// Resource directory consists of two counts, following by a variable length
// array of directory entries.  The first count is the number of entries at
// beginning of the array that have actual names associated with each entry.
// The entries are in ascending order, case insensitive strings.  The second
// count is the number of entries that immediately follow the named entries.
// This second count identifies the number of entries that have 16-bit integer
// Ids as their name.  These entries are also sorted in ascending order.
//
// This structure allows fast lookup by either name or number, but for any
// given resource entry only one form of lookup is supported, not both.
// This is consistant with the syntax of the .RC file and the .RES file.
//

// Predefined resource types ... there may be some more, but I don't have
//                               the information yet.  .....sang cho.....

#define    RT_NEWRESOURCE   0x2000
#define    RT_ERROR         0x7fff
#define    RT_CURSOR        1
#define    RT_BITMAP        2
#define    RT_ICON          3
#define    RT_MENU          4
#define    RT_DIALOG        5
#define    RT_STRING        6
#define    RT_FONTDIR       7
#define    RT_FONT          8
#define    RT_ACCELERATORS  9
#define    RT_RCDATA        10
#define    RT_MESSAGETABLE  11
#define    RT_GROUP_CURSOR  12
#define    RT_GROUP_ICON    14
#define    RT_VERSION       16
#define    NEWBITMAP        (RT_BITMAP|RT_NEWRESOURCE)
#define    NEWMENU          (RT_MENU|RT_NEWRESOURCE)
#define    NEWDIALOG        (RT_DIALOG|RT_NEWRESOURCE)


typedef struct _IMAGE_RESOURCE_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    WORD    NumberOfNamedEntries;
    WORD    NumberOfIdEntries;
//  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[1];
} IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;

#define IMAGE_RESOURCE_NAME_IS_STRING        0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY     0x80000000

//
// Each directory contains the 32-bit Name of the entry and an offset,
// relative to the beginning of the resource directory of the data associated
// with this directory entry.  If the name of the entry is an actual text
// string instead of an integer Id, then the high order bit of the name field
// is set to one and the low order 31-bits are an offset, relative to the
// beginning of the resource directory of the string, which is of type
// IMAGE_RESOURCE_DIRECTORY_STRING.  Otherwise the high bit is clear and the
// low-order 16-bits are the integer Id that identify this resource directory
// entry. If the directory entry is yet another resource directory (i.e. a
// subdirectory), then the high order bit of the offset field will be
// set to indicate this.  Otherwise the high bit is clear and the offset
// field points to a resource data entry.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
    DWORD    Name;
	DWORD    OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;

//
// For resource directory entries that have actual string names, the Name
// field of the directory entry points to an object of the following type.
// All of these string objects are stored together after the last resource
// directory entry and before the first resource data object.  This minimizes
// the impact of these variable length objects on the alignment of the fixed
// size directory entry objects.
//

typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
    WORD    Length;
    CHAR    NameString[ 1 ];
} IMAGE_RESOURCE_DIRECTORY_STRING, *PIMAGE_RESOURCE_DIRECTORY_STRING;


typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
    WORD    Length;
    WCHAR   NameString[ 1 ];
} IMAGE_RESOURCE_DIR_STRING_U, *PIMAGE_RESOURCE_DIR_STRING_U;


//
// Each resource data entry describes a leaf node in the resource directory
// tree.  It contains an offset, relative to the beginning of the resource
// directory of the data for the resource, a size field that gives the number
// of bytes of data at that offset, a CodePage that should be used when
// decoding code point values within the resource data.  Typically for new
// applications the code page would be the unicode code page.
//

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
    DWORD   OffsetToData;
    DWORD   Size;
    DWORD   CodePage;
    DWORD   Reserved;
} IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;


//  Menu Resources	 ... added by .....sang cho....

// Menu resources are composed of a menu header followed by a sequential list
// of menu items. There are two types of menu items: pop-ups and normal menu
// itmes. The MENUITEM SEPARATOR is a special case of a normal menu item with
// an empty name, zero ID, and zero flags.

typedef struct _IMAGE_MENU_HEADER{
    WORD   wVersion;	  // Currently zero
	WORD   cbHeaderSize;  // Also zero
} IMAGE_MENU_HEADER, *PIMAGE_MENU_HEADER;

typedef struct _IMAGE_POPUP_MENU_ITEM{
    WORD   fItemFlags;	
	WCHAR  szItemText[1];
} IMAGE_POPUP_MENU_ITEM, *PIMAGE_POPUP_MENU_ITEM;

typedef struct _IMAGE_NORMAL_MENU_ITEM{
    WORD   fItemFlags;	
	WORD   wMenuID;
	WCHAR  szItemText[1];
} IMAGE_NORMAL_MENU_ITEM, *PIMAGE_NORMAL_MENU_ITEM;

#define MI_GRAYED       0x0001 // GRAYED keyword
#define MI_INACTIVE     0x0002 // INACTIVE keyword
#define MI_BITMAP       0x0004 // BITMAP keyword
#define MI_OWNERDRAW    0x0100 // OWNERDRAW keyword
#define MI_CHECKED      0x0008 // CHECKED keyword
#define MI_POPUP        0x0010 // used internally
#define MI_MENUBARBREAK 0x0020 // MENUBARBREAK keyword
#define MI_MENUBREAK    0x0040 // MENUBREAK keyword
#define MI_ENDMENU      0x0080 // used internally


// Dialog Box Resources	.................. added by sang cho.

// A dialog box is contained in a single resource and has a header and 
// a portion repeated for each control in the dialog box.
// The item DWORD IStyle is a standard window style composed of flags found
// in WINDOWS.H.
// The default style for a dialog box is:
// WS_POPUP | WS_BORDER | WS_SYSMENU
// 
// The itme marked "Name or Ordinal" are :
// If the first word is an 0xffff, the next two bytes contain an ordinal ID.
// Otherwise, the first one or more WORDS contain a double-null-terminated string.
// An empty string is represented by a single WORD zero in the first location.
// 
// The WORD wPointSize and WCHAR szFontName entries are present if the FONT
// statement was included for the dialog box. This can be detected by checking
// the entry IStyle. If IStyle & DS_SETFONT ( which is 0x40), then these
// entries will be present.

typedef struct _IMAGE_DIALOG_BOX_HEADER1{
    DWORD  IStyle;
	DWORD  IExtendedStyle;    // New for Windows NT
	WORD   nControls;         // Number of Controls
	WORD   x;
	WORD   y;
	WORD   cx;
	WORD   cy;
//	N_OR_O MenuName;         // Name or Ordinal ID
//	N_OR_O ClassName;		 // Name or Ordinal ID
//	WCHAR  szCaption[];
//	WORD   wPointSize;       // Only here if FONT set for dialog
//	WCHAR  szFontName[];     // This too
} IMAGE_DIALOG_HEADER, *PIMAGE_DIALOG_HEADER;

typedef union _NAME_OR_ORDINAL{    // Name or Ordinal ID
	struct _ORD_ID{
	    WORD   flgId;
        WORD   Id;
	} ORD_ID;
	WCHAR  szName[1];      
} NAME_OR_ORDINAL, *PNAME_OR_ORDINAL;

// The data for each control starts on a DWORD boundary (which may require
// some padding from the previous control), and its format is as follows:

typedef struct _IMAGE_CONTROL_DATA{
    DWORD   IStyle;
	DWORD   IExtendedStyle;
	WORD    x;
	WORD    y;
	WORD    cx;
	WORD    cy;
	WORD    wId;
//  N_OR_O  ClassId;
//  N_OR_O  Text;
//  WORD    nExtraStuff;
} IMAGE_CONTROL_DATA, *PIMAGE_CONTROL_DATA;

#define BUTTON       0x80
#define EDIT         0x81
#define STATIC       0x82
#define LISTBOX      0x83
#define SCROLLBAR    0x84
#define COMBOBOX     0x85

// The various statements used in a dialog script are all mapped to these
// classes along with certain modifying styles. The values for these styles
// can be found in WINDOWS.H. All dialog controls have the default styles
// of WS_CHILD and WS_VISIBLE. A list of the default styles used follows:
//
// Statement           Default Class         Default Styles
// CONTROL             None                  WS_CHILD|WS_VISIBLE
// LTEXT               STATIC                ES_LEFT
// RTEXT               STATIC                ES_RIGHT
// CTEXT               STATIC                ES_CENTER
// LISTBOX             LISTBOX               WS_BORDER|LBS_NOTIFY
// CHECKBOX            BUTTON                BS_CHECKBOX|WS_TABSTOP
// PUSHBUTTON          BUTTON                BS_PUSHBUTTON|WS_TABSTOP
// GROUPBOX            BUTTON                BS_GROUPBOX
// DEFPUSHBUTTON       BUTTON                BS_DFPUSHBUTTON|WS_TABSTOP
// RADIOBUTTON         BUTTON                BS_RADIOBUTTON
// AUTOCHECKBOX        BUTTON                BS_AUTOCHECKBOX
// AUTO3STATE          BUTTON                BS_AUTO3STATE
// AUTORADIOBUTTON     BUTTON                BS_AUTORADIOBUTTON
// PUSHBOX             BUTTON                BS_PUSHBOX
// STATE3              BUTTON                BS_3STATE
// EDITTEXT            EDIT                  ES_LEFT|WS_BORDER|WS_TABSTOP
// COMBOBOX            COMBOBOX              None
// ICON                STATIC                SS_ICON
// SCROLLBAR           SCROLLBAR             None
///

#define WS_OVERLAPPED   0x00000000L
#define WS_POPUP        0x80000000L
#define WS_CHILD        0x40000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_VISIBLE      0x10000000L
#define WS_DISABLED     0x08000000L
#define WS_MINIMIZE     0x20000000L
#define WS_MAXIMIZE     0x01000000L
#define WS_CAPTION      0x00C00000L
#define WS_BORDER       0x00800000L
#define WS_DLGFRAME     0x00400000L
#define WS_VSCROLL      0x00200000L
#define WS_HSCROLL      0x00100000L
#define WS_SYSMENU      0x00080000L
#define WS_THICKFRAME   0x00040000L
#define WS_MINIMIZEBOX  0x00020000L
#define WS_MAXIMIZEBOX  0x00010000L
#define WS_GROUP        0x00020000L
#define WS_TABSTOP      0x00010000L

// other aliases
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW  (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW  (WS_CHILD)
#define WS_TILED        WS_OVERLAPPED
#define WS_ICONIC       WS_MINIMIZE
#define WS_SIZEBOX      WS_THICKFRAME
#define WS_TILEDWINDOW  WS_OVERLAPPEDWINDOW

#define WS_EX_DLGMODALFRAME     0x00000001L
#define WS_EX_NOPARENTNOTIFY    0x00000004L
#define WS_EX_TOPMOST           0x00000008L
#define WS_EX_ACCEPTFILES       0x00000010L
#define WS_EX_TRANSPARENT       0x00000020L

#define BS_PUSHBUTTON           0x00000000L
#define BS_DEFPUSHBUTTON        0x00000001L
#define BS_CHECKBOX             0x00000002L
#define BS_AUTOCHECKBOX         0x00000003L
#define BS_RADIOBUTTON          0x00000004L
#define BS_3STATE               0x00000005L
#define BS_AUTO3STATE           0x00000006L
#define BS_GROUPBOX             0x00000007L
#define BS_USERBUTTON           0x00000008L
#define BS_AUTORADIOBUTTON      0x00000009L
#define BS_OWNERDRAW            0x0000000BL
#define BS_LEFTTEXT             0x00000020L

#define ES_LEFT         0x00000000L
#define ES_CENTER       0x00000001L
#define ES_RIGHT        0x00000002L
#define ES_MULTILINE    0x00000004L
#define ES_UPPERCASE    0x00000008L
#define ES_LOWERCASE    0x00000010L
#define ES_PASSWORD     0x00000020L
#define ES_AUTOVSCROLL  0x00000040L
#define ES_AUTOHSCROLL  0x00000080L
#define ES_NOHIDESEL    0x00000100L
#define ES_OEMCONVERT   0x00000400L
#define ES_READONLY     0x00000800L
#define ES_WANTRETURN   0x00001000L

#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_DISABLENOSCROLL   0x1000L

#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
#define SS_BITMAP           0x0000000EL

//
// Debug Format
//

typedef struct _IMAGE_DEBUG_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Type;
    DWORD   SizeOfData;
    DWORD   AddressOfRawData;
    DWORD   PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_UNKNOWN          0
#define IMAGE_DEBUG_TYPE_COFF             1
#define IMAGE_DEBUG_TYPE_CODEVIEW         2
#define IMAGE_DEBUG_TYPE_FPO              3
#define IMAGE_DEBUG_TYPE_MISC             4
#define IMAGE_DEBUG_TYPE_EXCEPTION        5
#define IMAGE_DEBUG_TYPE_FIXUP            6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC      7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC    8


typedef struct _IMAGE_DEBUG_MISC {
    DWORD       DataType;               // type of misc data, see defines
    DWORD       Length;                 // total length of record, rounded to four
                                        // byte multiple.
    BOOLEAN     Unicode;                // TRUE if data is unicode string
    BYTE        Reserved[ 3 ];
    BYTE        Data[ 1 ];              // Actual data
} IMAGE_DEBUG_MISC, *PIMAGE_DEBUG_MISC;


//
// Debugging information can be stripped from an image file and placed
// in a separate .DBG file, whose file name part is the same as the
// image file name part (e.g. symbols for CMD.EXE could be stripped
// and placed in CMD.DBG).  This is indicated by the IMAGE_FILE_DEBUG_STRIPPED
// flag in the Characteristics field of the file header.  The beginning of
// the .DBG file contains the following structure which captures certain
// information from the image file.  This allows a debug to proceed even if
// the original image file is not accessable.  This header is followed by
// zero of more IMAGE_SECTION_HEADER structures, followed by zero or more
// IMAGE_DEBUG_DIRECTORY structures.  The latter structures and those in
// the image file contain file offsets relative to the beginning of the
// .DBG file.
//
// If symbols have been stripped from an image, the IMAGE_DEBUG_MISC structure
// is left in the image file, but not mapped.  This allows a debugger to
// compute the name of the .DBG file, from the name of the image in the
// IMAGE_DEBUG_MISC structure.
//

typedef struct _IMAGE_SEPARATE_DEBUG_HEADER {
    WORD        Signature;
    WORD        Flags;
    WORD        Machine;
    WORD        Characteristics;
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    DWORD       ImageBase;
    DWORD       SizeOfImage;
    DWORD       NumberOfSections;
    DWORD       ExportedNamesSize;
    DWORD       DebugDirectorySize;
    DWORD       SectionAlignment;
    DWORD       Reserved[2];
} IMAGE_SEPARATE_DEBUG_HEADER, *PIMAGE_SEPARATE_DEBUG_HEADER;

#define IMAGE_SEPARATE_DEBUG_SIGNATURE  0x4944

#define IMAGE_SEPARATE_DEBUG_FLAGS_MASK 0x8000
#define IMAGE_SEPARATE_DEBUG_MISMATCH   0x8000  // when DBG was updated, the
                                                // old checksum didn't match.


//
// End Image Format
//


#define SIZE_OF_NT_SIGNATURE	sizeof (DWORD)
#define MAXRESOURCENAME 	13

/* global macros to define header offsets into file */
/* offset to PE file signature				       */
#define NTSIGNATURE(a) ((LPVOID)((BYTE *)a		     +	\
			((PIMAGE_DOS_HEADER)a)->e_lfanew))

/* DOS header identifies the NT PEFile signature dword
   the PEFILE header exists just after that dword	       */
#define PEFHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE))

/* PE optional header is immediately after PEFile header       */
#define OPTHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)))

/* section headers are immediately after PE optional header    */
#define SECHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)	     +	\
			 sizeof (IMAGE_OPTIONAL_HEADER)))


typedef struct tagImportDirectory
    {
    DWORD    dwRVAFunctionNameList;
    DWORD    dwUseless1;
    DWORD    dwUseless2;
    DWORD    dwRVAModuleName;
    DWORD    dwRVAFunctionAddressList;
    }IMAGE_IMPORT_MODULE_DIRECTORY, * PIMAGE_IMPORT_MODULE_DIRECTORY;

