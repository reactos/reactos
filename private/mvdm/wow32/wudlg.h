/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUDLG.H
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/



/* Function prototypes
 */

ULONG FASTCALL   WU32CheckDlgButton(PVDMFRAME pFrame);
ULONG FASTCALL   WU32CheckRadioButton(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DialogBoxParam(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DlgDirList(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DlgDirListComboBox(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DlgDirSelect(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DlgDirSelectComboBox(PVDMFRAME pFrame);
ULONG FASTCALL   WU32EndDialog(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetDialogBaseUnits(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetDlgCtrlID(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetDlgItem(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetDlgItemInt(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetDlgItemText(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetNextDlgGroupItem(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetNextDlgTabItem(PVDMFRAME pFrame);
ULONG FASTCALL   WU32IsDialogMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32IsDlgButtonChecked(PVDMFRAME pFrame);
ULONG FASTCALL   WU32MapDialogRect(PVDMFRAME pFrame);
ULONG FASTCALL   WU32MessageBox(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SetDlgItemInt(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SetDlgItemText(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SysErrorBox(PVDMFRAME pFrame);
