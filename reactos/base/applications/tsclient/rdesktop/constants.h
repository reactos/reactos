/*
   rdesktop: A Remote Desktop Protocol client.
   Miscellaneous protocol constants
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* TCP port for Remote Desktop Protocol */
#define TCP_PORT_RDP 3389

#define DEFAULT_CODEPAGE	"UTF-8"
#define WINDOWS_CODEPAGE	"UTF-16LE"

/* ISO PDU codes */
enum ISO_PDU_CODE
{
	ISO_PDU_CR = 0xE0,	/* Connection Request */
	ISO_PDU_CC = 0xD0,	/* Connection Confirm */
	ISO_PDU_DR = 0x80,	/* Disconnect Request */
	ISO_PDU_DT = 0xF0,	/* Data */
	ISO_PDU_ER = 0x70	/* Error */
};

/* MCS PDU codes */
enum MCS_PDU_TYPE
{
	MCS_EDRQ = 1,		/* Erect Domain Request */
	MCS_DPUM = 8,		/* Disconnect Provider Ultimatum */
	MCS_AURQ = 10,		/* Attach User Request */
	MCS_AUCF = 11,		/* Attach User Confirm */
	MCS_CJRQ = 14,		/* Channel Join Request */
	MCS_CJCF = 15,		/* Channel Join Confirm */
	MCS_SDRQ = 25,		/* Send Data Request */
	MCS_SDIN = 26		/* Send Data Indication */
};

#define MCS_CONNECT_INITIAL	0x7f65
#define MCS_CONNECT_RESPONSE	0x7f66

#define BER_TAG_BOOLEAN		1
#define BER_TAG_INTEGER		2
#define BER_TAG_OCTET_STRING	4
#define BER_TAG_RESULT		10
#define MCS_TAG_DOMAIN_PARAMS	0x30

#define MCS_GLOBAL_CHANNEL	1003
#define MCS_USERCHANNEL_BASE    1001

/* RDP secure transport constants */
#define SEC_RANDOM_SIZE		32
#define SEC_MODULUS_SIZE	64
#define SEC_MAX_MODULUS_SIZE	256
#define SEC_PADDING_SIZE	8
#define SEC_EXPONENT_SIZE	4

#define SEC_CLIENT_RANDOM	0x0001
#define SEC_ENCRYPT		0x0008
#define SEC_LOGON_INFO		0x0040
#define SEC_LICENCE_NEG		0x0080
#define SEC_REDIRECT_ENCRYPT	0x0C00

#define SEC_TAG_SRV_INFO	0x0c01
#define SEC_TAG_SRV_CRYPT	0x0c02
#define SEC_TAG_SRV_CHANNELS	0x0c03

#define SEC_TAG_CLI_INFO	0xc001
#define SEC_TAG_CLI_CRYPT	0xc002
#define SEC_TAG_CLI_CHANNELS    0xc003
#define SEC_TAG_CLI_4           0xc004

#define SEC_TAG_PUBKEY		0x0006
#define SEC_TAG_KEYSIG		0x0008

#define SEC_RSA_MAGIC		0x31415352	/* RSA1 */

/* RDP licensing constants */
#define LICENCE_TOKEN_SIZE	10
#define LICENCE_HWID_SIZE	20
#define LICENCE_SIGNATURE_SIZE	16

#define LICENCE_TAG_DEMAND	0x01
#define LICENCE_TAG_AUTHREQ	0x02
#define LICENCE_TAG_ISSUE	0x03
#define LICENCE_TAG_REISSUE	0x04
#define LICENCE_TAG_PRESENT	0x12
#define LICENCE_TAG_REQUEST	0x13
#define LICENCE_TAG_AUTHRESP	0x15
#define LICENCE_TAG_RESULT	0xff

#define LICENCE_TAG_USER	0x000f
#define LICENCE_TAG_HOST	0x0010

/* RDP PDU codes */
enum RDP_PDU_TYPE
{
	RDP_PDU_DEMAND_ACTIVE = 1,
	RDP_PDU_CONFIRM_ACTIVE = 3,
	RDP_PDU_REDIRECT = 4,	/* MS Server 2003 Session Redirect */
	RDP_PDU_DEACTIVATE = 6,
	RDP_PDU_DATA = 7
};

enum RDP_DATA_PDU_TYPE
{
	RDP_DATA_PDU_UPDATE = 2,
	RDP_DATA_PDU_CONTROL = 20,
	RDP_DATA_PDU_POINTER = 27,
	RDP_DATA_PDU_INPUT = 28,
	RDP_DATA_PDU_SYNCHRONISE = 31,
	RDP_DATA_PDU_BELL = 34,
	RDP_DATA_PDU_CLIENT_WINDOW_STATUS = 35,
	RDP_DATA_PDU_LOGON = 38,
	RDP_DATA_PDU_FONT2 = 39,
	RDP_DATA_PDU_KEYBOARD_INDICATORS = 41,
	RDP_DATA_PDU_DISCONNECT = 47
};

enum RDP_CONTROL_PDU_TYPE
{
	RDP_CTL_REQUEST_CONTROL = 1,
	RDP_CTL_GRANT_CONTROL = 2,
	RDP_CTL_DETACH = 3,
	RDP_CTL_COOPERATE = 4
};

enum RDP_UPDATE_PDU_TYPE
{
	RDP_UPDATE_ORDERS = 0,
	RDP_UPDATE_BITMAP = 1,
	RDP_UPDATE_PALETTE = 2,
	RDP_UPDATE_SYNCHRONIZE = 3
};

enum RDP_POINTER_PDU_TYPE
{
	RDP_POINTER_SYSTEM = 1,
	RDP_POINTER_MOVE = 3,
	RDP_POINTER_COLOR = 6,
	RDP_POINTER_CACHED = 7
};

enum RDP_SYSTEM_POINTER_TYPE
{
	RDP_NULL_POINTER = 0,
	RDP_DEFAULT_POINTER = 0x7F00
};

enum RDP_INPUT_DEVICE
{
	RDP_INPUT_SYNCHRONIZE = 0,
	RDP_INPUT_CODEPOINT = 1,
	RDP_INPUT_VIRTKEY = 2,
	RDP_INPUT_SCANCODE = 4,
	RDP_INPUT_MOUSE = 0x8001
};

/* Device flags */
#define KBD_FLAG_RIGHT          0x0001
#define KBD_FLAG_EXT            0x0100
#define KBD_FLAG_QUIET          0x1000
#define KBD_FLAG_DOWN           0x4000
#define KBD_FLAG_UP             0x8000

/* These are for synchronization; not for keystrokes */
#define KBD_FLAG_SCROLL   0x0001
#define KBD_FLAG_NUMLOCK  0x0002
#define KBD_FLAG_CAPITAL  0x0004

/* See T.128 */
#define RDP_KEYPRESS 0
#define RDP_KEYRELEASE (KBD_FLAG_DOWN | KBD_FLAG_UP)

#define MOUSE_FLAG_MOVE         0x0800
#define MOUSE_FLAG_BUTTON1      0x1000
#define MOUSE_FLAG_BUTTON2      0x2000
#define MOUSE_FLAG_BUTTON3      0x4000
#define MOUSE_FLAG_BUTTON4      0x0280
#define MOUSE_FLAG_BUTTON5      0x0380
#define MOUSE_FLAG_DOWN         0x8000

/* Raster operation masks */
#define ROP2_S(rop3) (rop3 & 0xf)
#define ROP2_P(rop3) ((rop3 & 0x3) | ((rop3 & 0x30) >> 2))

#define ROP2_COPY	0xc
#define ROP2_XOR	0x6
#define ROP2_AND	0x8
#define ROP2_NXOR	0x9
#define ROP2_OR		0xe

#define MIX_TRANSPARENT	0
#define MIX_OPAQUE	1

#define TEXT2_VERTICAL		0x04
#define TEXT2_IMPLICIT_X	0x20

#define ALTERNATE	1
#define WINDING		2

/* RDP bitmap cache (version 2) constants */
#define BMPCACHE2_C0_CELLS	0x78
#define BMPCACHE2_C1_CELLS	0x78
#define BMPCACHE2_C2_CELLS	0x150
#define BMPCACHE2_NUM_PSTCELLS	0x9f6

#define PDU_FLAG_FIRST		0x01
#define PDU_FLAG_LAST		0x02

/* RDP capabilities */
#define RDP_CAPSET_GENERAL	1	/* Maps to generalCapabilitySet in T.128 page 138 */
#define RDP_CAPLEN_GENERAL	0x18
#define OS_MAJOR_TYPE_UNIX	4
#define OS_MINOR_TYPE_XSERVER	7

#define RDP_CAPSET_BITMAP	2
#define RDP_CAPLEN_BITMAP	0x1C

#define RDP_CAPSET_ORDER	3
#define RDP_CAPLEN_ORDER	0x58
#define ORDER_CAP_NEGOTIATE	2
#define ORDER_CAP_NOSUPPORT	4

#define RDP_CAPSET_BMPCACHE	4
#define RDP_CAPLEN_BMPCACHE	0x28

#define RDP_CAPSET_CONTROL	5
#define RDP_CAPLEN_CONTROL	0x0C

#define RDP_CAPSET_ACTIVATE	7
#define RDP_CAPLEN_ACTIVATE	0x0C

#define RDP_CAPSET_POINTER	8
#define RDP_CAPLEN_POINTER	0x08

#define RDP_CAPSET_SHARE	9
#define RDP_CAPLEN_SHARE	0x08

#define RDP_CAPSET_COLCACHE	10
#define RDP_CAPLEN_COLCACHE	0x08

#define RDP_CAPSET_BMPCACHE2	19
#define RDP_CAPLEN_BMPCACHE2	0x28
#define BMPCACHE2_FLAG_PERSIST	((uint32)1<<31)

#define RDP_SOURCE		"MSTSC"

/* Logon flags */
#define RDP_LOGON_AUTO		0x0008
#define RDP_LOGON_NORMAL	0x0033
#define RDP_LOGON_COMPRESSION	0x0080	/* mppc compression with 8kB histroy buffer */
#define RDP_LOGON_BLOB		0x0100
#define RDP_LOGON_COMPRESSION2	0x0200	/* rdp5 mppc compression with 64kB history buffer */
#define RDP_LOGON_LEAVE_AUDIO	0x2000

#define RDP5_DISABLE_NOTHING	0x00
#define RDP5_NO_WALLPAPER	0x01
#define RDP5_NO_FULLWINDOWDRAG	0x02
#define RDP5_NO_MENUANIMATIONS	0x04
#define RDP5_NO_THEMING		0x08
#define RDP5_NO_CURSOR_SHADOW	0x20
#define RDP5_NO_CURSORSETTINGS	0x40	/* disables cursor blinking */

/* compression types */
#define RDP_MPPC_BIG		0x01
#define RDP_MPPC_COMPRESSED	0x20
#define RDP_MPPC_RESET		0x40
#define RDP_MPPC_FLUSH		0x80
#define RDP_MPPC_DICT_SIZE      65536

#define RDP5_COMPRESSED		0x80

/* Keymap flags */
#define MapRightShiftMask   (1<<0)
#define MapLeftShiftMask    (1<<1)
#define MapShiftMask (MapRightShiftMask | MapLeftShiftMask)

#define MapRightAltMask     (1<<2)
#define MapLeftAltMask      (1<<3)
#define MapAltGrMask MapRightAltMask

#define MapRightCtrlMask    (1<<4)
#define MapLeftCtrlMask     (1<<5)
#define MapCtrlMask (MapRightCtrlMask | MapLeftCtrlMask)

#define MapRightWinMask     (1<<6)
#define MapLeftWinMask      (1<<7)
#define MapWinMask (MapRightWinMask | MapLeftWinMask)

#define MapNumLockMask      (1<<8)
#define MapCapsLockMask     (1<<9)

#define MapLocalStateMask   (1<<10)

#define MapInhibitMask      (1<<11)

#define MASK_ADD_BITS(var, mask) (var |= mask)
#define MASK_REMOVE_BITS(var, mask) (var &= ~mask)
#define MASK_HAS_BITS(var, mask) ((var & mask)>0)
#define MASK_CHANGE_BIT(var, mask, active) (var = ((var & ~mask) | (active ? mask : 0)))

/* Clipboard constants, "borrowed" from GCC system headers in
   the w32 cross compiler */

#if 0
#define CF_TEXT         1
#define CF_BITMAP       2
#define CF_METAFILEPICT 3
#define CF_SYLK         4
#define CF_DIF          5
#define CF_TIFF         6
#define CF_OEMTEXT      7
#define CF_DIB          8
#define CF_PALETTE      9
#define CF_PENDATA      10
#define CF_RIFF         11
#define CF_WAVE         12
#define CF_UNICODETEXT  13
#define CF_ENHMETAFILE  14
#define CF_HDROP        15
#define CF_LOCALE       16
#define CF_MAX          17
#define CF_OWNERDISPLAY 128
#define CF_DSPTEXT      129
#define CF_DSPBITMAP    130
#define CF_DSPMETAFILEPICT      131
#define CF_DSPENHMETAFILE       142
#define CF_PRIVATEFIRST 512
#define CF_PRIVATELAST  767
#define CF_GDIOBJFIRST  768
#define CF_GDIOBJLAST   1023

/* Sound format constants */
#define WAVE_FORMAT_PCM		1
#define WAVE_FORMAT_ADPCM	2
#define WAVE_FORMAT_ALAW	6
#define WAVE_FORMAT_MULAW	7

/* Virtual channel options */
#define CHANNEL_OPTION_INITIALIZED	0x80000000
#define CHANNEL_OPTION_ENCRYPT_RDP	0x40000000
#define CHANNEL_OPTION_COMPRESS_RDP	0x00800000
#define CHANNEL_OPTION_SHOW_PROTOCOL	0x00200000

/* NT status codes for RDPDR */
#define STATUS_SUCCESS			0x00000000
#define STATUS_NOT_IMPLEMENTED          0x00000001
#define STATUS_PENDING                  0x00000103

#define STATUS_NO_MORE_FILES            0x80000006
#define STATUS_DEVICE_PAPER_EMPTY       0x8000000e
#define STATUS_DEVICE_POWERED_OFF       0x8000000f
#define STATUS_DEVICE_OFF_LINE          0x80000010
#define STATUS_DEVICE_BUSY              0x80000011

#define STATUS_INVALID_HANDLE           0xc0000008
#define STATUS_INVALID_PARAMETER	0xc000000d
#define STATUS_NO_SUCH_FILE             0xc000000f
#define STATUS_INVALID_DEVICE_REQUEST	0xc0000010
#define STATUS_ACCESS_DENIED		0xc0000022
#define STATUS_OBJECT_NAME_COLLISION    0xc0000035
#define STATUS_DISK_FULL                0xc000007f
#define STATUS_FILE_IS_A_DIRECTORY      0xc00000ba
#define STATUS_NOT_SUPPORTED            0xc00000bb
#define STATUS_TIMEOUT                  0xc0000102
#define STATUS_NOTIFY_ENUM_DIR          0xc000010c
#define STATUS_CANCELLED                0xc0000120

/* RDPDR constants */
#define RDPDR_MAX_DEVICES               0x10
#define DEVICE_TYPE_SERIAL              0x01
#define DEVICE_TYPE_PARALLEL            0x02
#define DEVICE_TYPE_PRINTER             0x04
#define DEVICE_TYPE_DISK                0x08
#define DEVICE_TYPE_SCARD               0x20

#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY  0x00800000

/* RDP5 disconnect PDU */
#define exDiscReasonNoInfo				0x0000
#define exDiscReasonAPIInitiatedDisconnect		0x0001
#define exDiscReasonAPIInitiatedLogoff			0x0002
#define exDiscReasonServerIdleTimeout			0x0003
#define exDiscReasonServerLogonTimeout			0x0004
#define exDiscReasonReplacedByOtherConnection		0x0005
#define exDiscReasonOutOfMemory				0x0006
#define exDiscReasonServerDeniedConnection		0x0007
#define exDiscReasonServerDeniedConnectionFips		0x0008
#define exDiscReasonLicenseInternal			0x0100
#define exDiscReasonLicenseNoLicenseServer		0x0101
#define exDiscReasonLicenseNoLicense			0x0102
#define exDiscReasonLicenseErrClientMsg			0x0103
#define exDiscReasonLicenseHwidDoesntMatchLicense	0x0104
#define exDiscReasonLicenseErrClientLicense		0x0105
#define exDiscReasonLicenseCantFinishProtocol		0x0106
#define exDiscReasonLicenseClientEndedProtocol		0x0107
#define exDiscReasonLicenseErrClientEncryption		0x0108
#define exDiscReasonLicenseCantUpgradeLicense		0x0109
#define exDiscReasonLicenseNoRemoteConnections		0x010a
#endif

#if 0
/* SeamlessRDP constants */
#define SEAMLESSRDP_NOTYETMAPPED -1
#define SEAMLESSRDP_NORMAL 0
#define SEAMLESSRDP_MINIMIZED 1
#define SEAMLESSRDP_MAXIMIZED 2
#define SEAMLESSRDP_POSITION_TIMER 200000

#define SEAMLESSRDP_CREATE_MODAL	0x0001

#define SEAMLESSRDP_HELLO_RECONNECT	0x0001
#define SEAMLESSRDP_HELLO_HIDDEN	0x0002
#endif
