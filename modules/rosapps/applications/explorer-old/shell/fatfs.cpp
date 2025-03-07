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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // fatfs.cpp
 //
 // Martin Fuchs, 01.02.2004
 //


#include <precomp.h>

#include "fatfs.h"

#ifdef _DEBUG

static union DEntry* link_dir_entries(struct dirent* dir, struct Kette* K, int cnt)
{
	union DEntry* Ent = (union DEntry*) dir;
	struct Kette* L = NULL;

	for(; cnt; cnt--) {
		K->Rueck = L;
		(L=K)->Ent = Ent;
		AddP(K, sizeof(struct Kette));
		L->Vorw = K;
		AddP(Ent, sizeof(union DEntry));
	}

	L->Vorw = NULL;

	return Ent;
}

void FATDirectory::read_directory(int scan_flags)
{
	CONTEXT("FATDirectory::read_directory()");

	read_dir();

	union DEntry* p = (union DEntry*) _dir;
	int i = 0;

	do {
/*		if (!IS_LNAME(p->E.attr) && p->E.name[0]!=FAT_DEL_CHAR)
			gesBytes += p->E.size;
*/

		AddP(p, sizeof(union DEntry));
	} while(++i<_ents && p->E.name[0]);

	_alloc = (struct Kette*) malloc((size_t)((_ents=i)+8)*sizeof(struct Kette));
	if (!_alloc)
		return;

	link_dir_entries(_dir, _alloc, i);

	Entry* first_entry = NULL;
	int level = _level + 1;

	Entry* last = NULL;

	WIN32_FIND_DATA w32fd;
	FAT_attribute attr;
	String long_name;

	TCHAR buffer[MAX_PATH];

	_tcscpy(buffer, (LPCTSTR)_path);
	LPTSTR pname = buffer + _tcslen(buffer);
	int plen = COUNTOF(buffer) - _tcslen(buffer);

	*pname++ = '\\';
	--plen;

	for(Kette*p=_alloc; p; p=p->Vorw) {
		memset(&w32fd, 0, sizeof(WIN32_FIND_DATA));

		DEntry_E& e = p->Ent->E;

		 // get file/directory attributes
		attr.b = e.attr;

		if (attr.b & (_A_DELETED | _A_ILLEGAL))
			attr.b |= _A_ILLEGAL;

		const char* s = e.name;
		LPTSTR d = w32fd.cFileName;

		if (!IS_LNAME(attr.b) || e.name[0]==FAT_DEL_CHAR) {
			if (e.name[0] == FAT_DEL_CHAR)
				w32fd.dwFileAttributes |= ATTRIBUTE_ERASED;
			else if (IS_LNAME(attr.b))
				w32fd.dwFileAttributes |= ATTRIBUTE_LONGNAME;
			else if (attr.a.directory)
				w32fd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
			else if (attr.a.volume)
				w32fd.dwFileAttributes |= ATTRIBUTE_VOLNAME;	//@@ -> in Volume-Name der Root kopieren

			 // get file name
			*d++ = *s==FAT_DEL_CHAR? '?': *s;
			++s;

			for(i=0; i<7; ++i)
				*d++ = *s++;

			while(d>w32fd.cFileName && d[-1]==' ')
				--d;

			*d++ = '.';

			for(; i<10; ++i)
				*d++ = *s++;

			while(d>w32fd.cFileName && d[-1]==' ')
				--d;

			if (d>w32fd.cFileName && d[-1]=='.')
				--d;

			*d = '\0';
		} else {
			s = (const char*)p->Ent->B;	// no change of the pointer, just to avoid overung warnings in code checkers

			 // read long file name
			char lname[] = {s[1], s[3], s[5], s[7], s[9], s[14], s[16], s[18], s[20], s[22], s[24], s[28], s[30]};

			long_name = String(lname, 13) + long_name;
		}

		if (!IS_LNAME(attr.b) && !attr.a.volume) {
			 // get file size
			w32fd.nFileSizeLow = e.size;

			 // convert date/time attribute into FILETIME
			const filedate& date = e.date;
			const filetime& time = e.time;
			SYSTEMTIME stime;
			FILETIME ftime;

			stime.wYear = date.year + 1980;
			stime.wMonth = date.month;
			stime.wDayOfWeek = (WORD)-1;
			stime.wDay = date.day;
			stime.wHour = time.hour;
			stime.wMinute = time.min;
			stime.wSecond = time.sec2 * 2;
			stime.wMilliseconds = 0;

			if (SystemTimeToFileTime(&stime, &ftime))
				LocalFileTimeToFileTime(&ftime, &w32fd.ftLastWriteTime);

			if (!(w32fd.dwFileAttributes & ATTRIBUTE_ERASED)) { //@@
				Entry* entry;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					_tcscpy_s(pname, plen, w32fd.cFileName);
					entry = new FATDirectory(_drive, this, buffer, e.fclus);
				} else
					entry = new FATEntry(this, e.fclus);

				memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

				if (!long_name.empty()) {
					entry->_content = _tcsdup(long_name);
					long_name.erase();
				}

				if (!first_entry)
					first_entry = entry;

				if (last)
					last->_next = entry;

				entry->_level = level;

				last = entry;
			}
		}
	}

	if (last)
		last->_next = NULL;

	_down = first_entry;
	_scanned = true;
}


const void* FATDirectory::get_next_path_component(const void* p) const
{
	LPCTSTR s = (LPCTSTR) p;

	while(*s && *s!=TEXT('\\') && *s!=TEXT('/'))
		++s;

	while(*s==TEXT('\\') || *s==TEXT('/'))
		++s;

	if (!*s)
		return NULL;

	return s;
}


Entry* FATDirectory::find_entry(const void* p)
{
	LPCTSTR name = (LPCTSTR)p;

	for(Entry*entry=_down; entry; entry=entry->_next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->_data.cFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));

		p = name;
		q = entry->_data.cAlternateFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));
	}

	return NULL;
}


 // get full path of specified directory entry
bool FATEntry::get_path(PTSTR path, size_t path_count) const
{
	return get_path_base ( path, path_count, ET_FAT );
}

ShellPath FATEntry::create_absolute_pidl() const
{
	CONTEXT("WinEntry::create_absolute_pidl()");

	return (LPCITEMIDLIST)NULL;
/* prepend root path if the drive is currently actually mounted in the file system -> return working PIDL
	TCHAR path[MAX_PATH];

	if (get_path(path, COUNTOF(path)))
		return ShellPath(path);

	return ShellPath();
*/
}


FATDirectory::FATDirectory(FATDrive& drive, LPCTSTR root_path)
 :	FATEntry(),
	_drive(drive)
{
	_path = _tcsdup(root_path);

	_secarr 	= NULL;
	_cur_bufs	= 0;
	_ents		= 0;
	_dir		= NULL;
	_cluster	= 0;
}

FATDirectory::FATDirectory(FATDrive& drive, Entry* parent, LPCTSTR path, unsigned cluster)
 :	FATEntry(parent, cluster),
	_drive(drive)
{
	_path = _tcsdup(path);

	_secarr 	= NULL;
	_cur_bufs	= 0;
	_ents		= 0;
	_dir		= NULL;
}

FATDirectory::~FATDirectory()
{
	free(_path);
	_path = NULL;
}

bool FATDirectory::read_dir()
{
	int i;

	if (_cluster == 0) {
		if (!_drive._boot_sector.SectorsPerFAT) {	// FAT32? [boot_sector32->reserved0==0]
			BootSector32* boot_sector32 = (BootSector32*) &_drive._boot_sector;
			DWORD sect = _drive._boot_sector.ReservedSectors + _drive._boot_sector.NumberFATs*boot_sector32->SectorsPerFAT32;  // lese Root-Directory ein
			int RootEntries = boot_sector32->RootSectors * 32;	//@@

			_secarr = (struct dirsecz*)malloc(sizeof(DWORD) * (_cur_bufs = (int)((_ents=RootEntries)/_drive._bufents)));

			for(i=0; i<_cur_bufs; i++)
				_secarr->s[i] = sect+i;

			_dir = (struct dirent*)malloc((size_t)(_ents+16)*sizeof(union DEntry));
			if (!_dir)
				return false;

			if (!(_drive.read_sector(*_secarr->s,(Buffer*)_dir,_cur_bufs)))
				return false;
		} else {
			DWORD sect = _drive._boot_sector.ReservedSectors + _drive._boot_sector.NumberFATs*_drive._boot_sector.SectorsPerFAT;  // read in root directory

			_secarr = (struct dirsecz*)malloc(sizeof(DWORD) * (_cur_bufs = (int)((_ents=_drive._boot_sector.RootEntries)/_drive._bufents)));

			for(i=0; i<_cur_bufs; i++)
				_secarr->s[i] = sect+i;

			_dir = (struct dirent*)malloc((size_t)(_ents+16)*sizeof(union DEntry));
			if (!_dir)
				return false;

			if (!_drive.read_sector(*_secarr->s,(Buffer*)_dir,_cur_bufs))
				return false;
		}
	} else {
		Buffer* buf;
		bool ok;

		DWORD h = _cluster;

		_cur_bufs = 0;

		do {
			h = _drive.read_FAT(h, ok);

			if (!ok)
				return false;

			_cur_bufs++;
		} while (h<0x0ffffff0 && h);

		_secarr = (struct dirsecz*) malloc(sizeof(DWORD) * _cur_bufs);

		if (!_secarr)
			return false;

		_ents = _drive._bufents * (size_t)_cur_bufs * _drive._SClus;

		if ((buf=(Buffer*)(_dir=(struct dirent*)malloc((size_t) (_ents+16)*sizeof(union DEntry)))) == NULL)
			return false;

		h = _cluster;

		DWORD fdatsec;

		if (!_drive._boot_sector.SectorsPerFAT) {	// FAT32 ?
			BootSector32* boot_sector32 = (BootSector32*) &_drive._boot_sector;
			//int RootEntries = boot_sector32->RootSectors * 32;	//@@
			//fdatsec = _drive._boot_sector.ReservedSectors + _drive._boot_sector.NumberFATs*boot_sector32->SectorsPerFAT32 + RootEntries*sizeof(DEntry)/_drive._boot_sector.BytesPerSector;	// dpb.fdirsec
			fdatsec = _drive._boot_sector.ReservedSectors +
						_drive._boot_sector.NumberFATs*boot_sector32->SectorsPerFAT32 + boot_sector32->RootSectors;
		} else
			fdatsec = _drive._boot_sector.ReservedSectors +
						_drive._boot_sector.NumberFATs*_drive._boot_sector.SectorsPerFAT +
						_drive._boot_sector.RootEntries*sizeof(DEntry)/_drive._boot_sector.BytesPerSector;	// dpb.fdirsec

			for(i=0; i<_cur_bufs; i++) {
				_secarr->s[i] = fdatsec + (DWORD)_drive._SClus*(h-2);

				h = _drive.read_FAT(h, ok);

				if (!ok)
					return false;
			}

			for(i=0; i<_cur_bufs; i++) {
				if ((ok = (_drive.read_sector(_secarr->s[i], buf, _drive._SClus))) == true)
					AddP(buf, _drive._bufl*_drive._SClus)
				else {
					//@@FPara = _secarr->s[i];
					return false;
				}
			}

		buf->dat[0] = 0;	 // Endekennzeichen für Rekurs setzen
	}

	return true;
}


#ifdef _MSC_VER
#pragma warning(disable: 4355)
#endif

FATDrive::FATDrive(LPCTSTR path)
 :	FATDirectory(*this, TEXT("\\"))
{
	_bufl = 0;
	_bufents = 0;
	_SClus = 0;
	_FATCache = NULL;
	_CacheCount = 0;
	_CacheSec = NULL;
	_CacheCnt = NULL;
	_CacheDty = NULL;
	_Caches = 0;

	_hDrive = CreateFile(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

	if (_hDrive != INVALID_HANDLE_VALUE) {
		_boot_sector.BytesPerSector = 512;

		if (read_sector(0, (Buffer*)&_boot_sector, 1)) {
			_bufl = _boot_sector.BytesPerSector;
			_SClus = _boot_sector.SectorsPerCluster;
			_bufents = _bufl / sizeof(union DEntry);
		}

		small_cache();
	}
}

FATDrive::~FATDrive()
{
	if (_hDrive != INVALID_HANDLE_VALUE)
		CloseHandle(_hDrive);

	free(_path);
	_path = NULL;
}

void FATDrive::small_cache()
{
	if (_FATCache)
		free(_FATCache);

	if (_CacheSec) {
		free(_CacheSec), _CacheSec = NULL;
		free(_CacheCnt);
		free(_CacheDty);
	}

	_Caches = CACHE_SIZE_LOW;
	_FATCache = (struct Cache *) malloc((_Caches+1) * _drive._bufl);

	reset_cache();
}

void FATDrive::reset_cache()	// mark cache as empty
{
	int i;

	if (!_CacheSec) {
		_CacheSec = (DWORD*) malloc(_Caches * sizeof(DWORD));
		_CacheCnt = (int*) malloc(_Caches * sizeof(int));
		_CacheDty = (bool*) malloc(_Caches * sizeof(bool));
	} else {
		_CacheSec = (DWORD*) realloc(_CacheSec, _Caches * sizeof(DWORD));
		_CacheCnt = (int*) realloc(_CacheCnt, _Caches * sizeof(int));
		_CacheDty = (bool*) realloc(_CacheDty, _Caches * sizeof(bool));
	}

	for(i=0; i<_Caches; i++)
		_CacheSec[i] = 0;

	_read_ahead = (_Caches+1) / 2;
}

bool FATDrive::read_sector(DWORD sec, Buffer* buf, int len)
{
	sec += 63;	//@@ jump to first partition

	if (SetFilePointer(_hDrive, sec*_drive._boot_sector.BytesPerSector, 0, 0) == INVALID_SET_FILE_POINTER)
		return false;

	DWORD read;

	if (!ReadFile(_hDrive, buf, len*_drive._boot_sector.BytesPerSector, &read, 0))
		return false;

	return true;
}

DWORD FATDrive::read_FAT(DWORD cluster, bool& ok)	//@@ use exception handling
{
	DWORD nClus;
	Buffer* FATBuf;

	DWORD nclus = (_boot_sector.Sectors32? _boot_sector.Sectors32: _boot_sector.Sectors16) / _boot_sector.SectorsPerCluster;	///@todo cache result

	if (cluster > nclus) {
		ok = false;
		return (DWORD)-1;
	}

	if (nclus >= 65536) {		// FAT32
		DWORD FATsec = cluster / (_boot_sector.BytesPerSector/4);
		DWORD z = (cluster - _boot_sector.BytesPerSector/4 * FATsec)*4;
		FATsec += _boot_sector.ReservedSectors;
		if (!read_cache(FATsec, &FATBuf))
			ok = false;
		nClus = dpeek(&FATBuf->dat[z]);
	} else if (nclus >= 4096) {	// 16 Bit-FAT
		DWORD FATsec = cluster / (_boot_sector.BytesPerSector/2);
		DWORD z = (cluster - _boot_sector.BytesPerSector/2 * FATsec)*2;
		FATsec += _boot_sector.ReservedSectors;
		if (!read_cache(FATsec, &FATBuf))
			ok = false;
		nClus = wpeek(&FATBuf->dat[z]);

		if (nClus >= 0xfff0)
			nClus |= 0x0fff0000;
	} else {						// 12 Bit-FAT
		DWORD FATsec = cluster*3 / (_boot_sector.BytesPerSector*2);
		DWORD z = (cluster*3 - _boot_sector.BytesPerSector*2*FATsec)/2;
		FATsec += _boot_sector.ReservedSectors;
		if (!read_cache(FATsec,&FATBuf))
			ok = false;
		BYTE a = FATBuf->dat[z++];

		if (z >= _boot_sector.BytesPerSector)
			if (!read_cache(FATsec+1,&FATBuf))
				ok = false;
		z = 0;

		BYTE b = FATBuf->dat[z];

		if (cluster & 1)
			nClus = (a>>4) | (b<<4);
		else
			nClus = a | ((b & 0xf)<<8);

		if (nClus >= 0xff0)
			nClus |= 0x0ffff000;
	}

	return nClus;
}

bool FATDrive::read_cache(DWORD sec, Buffer** bufptr)
{
 int i, C, anz;

 if (_boot_sector.BytesPerSector != BufLen)  // no standard sector size?
  return read_sector(sec, *bufptr=(Buffer*)&_FATCache[0], 1);

 _CacheCount++;

 for(i=0; _CacheSec[i]!=sec && i<_Caches; )
  ++i;

 if (i < _Caches)
  {
   *bufptr = (Buffer*) &_FATCache[i];	 // FAT-Sektor schon gepuffert
   _CacheCnt[i]++;
   return true;
  }

 i = get_cache_buffer();

 if (_cache_empty)		// von get_cache_buffer() gesetzt
  {
   C = _CacheCount-1;
   anz = _boot_sector.SectorsPerFAT*_boot_sector.NumberFATs - sec;

   if (anz > _read_ahead)
	anz = _read_ahead;

   for(i=0; i<anz; i++) {
	_CacheSec[i] = sec++;
	_CacheCnt[i] = C;
	_CacheDty[i] = 0;
   }

   _CacheCnt[0] = _CacheCount;

   return read_sector(_CacheSec[0], *bufptr=(Buffer*) &_FATCache[0], anz);
  }
 else
  {
   _CacheDty[i] = 0;
   _CacheCnt[i] = _CacheCount;

   return read_sector(_CacheSec[i]=sec, *bufptr=(Buffer*) &_FATCache[i], 1);
  }
}

int FATDrive::get_cache_buffer()	// search for free cache buffer
{
 int i, j, minCnt;

 for(i=0; i<_Caches; i++)
  if (_CacheSec[i])
   break;

 _cache_empty = i==_Caches? true: false;

 for(i=0; _CacheSec[i] && i<_Caches; )
  ++i;

 if (i < _Caches)
  j = i;
 else
  {
   minCnt = 0;			// search for least used buffer

   for(j=i=0; i<_Caches; i++)
	if (minCnt < _CacheCnt[i]) {
	 minCnt = _CacheCnt[i];
	 j = i;
	}

/**@todo enable write back
   if (CacheDty[j]) 	// Dirty-Flag gesetzt?
	if (writesec(_CacheSec[j], (Buffer*) &_FATCache[j], 1))
	 FPara = _CacheSec[j], Frag(SecWriteErr);
*/
  }

 return j;
}

#endif // _DEBUG
