//
//
// This program was written by Sang Cho, assistant professor at 
//                                       the department of 
// 										 computer science and engineering
//										 chongju university
// this program is based on the program pefile.c
// which is written by Randy Kath(Microsoft Developmer Network Technology Group)
// in june 12, 1993.
// I have investigated P.E. file format as thoroughly as possible,
// but I cannot claim that I am an expert yet, so some of its information  
// may give you wrong results.
//
//
//
// language used: djgpp
// date of creation: September 28, 1997
//
// date of first release: October 15, 1997
//
//
//	you can contact me: e-mail address: sangcho@alpha94.chongju.ac.kr
//                            hitel id: chokhas
//                        phone number:	(0431) 229-8491	   +82-431-229-8491
//
//	      real address: Sang Cho
//                      Computer and Information Engineering
//                      ChongJu University
//                      NaeDok-Dong 36 
//                      ChongJu 360-764
//                      South Korea
//
//   Copyright (C) 1997.                                 by Sang Cho.
//
//   Permission is granted to make and distribute verbatim copies of this
// program provided the copyright notice and this permission notice are
// preserved on all copies.
//
//
// File: pedump.c ( I included header file into source file. )

# include "ccx.h"

//typedef char                CHAR;
//typedef short               WCHAR;
//typedef short               SHORT;
//typedef long                LONG;
//typedef unsigned short      USHORT;
//typedef unsigned long       DWORD;
//typedef int                 BOOL;
//typedef unsigned char       BYTE;
//typedef unsigned short      WORD;
//typedef BYTE               *PBYTE;
//typedef WORD               *PWORD;
//typedef DWORD              *PDWORD;
//typedef void               *LPVOID;

#define VOID                void
#define BOOLEAN             boolean
#define NULL                0
#define FALSE               0
#define TRUE                1
#define CONST               const
#define LOWORD(l)           ((WORD)(l))
#define WINAPI

//
// Image Format
//

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

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

//
// File header format.
//



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

#define GRAYED       0x0001 // GRAYED keyword
#define INACTIVE     0x0002 // INACTIVE keyword
#define BITMAP       0x0004 // BITMAP keyword
#define OWNERDRAW    0x0100 // OWNERDRAW keyword
#define CHECKED      0x0008 // CHECKED keyword
#define POPUP        0x0010 // used internally
#define MENUBARBREAK 0x0020 // MENUBARBREAK keyword
#define MENUBREAK    0x0040 // MENUBREAK keyword
#define ENDMENU      0x0080 // used internally


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


/* global prototypes for functions in pefile.c */
/* PE file header info */
BOOL	WINAPI GetDosHeader (LPVOID, PIMAGE_DOS_HEADER);
DWORD	WINAPI ImageFileType (LPVOID);
BOOL	WINAPI GetPEFileHeader (LPVOID, PIMAGE_FILE_HEADER);

/* PE optional header info */
BOOL	WINAPI GetPEOptionalHeader (LPVOID, PIMAGE_OPTIONAL_HEADER);
LPVOID	WINAPI GetModuleEntryPoint (LPVOID);
int	    WINAPI NumOfSections (LPVOID);
LPVOID	WINAPI GetImageBase (LPVOID);
LPVOID	WINAPI ImageDirectoryOffset (LPVOID, DWORD);
LPVOID  WINAPI ImageDirectorySection (LPVOID, DWORD);

/* PE section header info */
//int	WINAPI GetSectionNames (LPVOID, HANDLE, char **);
int	    WINAPI GetSectionNames (LPVOID, char **);
BOOL	WINAPI GetSectionHdrByName (LPVOID, PIMAGE_SECTION_HEADER, char *);

//
// structur to store string tokens
//
typedef struct _Str_P {
    char    flag;		  // string_flag '@' or '%' or '#'
    char    *pos;		  // starting postion of string
    int     length;  	  // length of string
	BOOL    wasString;    // if it were stringMode or not
} Str_P;

/* import section info */
int	   WINAPI GetImportModuleNames (LPVOID, char  **);
int	   WINAPI GetImportFunctionNamesByModule (LPVOID, char *, char  **);

// import function name reporting
int    WINAPI GetStringLength (char *);
void   WINAPI GetPreviousParamString (char *, char *);
void   WINAPI TranslateParameters (char **, char **, char **);
BOOL   WINAPI StringExpands (char **, char **, char **, Str_P *);
LPVOID WINAPI TranslateFunctionName (char *);

/* export section info */
int	WINAPI GetExportFunctionNames (LPVOID, char **);

/* resource section info */
int	   WINAPI GetNumberOfResources (LPVOID);
int	   WINAPI GetListOfResourceTypes (LPVOID, char **);
int    WINAPI MenuScan (int *, WORD **);
int    WINAPI MenuFill (char **, WORD **);
void   WINAPI StrangeMenuFill (char **, WORD **, int);
int	   WINAPI GetContentsOfMenu (LPVOID, char **);
int	   WINAPI PrintMenu (int, char **);
int	   WINAPI PrintStrangeMenu (char **);

/* debug section info */
BOOL   WINAPI IsDebugInfoStripped (LPVOID);
int	   WINAPI RetrieveModuleName (LPVOID, char **);
BOOL   WINAPI IsDebugFile (LPVOID);
BOOL   WINAPI GetSeparateDebugHeader (LPVOID, PIMAGE_SEPARATE_DEBUG_HEADER);


/* copy dos header information to structure */
BOOL  WINAPI GetDosHeader (
    LPVOID		 lpFile,
    PIMAGE_DOS_HEADER	 pHeader)
{
    /* dos header rpresents first structure of bytes in file */
    if (*(USHORT *)lpFile == IMAGE_DOS_SIGNATURE)
	bcopy(lpFile, (LPVOID)pHeader, sizeof (IMAGE_DOS_HEADER));
    else
	return FALSE;

    return TRUE;
}




/* return file signature */
DWORD  WINAPI ImageFileType (
    LPVOID    lpFile)
{
    /* dos file signature comes first */
    if (*(USHORT *)lpFile == IMAGE_DOS_SIGNATURE)
	{
	/* determine location of PE File header from dos header */
	if (LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE ||
	    LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE_LE)
	    return (DWORD)LOWORD(*(DWORD *)NTSIGNATURE (lpFile));

	else if (*(DWORD *)NTSIGNATURE (lpFile) == IMAGE_NT_SIGNATURE)
	    return IMAGE_NT_SIGNATURE;

	else
	    return IMAGE_DOS_SIGNATURE;
	}

    else
	/* unknown file type */
	return 0;
}




/* copy file header information to structure */
BOOL  WINAPI GetPEFileHeader (
    LPVOID		  lpFile,
    PIMAGE_FILE_HEADER	  pHeader)
{
    /* file header follows dos header */
    if (ImageFileType (lpFile) == IMAGE_NT_SIGNATURE)
	bcopy(PEFHDROFFSET (lpFile), (LPVOID)pHeader,  sizeof (IMAGE_FILE_HEADER));
    else
	return FALSE;

    return TRUE;
}





/* copy optional header info to structure */
BOOL WINAPI GetPEOptionalHeader (
    LPVOID		      lpFile,
    PIMAGE_OPTIONAL_HEADER    pHeader)
{
    /* optional header follows file header and dos header */
    if (ImageFileType (lpFile) == IMAGE_NT_SIGNATURE)
	bcopy (OPTHDROFFSET (lpFile), (LPVOID)pHeader,  sizeof (IMAGE_OPTIONAL_HEADER));
    else
	return FALSE;

    return TRUE;
}




/* function returns the entry point for an exe module lpFile must
   be a memory mapped file pointer to the beginning of the image file */
LPVOID	WINAPI GetModuleEntryPoint (
    LPVOID    lpFile)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);

    if (poh != NULL)
	return (LPVOID)(poh->AddressOfEntryPoint);
    else
	return NULL;
}




/* return the total number of sections in the module */
int   WINAPI NumOfSections (
    LPVOID    lpFile)
{
    /* number os sections is indicated in file header */
    return ((int)((PIMAGE_FILE_HEADER)PEFHDROFFSET (lpFile))->NumberOfSections);
}




/* retrieve entry point */
LPVOID	WINAPI GetImageBase (
    LPVOID    lpFile)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);

    if (poh != NULL)
	return (LPVOID)(poh->ImageBase);
    else
	return NULL;
}



//
// This function is written by sang cho
//						   .. october 5, 1997
//
/* function returns the actual address of given RVA,      lpFile must
   be a memory mapped file pointer to the beginning of the image file */
LPVOID	WINAPI GetActualAddress (
	LPVOID	  lpFile,
	DWORD	  dwRVA)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
    PIMAGE_SECTION_HEADER    psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile);
    int 		     nSections = NumOfSections (lpFile);
    int 		     i = 0;

    if (dwRVA == NULL) return NULL;
	if (dwRVA & 0x80000000) 
	{
	    //return (LPVOID)dwRVA;
		printf ("\n$$ what is going on $$");
		exit (0);
	}

    /* locate section containing image directory */
    while (i++<nSections)
	{
	    if (psh->VirtualAddress <= (DWORD)dwRVA &&
	        psh->VirtualAddress + psh->SizeOfRawData > (DWORD)dwRVA)
	    break;
	    psh++;
	}

    if (i > nSections)
	return NULL;

    /* return image import directory offset */
    return (LPVOID)(((int)lpFile + (int)dwRVA - psh->VirtualAddress) +
				   (int)psh->PointerToRawData);
}


//
// This function is modified by sang cho
//
//
/* return offset to specified IMAGE_DIRECTORY entry */
LPVOID	WINAPI ImageDirectoryOffset (
	LPVOID	  lpFile,
	DWORD	  dwIMAGE_DIRECTORY)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
    PIMAGE_SECTION_HEADER    psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile);
    int 		     nSections = NumOfSections (lpFile);
    int 		     i = 0;
    LPVOID		     VAImageDir;

    /* must be 0 thru (NumberOfRvaAndSizes-1) */
    if (dwIMAGE_DIRECTORY >= poh->NumberOfRvaAndSizes)
	return NULL;

    /* locate specific image directory's relative virtual address */
    VAImageDir = (LPVOID)poh->DataDirectory[dwIMAGE_DIRECTORY].VirtualAddress;

    if (VAImageDir == NULL) return NULL;
    /* locate section containing image directory */
    while (i++<nSections)
	{
	    if (psh->VirtualAddress <= (DWORD)VAImageDir &&
	        psh->VirtualAddress + psh->SizeOfRawData > (DWORD)VAImageDir)
	    break;
	    psh++;
	}

    if (i > nSections)
	return NULL;

    /* return image import directory offset */
    return (LPVOID)(((int)lpFile + (int)VAImageDir - psh->VirtualAddress) +
				   (int)psh->PointerToRawData);
}


/* function retrieve names of all the sections in the file */
int WINAPI GetSectionNames (
    LPVOID    lpFile,
    char      **pszSections)
{
    int 		     nSections = NumOfSections (lpFile);
    int 		     i, nCnt = 0;
    PIMAGE_SECTION_HEADER    psh;
    char		     *ps;


    if (ImageFileType (lpFile) != IMAGE_NT_SIGNATURE ||
	(psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile)) == NULL)
	return 0;

    /* count the number of chars used in the section names */
    for (i=0; i<nSections; i++)
	nCnt += strlen (psh[i].Name) + 1;

    /* allocate space for all section names from heap */
    ps = *pszSections = (char *)calloc (nCnt, 1);


    for (i=0; i<nSections; i++)
	{
	    strcpy (ps, psh[i].Name);
	    ps += strlen (psh[i].Name) + 1;
	}

    return nCnt;
}




/* function gets the function header for a section identified by name */
BOOL	WINAPI GetSectionHdrByName (
    LPVOID		     lpFile,
    IMAGE_SECTION_HEADER     *sh,
    char		     *szSection)
{
    PIMAGE_SECTION_HEADER    psh;
    int 		     nSections = NumOfSections (lpFile);
    int 		     i;


    if ((psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile)) != NULL)
	{
	/* find the section by name */
	    for (i=0; i<nSections; i++)
	    {
	        if (!strcmp (psh->Name, szSection))
		    {
		        /* copy data to header */
		        bcopy ((LPVOID)psh, (LPVOID)sh, sizeof (IMAGE_SECTION_HEADER));
		        return TRUE;
		    }
	        else psh++;
	    }
	}
    return FALSE;
}



//
// This function is modified by sang cho
//
//
/* get import modules names separated by null terminators, return module count */
int  WINAPI GetImportModuleNames (
    LPVOID    lpFile,
    char      **pszModules)
{
    PIMAGE_IMPORT_MODULE_DIRECTORY  pid = (PIMAGE_IMPORT_MODULE_DIRECTORY)
	ImageDirectoryOffset (lpFile, IMAGE_DIRECTORY_ENTRY_IMPORT);
	//
	// sometimes there may be no section for idata or edata
	// instead rdata or data section may contain these sections ..
	// or even module names or function names are in different section.
	// so that's why we need to get actual address of RVAs each time.
	//         ...................sang cho..................
	//
	// PIMAGE_SECTION_HEADER     psh = (PIMAGE_SECTION_HEADER)
	// ImageDirectorySection (lpFile, IMAGE_DIRECTORY_ENTRY_IMPORT);
    // BYTE		     *pData = (BYTE *)pid;
	DWORD            *pdw = (DWORD *)pid;
    int 		     nCnt = 0, nSize = 0, i;
    char		     *pModule[1024];  /* hardcoded maximum number of modules?? */
    int               pidTab[1024];
	char		     *psz;

	if (pid == NULL) return 0;

    // pData = (BYTE *)((int)lpFile + psh->PointerToRawData - psh->VirtualAddress);

    /* extract all import modules */
    while (pid->dwRVAModuleName)
	{
	/* allocate temporary buffer for absolute string offsets */
	    //pModule[nCnt] = (char *)(pData + pid->dwRVAModuleName);
		pModule[nCnt] = (char *)GetActualAddress (lpFile, pid->dwRVAModuleName);
		pidTab[nCnt] = (int)pid;
	    nSize += strlen (pModule[nCnt]) + 1 + 4;

	/* increment to the next import directory entry */
	    pid++;
	    nCnt++;
	}

    /* copy all strings to one chunk of memory */
	*pszModules = (char *)calloc(nSize, 1);
	piNameBuffSize = nSize;
    psz = *pszModules;
    for (i=0; i<nCnt; i++)
	{
	    *(int *)psz = pidTab[i]; 
		strcpy (psz+4, pModule[i]);
	    psz += strlen (psz+4) + 1 + 4;
	}
	return nCnt;
}


//
// This function is rewritten by sang cho
//
//
/* get import module function names separated by null terminators, return function count */
int  WINAPI GetImportFunctionNamesByModule (
    LPVOID    lpFile,
    char      *pszModule,
    char      **pszFunctions)
{
    PIMAGE_IMPORT_MODULE_DIRECTORY  pid;
	
	//= (PIMAGE_IMPORT_MODULE_DIRECTORY)
	//ImageDirectoryOffset (lpFile, IMAGE_DIRECTORY_ENTRY_IMPORT);
	// modified by sangcho 1998.1.25
	
	//
	// sometimes there may be no section for idata or edata
	// instead rdata or data section may contain these sections ..
	// or even module names or function names are in different section.
	// so that's why we need to get actual address each time.
	//         ...................sang cho..................
	//
	//PIMAGE_SECTION_HEADER           psh = (PIMAGE_SECTION_HEADER)
	//ImageDirectorySection (lpFile, IMAGE_DIRECTORY_ENTRY_IMPORT);
    //DWORD		     dwBase;
    
	extern Bnode    *head;	         // label data B-Tree header
    extern int       btn;           // label data B-Tree control number
	
	int 		     nCnt = 0, nSize = 0;
	int              nnid = 0;
	int              mnlength, i;
    DWORD		     dwFunctionName;
	DWORD            dwFunctionAddress;
	char             name[128];
	char             buff[256];		// enough for any string ??
    char		     *psz;
	DWORD            *pdw;
	int              r,rr;
	_key_            k;


   	//dwBase = (DWORD)((int)lpFile + psh->PointerToRawData - psh->VirtualAddress);

    /* find module's pid */
    //while (pid->dwRVAModuleName &&
	//   strcmp (pszModule, (char *)GetActualAddress(lpFile, pid->dwRVAModuleName)))
	//pid++;

    pid = (PIMAGE_IMPORT_MODULE_DIRECTORY)(*(DWORD *)pszModule);

	//fprintf(stderr, "\npid=%08X", pid),getch();
	
	/* exit if the module is not found */
    if (!pid->dwRVAModuleName)
	return 0;

	// I am doing this to get rid of .dll from module name
	strcpy (name, pszModule+4);
	mnlength = strlen (pszModule+4);
	for (i=0; i<mnlength; i++) if (name[i] == '.') break;
	name[i] = 0;
	mnlength = i;

    /* count number of function names and length of strings */
    dwFunctionName = pid->dwRVAFunctionNameList;
	
	// IMAGE_IMPORT_BY_NAME	OR IMAGE_THUNK_DATA
	// modified by Sang Cho

	
	//fprintf(stderr,"pid = %08X dwFunctionName = %08X name = %s", 
	//(int)pid-(int)lpFile, dwFunctionName,name),getch();

    // modified by sang cho 1998.1.24

	if (dwFunctionName==0) dwFunctionName = pid->dwRVAFunctionAddressList;

	while (dwFunctionName &&
	   *(pdw=(DWORD *)GetActualAddress (lpFile, dwFunctionName)) )      
	{
	    if ((*pdw) & 0x80000000 )	nSize += mnlength + 11 + 1 + 6;
	    else nSize += strlen ((char *)GetActualAddress (lpFile, *pdw+2)) + 1+6;
	    dwFunctionName += 4;
	    nCnt++;
	}
	
    /* allocate memory  for function names */
	*pszFunctions = (char *)calloc (nSize, 1);
    psz = *pszFunctions;

	//
	// I modified this part to store function address (4 bytes),
	//                               ord number (2 bytes),
	//							and	 name strings (which was there originally)
	// so that's why there are 6 more bytes...... +6,  or +4 and +2 etc.
	// these informations are used where they are needed.
	//                      ...........sang cho..................
	//
    /* copy function names to mempry pointer */
    dwFunctionName = pid->dwRVAFunctionNameList;
	// modified by sang cho 1998.1.24
	if (dwFunctionName==0) dwFunctionName = pid->dwRVAFunctionAddressList;
	dwFunctionAddress = pid->dwRVAFunctionAddressList;
    while (dwFunctionName			   &&
	   *(pdw=(DWORD *)GetActualAddress (lpFile, dwFunctionName)) )
	{
	    if ((*pdw) & 0x80000000)
	    {
	        *(int *)psz=(int)(*(DWORD *)GetActualAddress (lpFile, dwFunctionAddress));
			psz += 4;
	        *(short *)psz=*(short *)pdw;
	        psz += 2;
	        sprintf(buff, "%s:NoName%04d", name, nnid++);
	        strcpy (psz, buff);	psz += strlen (buff) + 1;
	    }
	    else
	    {
	        r=*(int *)psz=(int)(*(DWORD *)GetActualAddress (lpFile, dwFunctionAddress));
			psz += 4;
	        *(short *)psz=(*(short *)GetActualAddress(lpFile, *pdw));
	        psz += 2;	 rr=(int)GetActualAddress(lpFile, *pdw + 2);
	        strcpy (psz, (char *)rr);
	        psz += strlen ((char *)GetActualAddress(lpFile, *pdw + 2)) + 1;
	    	
			// this one is needed to link import function names to codes..
			k.class=0; k.c_ref= r; k.c_pos=-rr;
			MyBtreeInsert(&k);
			k.class=0; k.c_ref=-(rr); k.c_pos=(int)pszModule+4;
			MyBtreeInsert(&k);
		}
	    dwFunctionName += 4;
	    dwFunctionAddress += 4;
	}

    return nCnt;
}




//
// This function is written by sang cho
//							   October 6, 1997
//
/* get numerically expressed string length */
int WINAPI GetStringLength (
    char      *psz)
{
    if (!isdigit (*psz)) return 0; 
    if (isdigit (*(psz+1))) return (*psz - '0')*10 + *(psz+1) - '0';
	else return *psz - '0';
}




//
// This function is written by sang cho
//							   October 12, 1997
//

/* translate parameter part of condensed name */
void   WINAPI GetPreviousParamString ( 
    char       *xpin,	                  // read-only source
	char       *xpout)		              // translated result
{
    int         n=0;
	char        *pin, *pout;           

	pin  = xpin;
	pout = xpout;

	pin--;
	if (*pin == ',') pin--;
	else { printf ("\n **error PreviousParamString1 char = %c", *pin); exit(0); }

	while (*pin)
	{
	         if (*pin == '>') n++;
		else if (*pin == '<') n--;
		else if (*pin == ')') n++;
		
		if (n > 0) 
		{
		    if (*pin == '(') n--;
		}
		else if (strchr (",(", *pin)) break;
		pin--;
	}

	//printf("\n ----- %s", pin);
	if (strchr (",(", *pin)) {pin++;} // printf("\n %s", pin); }
	else { printf ("\n **error PreviousParamString2"); exit(0); }

	n = xpin - pin - 1;
	strncpy (pout, pin, n);
	*(pout + n) = 0;
}
	    



//
// This function is written by sang cho
//							   October 10, 1997
//

/* translate parameter part of condensed name */
void   WINAPI TranslateParameters ( 
    char      **ppin,	                  // read-only source
	char      **ppout,		              // translated result
	char      **pps)					  // parameter stack
{
    int         i, n;
	char        c;
	char        name[128];
	char        *pin, *pout, *ps;           

	//printf(" %c ", **in);
	pin  = *ppin;
	pout = *ppout;
	ps   = *pps;
	c = *pin;
	switch (c)
	{
		// types processing
		case 'b': strcpy (pout, "byte");       pout +=  4; pin++;  break;
		case 'c': strcpy (pout, "char");       pout +=  4; pin++;  break; 
		case 'd': strcpy (pout, "double");     pout +=  6; pin++;  break;
		case 'f': strcpy (pout, "float");      pout +=  5; pin++;  break;
		case 'g': strcpy (pout, "long double");pout += 11; pin++;  break;
		case 'i': strcpy (pout, "int");        pout +=  3; pin++;  break; 
		case 'l': strcpy (pout, "long");       pout +=  4; pin++;  break;
		case 's': strcpy (pout, "short");      pout +=  5; pin++;  break; 
	    case 'v': strcpy (pout, "void");       pout +=  4; pin++;  break;
		// postfix processing
		case 'M':
		case 'p': 
		    if (*(pin+1) == 'p') { *ps++ = 'p'; pin += 2; }
			else { *ps++ = '*'; pin++; }
			*ppin = pin; *ppout = pout; *pps = ps;
			return;
		case 'q':
		    *pout++ = '('; pin++;
			*ps++ = 'q';
			*ppin = pin; *ppout = pout; *pps = ps;
		    return;
		case 'r':
		    if (*(pin+1) == 'p') { *ps++ = 'r'; pin += 2; }
			else { *ps++ = '&'; pin++; }
			*ppin = pin; *ppout = pout; *pps = ps;
			return;
		// repeat processing
		case 't':
		    if (isdigit(*(pin+1)))
			{ 
			    n = *(pin+1) - '0'; pin++; pin++;
			    GetPreviousParamString (pout, name);
			    strcpy (pout, name); pout += strlen (name);
			    for (i=1; i<n; i++)
				{
				    *pout++ = ',';
					strcpy (pout, name); pout += strlen (name);
				}
			}
			else pin++;
			break;
		// prefix processing
		case 'u':
		    strcpy (pout, "u");        pout +=  1; pin++;  
			*ppin = pin; *ppout = pout; *pps = ps;
			return;
		case 'x':
		    strcpy (pout, "const ");   pout +=  6; pin++;  
			*ppin = pin; *ppout = pout; *pps = ps;
			return;
		case 'z':
		    strcpy (pout, "static ");  pout +=  7; pin++;  
			*ppin = pin; *ppout = pout; *pps = ps;
			return;
		default:  strcpy (pout, "!1!");pout +=  3; *pout++=*pin++;
		    *ppin = pin; *ppout = pout; *pps = ps;
		    return;
	}
	// need to process postfix finally
	c = *(ps-1);
	if (strchr ("tqx", c))
	{ if (*(pin)&& !strchr( "@$%", *(pin))) *pout++ = ','; 
	  *ppin = pin; *ppout = pout; *pps = ps; return; }
	switch (c)
	{
	    case 'r': strcpy (pout, "*&");  pout += 2;  ps--; break;
		case 'p': strcpy (pout, "**");  pout += 2;  ps--; break;
		case '&': strcpy (pout, "&");   pout += 1;  ps--; break;
		case '*': strcpy (pout, "*");   pout += 1;  ps--; break;
		default:  strcpy (pout, "!2!"); pout += 3;  ps--; break;
	}
	if (*(pin) && !strchr( "@$%", *(pin))) *pout++ = ',';
	*ppin = pin; *ppout = pout; *pps = ps;
}


//
// This function is written by sang cho
//							   October 11, 1997
//

/* translate parameter part of condensed name */
BOOL   WINAPI StringExpands ( 
    char      **ppin,	                  // read-only source
	char      **ppout,		              // translated result
	char      **pps,					  // parameter stack
	Str_P      *pcstr)                    // currently stored string
{
    int         n;
	char        c;
	char        *pin, *pout, *ps;  
	Str_P       c_str;
	BOOL        stringMode = TRUE;

	pin  = *ppin;
	pout = *ppout;
	ps   = *pps;
	c_str = *pcstr;

	     if (strncmp (pin, "bctr", 4) == 0)
	{  strncpy (pout, c_str.pos, c_str.length); 
	   pout += c_str.length; pin += 4; }
	else if (strncmp (pin, "bdtr", 4) == 0)
	{  *pout++ = '~'; 
	   strncpy (pout, c_str.pos, c_str.length);     
	   pout += c_str.length; pin += 4; }
	else if (*pin == 'o')	 
	{  strcpy(pout, "const ");             pout +=  6;  pin++;
	   stringMode = FALSE;
	}
	else if (*pin == 'q')	 
	{  *pout++ = '(';  pin++;
	   *ps++ = 'q';	   stringMode = FALSE;
	}
	else if (*pin == 't')
	{
	   //if (*(ps-1) == 't') { *pout++ = ','; pin++; }	 // this also got me...
	   //else											   october 12  .. sang
	   {  *pout++ = '<';  pin++;
	      *ps++ = 't';	  
	   }
	   stringMode = FALSE;
	}
	else if (strncmp (pin, "xq", 2) == 0)
	{  *pout++ = '('; pin += 2;
	   *ps++ = 'x'; *ps++ = 'q';
	   stringMode = FALSE;
	}
	else if (strncmp (pin, "bcall", 5) == 0)
	{  strcpy (pout, "operator ()");       pout += 11; pin += 5; }
	else if (strncmp (pin, "bsubs", 5) == 0)
	{  strcpy (pout, "operator []");       pout += 11; pin += 5; }
	else if (strncmp (pin, "bnwa", 4) == 0) 
	{  strcpy (pout, "operator new[]");    pout += 14; pin += 4; }
	else if (strncmp (pin, "bdla", 4) == 0) 
	{  strcpy (pout, "operator delete[]"); pout += 17; pin += 4; }
	else if (strncmp (pin, "bnew", 4) == 0)
	{  strcpy (pout, "operator new");      pout += 12; pin += 4; }
	else if (strncmp (pin, "bdele", 5) == 0)
	{  strcpy (pout, "operator delete");   pout += 15; pin += 5; }
	else if (strncmp (pin, "blsh", 4) == 0)
	{  strcpy (pout, "operator <<");       pout += 11; pin += 4; }
	else if (strncmp (pin, "brsh", 4) == 0)
	{  strcpy (pout, "operator >>");       pout += 11; pin += 4; }
	else if (strncmp (pin, "binc", 4) == 0)
	{  strcpy (pout, "operator ++");       pout += 11; pin += 4; }
	else if (strncmp (pin, "bdec", 4) == 0)
	{  strcpy (pout, "operator --");       pout += 11; pin += 4; }
	else if (strncmp (pin, "badd", 4) == 0)
	{  strcpy (pout, "operator +");        pout += 10; pin += 4; }
	else if (strncmp (pin, "brplu", 5) == 0)
	{  strcpy (pout, "operator +=");       pout += 11; pin += 5; }
	else if (strncmp (pin, "bdiv", 4) == 0)
	{  strcpy (pout, "operator /");        pout += 10; pin += 4; }
	else if (strncmp (pin, "brdiv", 5) == 0)
	{  strcpy (pout, "operator /=");       pout += 11; pin += 5; }
	else if (strncmp (pin, "bmul", 4) == 0)
	{  strcpy (pout, "operator *");        pout += 10; pin += 4; }
	else if (strncmp (pin, "brmul", 5) == 0)
	{  strcpy (pout, "operator *=");       pout += 11; pin += 5; }
	else if (strncmp (pin, "basg", 4) == 0)
	{  strcpy (pout, "operator =");        pout += 10; pin += 4; }
	else if (strncmp (pin, "beql", 4) == 0)
	{  strcpy (pout, "operator ==");       pout += 11; pin += 4; }
	else if (strncmp (pin, "bneq", 4) == 0)
	{  strcpy (pout, "operator !=");       pout += 11; pin += 4; }
	else if (strncmp (pin, "bor", 3) == 0)
	{  strcpy (pout, "operator |");        pout += 10; pin += 3; }
	else if (strncmp (pin, "bror", 4) == 0)
	{  strcpy (pout, "operator |=");       pout += 11; pin += 4; }
	else if (strncmp (pin, "bcmp", 4) == 0)
	{  strcpy (pout, "operator ~");        pout += 10; pin += 4; }
	else if (strncmp (pin, "bnot", 4) == 0)
	{  strcpy (pout, "operator !");        pout += 10; pin += 4; }
	else if (strncmp (pin, "band", 4) == 0)
	{  strcpy (pout, "operator &");        pout += 10; pin += 4; }
	else if (strncmp (pin, "brand", 5) == 0)
	{  strcpy (pout, "operator &=");       pout += 11; pin += 5; }
	else if (strncmp (pin, "bxor", 4) == 0)
	{  strcpy (pout, "operator ^");        pout += 10; pin += 4; }
	else if (strncmp (pin, "brxor", 5) == 0)
	{  strcpy (pout, "operator ^=");       pout += 11; pin += 5; }
	else 	 
	{  
	   strcpy (pout, "!$$$!"); pout += 5; 
	}
	*ppin = pin; *ppout = pout; *pps = ps;
	return stringMode;
}   // end of '$' processing



//----------------------------------------------------------------------
// structure to store string tokens
//----------------------------------------------------------------------
//typedef struct _Str_P {
//    char    flag;		  // string_flag '@' or '%' or '#'
//    char    *pos;		  // starting postion of string
//    int     length;  	  // length of string
//	BOOL    wasString;    // if it were stringMode or not
//} Str_P;
//----------------------------------------------------------------------
//
// I think I knocked it down finally. But who knows? 
//                            october 12, 1997 ... sang
//
// well I have to rewrite whole part of TranslateFunctionName..
// this time I am a little bit more experienced than 5 days ago.
// or am i??? anyway i use stacks instead of recurcive calls
// and i hope this will take care of every symptoms i have experienced..
// 							  october 10, 1997 .... sang
// It took a lot of time for me to figure out what is all about....
// but still some prefixes like z (static) 
//     -- or some types like b (byte) ,g (long double) ,s (short) --
//	   -- or postfix  like M ( * )
//     -- or $or ( & ) which is pretty wierd.         .. added.. october 12
//     -- also $t business is quite tricky too. (templates) 
//             there may be a lot of things undiscovered yet....
// I am not so sure my interpretation is correct or not
// If I am wrong please let me know.
//                             october 8, 1997 .... sang
//
//
// This function is written by sang cho
//							   October 5, 1997
//
/* translate condesed import function name */
LPVOID WINAPI TranslateFunctionName (
    char      *psz)
{
	
    
    int 			        i, j, n;
	char                    c, cc;

	static char             buff[512];	// result of translation

	int                     is=0;
	char                    pStack[32]; // parameter processing stack
	Str_P                   sStack[32]; // String processing stack
	Str_P                   tok;        // String token
	Str_P                   c_str;      // current string 

	int                     iend=0;
	char                    *endTab[8];  // end of string position check

	char                   *ps;
	char			       *pin, *pout;
	BOOL                    stringMode=TRUE;

	if (*psz != '@') return psz;
	pin  = psz;
	pout = buff;
	ps   = pStack;
	
	//................................................................
	// serious users may need to run the following code.
	// so I may need to include some flag options...
	// If you want to know about how translation is done,
	// you can just revive following line and you can see it.
	//						   october 6, 1997 ... sang cho
	//printf ("\n................................... %s", psz); // for debugging...
	
	//pa = pb = pout;
	pin++;						   
    tok.flag = 'A'; tok.pos = pout; tok.length = 0;	tok.wasString = stringMode;
	sStack[is++] = tok;       // initialize sStack with dummy marker
	
	while (*pin)
	{
	    while (*pin)
	    {
	        c = *pin;

			//---------------------------------------------
			// check for the end of number specified string
			//---------------------------------------------
			
			if (iend>0)
			{
			    for (i=0;i<iend;i++) if (pin == endTab[i]) break;
				if (i<iend) 
				{ 
				    // move the end of endTab to ith position
				    endTab[i] = endTab[iend-1]; iend--;

					// get top of the string stack
					tok = sStack[is-1];

					// I am expecting '#' token from stack
					if (tok.flag != '#') 

					{ printf("\n**some serious error1** %c is = %d char = %c", 
					  tok.flag, is, *pin); 
					  exit(0);}

					// pop '#' token  I am happy now.
					else
					{	//if (c)
					    //printf("\n pop # token ... current char = %c", c);
						//else printf("\n pop percent token..next char = NULL");
					    is--;	
					}

					stringMode = tok.wasString;

					if (!stringMode) 
					{
						// need to process postfix finally
	                    cc = *(ps-1);
						if (strchr ("qtx", cc))
						{    if (!strchr ("@$%", c)) *pout++ = ',';
						}
						else
						{
	                        switch (cc)
	                        {
	    case 'r': strcpy (pout, "*&");  pout += 2;  ps--; break;
		case 'p': strcpy (pout, "**");  pout += 2;  ps--; break;
		case '&': strcpy (pout, "&");   pout += 1;  ps--; break;
		case '*': strcpy (pout, "*");   pout += 1;  ps--; break;
		default:  strcpy (pout, "!3!"); pout += 3;  ps--; break;
	                        }
						    if (!strchr ("@$%", c)) *pout++ = ',';
						}
					}
					// string mode restored...
					else;
				}
				else ; // do nothing.. 
			}

			//------------------------------------------------
			// special control symbol processing:
			//------------------------------------------------

			if (strchr ("@$%", c))  break;

			//---------------------------------------------------------------
			// string part processing : no '$' met yet 
			//                       or inside of '%' block
			//                       or inside of '#' block (numbered string)
			//---------------------------------------------------------------

			else if (stringMode)     *pout++ = *pin++;
			//else if (is > 1)         *pout++ = *pin++;

			//------------------------------------------------ 
			// parameter part processing: '$' met
			//------------------------------------------------

			else 		     // parameter processing
			{
			    if (!isdigit (c)) TranslateParameters (&pin, &pout, &ps);
				else         // number specified string processing
				{
				    n = GetStringLength (pin);
					if (n<10) pin++; else pin += 2;

					// push '#' token
					//if (*pin)
					//printf("\n push # token .. char = %c", *pin);
					//else printf("\n push percent token..next char = NULL");
					tok.flag = '#'; tok.pos = pout; 
					tok.length = 0; tok.wasString = stringMode;
					sStack[is++] = tok;

					// mark end of input string
					endTab[iend++] = pin + n; 
					stringMode = TRUE;
				}
			}	
	    }	// end of inner while loop
		//
		// beginning of new string or end of string ( quotation mark )
		//
		if (c == '%')
	    {
		    pin++;               // anyway we have to proceed...
	        tok = sStack[is-1];  // get top of the sStack
			if (tok.flag == '%') 
			{ 					
			    // pop '%' token and set c_str 
				//if (*pin)
				//printf("\n pop percent token..next char = %c", *pin);
				//else printf("\n pop percent token..next char = NULL");
				is--;
				c_str = tok; c_str.length = pout - c_str.pos; 
				if (*(ps-1) == 't') 
				{ 
				    *pout++ = '>'; ps--;  
					stringMode = tok.wasString;
				}
				else { printf("\n**some string error3** stack = %c", *(ps-1)); 
				exit(0); }
			}
			else if (tok.flag == 'A' || tok.flag == '#')
			{
			    // push '%' token
				//if (*pin)
				//printf("\n push percent token..next char = %c", *pin);
				//else printf("\n push percent token..next char = NULL");
			    tok.flag = '%'; tok.pos = pout; tok.length = 0;
				tok.wasString = stringMode;
				sStack[is++] = tok;      
			}
			else  { printf("\n**some string error5**"); exit(0); }
	    }
		//
		// sometimes we need string to use as constructor name or destructor name
		//
	    else if (c == '@') // get string from previous marker  upto here. 
		{ 
		    pin++;
		    tok = sStack[is-1];
			c_str.flag = 'S'; 
			c_str.pos = tok.pos;
			c_str.length = pout - tok.pos;
			c_str.wasString = stringMode;
			*pout++ = ':'; *pout++ = ':';
		}
		//
		// we need to take care of parameter control sequence
		//
	    else if (c == '$') // need to precess template or parameter part
	    {
			pin++;
			if (stringMode) 
			    stringMode = StringExpands (&pin, &pout, &ps, &c_str);
			else
			{	// template parameter mode I guess  "$t"
			    if (is>1) 
				{  
				    if (*pin == 't') pin++;
					else { printf("\nMYGOODNESS1 %c", *pin); exit(0);}
				    //ps--;
					//if (*ps == 't') *pout++ = '>';
					//else { printf("\nMYGOODNESS2"); exit(0);}
				    *pout++ = ','; //pin++; ..this almost blowed me....
			    }
				// real parameter mode I guess
				// unexpected case is found ... humm what can I do...
				else
				{  
				    // this is newly found twist.. it really hurts.
				    if (ps <= pStack)
					{  if (*pin == 'q') { *ps++ = 'q'; *pout++ = '('; pin++; }
					   else {printf("\n** I GIVEUP ***"); exit(0);}
					   continue;
					}
				    ps--;
					while (*ps != 'q') 
					{       if (*ps == '*') *pout++ = '*';
					   else if (*ps == '&') *pout++ = '&';
					   else if (*ps == 'p'){*pout++ = '*'; *pout++ = '*'; }
					   else if (*ps == 'r'){*pout++ = '*'; *pout++ = '&'; }
					   else {printf("\n*** SOMETHING IS WRONG1*** char= %c",*pin); 
					   exit(0);}
					   ps--;
					}
		            *pout++ = ')'; 
					ps--;
					while (*ps != 'q') 
					{       if (*ps == '*') *pout++ = '*';
					   else if (*ps == '&') *pout++ = '&';
					   else if (*ps == 'p'){*pout++ = '*'; *pout++ = '*'; }
					   else if (*ps == 'r'){*pout++ = '*'; *pout++ = '&'; }
					   else {printf("\n*** SOMETHING IS WRONG2***"); exit(0);}
					   ps--;
					}
		            ps++; *pout++ = ',';
				}
			}
	    }   // end of '$' processing
	}	// end of outer while loop
	//
	// need to process remaining parameter stack
	//
	while (ps>pStack)
	{
	    ps--;
	    switch(*ps)
		{
		    case 't': *pout++ = '>';                      break;
	        case 'q': *pout++ = ')';                      break;
	        case 'x': strcpy (pout, " const"); pout += 6; break;
	        case 'r': strcpy (pout, "*&");     pout += 2; break;
		    case 'p': strcpy (pout, "**");     pout += 2; break;
		    case '&': *pout++ = '&';                      break;
		    case '*': *pout++ = '*';                      break;
		    default:  strcpy (pout, "!4!");    pout += 3; *pout++ = *ps;
		}
	}
	*pout = 0;
	return buff;
}



//
// This function is written by sang cho
//
//
/* get exported function names separated by null terminators, return count of functions */
int  WINAPI GetExportFunctionNames (
    LPVOID    lpFile,
    char      **pszFunctions)
{
    //PIMAGE_SECTION_HEADER      psh;
    PIMAGE_EXPORT_DIRECTORY    ped;
	//DWORD                      dwBase;
	DWORD                      imageBase;			//===========================
	char		              *pfns[8192]={NULL,}; // maximum number of functions
	                                              //=============================  
	char                       buff[256];	     // enough for any string ??
	char                      *psz;				//===============================
	DWORD                     *pdwAddress;
	DWORD                     *pdw1;
	DWORD                     *pdwNames;
	WORD                      *pwOrd;
    int 		               i, nCnt=0, ntmp=0;
	int                        enid=0, ordBase=1; // usally ordBase is 1....
	int                        enames=0;

    /* get section header and pointer to data directory for .edata section */
    ped = (PIMAGE_EXPORT_DIRECTORY)
	ImageDirectoryOffset(lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT);

	if (ped == NULL) return 0;

	//
	// sometimes there may be no section for idata or edata
	// instead rdata or data section may contain these sections ..
	// or even module names or function names are in different section.
	// so that's why we need to get actual address each time.
	//         ...................sang cho..................
	//
    //psh = (PIMAGE_SECTION_HEADER)
	//ImageDirectorySection(lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT);

	//if (psh == NULL) return 0;

	//dwBase = (DWORD)((int)lpFile + psh->PointerToRawData - psh->VirtualAddress);


    /* determine the offset of the export function names */

	pdwAddress = (DWORD *)GetActualAddress (lpFile, (DWORD)ped->AddressOfFunctions);

	imageBase = (DWORD)GetImageBase (lpFile);
    
	ordBase = ped->Base;

	if (ped->NumberOfNames > 0)
	{
        pdwNames = (DWORD *)
		           GetActualAddress (lpFile, (DWORD)ped->AddressOfNames);
		pwOrd = (WORD *)
		        GetActualAddress (lpFile, (DWORD)ped->AddressOfNameOrdinals);
		pdw1 = pdwAddress;

    /* figure out how much memory to allocate for all strings */
		for (i=0; i < (int)ped->NumberOfNames; i++)
		{
		    nCnt += strlen ((char *)
			                GetActualAddress (lpFile, *(DWORD *)pdwNames)) + 1 + 6;
			pdwNames++;
		}
		// get the number of unnamed functions
		for (i=0; i < (int)ped->NumberOfFunctions; i++)
		    if (*pdw1++) ntmp++;
		// add memory required to show unnamed functions.
		if (ntmp > (int)ped->NumberOfNames)
		    nCnt += 18*(ntmp - (int)ped->NumberOfNames);

    /* allocate memory  for function names */
	    *pszFunctions = (char *)calloc (nCnt, 1);
	    fprintf(stderr,"GetExportFunctionNames base %p size 0x%08x\n",*pszFunctions,nCnt);
            memset(*pszFunctions,0,nCnt);
		peNameBuffSize=nCnt;
		pdwNames = (DWORD *)GetActualAddress (lpFile, (DWORD)ped->AddressOfNames);

    /* copy string pointer to buffer */
	    
	    for (i=0; i < (int)ped->NumberOfNames; i++)
	    {
			pfns[(int)(*pwOrd)+ordBase] = 
			(char *)GetActualAddress (lpFile, *(DWORD *)pdwNames);
		    pdwNames++;
			pwOrd++;
	    }

	    psz = *pszFunctions;
	}	

	for (i=ordBase; i < (int)ped->NumberOfFunctions + ordBase; i++)
	{
		if (*pdwAddress > 0)
		{
			*(DWORD *)psz = imageBase + *pdwAddress;
	        psz += 4;
	        *(WORD *)psz = (WORD)(i);
	        psz += 2;
	        if (pfns[i])
			{
			    strcpy (psz, pfns[i]);
		            psz += strlen(psz) + 1;
			}
			else
			{
			    sprintf (buff, "ExpFn%04d()", enid++);
	                    strcpy (psz, buff);
		            psz += 12;
			}
			enames++;
		}
		pdwAddress++;
    }
	
	return enames;

}


/* determine the total number of resources in the section */
int	WINAPI GetNumberOfResources (
    LPVOID    lpFile)
{
    PIMAGE_RESOURCE_DIRECTORY	       prdRoot, prdType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY    prde;
    int 			       nCnt=0, i;


    /* get root directory of resource tree */
    if ((prdRoot = (PIMAGE_RESOURCE_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_RESOURCE)) == NULL)
	return 0;

    /* set pointer to first resource type entry */
    prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((DWORD)prdRoot + sizeof (IMAGE_RESOURCE_DIRECTORY));

    /* loop through all resource directory entry types */
    for (i=0; i<prdRoot->NumberOfIdEntries; i++)
	{
	/* locate directory or each resource type */
	prdType = (PIMAGE_RESOURCE_DIRECTORY)((int)prdRoot + (int)prde->OffsetToData);

	/* mask off most significant bit of the data offset */
	prdType = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)prdType ^ 0x80000000);

	/* increment count of name'd and ID'd resources in directory */
	nCnt += prdType->NumberOfNamedEntries + prdType->NumberOfIdEntries;

	/* increment to next entry */
	prde++;
	}

    return nCnt;
}



//
// This function is rewritten by sang cho
//
//
/* name each type of resource in the section */
int	WINAPI GetListOfResourceTypes (
    LPVOID    lpFile,
    char      **pszResTypes)
{
    PIMAGE_RESOURCE_DIRECTORY	       prdRoot;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY    prde;
    char			       *pMem;
	char                    buff[32];
    int 			       nCnt, i;
	DWORD                  prdeName;


    /* get root directory of resource tree */
    if ((prdRoot = (PIMAGE_RESOURCE_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_RESOURCE)) == NULL)
	return 0;

    /* allocate enuff space  to cover all types */
    nCnt = prdRoot->NumberOfIdEntries * (MAXRESOURCENAME + 1);
	*pszResTypes = (char *)calloc (nCnt, 1);
    if ((pMem = *pszResTypes) == NULL)
	return 0;

    /* set pointer to first resource type entry */
    prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((DWORD)prdRoot + sizeof (IMAGE_RESOURCE_DIRECTORY));

    /* loop through all resource directory entry types */
    for (i=0; i<prdRoot->NumberOfIdEntries; i++)
	{
	prdeName=prde->Name;

	//if (LoadString (hDll, prde->Name, pMem, MAXRESOURCENAME))
	//    pMem += strlen (pMem) + 1;
	//
	// modified by ...................................Sang Cho..
	// I can't user M/S provied funcitons here so I have to figure out
	// how to do above functions. But I can settle down with the following
	// code, which works pretty good for me.
	//
	if      (prdeName== 1){strcpy(pMem, "RT_CURSOR");       pMem+=10;}
	else if (prdeName== 2){strcpy(pMem, "RT_BITMAP");       pMem+=10;}
	else if (prdeName== 3){strcpy(pMem, "RT_ICON  ");       pMem+=10;}
	else if (prdeName== 4){strcpy(pMem, "RT_MENU  ");       pMem+=10;}
	else if (prdeName== 5){strcpy(pMem, "RT_DIALOG");       pMem+=10;}
	else if (prdeName== 6){strcpy(pMem, "RT_STRING");       pMem+=10;}
	else if (prdeName== 7){strcpy(pMem, "RT_FONTDIR");      pMem+=11;}
	else if (prdeName== 8){strcpy(pMem, "RT_FONT  ");       pMem+=10;}
	else if (prdeName== 9){strcpy(pMem, "RT_ACCELERATORS"); pMem+=16;}
	else if (prdeName==10){strcpy(pMem, "RT_RCDATA");       pMem+=10;}
	else if (prdeName==11){strcpy(pMem, "RT_MESSAGETABLE"); pMem+=16;}
	else if (prdeName==12){strcpy(pMem, "RT_GROUP_CURSOR"); pMem+=16;}
	else if (prdeName==14){strcpy(pMem, "RT_GROUP_ICON  "); pMem+=16;}
	else if (prdeName==16){strcpy(pMem, "RT_VERSION");      pMem+=11;}
	else if (prdeName==17){strcpy(pMem, "RT_DLGINCLUDE  "); pMem+=16;}
    else if (prdeName==19){strcpy(pMem, "RT_PLUGPLAY    "); pMem+=16;}
    else if (prdeName==20){strcpy(pMem, "RT_VXD   ");       pMem+=10;}
    else if (prdeName==21){strcpy(pMem, "RT_ANICURSOR   "); pMem+=16;}
    else if (prdeName==22){strcpy(pMem, "RT_ANIICON");      pMem+=11;}
    else if (prdeName== 0x2002)
	                      {strcpy(pMem, "RT_NEWBITMAP");    pMem+=13;}
	else if (prdeName== 0x2004)
	                      {strcpy(pMem, "RT_NEWMENU");      pMem+=11;}
	else if (prdeName== 0x2005)
	                      {strcpy(pMem, "RT_NEWDIALOG");    pMem+=13;}
	else if (prdeName== 0x7fff)
	                      {strcpy(pMem, "RT_ERROR ");       pMem+=10;}
	else                  {sprintf(buff, "RT_UNKNOWN:%08X", prdeName);
						   strcpy(pMem, buff);              pMem+=20;}
	prde++;
	}

    return prdRoot->NumberOfIdEntries;
}



//
// This function is written by sang cho
//							   October 12, 1997
//
/* copy menu information */
void  WINAPI StrangeMenuFill (
    char      **psz,		  // results
    WORD      **pMenu,		  // read-only
	int         size)
{
	WORD      *pwd;
	WORD      *ptr, *pmax;

	pwd = *pMenu;
	pmax = (WORD *)((DWORD)pwd + size);
	ptr = (WORD *)(*psz);
	
	while (pwd < pmax)
	{
	    *ptr++=*pwd++;
	}
	*psz = (char *)ptr;
	*pMenu = pwd;
}



//
// This function is written by sang cho
//							   October 1, 1997
//
/* obtain menu information */
int	WINAPI MenuScan (
    int        *len,
    WORD      **pMenu)
{
    int        num = 0;
	int        ndetails;
	WORD      *pwd;
	WORD       flag, flag1;
	WORD       id, ispopup;

	
	pwd = *pMenu;
	
	flag = *pwd;  // so difficult to correctly code this so let's try this
	pwd++;
	(*len) += 2;                      // flag store
	if ((flag & 0x0010) == 0)
	{
	    ispopup = flag;
	    id = *pwd;
		pwd++;
		(*len) += 2;                  // id store
	}
	else 
	{
	    ispopup = flag;
	}

	while (*pwd) {(*len)++; pwd++;}
	(*len)++;			     			  // name and null character
	pwd++;								 // skip double null
	if ((flag & 0x0010) == 0)           // normal node: done
	{
	    *pMenu = pwd;
	    return (int)flag;
	}		    
	     	                       // popup node: need to go on...
	while (1)
	{	
		*pMenu = pwd;
		flag1 = (WORD)MenuScan (len, pMenu);
		pwd = *pMenu;
        if (flag1 & 0x0080) break; 
	}
//  fill # of details to num above 
    //(*len) += 2;
	*pMenu = pwd;
	return flag;
}


//
// This function is written by sang cho
//							   October 2, 1997
//
/* copy menu information */
int	WINAPI MenuFill (
    char      **psz,
    WORD      **pMenu)
{
    int        num = 0;
	int        ndetails;
	char      *ptr, *pTemp;
	WORD      *pwd;
	WORD       flag, flag1;
	WORD       id, ispopup;

	ptr = *psz;
	pwd = *pMenu;
	//flag = (*(PIMAGE_POPUP_MENU_ITEM *)pwd)->fItemFlags;
	flag = *pwd;  // so difficult to correctly code this so let's try this
	pwd++;                    
	if ((flag & 0x0010) == 0)
	{
	    *(WORD *)ptr = flag;             // flag store
		ptr += 2;
	    *(WORD *)ptr = id = *pwd;          // id store
		ptr += 2;
		pwd++;
	}
	else 
	{
	    *(WORD *)ptr = flag;             // flag store
		ptr += 2;
	}

	while (*pwd) 						 // name extract
	{
	    *ptr = *(char *)pwd;
	    ptr++; pwd++;
	} //name and null character
	*ptr=0;
	ptr++;
	pwd++;						 // skip double null
	
	if ((flag & 0x0010) == 0)           // normal node: done
	{
	    *pMenu = pwd;
		*psz = ptr;
	    return (int)flag;
	}		
	//pTemp = ptr;
	//ptr += 2;
	                             // popup node: need to go on...
	while (1)
	{	
	    //num++;
		*pMenu = pwd;
		*psz = ptr;
		flag1 = (WORD)MenuFill (psz, pMenu);
		pwd = *pMenu;
		ptr = *psz;
        if (flag1 & 0x0080) break; 
	}
//  fill # of details to num above 
    //*(WORD *)pTemp = (WORD)num;
	*pMenu = pwd;
	*psz = ptr;
	return flag;
}


//
//==============================================================================
// The following program is based on preorder-tree-traversal.
// once you understand how to traverse..... 
// the rest is pretty straight forward.
// still we need to scan it first and fill it next time.
// and finally we can print it.
//
// This function is written by sang cho
//							   September 29, 1997
//							   revised october 2, 1997
//                             revised october 12, 1997
// ..............................................................................
// ------------------------------------------------------------------------------
// I use same structure - which is used in P.E. programs - for my reporting.
// So, my structure is as follows:
// 	  # of menu name is stored else where ( in directory I suppose )
//     supermenuname			null terminated string, only ascii is considered.
//         flag                 tells : node is a leaf or a internal node.
//         popupname			null terminated string
//         	
//              flag                normal menu flag (leaf node)
//       		id				    normal menu id
//              name		        normal menu name
//         or   			 or
//              flag			    popup menu flag (internal node)
//              popupname		    popup menu name 
//             
//                 flag				    it may folows
//      		   id					normal menu id
//                 name					normal menu name
//             or 				  or
//                 flag					popup menu
//                 popupname			popup menu name
//				   .........
//                                it goes on like this,
//                                 but usually, it only goes a few steps,...
// ------------------------------------------------------------------------------
/* scan menu and copy menu */
int	WINAPI GetContentsOfMenu (
	LPVOID    lpFile,
    char      **pszResTypes)
{
    PIMAGE_RESOURCE_DIRECTORY	       prdType, prdName, prdLanguage;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY    prde, prde1;
	PIMAGE_RESOURCE_DIR_STRING_U       pMenuName;
	PIMAGE_RESOURCE_DATA_ENTRY         prData;
	//PIMAGE_SECTION_HEADER              psh = (PIMAGE_SECTION_HEADER)
	//ImageDirectorySection (lpFile, IMAGE_DIRECTORY_ENTRY_RESOURCE);
    PIMAGE_MENU_HEADER                 pMenuHeader;
	PIMAGE_POPUP_MENU_ITEM             pPopup;
	PIMAGE_NORMAL_MENU_ITEM            pNormal;
	char                    buff[32];
    int 			        nCnt=0, i, j;
	int                     num=0;
	int                     size;
	int                     sLength, nMenus;
	WORD                    flag;
	WORD                   *pwd;
	DWORD                   prdeName;
	//DWORD                   dwBase;    obsolete
	char			       *pMem, *pTemp;
	BOOL                    isStrange = FALSE;


    /* get root directory of resource tree */
    if ((prdType = (PIMAGE_RESOURCE_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_RESOURCE)) == NULL)
	return 0;

    /* set pointer to first resource type entry */
    prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	       ((DWORD)prdType + sizeof (IMAGE_RESOURCE_DIRECTORY));
 
	for (i=0; i<prdType->NumberOfIdEntries; i++)
	{
    if (prde->Name == RT_MENU) break;
	prde++;
	}
	if (prde->Name != RT_MENU) return 0; 

	prdName = (PIMAGE_RESOURCE_DIRECTORY)
	          ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000) ); 
	if (prdName == NULL) return 0;

	prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	       ((DWORD)prdName + sizeof (IMAGE_RESOURCE_DIRECTORY));

	// sometimes previous code tells you lots of things hidden underneath
	// I wish I could save all the revisions I made ... but again .... sigh.
	//                                  october 12, 1997    sang
	//dwBase = (DWORD)((int)lpFile + psh->PointerToRawData - psh->VirtualAddress);

	nMenus=prdName->NumberOfNamedEntries + prdName->NumberOfIdEntries;
	sLength=0;
	
	for (i=0; i<prdName->NumberOfNamedEntries; i++)
	{
	    pMenuName = (PIMAGE_RESOURCE_DIR_STRING_U) 
	                ((DWORD)prdType + (prde->Name ^ 0x80000000));
	    sLength += pMenuName->Length + 1;
	    
		prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) continue;
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) continue;
		
		pMenuHeader =  (PIMAGE_MENU_HEADER)
		               GetActualAddress (lpFile, prData->OffsetToData);
		
		//
		// normally wVersion and cbHeaderSize should be zero
		// but if it is not then nothing is known to us...
		// so let's do our best ... namely guessing .... and trying ....
		//                      ... and suffering   ... 
		// it gave me many sleepless (not exactly but I like to say this) nights.
		//

		// strange case
		if (pMenuHeader->wVersion | pMenuHeader->cbHeaderSize)
		{
		    //isStrange = TRUE;
		    pwd = (WORD *)((DWORD)pMenuHeader + 16);
			size = prData->Size;
			// expect to return the length needed to report.
			// sixteen more bytes to do something
			sLength += 16+size;
			//StrangeMenuScan (&sLength, &pwd, size);   
		}
		// normal case
		else
		{
		    pPopup = (PIMAGE_POPUP_MENU_ITEM)
		             ((DWORD)pMenuHeader + sizeof (IMAGE_MENU_HEADER));
		    while (1)
	        {	
		        flag = (WORD)MenuScan (&sLength, (WORD **)(&pPopup) );
                if (flag & 0x0080) break;
			}
		}
		prde++;
	}
	for (i=0; i<prdName->NumberOfIdEntries; i++)
	{
	    sLength += 12;
		
	    prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) continue;
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) continue;
		
		pMenuHeader =  (PIMAGE_MENU_HEADER)
		               GetActualAddress (lpFile, prData->OffsetToData);
		// strange case
		if (pMenuHeader->wVersion | pMenuHeader->cbHeaderSize)
		{
		    pwd = (WORD *)((DWORD)pMenuHeader + 16);
			size = prData->Size;
			// expect to return the length needed to report.
			// sixteen more bytes to do something
			sLength += 16+size;
			//StrangeMenuScan (&sLength, &pwd, size);
		}
		// normal case
		else
		{
		    pPopup = (PIMAGE_POPUP_MENU_ITEM)
		             ((DWORD)pMenuHeader + sizeof (IMAGE_MENU_HEADER));
		    while (1)
	        {	
		        flag = (WORD)MenuScan (&sLength, (WORD **)(&pPopup) );
                if (flag & 0x0080) break; 
	        }
		}
		prde++;
	}
	//
	// allocate memory for menu names
	//
	*pszResTypes = (char *)calloc (sLength, 1);

	pMem = *pszResTypes;
	//
	// and start all over again
	//
	prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	       ((DWORD)prdName + sizeof (IMAGE_RESOURCE_DIRECTORY));

	for (i=0; i<prdName->NumberOfNamedEntries; i++)
	{
	    pMenuName = (PIMAGE_RESOURCE_DIR_STRING_U) 
	                ((DWORD)prdType + (prde->Name ^ 0x80000000));
		
		
		for (j=0; j<pMenuName->Length; j++)
		    *pMem++ = (char)(pMenuName->NameString[j]);
		*pMem = 0;
		pMem++;
		

	    prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) continue;
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) continue;
		
		pMenuHeader =  (PIMAGE_MENU_HEADER)
		               GetActualAddress (lpFile, prData->OffsetToData);
		// strange case
		if (pMenuHeader->wVersion | pMenuHeader->cbHeaderSize)
		{
		    pwd = (WORD *)((DWORD)pMenuHeader);
			size = prData->Size;
			strcpy (pMem, ":::::::::::"); pMem +=12;
			*(int *)pMem = size;          pMem += 4;
			StrangeMenuFill (&pMem, &pwd, size);
		}
		// normal case
		else
		{
		    pPopup = (PIMAGE_POPUP_MENU_ITEM)
		             ((DWORD)pMenuHeader + sizeof (IMAGE_MENU_HEADER));
		    while (1)
	        {	
		        flag = (WORD)MenuFill (&pMem, (WORD **)(&pPopup) );
                if (flag & 0x0080) break; 
	        }
		}
        prde++;
	}
	for (i=0; i<prdName->NumberOfIdEntries; i++)
	{
	    
		sprintf (buff, "MenuId_%04X", (prde->Name));
		strcpy (pMem, buff);
		pMem += strlen (buff) + 1;

	    prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) continue;
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) continue;
		
		pMenuHeader =  (PIMAGE_MENU_HEADER)
		               GetActualAddress (lpFile, prData->OffsetToData);
		// strange case
		if (pMenuHeader->wVersion | pMenuHeader->cbHeaderSize)
		{
		    pwd = (WORD *)((DWORD)pMenuHeader);
			size = prData->Size;
			strcpy (pMem, ":::::::::::"); pMem +=12;
			*(int *)pMem = size;          pMem += 4;
			StrangeMenuFill (&pMem, &pwd, size);
		}
		// normal case
		else
		{
		    pPopup = (PIMAGE_POPUP_MENU_ITEM)
		             ((DWORD)pMenuHeader + sizeof (IMAGE_MENU_HEADER));
		    while (1)
	        {
		        flag = (WORD)MenuFill (&pMem, (WORD **)(&pPopup) );
                if (flag & 0x0080) break; 
	        }
		}
		prde++;
	}

    return nMenus;
}


//
// This function is written by sang cho
//							   October 12, 1997
//
/* print contents of menu */
int	WINAPI PrintStrangeMenu (
    char      **psz)
{
    
    int 			        i, j, l;
	int                     num;
	WORD                    flag1, flag2;
	char                    buff[128];
	char			       *ptr, *pmax;

    //return dumpMenu (psz, size);

	ptr  = *psz;

	if(strncmp (ptr, ":::::::::::", 11) != 0) 
	{
	    printf ("\n#### I don't know why!!!");
		dumpMenu (psz, 1024);
		exit (0);
	}

	ptr += 12;
	num = *(int *)ptr;
	ptr += 4;
	pmax = ptr+num;

	*psz = ptr;
	return dumpMenu (psz, num);

	// I will write some code later...

}




//
// This function is written by sang cho
//							   October 2, 1997
//
/* print contents of menu */
int	WINAPI PrintMenu (
    int         indent,
    char      **psz)
{
    
    int 			        i, j, k, l;
	WORD                    id, num;
	WORD                    flag;
	char                    buff[128];
	char			       *ptr;


	ptr = *psz;
	//num = *(WORD *)ptr;
	//ptr += 2;
	while (1)
	{	
		flag = *(WORD *)ptr;
	    if (flag & 0x0010)	 // flag == popup
		{
		    printf ("\n\n");
			for (j=0; j<indent; j++) printf (" ");
			ptr += 2;
		    printf ("%s  {Popup}\n", ptr);
			ptr += strlen (ptr) + 1;
			*psz = ptr;
			PrintMenu (indent+5, psz);
			ptr = *psz;
		}
		else                     // ispopup == 0 
		{
		    printf ("\n");
			for (j=0; j<indent; j++) printf (" ");
			ptr += 2;
			id = *(WORD *)ptr;
			ptr += 2;
			strcpy (buff, ptr);
			l = strlen (ptr);
			ptr += l+1;
			if (strchr (buff, 0x09) != NULL)
			{
			    for (k=0; k<l; k++) if (buff[k] == 0x09) break;
				for (j=0; j<l-k; j++) buff[31-j]=buff[l-j];
				for (j=k; j<32+k-l; j++) buff[j]=32;
			}
			if (strchr (buff, 0x08) != NULL)
			{
			    for (k=0; k<l; k++) if (buff[k] == 0x08) break;
				for (j=0; j<l-k; j++) buff[31-j]=buff[l-j];
				for (j=k; j<32+k-l; j++) buff[j]=32;
			}
			printf ("%s", buff);
			l = strlen (buff);
			for (j=l; j<32; j++) printf(" ");
			printf ("[ID=%04Xh]", id);
			*psz = ptr;
		}
		if (flag & 0x0080) break;
	}

}


//
// This function is written by sang cho
//							   October 2, 1997
//
/* the format of menu is not known so I'll do my best */
int	WINAPI dumpMenu (
    char      **psz,
	int         size)
{
    
    int 			        i, j, k, n, l,c;
	char                    buff[32];
	char			       *ptr, *pmax;

	ptr  = *psz;
	pmax = ptr+size;
    for (i=0; i<(size/16)+1; i++)
	{
	    n = 0;
	    for (j=0; j<16; j++)
		{
		    c = (int)(*ptr);
			if (c<0) c+=256;
			buff[j] = c;
			printf ("%02X",c);
			ptr++; 
			if (ptr >= pmax) break;
			n++;
			if (n%4 == 0) printf (" "); 
		}
		n++; if (n%4 == 0) printf (" ");
		l = j;
		j++;
		for (; j<16; j++) 
		{ n++; if (n%4 == 0) printf ("   "); else printf ("  "); }
		printf ("   ");
		for (k=0; k<l; k++)
		    if (isprint(c=buff[k])) printf("%c", c); else printf(".");
		printf ("\n");
		if (ptr >= pmax) break;
	}

	*psz = ptr;

}




//
// This function is written by sang cho
//							   October 13, 1997
//
/* scan dialog box and copy dialog box */
int	WINAPI GetContentsOfDialog (
	LPVOID    lpFile,
    char      **pszResTypes)
{
    PIMAGE_RESOURCE_DIRECTORY	       prdType, prdName, prdLanguage;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY    prde, prde1;
	PIMAGE_RESOURCE_DIR_STRING_U       pDialogName;
	PIMAGE_RESOURCE_DATA_ENTRY         prData;
    PIMAGE_DIALOG_HEADER               pDialogHeader;
	PIMAGE_CONTROL_DATA                pControlData;
	char                    buff[32];
    int 			        nCnt=0, i, j;
	int                     num=0;
	int                     size;
	int                     sLength, nDialogs;
	WORD                    flag;
	WORD                   *pwd;
	DWORD                   prdeName;
	char			       *pMem, *pTemp;
	BOOL                    isStrange = FALSE;


    /* get root directory of resource tree */
    if ((prdType = (PIMAGE_RESOURCE_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_RESOURCE)) == NULL)
	return 0;

    /* set pointer to first resource type entry */
    prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	       ((DWORD)prdType + sizeof (IMAGE_RESOURCE_DIRECTORY));
 
	for (i=0; i<prdType->NumberOfIdEntries; i++)
	{
    if (prde->Name == RT_DIALOG) break;
	prde++;
	}
	if (prde->Name != RT_DIALOG) return 0; 

	prdName = (PIMAGE_RESOURCE_DIRECTORY)
	          ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000) ); 
	if (prdName == NULL) return 0;

	prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	       ((DWORD)prdName + sizeof (IMAGE_RESOURCE_DIRECTORY));


	nDialogs=prdName->NumberOfNamedEntries + prdName->NumberOfIdEntries;
	sLength=0;
	
	for (i=0; i<prdName->NumberOfNamedEntries; i++)
	{
	    pDialogName = (PIMAGE_RESOURCE_DIR_STRING_U) 
	                  ((DWORD)prdType + (prde->Name ^ 0x80000000));
	    sLength += pDialogName->Length + 1;
	    
		prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) continue;
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) continue;
		
		size = prData->Size; 
		sLength += 4+size;	 
		prde++;
	}
	for (i=0; i<prdName->NumberOfIdEntries; i++)
	{
	    sLength += 14;
		
	    prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) continue;
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) continue;
		
		size = prData->Size; 
		sLength += 4+size;	 
		prde++;
	}
	//
	// allocate memory for menu names
	//
	*pszResTypes = (char *)calloc (sLength, 1);

	pMem = *pszResTypes;
	//
	// and start all over again
	//
	prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	       ((DWORD)prdName + sizeof (IMAGE_RESOURCE_DIRECTORY));

	for (i=0; i<prdName->NumberOfNamedEntries; i++)
	{
	    pDialogName = (PIMAGE_RESOURCE_DIR_STRING_U) 
	                  ((DWORD)prdType + (prde->Name ^ 0x80000000));
		
		
		for (j=0; j<pDialogName->Length; j++)
		    *pMem++ = (char)(pDialogName->NameString[j]);
		*pMem = 0;
		pMem++;
		

	    prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) continue;
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) continue;
		
		pDialogHeader =  (PIMAGE_DIALOG_HEADER)
		                 GetActualAddress (lpFile, prData->OffsetToData);
     
		
		
		pwd = (WORD *)((DWORD)pDialogHeader);
		size = prData->Size;
		*(int *)pMem = size;          pMem += 4;
		StrangeMenuFill (&pMem, &pwd, size);
		
        prde++;
	}
	for (i=0; i<prdName->NumberOfIdEntries; i++)
	{
	    
		sprintf (buff, "DialogId_%04X", (prde->Name));
		strcpy (pMem, buff);
		pMem += strlen (buff) + 1;

	    prdLanguage = (PIMAGE_RESOURCE_DIRECTORY)
	                  ((DWORD)prdType + (prde->OffsetToData ^ 0x80000000));
	    if (prdLanguage == NULL) 
		{   printf ("\nprdLanguage = NULL"); exit (0); }
	    
		prde1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
	            ((DWORD)prdLanguage + sizeof (IMAGE_RESOURCE_DIRECTORY));
	    
		prData = (PIMAGE_RESOURCE_DATA_ENTRY)
	             ((DWORD)prdType + prde1->OffsetToData);
	   	if (prData == NULL) 
		{   printf ("\nprData = NULL"); exit (0); }

		pDialogHeader =  (PIMAGE_DIALOG_HEADER)
		                 GetActualAddress (lpFile, prData->OffsetToData);
		
		
		pwd = (WORD *)((DWORD)pDialogHeader);
		size = prData->Size;
		*(int *)pMem = size;          pMem += 4;
		StrangeMenuFill (&pMem, &pwd, size);
		
		prde++;
	}

    return nDialogs;
}


//
// This function is written by sang cho
//							   October 14, 1997
//
/* print contents of dialog */
int	WINAPI PrintNameOrOrdinal (
    char      **psz)
{    
	char			       *ptr;

	ptr = *psz;
	if (*(WORD *)ptr == 0xFFFF) 
	{	ptr += 2; 
	    printf ("%04X", *(WORD *)ptr);
	    ptr += 2;
	}
	else 
	{ 
	    printf ("%c", '"');
	    while (*(WORD *)ptr) { printf("%c", *ptr); ptr+= 2; } 
		ptr += 2;
		printf ("%c", '"');
	}
	*psz = ptr;
}


//
// This function is written by sang cho
//							   October 14, 1997
//
/* print contents of dialog */
int	WINAPI PrintDialog (
    char      **psz)
{    
    int 			        i, j, k, l, n, c;
	int                     num, size;
	DWORD                  	flag;
	WORD                    class;
	char                    buff[32];
	char			       *ptr, *pmax;
	BOOL                    isStrange=FALSE;

	ptr  = *psz;
	size = *(int *)ptr;
	ptr += 4;
	pmax = ptr+size;
	
	// IStype of Dialog Header
	flag = *(DWORD *)ptr;
	//
	// check if flag is right or not
	// it has been observed that some dialog information is strange
	// and extra work is needed to fix that ... so let's try something
	//
	
	if ((flag & 0xFFFF0000) == 0xFFFF0000)
	{
	    flag = *(DWORD *)(ptr+12);  
		num = *(short *)(ptr+16); 
		isStrange = TRUE;
		ptr += 26;
	}
	else 
	{
	    num  = *(short *)(ptr+8);
		ptr += 18;
	}
	printf (", # of Controls=%03d, Caption:%c", num, '"');
    
	// Menu name
	     if (*(WORD *)ptr == 0xFFFF) ptr += 4;		  // ordinal
	else { while (*(WORD *)ptr) ptr += 2; ptr += 2; } // name
	
	// Class name
	     if (*(WORD *)ptr == 0xFFFF) ptr += 4;		  // ordinal
	else { while (*(WORD *)ptr) ptr += 2; ptr += 2; } // name

	// Caption
	while (*(WORD *)ptr) { printf("%c", *ptr); ptr+= 2; }
	ptr += 2;
	printf ("%c", '"');
	
	// FONT present
	if (flag & 0x00000040)
	{
	    if (isStrange) ptr += 6; else ptr += 2; 	 // FONT size
		while (*(WORD *)ptr)  ptr += 2; 			 // WCHARs
		ptr += 2;                                    // double null  
	}

	// strange case adjust
	if (isStrange) ptr += 8;

	// DWORD padding
	if ((ptr-*psz) % 4) ptr += 4 - ((ptr-*psz) % 4);

	// start reporting .. finally
	for (i=0; i<num; i++)
	{
	    flag = *(DWORD *)ptr;
		if (isStrange) ptr += 14; else ptr += 16;
		printf ("\n     Control::%03d - ID:", i+1);
		
		// Control ID
		printf ("%04X, Class:", *(WORD *)ptr);
		ptr += 2;
	   
		// Control Class
		if (*(WORD *)ptr == 0xFFFF) 
	    {	
		    ptr += 2;  class = *(WORD *)ptr;   ptr += 2;
			switch (class)
			{
			    case 0x80: printf ("BUTTON   ");        break;       
			    case 0x81: printf ("EDIT     ");        break;    
			    case 0x82: printf ("STATIC   ");        break;    
			    case 0x83: printf ("LISTBOX  ");        break;    
			    case 0x84: printf ("SCROLLBAR");        break;    
			    case 0x85: printf ("COMBOBOX ");        break;    
				default:   printf ("%04X     ", class); break;
			}
	    }
		else PrintNameOrOrdinal (&ptr);

		printf (" Text:");

		// Text
		PrintNameOrOrdinal (&ptr);

		// nExtraStuff
		ptr += 2;
	    
		// strange case adjust
		if (isStrange) ptr += 8;

		// DWORD padding
	    if ((ptr-*psz) % 4) ptr += 4 - ((ptr-*psz) % 4);
	}

	/*
	ptr = *psz;
	printf("\n");

    for (i=0; i<(size/16)+1; i++)
	{
	    n = 0;
	    for (j=0; j<16; j++)
		{
		    c = (int)(*ptr);
			if (c<0) c+=256;
			buff[j] = c;
			printf ("%02X",c);
			ptr++; 
			if (ptr >= pmax) break;
			n++;
			if (n%4 == 0) printf (" "); 
		}
		n++; if (n%4 == 0) printf (" ");
		l = j;
		j++;
		for (; j<16; j++) 
		{ n++; if (n%4 == 0) printf ("   "); else printf ("  "); }
		printf ("   ");
		for (k=0; k<l; k++)
		    if (isprint(c=buff[k])) printf("%c", c); else printf(".");
		printf ("\n");
		if (ptr >= pmax) break;
	}
	*/

	*psz = pmax;

}

 




/* function indicates whether debug  info has been stripped from file */
BOOL	WINAPI IsDebugInfoStripped (
    LPVOID    lpFile)
{
    PIMAGE_FILE_HEADER	  pfh;

    pfh = (PIMAGE_FILE_HEADER)PEFHDROFFSET (lpFile);

    return (pfh->Characteristics & IMAGE_FILE_DEBUG_STRIPPED);
}




/* retrieve the module name from the debug misc. structure */
int    WINAPI RetrieveModuleName (
    LPVOID    lpFile,
    char      **pszModule)
{

    PIMAGE_DEBUG_DIRECTORY    pdd;
    PIMAGE_DEBUG_MISC	      pdm = NULL;
    int 		      nCnt;

    if (!(pdd = (PIMAGE_DEBUG_DIRECTORY)ImageDirectoryOffset (lpFile, IMAGE_DIRECTORY_ENTRY_DEBUG)))
	return 0;

    while (pdd->SizeOfData)
	{
	if (pdd->Type == IMAGE_DEBUG_TYPE_MISC)
	    {
	    pdm = (PIMAGE_DEBUG_MISC)((DWORD)pdd->PointerToRawData + (DWORD)lpFile);
		*pszModule = (char *)calloc ((nCnt = (strlen (pdm->Data)))+1, 1);
		// may need some unicode business here...above
		bcopy (pdm->Data, *pszModule, nCnt);

	    break;
	    }

	pdd ++;
	}

    if (pdm != NULL)
	return nCnt;
    else
	return 0;
}





/* determine if this is a valid debug file */
BOOL	WINAPI IsDebugFile (
    LPVOID    lpFile)
{
    PIMAGE_SEPARATE_DEBUG_HEADER    psdh;

    psdh = (PIMAGE_SEPARATE_DEBUG_HEADER)lpFile;

    return (psdh->Signature == IMAGE_SEPARATE_DEBUG_SIGNATURE);
}




/* copy separate debug header structure from debug file */
BOOL	WINAPI GetSeparateDebugHeader (
    LPVOID			    lpFile,
    PIMAGE_SEPARATE_DEBUG_HEADER    psdh)
{
    PIMAGE_SEPARATE_DEBUG_HEADER    pdh;

    pdh = (PIMAGE_SEPARATE_DEBUG_HEADER)lpFile;

    if (pdh->Signature == IMAGE_SEPARATE_DEBUG_SIGNATURE)
	{
	bcopy ((LPVOID)pdh, (LPVOID)psdh, sizeof (IMAGE_SEPARATE_DEBUG_HEADER));
	return TRUE;
	}

    return FALSE;
}


/* I need to place these data here to keep integrity of codes */

LPVOID          lpFile;        /* pointer to the contents of the input file */
LPVOID          lpMap;         /* pointere to the map of codes processed */
int             nSections;	        // number of sections
int             nResources;	        // number of resources
int             nMenus;		        // number of menus
int             nDialogs;            // number of dialogs
int             nImportedModules;	// number of imported modules
int             nFunctions;			// number of functions in the imported module
int             nExportedFunctions;	// number of exported funcions
int             imageBase;			// image base of the file
int             entryPoint;			// entry point of the file
int             imagebaseRVA;  /* imagebase + RVA of the code */
int             CodeOffset;    /* starting point of code   */
int             CodeSize;      /* size of code             */
int             vCodeOffset;    /* starting point of code   */
int             vCodeSize;      /* size of code             */
int             MapSize;       /* size of code map         */
int             maxRVA;        /* the largest RVA of sections */
int             maxRVAsize;    /* size of that section */

char           *piNameBuff;	  // import module name buffer
char           *pfNameBuff;	  // import functions in the module name buffer
char           *peNameBuff;	  // export function name buffer
char           *pmNameBuff;   // menu name buffer
char           *pdNameBuff;	  // dialog name buffer
int             piNameBuffSize;	  // import module name buffer
int             pfNameBuffSize;	  // import functions in the module name buffer
int             peNameBuffSize;	  // export function name buffer
int             pmNameBuffSize;   // menu name buffer
int             pdNameBuffSize;	  // dialog name buffer

extern Bnode *head;	         // label data B-Tree header
extern int    btn;           // label data B-Tree control number

int AddressCheck(int);

//
// I tried to immitate the output of w32dasm disassembler.
// which is a pretty good program.
// but I am disappointed with this program and I myself 
// am writting a disassembler.
// This PEdump program is a byproduct of that project.
// so enjoy this program and I hope we will have a little more
// knowledge on windows programming world.
//							  .... sang cho

#define  MAXSECTIONNUMBER 16
#define  MAXNAMESTRNUMBER 40
int pedump (argc,argv)\
int argc; char **argv;
{
	DWORD                           fileType;
	
	IMAGE_DOS_HEADER                dosHdr;
	PIMAGE_FILE_HEADER              pfh;
	PIMAGE_OPTIONAL_HEADER          poh;
	PIMAGE_SECTION_HEADER           psh;
	IMAGE_SECTION_HEADER            idsh;
	IMAGE_SECTION_HEADER            shdr [MAXSECTIONNUMBER];
	PIMAGE_IMPORT_MODULE_DIRECTORY  pid;

	int       i, j, n;
	int       mnsize;
	int       nCnt;
	int       nSize;
	
	char     *pnstr;
	char     *pst;
//	char     *piNameBuff;	  // import module name buffer
//	char     *pfNameBuff;	  // import functions in the module name buffer
//	char     *peNameBuff;	  // export function name buffer
//	char     *pmNameBuff;     // menu name buffer
//	char     *pdNameBuff;	  // dialog name buffer

	unsigned char          *p, *q;
	_key_                   k;
	PKEY                    pk;

	GetDosHeader (lpFile, &dosHdr);

	if (dosHdr.e_magic == IMAGE_DOS_SIGNATURE)
	{
	    if ((dosHdr.e_lfanew > 4096) || (dosHdr.e_lfanew < 64))
	    {
	printf ("This file is not PE format ... sorry, it looks like DOS format\n");
	exit (0);
	    }
	}
	else 
	{
	printf ("This doesn't look like executable file .. sorry, ...\n");
	exit (0);
	}

	fileType = ImageFileType (lpFile);

	if (fileType != IMAGE_NT_SIGNATURE) 
	{
	    printf ("This file is not PE format ... sorry,\n");
		exit (0);
	}
	
	//=====================================
	// now we can really start processing
	//=====================================

	pfh = (PIMAGE_FILE_HEADER) PEFHDROFFSET (lpFile);

	poh = (PIMAGE_OPTIONAL_HEADER) OPTHDROFFSET (lpFile);
        printf("PIMAGE_OPTIONAL_HEADER:CodeSize         0x%08x\n",poh->SizeOfCode);
        printf("PIMAGE_OPTIONAL_HEADER:ImageBase        0x%08x\n",poh->ImageBase);
        printf("PIMAGE_OPTIONAL_HEADER:EntryPoint       0x%08x\n",poh->AddressOfEntryPoint);
        printf("PIMAGE_OPTIONAL_HEADER:BaseOfCode       0x%08x\n",poh->BaseOfCode);
        CodeSize=poh->SizeOfCode;
	psh = (PIMAGE_SECTION_HEADER) SECHDROFFSET (lpFile);

	nSections = pfh->NumberOfSections;

	imageBase = poh->ImageBase;

	entryPoint = poh->AddressOfEntryPoint;

	if (psh == NULL) return 0;

	/* store section headers */
	
	for (i=0; i < nSections; i++)
	{	
	    shdr[i] = *psh++;
	}

	// get Code offset and size, Data offset and size
	maxRVA = 0;

	for (i=0; i < nSections; i++)
	{	
		if (!strcasecmp(shdr[i].Name,".text")) 
		    {
		      fprintf(stderr,"Found Code size 0x%08x\n",shdr[i].SizeOfRawData);
		      if (shdr[i].SizeOfRawData > CodeSize)
			CodeSize =shdr[i].SizeOfRawData;
		    }
		else
		  fprintf(stderr,"Found string '%s'\n",shdr[i].Name);
		if (poh->BaseOfCode == shdr[i].VirtualAddress)
		{
		    printf("Code Offset = %08X, Code Size = %08X \n", 
	                shdr[i].PointerToRawData, shdr[i].SizeOfRawData);
                    CodeOffset = shdr[i].PointerToRawData;
                    imagebaseRVA = imageBase + shdr[i].VirtualAddress;
 		}
		if (shdr[i].VirtualAddress>maxRVA) 
		{
		    maxRVA = shdr[i].VirtualAddress;
			maxRVAsize = shdr[i].SizeOfRawData;
		}
		if (((shdr[i].Characteristics) & 0xC0000040) == 0xC0000040)
		//if (poh->BaseOfData == shdr[i].VirtualAddress)
		{
	        printf ("Data Offset = %08X, Data Size = %08X \n",
	                shdr[i].PointerToRawData, shdr[i].SizeOfRawData);
		    break;
		}
	}
	for (   ; i < nSections; i++)
	{	
		if (shdr[i].VirtualAddress>maxRVA) 
		{
		    maxRVA = shdr[i].VirtualAddress;
			maxRVAsize = shdr[i].SizeOfRawData;
		}
	}

	printf ("\n");
	
	printf ("Number of Objects = %04d (dec), Imagebase = %08Xh \n",
	        nSections, imageBase);

	// object name alignment
        for (i=0; i < nSections; i++)
	{
	    for (j=0;j<7;j++) if (shdr[i].Name[j]==0) shdr[i].Name[j]=32;
	    shdr[i].Name[7]=0;
	}
	for (i=0; i < nSections; i++)
	  {
		printf ("\n   Object%02d: %8s RVA: %08X Offset: %08X Size: %08X Flags: %08X ",
		        i+1, shdr[i].Name, shdr[i].VirtualAddress, shdr[i].PointerToRawData,
				shdr[i].SizeOfRawData, shdr[i].Characteristics);
	  }
	// Get List of Resources
	nResources = GetListOfResourceTypes (lpFile, &pnstr);
	pst = pnstr;
	printf ("\n");
	printf ("\n+++++++++++++++++++ RESOURCE INFORMATION +++++++++++++++++++");
	printf ("\n");
	if (nResources==0)
	printf ("\n        There are no Resources in This Application.\n");
	else
	{
	    printf ("\nNumber of Resource Types = %4d (decimal)\n", nResources);
	    for (i=0; i < nResources; i++)
	    {
	        printf ("\n   Resource Type %03d: %s",i+1, pst);
	        pst += strlen ((char *)(pst)) + 1;
	    }
	    free ((void *)pnstr);
		
	    printf ("\n");
	    printf ("\n+++++++++++++++++++ MENU INFORMATION +++++++++++++++++++");
	    printf ("\n");

		nMenus = GetContentsOfMenu (lpFile, &pmNameBuff);
		
		if (nMenus == 0)
	    {
	        printf("\n        There are no Menus in This Application.");
	    }
	    else
	    {
		    pst = pmNameBuff;
			printf ("\nNumber of Menus = %4d (decimal)", nMenus);

			//dumpMenu(&pst, 8096); 
		    for (i=0; i < nMenus; i++)
	        {
			    // menu ID print
			    printf ("\n\n%s", pst);
				pst += strlen (pst) + 1;
				printf ("\n-------------");
				if (strncmp (pst, ":::::::::::", 11) == 0)
				{    
				    printf("\n");
				    PrintStrangeMenu (&pst);
				}
				else 
				{
				    PrintMenu (6, &pst);
				}
				//else PrintStrangeMenu(&pst);
	        }						
		    free ((void *)pmNameBuff); 
			printf ("\n");
		}

		printf ("\n");
	    printf ("\n+++++++++++++++++ DIALOG INFORMATION +++++++++++++++++++");
	    printf ("\n");

		nDialogs = GetContentsOfDialog (lpFile, &pdNameBuff);
		
		if (nDialogs == 0)
	    {
	        printf("\n        There are no Dialogs in This Application.");
	    }
	    else
	    {
		    pst = pdNameBuff;
			printf ("\nNumber of Dialogs = %4d (decimal)", nDialogs);

			printf ("\n");

		    for (i=0; i < nDialogs; i++)
	        {
			    // Dialog ID print
			    printf ("\nName: %s", pst);
				pst += strlen (pst) + 1;
				PrintDialog (&pst);
	        }						    
		free ((void *)pdNameBuff); 
		}
		printf ("\n");
	}

	printf ("\n+++++++++++++++++++ IMPORTED FUNCTIONS +++++++++++++++++++");

	nImportedModules = GetImportModuleNames (lpFile, &piNameBuff);
	if (nImportedModules == 0)
	{
	    printf("\n        There are no imported Functions in This Application.\n");
	}
	else
	{
	    pnstr = piNameBuff;
	    printf ("\nNumber of Imported Modules = %4d (decimal)\n", nImportedModules);
	    for (i=0; i < nImportedModules; i++)
	    {
	        printf ("\n   Import Module %03d: %s",i+1, pnstr + 4);
	        pnstr += strlen ((char *)(pnstr+4)) + 1 + 4;
	    }

	    printf("\n");
	    printf("\n+++++++++++++++++++ IMPORT MODULE DETAILS +++++++++++++++++");
	    pnstr = piNameBuff;
	    for (i=0; i < nImportedModules; i++)
	    {
	        printf ("\n\n   Import Module %03d: %s \n",i+1, pnstr + 4);
		    nFunctions = GetImportFunctionNamesByModule (lpFile, pnstr, &pfNameBuff);
			pnstr += strlen ((char *)(pnstr+4)) + 1 + 4;
		    pst = pfNameBuff;
		    for (j=0;j < nFunctions;j++)
		    {
		        printf ("\nAddr:%08X hint(%04X) Name: %s",
			            (*(int *)pst),(*(short *)(pst+4)), 
						//(pst+6));
						TranslateFunctionName(pst+6));
			    pst += strlen ((char *)(pst+6)) + 1 + 6;
		    }
			free ((void *)pfNameBuff);
	    }
	    //free ((void *)piNameBuff);
	}

	printf("\n");
	printf("\n+++++++++++++++++++ EXPORTED FUNCTIONS +++++++++++++++++++");

	nExportedFunctions = GetExportFunctionNames (lpFile, &peNameBuff);
	printf ("\nNumber of Exported Functions = %4d (decimal)\n", nExportedFunctions);
        MapSize = CodeSize + 1;
	lpMap = (void *) calloc (MapSize, 1);
        printf("lpMap %p max %p\n",imagebaseRVA, 
	       MapSize+1 + imagebaseRVA);
	if (nExportedFunctions > 0)
	{
	    pst = peNameBuff;
	    
	    for (i=0; i < nExportedFunctions-1; i++)
	    {
		    printf ("\nAddr:%08X Ord:%4d (%04Xh) Name: %s",
			       (*(int *)pst), (*(WORD *)(pst+4)), (*(WORD *)(pst+4)), 
				   //(pst+6));
				   TranslateFunctionName(pst+6));
			// this one is needed to link export function names to codes..
			k.class=2048; k.c_ref= *(int *)pst; k.c_pos=0;
			if (AddressCheck(k.c_ref))
			{
                            MyBtreeInsertEx(&k);
			    p=lpMap+k.c_ref-imagebaseRVA;
			    if ((p) && (((unsigned int)p -(unsigned int)lpMap) < MapSize))
			      {
				*p |= 0x60;
				k.class=0; k.c_pos=-(int)(pst+6);
				MyBtreeInsert(&k);
			      }
			    else
			      fprintf(stderr,"\nerror");
			}
			pst += strlen ((char *)(pst+6)) + 6+1;
		}
	    //free ((void *)peNameBuff);
	}	
	// free ((void *)lpFile);
}	
