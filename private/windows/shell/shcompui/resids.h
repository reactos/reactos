#ifndef __SHCOMPUI_RESIDS_H
#define __SHCOMPUI_RESIDS_H
///////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  FILE: RESIDS.H
//
//  DESCRIPTION:
//
//    Resource IDs for SHCOMPUI.DLL.
//
//
//    REVISIONS:
//
//    Date       Description                                         Programmer
//    ---------- --------------------------------------------------- ----------
//    09/15/95   Initial creation.                                   brianau
//
///////////////////////////////////////////////////////////////////////////////
#ifdef WINNT

//
// Confirmation dialog ids.
//
#define DLG_COMPRESS_CONFIRMATION     100  // The dialog.
#define IDC_COMPRESS_CONFIRM_TEXT     101  // Text message control ID.
#define IDC_COMPRESS_SUBFOLDERS       102  // "Also compress sub-folders." cbx control.
#define IDC_COMPRESS_ACTION_TEXT      103

//
// Progress dialog ids.
//
#define DLG_COMPRESS_PROGRESS         104
#define DLG_UNCOMPRESS_PROGRESS       105
#define DLG_COMPRESS_ERROR            106

//
// Compression progress dialog control IDs.
//
#define IDC_COMPRESS_FILE             107  // "Current" file name.
#define IDC_COMPRESS_DIR              108  // "Current" directory text.
#define IDC_COMPRESS_DIRCNT           109  // Count of directories processed.
#define IDC_COMPRESS_FILECNT          110  // Count of files processed.
#define IDC_COMPRESS_USIZE            111  // Cummulative pre-compression bytes.
#define IDC_COMPRESS_CSIZE            112  // Cummulative post-compression bytes.
#define IDC_COMPRESS_RATIO            113  // Cummulative compression ratio as pct.
#define IDC_TEXT1                     114

//
// Uncompression progress dialog control IDs.
//
#define IDC_UNCOMPRESS_FILE           115  // "Current" file name.
#define IDC_UNCOMPRESS_DIR            116  // "Current" directory text.
#define IDC_UNCOMPRESS_DIRCNT         117  // Count of directories processed.
#define IDC_UNCOMPRESS_FILECNT        118  // Count of files processed.

//
// Compression error dialog control IDs.
//
#define IDC_COMPRESS_IGNOREALL        119  // "Ignore all errors" button control.
#define IDC_COMPRESS_ERRTEXT          120  // Error message text control.


//
// Context menu extension string ids.
//
#define IDS_COMPRESS_CMDVERB          200  // Command string returned to shell.
#define IDS_COMPRESS_MENUITEM         201  // Context menu item.
#define IDS_COMPRESS_MENUITEM_ELLIP   202  // Context menu item with ellipsis.
#define IDS_COMPRESS_SBARTEXT         203  // Status bar text, single file.
#define IDS_COMPRESS_SBARTEXT_M       204  // Status bar text, multiple files.
#define IDS_COMPRESS_SBARTEXT_DRV     205  // Status bar text, single drive.
#define IDS_COMPRESS_SBARTEXT_DRV_M   206  // Status bar text, multiple drives.

#define IDS_UNCOMPRESS_CMDVERB        207  // Command string returned to shell.
#define IDS_UNCOMPRESS_MENUITEM       208  // Context menu item.
#define IDS_UNCOMPRESS_MENUITEM_ELLIP 209  // Context menu item with ellipsis.
#define IDS_UNCOMPRESS_SBARTEXT       210  // Status bar text, single file.
#define IDS_UNCOMPRESS_SBARTEXT_M     211  // Status bar text, multiple file.
#define IDS_UNCOMPRESS_SBARTEXT_DRV   212  // Status bar text, single drive.
#define IDS_UNCOMPRESS_SBARTEXT_DRV_M 213  // Status bar text, multiple drives.

//
// Miscellaneous compression support string ids.
//
#define IDS_COMPRESS_DIR              214
#define IDS_UNCOMPRESS_DIR            215
#define IDS_COMPRESS_ATTRIB_ERR       216
#define IDS_NTLDR_COMPRESS_ERR        217
#define IDS_MULTI_COMPRESS_ERR        218

//
// Confirmation dialog string ids.
//
#define IDS_COMPRESS_CONFIRMATION     219  // Compress text message string.
#define IDS_UNCOMPRESS_CONFIRMATION   220  // Uncompress text message string.
#define IDS_COMPRESS_ALSO             221  // "Also compress sub-folders." message.
#define IDS_UNCOMPRESS_ALSO           222  // "Also uncompress sub-folders." message.
#define IDS_COMPRESS_ACTION           223  // "This action compresses..."
#define IDS_UNCOMPRESS_ACTION         224

#define IDS_APP_NAME                  225  // "Explorer"

//
// Byte count display fmt strings.
//
#define IDS_BYTECNT_FMT               226  // "bytes %1".

#define IDS_UNCOMPRESS_DISKFULL       227

#endif  // ifdef WINNT

#endif  // ifdef __SHCOMPUI_RESIDS_H
