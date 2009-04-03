/*
 * Copyright 2004 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // fatfs.h
 //
 // Martin Fuchs, 01.02.2004
 //


 /// FAT file system file-entry
struct FATEntry : public Entry
{
	FATEntry(Entry* parent, unsigned cluster) : Entry(parent, ET_FAT), _cluster(cluster) {}

protected:
	FATEntry() : Entry(ET_FAT) {}

	virtual bool get_path(PTSTR path, size_t path_count) const;
	virtual ShellPath create_absolute_pidl() const;

	DWORD	_cluster;
};


struct FATDrive;

 /// FAT file system directory-entry
struct FATDirectory : public FATEntry, public Directory
{
	FATDirectory(FATDrive& drive, LPCTSTR root_path);
	FATDirectory(FATDrive& drive, Entry* parent, LPCTSTR path, unsigned cluster);
	~FATDirectory();

	virtual void read_directory(int scan_flags=0);
	virtual const void* get_next_path_component(const void*) const;
	virtual Entry* find_entry(const void*);

protected:
	FATDrive&	_drive;

	struct dirsecz* _secarr;
	int 	_cur_bufs;
	int 	_ents;
	struct dirent* _dir;
	struct Kette* _alloc;

	bool	read_dir();
};


#pragma pack(push, 1)

struct BootSector {
	BYTE	jmp[3];
	char	OEM[8];
	WORD	BytesPerSector; 	// dpb.bsec
	BYTE	SectorsPerCluster;	// dpb.sclus + 1
	WORD	ReservedSectors;	// dpb.ffatsec
	BYTE	NumberFATs;
	WORD	RootEntries;		// dpb.ndir
	WORD	Sectors16;
	BYTE	MediaDescr;
	WORD	SectorsPerFAT;
	WORD	SectorsPerTrack;
	WORD	Heads;
	DWORD	HiddenSectors;
	DWORD	Sectors32;
	BYTE	DriveUnit;
	WORD	ExtBootFlag;
	DWORD	SerialNr;
	char	Label[11];
	char	FileSystem[8];
	BYTE	BootCode[448];
	BYTE	BootSignature[2];
};

struct BootSector32 {
	BYTE	jmp[3];
	char	OEM[8];
	WORD	BytesPerSector;
	BYTE	SectorsPerCluster;
	WORD	ReservedSectors;
	BYTE	NumberFATs;
	WORD	reserved1;	// immer 0 für FAT32
	WORD	Sectors16;
	BYTE	MediaDescr;
	WORD	reserved2;	// immer 0 für FAT32
	WORD	SectorsPerTrack;
	WORD	Heads;
	DWORD	HiddenSectors;
	DWORD	Sectors32;
	DWORD	SectorsPerFAT32;
	DWORD	unknown1;
	DWORD	RootSectors; // correct?
	char	unknown2[6];
	char	FileSystem[8];
	BYTE	BootCode[448];
	BYTE	BootSignature[2];
};


struct filetime {
	WORD	sec2	: 5;
	WORD	min 	: 6;
	WORD	hour	: 5;
};

struct filedate {
	WORD	day 	: 5;
	WORD	month	: 4;
	WORD	year	: 7;
};

typedef struct {
	unsigned readonly	: 1;
	unsigned hidden		: 1;
	unsigned system		: 1;
	unsigned volume		: 1;
	unsigned directory 	: 1;
	unsigned archived	: 1;
	unsigned deleted	: 1;
} fattr;

typedef union {
	char	b;
	fattr  a;
} FAT_attribute;

struct DEntry_E {
	char			name[8];
	char			ext[3];
	char			attr;
	char			rsrvd[10];
	struct filetime	time;
	struct filedate	date;
	WORD			fclus;
	DWORD 			size;
};

union DEntry {
	DEntry_E E;
	BYTE B[8+3+1+10+sizeof(struct filetime)+sizeof(struct filedate)+sizeof(WORD)+sizeof(DWORD)];
};

#pragma pack(pop)


#define BufLen	512

struct Buffer {
 BYTE	dat[BufLen];
};

struct Cache {
 BYTE	dat[BufLen];
};

struct dskrwblk {
 DWORD			sec;
 WORD			anz;
 struct buffer	far *buf;
};

#define RONLY			0x01
#define HIDDEN			0x02
#define SYSTEM			0x04
#define VOLUME			0x08
#define DIRENT			0x10
#define ARCHIVE 		0x20

#define _A_DELETED		0x40
#define _A_ILLEGAL		0x80
#define IS_LNAME(a) ((a&0xFF)==0x0F)	// "& 0xFF" correct?

#define FAT_DEL_CHAR	(char)0xe5

#define AddP(p,s)  {(int&)p += s;}

struct dirent {
	union DEntry  ent[1];
};

struct dirsecz {
	DWORD  s[32];  // 32 only as placeholder
};

struct Kette {
 struct Kette*	Vorw;
 struct Kette*	Rueck;
 union DEntry*	Ent;
};


#define	MK_P(ofs)		((void*) ((size_t)(ofs)))
#define	MK_LONG(l,h)	((DWORD)WORD(l)|((DWORD)WORD(h)<<16))

#define	spoke(ofs,w)	(*((BYTE*)MK_P(ofs)) = (BYTE)(w))
#define	wpoke(ofs,w)	(*((WORD*)MK_P(ofs)) = (WORD)(w))
#define	dpoke(ofs,w)	(*((DWORD*)MK_P(ofs)) = (DWORD)(w))
#define	speek(ofs)		(*((BYTE*)MK_P(ofs)))
#define	wpeek(ofs)		(*((WORD*)MK_P(ofs)))
#define	dpeek(p)		(*((DWORD*)MK_P(p)))


 /// FAT drive root entry
struct FATDrive : public FATDirectory
{
	FATDrive(LPCTSTR path);
/*
	FATDrive(Entry* parent, LPCTSTR path)
	 :	FATEntry(parent)
	{
		_path = _tcsdup(path);
	}
*/
	~FATDrive();

	HANDLE	_hDrive;
	BootSector	_boot_sector;
	int 	_bufl;
	int 	_bufents;
	int 	_SClus;

#define	CACHE_SIZE_LOW	32
	Cache*	_FATCache;
	int		_CacheCount;
	DWORD*	_CacheSec;	// numbers of buffered cache sectors
	int*	_CacheCnt;	// counters for cache usage
	bool*	_CacheDty;	// dirty flags for cache
	int		_Caches;
	bool	_cache_empty;
	int		_read_ahead;

	bool	read_sector(DWORD sec, Buffer* buf, int len);
	DWORD	read_FAT(DWORD Clus, bool& ok);

	void	small_cache();
	void	reset_cache();
	bool	read_cache(DWORD sec, Buffer** bufptr);
	int		get_cache_buffer();
};
