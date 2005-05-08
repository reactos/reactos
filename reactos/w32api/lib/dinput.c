/*
   DINPUT.C

   Author: Daniel Guerrero Miralles (daguer@geocities.com)
   Version: 1.1.2
   Date: 12/98

   ABSTRACT:
   DirectInput library static data source code. For DirectX 6.1 and
   earlier versions.

   LEGAL INFORMATION:
   This is PUBLIC DOMAIN source code. The source code in this file is
   provided "as is", without any warranty, including but not limited to,
   fitness for any particular purpose.

   REMARKS:
   - Fixed bug in c_dfDIMouse definition.

   TODO:
   Nothing.
*/

#if defined(__LCC__) || defined(__GNUC__) || defined(__WATCOMC__)
#include <windows.h>
#else
#include <basetyps.h>
#endif

/* --- Types and constants --- */

typedef struct DIOBJECTDATAFORMAT_TAG
{
	const GUID * pguid;
	DWORD dwOfw;
	DWORD dwType;
	DWORD dwFlags;
} DIOBJECTDATAFORMAT;

typedef struct DIDATAFORMAT_TAG {
	DWORD dwSize;
	DWORD dwObjSize;
	DWORD dwFlags;
	DWORD dwDataSize;
	DWORD dwNumObjs;
	DIOBJECTDATAFORMAT * rgodf;
} DIDATAFORMAT;

#define DIDF_ABSAXIS 1L
#define DIDF_RELAXIS 2L
#define DIDFT_AXIS 3L
#define DIDFT_BUTTON 12L
#define DIDFT_POV 16L
#define DIDFT_MAKEINSTANCE(x) ((WORD)(x)<<8)
#define DIDFT_ANYINSTANCE (DIDFT_MAKEINSTANCE(-1))
#define DIDOI_ASPECTPOSITION (1L<<8)
#define DIDOI_ASPECTVELOCITY (2L<<8)
#define DIDOI_ASPECTACCEL (3L<<8)
#define DIDOI_ASPECTFORCE (4L<<8)

extern GUID GUID_XAxis;
extern GUID GUID_YAxis;
extern GUID GUID_ZAxis;
extern GUID GUID_RxAxis;
extern GUID GUID_RyAxis;
extern GUID GUID_RzAxis;
extern GUID GUID_Slider;
extern GUID GUID_Key;
extern GUID GUID_POV;

/* --- Static data --- */

static DIOBJECTDATAFORMAT diodfKeyData[] =
{
	{
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 0,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(0),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 1,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(1),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 2,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(2),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 3,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 4,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(4),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 5,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 6,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(6),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 7,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(7),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 8,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(8),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 9,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(9),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 10,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(10),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 11,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(11),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 12,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(12),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 13,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(13),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 14,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(14),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 15,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(15),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 16,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(16),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 17,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(17),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 18,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(18),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 19,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(19),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 20,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(20),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 21,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(21),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 22,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(22),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 23,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(23),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 24,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(24),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 25,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(25),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 26,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(26),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 27,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(27),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 28,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(28),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 29,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(29),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 30,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(30),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 31,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(31),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 32,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(32),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 33,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(33),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 34,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(34),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 35,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(35),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 36,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(36),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 37,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(37),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 38,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(38),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 39,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(39),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 40,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(40),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 41,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(41),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 42,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(42),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 43,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(43),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 44,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(44),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 45,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(45),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 46,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(46),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 47,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(47),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 48,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(48),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 49,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(49),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 50,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(50),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 51,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(51),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 52,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(52),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 53,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(53),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 54,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(54),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 55,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(55),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 56,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(56),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 57,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(57),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 58,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(58),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 59,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(59),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 60,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(60),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 61,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(61),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 62,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(62),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 63,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(63),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 64,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(64),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 65,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(65),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 66,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(66),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 67,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(67),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 68,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(68),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 69,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(69),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 70,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(70),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 71,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(71),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 72,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(72),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 73,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(73),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 74,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(74),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 75,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(75),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 76,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(76),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 77,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(77),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 78,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(78),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 79,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(79),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 80,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(80),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 81,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(81),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 82,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(82),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 83,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(83),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 84,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(84),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 85,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(85),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 86,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(86),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 87,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(87),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 88,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(88),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 89,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(89),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 90,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(90),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 91,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(91),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 92,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(92),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 93,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(93),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 94,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(94),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 95,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(95),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 96,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(96),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 97,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(97),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 98,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(98),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 99,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(99),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 100,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(100),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 101,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(101),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 102,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(102),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 103,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(103),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 104,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(104),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 105,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(105),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 106,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(106),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 107,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(107),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 108,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(108),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 109,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(109),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 110,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(110),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 111,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(111),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 112,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(112),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 113,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(113),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 114,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(114),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 115,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(115),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 116,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(116),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 117,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(117),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 118,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(118),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 119,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(119),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 120,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(120),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 121,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(121),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 122,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(122),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 123,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(123),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 124,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(124),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 125,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(125),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 126,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(126),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 127,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(127),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 128,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(128),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 129,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(129),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 130,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(130),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 131,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(131),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 132,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(132),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 133,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(133),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 134,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(134),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 135,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(135),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 136,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(136),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 137,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(137),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 138,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(138),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 139,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(139),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 140,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(140),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 141,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(141),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 142,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(142),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 143,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(143),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 144,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(144),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 145,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(145),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 146,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(146),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 147,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(147),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 148,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(148),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 149,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(149),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 150,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(150),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 151,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(151),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 152,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(152),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 153,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(153),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 154,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(154),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 155,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(155),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 156,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(156),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 157,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(157),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 158,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(158),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 159,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(159),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 160,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(160),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 161,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(161),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 162,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(162),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 163,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(163),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 164,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(164),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 165,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(165),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 166,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(166),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 167,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(167),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 168,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(168),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 169,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(169),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 170,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(170),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 171,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(171),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 172,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(172),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 173,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(173),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 174,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(174),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 175,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(175),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 176,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(176),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 177,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(177),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 178,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(178),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 179,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(179),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 180,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(180),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 181,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(181),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 182,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(182),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 183,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(183),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 184,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(184),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 185,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(185),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 186,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(186),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 187,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(187),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 188,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(188),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 189,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(189),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 190,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(190),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 191,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(191),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 192,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(192),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 193,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(193),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 194,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(194),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 195,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(195),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 196,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(196),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 197,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(197),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 198,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(198),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 199,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(199),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 200,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(200),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 201,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(201),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 202,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(202),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 203,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(203),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 204,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(204),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 205,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(205),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 206,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(206),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 207,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(207),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 208,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(208),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 209,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(209),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 210,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(210),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 211,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(211),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 212,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(212),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 213,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(213),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 214,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(214),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 215,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(215),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 216,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(216),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 217,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(217),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 218,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(218),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 219,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(219),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 220,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(220),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 221,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(221),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 222,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(222),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 223,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(223),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 224,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(224),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 225,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(225),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 226,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(226),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 227,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(227),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 228,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(228),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 229,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(229),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 230,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(230),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 231,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(231),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 232,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(232),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 233,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(233),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 234,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(234),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 235,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(235),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 236,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(236),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 237,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(237),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 238,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(238),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 239,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(239),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 240,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(240),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 241,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(241),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 242,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(242),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 243,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(243),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 244,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(244),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 245,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(245),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 246,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(246),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 247,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(247),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 248,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(248),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 249,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(249),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 250,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(250),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 251,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(251),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 252,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(252),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 253,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(253),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 254,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(254),
	/* dwFlags = */ 0
	}, {
	/* pguid =   */ &GUID_Key,
	/* dwOfw =   */ 255,
	/* dwType =  */ 0x80000000|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(255),
	/* dwFlags = */ 0
        }
};

static DIOBJECTDATAFORMAT diodfJoyData[] =
{
        {
        /* pguid =   */ &GUID_XAxis,
        /* dwOfw =   */ 0,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_YAxis,
        /* dwOfw =   */ 4,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_ZAxis,
        /* dwOfw =   */ 8,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_RxAxis,
        /* dwOfw =   */ 12,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_RyAxis,
        /* dwOfw =   */ 16,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_RzAxis,
        /* dwOfw =   */ 20,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 24,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 28,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 32,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 36,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 40,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 44,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 48,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 49,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 50,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 51,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 52,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 53,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 54,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 55,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 56,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 57,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 58,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 59,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 60,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 61,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 62,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 63,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 64,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 65,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 66,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 67,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 68,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 69,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 70,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 71,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 72,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 73,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 74,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 75,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 76,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 77,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 78,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 79,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }
};

static DIOBJECTDATAFORMAT diodfJoy2Data[] =
{
        {
        /* pguid =   */ &GUID_XAxis,
        /* dwOfw =   */ 0,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_YAxis,
        /* dwOfw =   */ 4,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_ZAxis,
        /* dwOfw =   */ 8,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_RxAxis,
        /* dwOfw =   */ 12,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_RyAxis,
        /* dwOfw =   */ 16,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_RzAxis,
        /* dwOfw =   */ 20,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 24,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 28,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTPOSITION
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 32,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 36,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 40,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_POV,
        /* dwOfw =   */ 44,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_POV,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 48,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 49,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 50,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 51,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 52,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 53,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 54,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 55,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 56,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 57,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 58,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 59,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 60,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 61,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 62,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 63,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 64,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 65,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 66,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 67,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 68,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 69,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 70,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 71,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 72,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 73,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 74,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 75,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 76,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 77,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 78,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 79,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 80,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 81,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 82,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 83,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 84,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 85,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 86,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 87,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 88,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 89,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 90,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 91,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 92,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 93,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 94,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 95,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 96,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 97,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 98,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 99,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 100,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 101,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 102,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 103,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 104,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 105,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 106,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 107,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 108,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 109,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 110,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 111,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 112,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 113,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 114,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 115,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 116,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 117,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 118,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 119,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 120,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 121,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 122,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 123,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 124,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 125,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 126,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 127,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 128,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 129,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 130,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 131,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 132,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 133,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 134,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 135,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 136,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 137,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 138,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 139,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 140,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 141,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 142,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 143,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 144,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 145,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 146,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 147,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 148,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 149,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 150,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 151,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 152,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 153,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 154,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 155,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 156,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 157,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 158,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 159,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 160,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 161,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 162,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 163,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 164,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 165,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 166,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 167,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 168,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 169,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 170,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 171,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 172,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 173,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 174,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 175,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_XAxis,
        /* dwOfw =   */ 176,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_YAxis,
        /* dwOfw =   */ 180,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_ZAxis,
        /* dwOfw =   */ 184,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_RxAxis,
        /* dwOfw =   */ 188,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_RyAxis,
        /* dwOfw =   */ 192,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_RzAxis,
        /* dwOfw =   */ 196,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 24,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 28,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTVELOCITY
        }, {
        /* pguid =   */ &GUID_XAxis,
        /* dwOfw =   */ 208,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_YAxis,
        /* dwOfw =   */ 212,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_ZAxis,
        /* dwOfw =   */ 216,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_RxAxis,
        /* dwOfw =   */ 220,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_RyAxis,
        /* dwOfw =   */ 224,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_RzAxis,
        /* dwOfw =   */ 228,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 24,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 28,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTACCEL
        }, {
        /* pguid =   */ &GUID_XAxis,
        /* dwOfw =   */ 240,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }, {
        /* pguid =   */ &GUID_YAxis,
        /* dwOfw =   */ 244,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }, {
        /* pguid =   */ &GUID_ZAxis,
        /* dwOfw =   */ 248,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }, {
        /* pguid =   */ &GUID_RxAxis,
        /* dwOfw =   */ 252,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }, {
        /* pguid =   */ &GUID_RyAxis,
        /* dwOfw =   */ 256,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }, {
        /* pguid =   */ &GUID_RzAxis,
        /* dwOfw =   */ 260,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 24,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }, {
        /* pguid =   */ &GUID_Slider,
        /* dwOfw =   */ 28,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ DIDOI_ASPECTFORCE
        }
};

static DIOBJECTDATAFORMAT diodfMouseData[] =
{
        {
        /* pguid =   */ &GUID_XAxis,
        /* dwOfw =   */ 0,
        /* dwType =  */ DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_YAxis,
        /* dwOfw =   */ 4,
        /* dwType =  */ DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ &GUID_ZAxis,
        /* dwOfw =   */ 8,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_AXIS,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 12,
        /* dwType =  */ DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 13,
        /* dwType =  */ DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 14,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }, {
        /* pguid =   */ NULL,
        /* dwOfw =   */ 15,
        /* dwType =  */ 0x80000000|DIDFT_ANYINSTANCE|DIDFT_BUTTON,
        /* dwFlags = */ 0
        }
};

/* --- Library global variables --- */

const DIDATAFORMAT c_dfDIKeyboard =
{
        /* dwSize =     */ 24,
        /* dwObjSize =  */ 16,
        /* dwFlags =    */ DIDF_RELAXIS,
        /* dwDataSize = */ 256,
        /* dwNumObjs =  */ 256,
        /* rgodf =      */ &diodfKeyData[0]
};

const DIDATAFORMAT c_dfDIJoystick =
{
        /* dwSize =     */ 24,
        /* dwObjSize =  */ 16,
        /* dwFlags =    */ DIDF_ABSAXIS,
        /* dwDataSize = */ 80,
        /* dwNumObjs =  */ 44,
        /* rgodf =      */ &diodfJoyData[0]
};

const DIDATAFORMAT c_dfDIJoystick2 = {
        /* dwSize =     */ 24,
        /* dwObjSize =  */ 16,
        /* dwFlags =    */ DIDF_ABSAXIS,
        /* dwDataSize = */ 272,
        /* dwNumObjs =  */ 164,
        /* rgodf =      */ &diodfJoy2Data[0]
};

const DIDATAFORMAT c_dfDIMouse =
{
	/* dwSize =     */ 24,
	/* dwObjSize =  */ 16,
	/* dwFlags =    */ DIDF_RELAXIS,
	/* dwDataSize = */ 16,
	/* dwNumObjs =  */ 7,
	/* rgodf =      */ &diodfMouseData[0]
};
