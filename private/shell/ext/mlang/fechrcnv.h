#ifndef _FECHRCNV_H_
#define _FECHRCNV_H_

#ifdef __cplusplus
extern "C" {
#endif 

// Code Page
#define CP_JAPAN				932
#define CP_PRC					936
#define CP_KOREA				949
#define CP_TAIWAN				950

// Code Definition
#define ESC						0x1b
#define SO						0x0e
#define SI						0x0f
#define ISO2022_IN_CHAR			'$'
#define ISO2022_IN_JP_CHAR1		'B'
#define ISO2022_IN_JP_CHAR2		'@'
#define ISO2022_IN_JP_CHAR3_1	'('
#define ISO2022_IN_JP_CHAR3_2	'D'
#define ISO2022_IN_KR_CHAR_1	')'
#define ISO2022_IN_KR_CHAR_2	'C'
#define ISO2022_OUT_CHAR		'('
#define ISO2022_OUT_JP_CHAR1	'B'
#define ISO2022_OUT_JP_CHAR2	'J'
#define ISO2022_OUT_JP_CHAR3	'I' /* esc ( I - Kana mode */
#define ISO2022_OUT_JP_CHAR4	'H' /* treated as ESC ( J */

// Minimum length to determine if the string is EUC
#define MIN_JPN_DETECTLEN      48

typedef LPVOID HCINS;
typedef HCINS FAR* LPHCINS;

#ifdef __cplusplus
}
#endif 

#endif  // _FECHRCNV_H_

