#ifndef _SHELLP_H_
#define _SHELLP_H_

//
// shell private header
//


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // _SHELLP_H_
