#ifndef _INC_FILETYPE_
#define _INC_FILETYPE_


// File Type Attributes key's bitmap values (HKEY_CLASSES_ROOT\filetype,Attributes)
#define FTA_Exclude             0x00000001 //  1. used to exclude types like drvfile
#define FTA_Show                0x00000002 //  2. used to show types like folder that don't have associations
#define FTA_HasExtension        0x00000004 //  3. type has assoc extension
#define FTA_NoEdit              0x00000008 //  4. no editing of file type
#define FTA_NoRemove            0x00000010 //  5. no deling of the file type
#define FTA_NoNewVerb           0x00000020 //  6. no adding of verbs
#define FTA_NoEditVerb          0x00000040 //  7. no editing of predefined verbs
#define FTA_NoRemoveVerb        0x00000080 //  8. no deling of predefined verbs
#define FTA_NoEditDesc          0x00000100 //  9. no editing of file type description
#define FTA_NoEditIcon          0x00000200 // 10. no editing of doc icon
#define FTA_NoEditDflt          0x00000400 // 11. no changing of default verb
#define FTA_NoEditVerbCmd       0x00000800 // 12. no editing of the verbs command
#define FTA_NoEditVerbExe       0x00001000 // 13. no editing of the verbs exe
#define FTA_NoDDE               0x00002000 // 14. no editing of the DDE fields
#define FTA_ExtShellOpen        0x00004000 // 15. old style type: HKCR/.ext/shell/open/command
#define FTA_NoEditMIME          0x00008000 // 16. no editing of the Content Type or Default Extension fields
#define FTA_OpenIsSafe          0x00010000 // 17. the file class's open verb may be safely invoked for downloaded files
#define FTA_AlwaysUnsafe        0x00020000 // 18. don't allow the "Never ask me" checkbox to be enabled; File Type dialog still allows user to turn this off
#define FTA_AlwaysShowExt       0x00040000 // 19. always show the extension (even if the user has "hide extensions" displayed)
#define FTA_MigratedShowExt     0x00080000 // 20. has the old lame AlwaysShowExt reg key been migrated into the class flags yet?
#define FTA_NoRecentDocs        0x00100000 // 21. dont add this file type to the Recent Documents folder

#define FTAV_UserDefVerb        0x00000001 // 1. identifies verb as being user defined (!predefined)


//================================================================
// typedef's
//================================================================

typedef enum mimeflags
{

    MIME_FL_CONTENT_TYPES_ADDED   = 0x0001, // The Content Type combo box drop down has been filled with MIME types.

    /* flag combinations */

    ALL_MIME_FLAGS                = MIME_FL_CONTENT_TYPES_ADDED
} MIMEFLAGS;


#endif // _INC_FILETYPE_
