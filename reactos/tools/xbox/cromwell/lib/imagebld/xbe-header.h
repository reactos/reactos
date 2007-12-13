// XBE stuff
// Not used in any exported kernel calls, but still useful.


// XBE header information
typedef struct _XBE_HEADER {
	// 000 "XBEH"
	char Magic[4];
	// 004 RSA digital signature of the entire header area
	unsigned char HeaderSignature[256];
	// 104 Base address of XBE image (must be 0x00010000?)
	void* BaseAddress;
	// 108 Size of all headers combined - other headers must be within this
	unsigned int HeaderSize;
	// 10C Size of entire image
	unsigned int ImageSize;
	// 110 Size of this header (always 0x178?)
	unsigned int XbeHeaderSize;
	// 114 Image timestamp - unknown format
	unsigned int Timestamp;
	// 118 Pointer to certificate data (must be within HeaderSize)
	struct _XBE_CERTIFICATE *Certificate;
	// 11C Number of sections
	int NumSections;
	// 120 Pointer to section headers (must be within HeaderSize)
	struct _XBE_SECTION *Sections;
	// 124 Initialization flags
	unsigned int InitFlags;
	// 128 Entry point (XOR'd; see xboxhacker.net)
	void* EntryPoint;
	// 12C Pointer to TLS directory
	struct _XBE_TLS_DIRECTORY *TlsDirectory;
	// 130 Stack commit size
	unsigned int StackCommit;
	// 134 Heap reserve size
	unsigned int HeapReserve;
	// 138 Heap commit size
	unsigned int HeapCommit;
	// 13C PE base address (?)
	void* PeBaseAddress;
	// 140 PE image size (?)
	unsigned int PeImageSize;
	// 144 PE checksum (?)
	unsigned int PeChecksum;
	// 148 PE timestamp (?)
	unsigned int PeTimestamp;
	// 14C PC path and filename to EXE file from which XBE is derived
	char* PcExePath;
	// 150 PC filename (last part of PcExePath) from which XBE is derived
	char* PcExeFilename;
	// 154 PC filename (Unicode version of PcExeFilename)
	void* PcExeFilenameUnicode;
	// 158 Pointer to kernel thunk table (XOR'd; EFB1F152 debug)
	unsigned int *KernelThunkTable;
	// 15C Non-kernel import table (debug only)
	void* DebugImportTable;
	// 160 Number of library headers
	unsigned int NumLibraries;
	// 164 Pointer to library headers
	struct _XBE_LIBRARY *Libraries;
	// 168 Pointer to kernel library header
	struct _XBE_LIBRARY *KernelLibrary;
	// 16C Pointer to XAPI library
	struct _XBE_LIBRARY *XapiLibrary;
	// 170 Pointer to logo bitmap (NULL = use default of Microsoft)
	void* LogoBitmap;
	// 174 Size of logo bitmap
	unsigned int LogoBitmapSize;
	// 178
} XBE_HEADER, *PXBE_HEADER;

// Certificate structure
typedef struct _XBE_CERTIFICATE {
	// 000 Size of certificate
	unsigned int Size;
	// 004 Certificate timestamp (unknown format)
	unsigned int Timestamp;
	// 008 Title ID
	unsigned int TitleId;
	// 00C Name of the game (Unicode)
	short TitleName[40];
	// 05C Alternate title ID's (0-terminated)
	unsigned int AlternateTitleIds[16];
	// 09C Allowed media types - 1 bit match between XBE and media = boots
	unsigned int MediaTypes;
	// 0A0 Allowed game regions - 1 bit match between this and XBOX = boots
	unsigned int GameRegion;
	// 0A4 Allowed game ratings - 1 bit match between this and XBOX = boots
	unsigned int GameRating;
	// 0A8 Disk number (?)
	unsigned int DiskNumber;
	// 0AC Version (?)
	unsigned int Version;
	// 0B0 LAN key for this game
	unsigned char LanKey[16];
	// 0C0 Signature key for this game
	unsigned char SignatureKey[16];
	// 0D0 Signature keys for the alternate title ID's
	unsigned char AlternateSignatureKeys[16][16];
	// 1D0
} XBE_CERTIFICATE, *PXBE_CERTIFICATE;

// Section headers
typedef struct _XBE_SECTION {
	// 000 Flags
	unsigned int Flags;
	// 004 Virtual address (where this section loads in RAM)
	void* VirtualAddress;
	// 008 Virtual size (size of section in RAM; after FileSize it's 00'd)
	unsigned int VirtualSize;
	// 00C File address (where in the file from which this section comes)
	unsigned int FileAddress;
	// 010 File size (size of the section in the XBE file)
	unsigned int FileSize;
	// 014 Pointer to section name
	char* SectionName;
	// 018 Section reference count - when >= 1, section is loaded
	int SectionReferenceCount;
	// 01C Pointer to head shared page reference count
	short *HeadReferenceCount;
	// 020 Pointer to tail shared page reference count
	short *TailReferenceCount;
	// 024 SHA hash.  Hash int containing FileSize, then hash section.
	int ShaHash[5];
	// 038
} XBE_SECTION, *PXBE_SECTION;

// TLS directory information
typedef struct _XBE_TLS {
        // 000 raw data start address
        int RawStart;
        // 004 raw data end address
        int RawEnd;
        // 008 TLS index address
        int TlsIndex;
        // 00C TLS callbacks address
        int TlsCallbacks;
        // 010 size of zero fill
        int SizeZeroFill;
        // 014 Characteristics
        unsigned char Characteristics[8];
        // 01C
} XBE_TLS, *PXBE_TLS;

// Library version data
typedef struct _XBE_LIBRARY {
    // 000 Library Name
    unsigned char Name[8];
    // 008 Major Version
    short MajorVersion;
    // 00A Middle Version
    short MiddleVersion;
    // 00C Minor Version
    short MinorVersion;
    // 00E Flags
    short Flags;
    // 010
} XBE_LIBRARY, *PXBELIBRARY;

// Initialization flags
#define XBE_INIT_MOUNT_UTILITY          0x00000001
#define XBE_INIT_FORMAT_UTILITY         0x00000002
#define XBE_INIT_64M_RAM_ONLY           0x00000004
#define XBE_INIT_DONT_SETUP_HDD         0x00000008

// Region codes
#define XBE_REGION_US_CANADA            0x00000001
#define XBE_REGION_JAPAN                0x00000002
#define XBE_REGION_ELSEWHERE            0x00000004
#define XBE_REGION_DEBUG                0x80000000

// Media types
#define XBE_MEDIA_HDD                   0x00000001
#define XBE_MEDIA_XBOX_DVD              0x00000002
#define XBE_MEDIA_ANY_CD_OR_DVD         0x00000004
#define XBE_MEDIA_CD                    0x00000008
#define XBE_MEDIA_1LAYER_DVDROM         0x00000010
#define XBE_MEDIA_2LAYER_DVDROM         0x00000020
#define XBE_MEDIA_1LAYER_DVDR           0x00000040
#define XBE_MEDIA_2LAYER_DVDR           0x00000080
#define XBE_MEDIA_USB                   0x00000100
#define XBE_MEDIA_ALLOW_UNLOCKED_HDD    0x40000000

// Section flags
#define XBE_SEC_WRITABLE                0x00000001
#define XBE_SEC_PRELOAD                 0x00000002
#define XBE_SEC_EXECUTABLE              0x00000004
#define XBE_SEC_INSERTED_FILE           0x00000008
#define XBE_SEC_RO_HEAD_PAGE            0x00000010
#define XBE_SEC_RO_TAIL_PAGE            0x00000020

