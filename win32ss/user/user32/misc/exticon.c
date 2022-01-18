/*
 *	icon extracting
 *
 * taken and slightly changed from shell
 * this should replace the icon extraction code in shell32 and shell16 once
 * it needs a serious test for compliance with the native API
 *
 * Copyright 2000 Juergen Schmied
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <user32.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* Start of Hack section */

WINE_DEFAULT_DEBUG_CHANNEL(icon);

#include <pshpack1.h>

typedef struct
{
    BYTE        bWidth;          /* Width, in pixels, of the image	*/
    BYTE        bHeight;         /* Height, in pixels, of the image	*/
    BYTE        bColorCount;     /* Number of colors in image (0 if >=8bpp) */
    BYTE        bReserved;       /* Reserved ( must be 0)		*/
    WORD        wPlanes;         /* Color Planes			*/
    WORD        wBitCount;       /* Bits per pixel			*/
    DWORD       dwBytesInRes;    /* How many bytes in this resource?	*/
    DWORD       dwImageOffset;   /* Where in the file is this image?	*/
} icoICONDIRENTRY, *LPicoICONDIRENTRY;

typedef struct
{
    WORD            idReserved;   /* Reserved (must be 0) */
    WORD            idType;       /* Resource Type (RES_ICON or RES_CURSOR) */
    WORD            idCount;      /* How many images */
    icoICONDIRENTRY idEntries[1]; /* An entry for each image (idCount of 'em) */
} icoICONDIR, *LPicoICONDIR;

typedef struct
{
    WORD offset;
    WORD length;
    WORD flags;
    WORD id;
    WORD handle;
    WORD usage;
} NE_NAMEINFO;

typedef struct
{
    WORD  type_id;
    WORD  count;
    DWORD resloader;
} NE_TYPEINFO;

//  From: James Houghtaling
//  https://www.moon-soft.com/program/FORMAT/windows/ani.htm
typedef struct taganiheader
{
    DWORD cbsizeof;  // num bytes in aniheader (36 bytes)
    DWORD cframes;   // number of unique icons in this cursor
    DWORD csteps;    // number of blits before the animation cycles
    DWORD cx;        // reserved, must be zero.
    DWORD cy;        // reserved, must be zero.
    DWORD cbitcount; // reserved, must be zero.
    DWORD cplanes;   // reserved, must be zero.
    DWORD jifrate;   // default jiffies (1/60th sec) if rate chunk not present.
    DWORD flags;     // animation flag
} aniheader;

#define NE_RSCTYPE_ICON        0x8003
#define NE_RSCTYPE_GROUP_ICON  0x800e

#include <poppack.h>

#if 0
static void dumpIcoDirEnty ( LPicoICONDIRENTRY entry )
{
	TRACE("width = 0x%08x height = 0x%08x\n", entry->bWidth, entry->bHeight);
	TRACE("colors = 0x%08x planes = 0x%08x\n", entry->bColorCount, entry->wPlanes);
	TRACE("bitcount = 0x%08x bytesinres = 0x%08lx offset = 0x%08lx\n",
	entry->wBitCount, entry->dwBytesInRes, entry->dwImageOffset);
}
static void dumpIcoDir ( LPicoICONDIR entry )
{
	TRACE("type = 0x%08x count = 0x%08x\n", entry->idType, entry->idCount);
}
#endif

#ifndef WINE
DWORD get_best_icon_file_offset(const LPBYTE dir,
                                DWORD dwFileSize,
                                int cxDesired,
                                int cyDesired,
                                BOOL bIcon,
                                DWORD fuLoad,
                                POINT *ptHotSpot);
#endif

/**********************************************************************
 *  find_entry_by_id
 *
 * Find an entry by id in a resource directory
 * Copied from loader/pe_resource.c (FIXME: should use exported resource functions)
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                         WORD id, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int min, max, pos;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    min = dir->NumberOfNamedEntries;
    max = min + dir->NumberOfIdEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (entry[pos].Id == id)
            return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry[pos].OffsetToDirectory);
        if (entry[pos].Id > id) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}

/**********************************************************************
 *  find_entry_default
 *
 * Find a default entry in a resource directory
 * Copied from loader/pe_resource.c (FIXME: should use exported resource functions)
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_default( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                           const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    return (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + entry->OffsetToDirectory);
}

/*************************************************************************
 *				USER32_GetResourceTable
 */
static DWORD USER32_GetResourceTable(LPBYTE peimage,DWORD pesize,LPBYTE *retptr)
{
	IMAGE_DOS_HEADER	* mz_header;

	TRACE("%p %p\n", peimage, retptr);

	*retptr = NULL;

	mz_header = (IMAGE_DOS_HEADER*) peimage;

	if (mz_header->e_magic != IMAGE_DOS_SIGNATURE)
	{
	  if (mz_header->e_cblp == 1 || mz_header->e_cblp == 2)	/* .ICO or .CUR file ? */
	  {
	    *retptr = (LPBYTE)-1;	/* ICONHEADER.idType, must be 1 */
	    return mz_header->e_cblp;
	  }
	  else
	    return 0; /* failed */
	}
	if (mz_header->e_lfanew >= pesize) {
	    return 0; /* failed, happens with PKZIP DOS Exes for instance. */
	}
	if (*((DWORD*)(peimage + mz_header->e_lfanew)) == IMAGE_NT_SIGNATURE )
	  return IMAGE_NT_SIGNATURE;

	if (*((WORD*)(peimage + mz_header->e_lfanew)) == IMAGE_OS2_SIGNATURE )
	{
	  IMAGE_OS2_HEADER	* ne_header;

	  ne_header = (IMAGE_OS2_HEADER*)(peimage + mz_header->e_lfanew);

	  if (ne_header->ne_magic != IMAGE_OS2_SIGNATURE)
	    return 0;

	  if( (ne_header->ne_restab - ne_header->ne_rsrctab) <= sizeof(NE_TYPEINFO) )
	    *retptr = (LPBYTE)-1;
	  else
	    *retptr = peimage + mz_header->e_lfanew + ne_header->ne_rsrctab;

	  return IMAGE_OS2_SIGNATURE;
	}
	return 0; /* failed */
}
/*************************************************************************
 *			USER32_LoadResource
 */
static BYTE * USER32_LoadResource( LPBYTE peimage, NE_NAMEINFO* pNInfo, WORD sizeShift, ULONG *uSize)
{
	TRACE("%p %p 0x%08x\n", peimage, pNInfo, sizeShift);

	*uSize = (DWORD)pNInfo->length << sizeShift;
	return peimage + ((DWORD)pNInfo->offset << sizeShift);
}

/*************************************************************************
 *                      ICO_LoadIcon
 */
static BYTE * ICO_LoadIcon( LPBYTE peimage, LPicoICONDIRENTRY lpiIDE, ULONG *uSize)
{
	TRACE("%p %p\n", peimage, lpiIDE);

	*uSize = lpiIDE->dwBytesInRes;
	return peimage + lpiIDE->dwImageOffset;
}

/*************************************************************************
 *                      ICO_GetIconDirectory
 *
 * Reads .ico file and build phony ICONDIR struct
 */
static BYTE * ICO_GetIconDirectory( LPBYTE peimage, LPicoICONDIR* lplpiID, ULONG *uSize )
{
	CURSORICONDIR	* lpcid;	/* icon resource in resource-dir format */
	CURSORICONDIR	* lpID;		/* icon resource in resource format */
	int		i;

	TRACE("%p %p\n", peimage, lplpiID);

	lpcid = (CURSORICONDIR*)peimage;

	if( lpcid->idReserved || (lpcid->idType != 1) || (!lpcid->idCount) )
	  return 0;

	/* allocate the phony ICONDIR structure */
        *uSize = FIELD_OFFSET(CURSORICONDIR, idEntries[lpcid->idCount]);
	if( (lpID = HeapAlloc(GetProcessHeap(),0, *uSize) ))
	{
	  /* copy the header */
	  lpID->idReserved = lpcid->idReserved;
	  lpID->idType = lpcid->idType;
	  lpID->idCount = lpcid->idCount;

	  /* copy the entries */
	  for( i=0; i < lpcid->idCount; i++ )
	  {
            memcpy(&lpID->idEntries[i], &lpcid->idEntries[i], sizeof(CURSORICONDIRENTRY) - 2);
	    lpID->idEntries[i].wResId = i;
	  }

	  *lplpiID = (LPicoICONDIR)peimage;
	  return (BYTE *)lpID;
	}
	return 0;
}

/*************************************************************************
 *	ICO_ExtractIconExW		[internal]
 *
 * NOTES
 *  nIcons = 0: returns number of Icons in file
 *
 * returns
 *  invalid file: -1
 *  failure:0;
 *  success: number of icons in file (nIcons = 0) or nr of icons retrieved
 */
static UINT ICO_ExtractIconExW(
	LPCWSTR lpszExeFileName,
	HICON * RetPtr,
	INT nIconIndex,
	UINT nIcons,
	UINT cxDesired,
	UINT cyDesired,
	UINT *pIconId,
	UINT flags)
{
	UINT		ret = 0;
	UINT		cx1, cx2, cy1, cy2;
	LPBYTE		pData;
	DWORD		sig;
	HANDLE		hFile;
	UINT16		iconDirCount = 0,iconCount = 0;
	LPBYTE		peimage;
	HANDLE		fmapping;
	DWORD		fsizeh,fsizel;
#ifdef __REACTOS__
    WCHAR		szExpandedExePath[MAX_PATH];
#endif
        WCHAR		szExePath[MAX_PATH];
        DWORD		dwSearchReturn;

	TRACE("%s, %d, %d %p 0x%08x\n", debugstr_w(lpszExeFileName), nIconIndex, nIcons, pIconId, flags);

#ifdef __REACTOS__
    if (RetPtr)
        *RetPtr = NULL;

    if (ExpandEnvironmentStringsW(lpszExeFileName, szExpandedExePath, ARRAY_SIZE(szExpandedExePath)))
        lpszExeFileName = szExpandedExePath;
#endif

        dwSearchReturn = SearchPathW(NULL, lpszExeFileName, NULL, ARRAY_SIZE(szExePath), szExePath, NULL);
        if ((dwSearchReturn == 0) || (dwSearchReturn > ARRAY_SIZE(szExePath)))
        {
            WARN("File %s not found or path too long\n", debugstr_w(lpszExeFileName));
            return -1;
        }

	hFile = CreateFileW(szExePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) return ret;
	fsizel = GetFileSize(hFile,&fsizeh);

	/* Map the file */
	fmapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
	CloseHandle(hFile);
	if (!fmapping)
	{
          WARN("CreateFileMapping error %ld\n", GetLastError() );
	  return 0xFFFFFFFF;
	}

	if (!(peimage = MapViewOfFile(fmapping, FILE_MAP_READ, 0, 0, 0)))
	{
          WARN("MapViewOfFile error %ld\n", GetLastError() );
	  CloseHandle(fmapping);
	  return 0xFFFFFFFF;
	}
	CloseHandle(fmapping);

#ifdef __REACTOS__
    /* Check if we have a min size of 2 headers RIFF & 'icon'
     * at 8 chars each plus an anih header of 36 byptes.
     * Also, is this resource an animjated icon/cursor (RIFF) */
    if ((fsizel >= 52) && !memcmp(peimage, "RIFF", 4))
    {
        UINT anihOffset;
        UINT anihMax;
        /* Get size of the animation data */
        ULONG uSize = MAKEWORD(peimage[4], peimage[5]);

        /* Check if uSize is reasonable with respect to fsizel */
        if ((uSize < strlen("anih")) || (uSize > fsizel))
            goto end;

        /* Look though the reported size less search string length */
        anihMax = uSize - strlen("anih"); 
        /* Search for 'anih' indicating animation header */
        for (anihOffset = 0; anihOffset < anihMax; anihOffset++)
        {
            if (memcmp(&peimage[anihOffset], "anih", 4) == 0)
                break;
        }

        if (anihOffset + sizeof(aniheader) > fsizel)
            goto end;

        /* Get count of images for return value */
        ret = MAKEWORD(peimage[anihOffset + 12], peimage[anihOffset + 13]);

        TRACE("RIFF File with '%u' images at Offset '%u'.\n", ret, anihOffset);

        cx1 = LOWORD(cxDesired);
        cy1 = LOWORD(cyDesired);

        if (RetPtr)
        {
            RetPtr[0] = CreateIconFromResourceEx(peimage, uSize, TRUE, 0x00030000, cx1, cy1, flags);
        }
        goto end;
    }
#endif

	cx1 = LOWORD(cxDesired);
	cx2 = HIWORD(cxDesired);
	cy1 = LOWORD(cyDesired);
	cy2 = HIWORD(cyDesired);

	if (pIconId) /* Invalidate first icon identifier */
		*pIconId = 0xFFFFFFFF;

	if (!pIconId) /* if no icon identifier array present use the icon handle array as intermediate storage */
	  pIconId = (UINT*)RetPtr;

	sig = USER32_GetResourceTable(peimage, fsizel, &pData);

/* NE exe/dll */
	if (sig==IMAGE_OS2_SIGNATURE)
	{
	  BYTE		*pCIDir = 0;
	  NE_TYPEINFO	*pTInfo = (NE_TYPEINFO*)(pData + 2);
	  NE_NAMEINFO	*pIconStorage = NULL;
	  NE_NAMEINFO	*pIconDir = NULL;
	  LPicoICONDIR	lpiID = NULL;
	  ULONG		uSize = 0;

          TRACE("-- OS2/icon Signature (0x%08x)\n", sig);

	  if (pData == (BYTE*)-1)
	  {
	    pCIDir = ICO_GetIconDirectory(peimage, &lpiID, &uSize);	/* check for .ICO file */
	    if (pCIDir)
	    {
	      iconDirCount = 1; iconCount = lpiID->idCount;
              TRACE("-- icon found %p 0x%08x 0x%08x 0x%08x\n", pCIDir, uSize, iconDirCount, iconCount);
	    }
	  }
	  else while (pTInfo->type_id && !(pIconStorage && pIconDir))
	  {
	    if (pTInfo->type_id == NE_RSCTYPE_GROUP_ICON)	/* find icon directory and icon repository */
	    {
	      iconDirCount = pTInfo->count;
	      pIconDir = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE("\tfound directory - %i icon families\n", iconDirCount);
	    }
	    if (pTInfo->type_id == NE_RSCTYPE_ICON)
	    {
	      iconCount = pTInfo->count;
	      pIconStorage = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE("\ttotal icons - %i\n", iconCount);
	    }
	    pTInfo = (NE_TYPEINFO *)((char*)(pTInfo+1)+pTInfo->count*sizeof(NE_NAMEINFO));
	  }

	  if ((pIconStorage && pIconDir) || lpiID)	  /* load resources and create icons */
	  {
	    if (nIcons == 0)
	    {
	      ret = iconDirCount;
              if (lpiID)	/* *.ico file, deallocate heap pointer*/
	        HeapFree(GetProcessHeap(), 0, pCIDir);
	    }
	    else if (nIconIndex < iconDirCount)
	    {
	      UINT16   i, icon;
	      if (nIcons > iconDirCount - nIconIndex)
	        nIcons = iconDirCount - nIconIndex;

	      for (i = 0; i < nIcons; i++)
	      {
	        /* .ICO files have only one icon directory */
	        if (lpiID == NULL)	/* not *.ico */
	          pCIDir = USER32_LoadResource(peimage, pIconDir + i + nIconIndex, *(WORD*)pData, &uSize);
	        pIconId[i] = LookupIconIdFromDirectoryEx(pCIDir, TRUE, cx1, cy1, flags);
                if (cx2 && cy2) pIconId[++i] = LookupIconIdFromDirectoryEx(pCIDir, TRUE,  cx2, cy2, flags);
	      }
              if (lpiID)	/* *.ico file, deallocate heap pointer*/
	        HeapFree(GetProcessHeap(), 0, pCIDir);

	      for (icon = 0; icon < nIcons; icon++)
	      {
	        pCIDir = NULL;
	        if (lpiID)
	          pCIDir = ICO_LoadIcon(peimage, lpiID->idEntries + (int)pIconId[icon], &uSize);
	        else
	          for (i = 0; i < iconCount; i++)
	            if (pIconStorage[i].id == ((int)pIconId[icon] | 0x8000) )
	              pCIDir = USER32_LoadResource(peimage, pIconStorage + i, *(WORD*)pData, &uSize);

	        if (pCIDir)
                {
	          RetPtr[icon] = CreateIconFromResourceEx(pCIDir, uSize, TRUE, 0x00030000,
                                                                 cx1, cy1, flags);
                  if (cx2 && cy2)
                      RetPtr[++icon] = CreateIconFromResourceEx(pCIDir, uSize, TRUE, 0x00030000,
                                                                       cx2, cy2, flags);
                }
	        else
	          RetPtr[icon] = 0;
	      }
	      ret = icon;	/* return number of retrieved icons */
	    }
	  }
	}
    else
    if (sig == 1 || sig == 2) /* .ICO or .CUR file */
    {
        TRACE("-- icon Signature (0x%08x)\n", sig);

        if (pData == (BYTE*)-1)
        {
            INT cx[2] = {cx1, cx2}, cy[2] = {cy1, cy2};
            INT index;

            for(index = 0; index < (cx2 || cy2 ? 2 : 1); index++)
            {
                DWORD dataOffset;
                LPBYTE imageData;
                POINT hotSpot;
#ifndef __REACTOS__
                LPICONIMAGE entry;
#endif

                dataOffset = get_best_icon_file_offset(peimage, fsizel, cx[index], cy[index], sig == 1, flags, sig == 1 ? NULL : &hotSpot);

                if (dataOffset)
                {
                    HICON icon;
                    WORD *cursorData = NULL;
#ifdef __REACTOS__
                    BITMAPINFOHEADER bmih;
#endif

                    imageData = peimage + dataOffset;
#ifdef __REACTOS__
                    memcpy(&bmih, imageData, sizeof(BITMAPINFOHEADER));
#else
                    entry = (LPICONIMAGE)(imageData);
#endif

                    if(sig == 2)
                    {
#ifdef __REACTOS__
                         /* biSizeImage is the size of the raw bitmap data.
                          * A dummy 0 can be given for BI_RGB bitmaps.
                          * https://en.wikipedia.org/wiki/BMP_file_format */
                         if ((bmih.biCompression == BI_RGB) && (bmih.biSizeImage == 0))
                         {
                             bmih.biSizeImage = ((bmih.biWidth * bmih.biBitCount + 31) / 32) * 4 *
                                                (bmih.biHeight / 2);
                         }
#endif
                        /* we need to prepend the bitmap data with hot spots for CreateIconFromResourceEx */
#ifdef __REACTOS__
                        cursorData = HeapAlloc(GetProcessHeap(), 0, bmih.biSizeImage + 2 * sizeof(WORD));
#else
                        cursorData = HeapAlloc(GetProcessHeap(), 0, entry->icHeader.biSizeImage + 2 * sizeof(WORD));
#endif

                        if(!cursorData)
                            continue;

                        cursorData[0] = hotSpot.x;
                        cursorData[1] = hotSpot.y;

#ifdef __REACTOS__
                        memcpy(cursorData + 2, imageData, bmih.biSizeImage);
#else
                        memcpy(cursorData + 2, imageData, entry->icHeader.biSizeImage);
#endif

                        imageData = (LPBYTE)cursorData;
                    }

#ifdef __REACTOS__
                    icon = CreateIconFromResourceEx(imageData, bmih.biSizeImage, sig == 1, 0x00030000, cx[index], cy[index], flags);
#else
                    icon = CreateIconFromResourceEx(imageData, entry->icHeader.biSizeImage, sig == 1, 0x00030000, cx[index], cy[index], flags);
#endif

                    HeapFree(GetProcessHeap(), 0, cursorData);

                    if (icon)
                    {
                        if (RetPtr)
                            RetPtr[index] = icon;
                        else
                            DestroyIcon(icon);

                        iconCount = 1;
                        break;
                    }
                }
            }
        }
        ret = iconCount;	/* return number of retrieved icons */
    }
/* end ico file */

/* exe/dll */
	else if( sig == IMAGE_NT_SIGNATURE )
	{
        BYTE *idata, *igdata;
        const IMAGE_RESOURCE_DIRECTORY *rootresdir, *iconresdir, *icongroupresdir;
        const IMAGE_RESOURCE_DATA_ENTRY *idataent, *igdataent;
        const IMAGE_RESOURCE_DIRECTORY_ENTRY *xresent;
        ULONG size;
        UINT i;

        rootresdir = RtlImageDirectoryEntryToData((HMODULE)peimage, FALSE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size);
        if (!rootresdir)
        {
            WARN("haven't found section for resource directory.\n");
            goto end;
        }

	  /* search for the group icon directory */
	  if (!(icongroupresdir = find_entry_by_id(rootresdir, LOWORD(RT_GROUP_ICON), rootresdir)))
	  {
	    WARN("No Icongroupresourcedirectory!\n");
	    goto end;		/* failure */
	  }
	  iconDirCount = icongroupresdir->NumberOfNamedEntries + icongroupresdir->NumberOfIdEntries;

	  /* only number of icons requested */
	  if( !pIconId )
	  {
	    ret = iconDirCount;
	    goto end;		/* success */
	  }

	  if( nIconIndex < 0 )
	  {
	    /* search resource id */
	    int n = 0;
	    int iId = abs(nIconIndex);
	    const IMAGE_RESOURCE_DIRECTORY_ENTRY* xprdeTmp = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(icongroupresdir+1);

	    while(n<iconDirCount && xprdeTmp)
	    {
              if(xprdeTmp->Id ==  iId)
              {
                  nIconIndex = n;
                  break;
              }
              n++;
              xprdeTmp++;
	    }
	    if (nIconIndex < 0)
	    {
	      WARN("resource id %d not found\n", iId);
	      goto end;		/* failure */
	    }
	  }
	  else
	  {
	    /* check nIconIndex to be in range */
	    if (nIconIndex >= iconDirCount)
	    {
	      WARN("nIconIndex %d is larger than iconDirCount %d\n",nIconIndex,iconDirCount);
	      goto end;		/* failure */
	    }
	  }

	  /* assure we don't get too much */
	  if( nIcons > iconDirCount - nIconIndex )
	    nIcons = iconDirCount - nIconIndex;

	  /* starting from specified index */
	  xresent = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(icongroupresdir+1) + nIconIndex;

	  for (i=0; i < nIcons; i++,xresent++)
	  {
	    const IMAGE_RESOURCE_DIRECTORY *resdir;

	    /* go down this resource entry, name */
            resdir = (const IMAGE_RESOURCE_DIRECTORY *)((const char *)rootresdir + xresent->OffsetToDirectory);

	    /* default language (0) */
	    resdir = find_entry_default(resdir,rootresdir);
	    igdataent = (const IMAGE_RESOURCE_DATA_ENTRY*)resdir;

	    /* lookup address in mapped image for virtual address */
        igdata = RtlImageRvaToVa(RtlImageNtHeader((HMODULE)peimage), (HMODULE)peimage, igdataent->OffsetToData, NULL);
        if (!igdata)
	    {
	      FIXME("no matching real address for icongroup!\n");
	      goto end;	/* failure */
	    }
	    pIconId[i] = LookupIconIdFromDirectoryEx(igdata, TRUE, cx1, cy1, flags);
            if (cx2 && cy2) pIconId[++i] = LookupIconIdFromDirectoryEx(igdata, TRUE, cx2, cy2, flags);
	  }

	  if (!(iconresdir=find_entry_by_id(rootresdir,LOWORD(RT_ICON),rootresdir)))
	  {
	    WARN("No Iconresourcedirectory!\n");
	    goto end;		/* failure */
	  }

	  for (i=0; i<nIcons; i++)
	  {
	    const IMAGE_RESOURCE_DIRECTORY *xresdir;
	    xresdir = find_entry_by_id(iconresdir, LOWORD(pIconId[i]), rootresdir);
            if( !xresdir )
            {
              WARN("icon entry %d not found\n", LOWORD(pIconId[i]));
	      RetPtr[i]=0;
	      continue;
            }
	    xresdir = find_entry_default(xresdir, rootresdir);
	    idataent = (const IMAGE_RESOURCE_DATA_ENTRY*)xresdir;

        idata = RtlImageRvaToVa(RtlImageNtHeader((HMODULE)peimage), (HMODULE)peimage, idataent->OffsetToData, NULL);
        if (!idata)
	    {
	      WARN("no matching real address found for icondata!\n");
	      RetPtr[i]=0;
	      continue;
	    }
	    RetPtr[i] = CreateIconFromResourceEx(idata, idataent->Size, TRUE, 0x00030000, cx1, cy1, flags);
            if (cx2 && cy2)
                RetPtr[++i] = CreateIconFromResourceEx(idata, idataent->Size, TRUE, 0x00030000, cx2, cy2, flags);
	  }
	  ret = i;	/* return number of retrieved icons */
	}			/* if(sig == IMAGE_NT_SIGNATURE) */

end:
	UnmapViewOfFile(peimage);	/* success */
	return ret;
}

/***********************************************************************
 *           PrivateExtractIconsW			[USER32.@]
 *
 * NOTES
 *  If HIWORD(sizeX) && HIWORD(sizeY) 2 * ((nIcons + 1) MOD 2) icons are
 *  returned, with the LOWORD size icon first and the HIWORD size icon
 *  second.
 *  Also the Windows equivalent does extract icons in a strange way if
 *  nIndex is negative. Our implementation treats a negative nIndex as
 *  looking for that resource identifier for the first icon to retrieve.
 *
 * FIXME:
 *  should also support 16 bit EXE + DLLs, cursor and animated cursor as
 *  well as bitmap files.
 */

UINT WINAPI PrivateExtractIconsW (
	LPCWSTR lpwstrFile,
	int nIndex,
	int sizeX,
	int sizeY,
	HICON * phicon, /* [out] pointer to array of nIcons HICON handles */
	UINT* pIconId,  /* [out] pointer to array of nIcons icon identifiers or NULL */
	UINT nIcons,    /* [in] number of icons to retrieve */
	UINT flags )    /* [in] LR_* flags used by LoadImage */
{
	TRACE("%s %d %dx%d %p %p %d 0x%08x\n",
	      debugstr_w(lpwstrFile), nIndex, sizeX, sizeY, phicon, pIconId, nIcons, flags);

	if ((nIcons & 1) && HIWORD(sizeX) && HIWORD(sizeY))
	{
	  WARN("Uneven number %d of icons requested for small and large icons!\n", nIcons);
	}
	return ICO_ExtractIconExW(lpwstrFile, phicon, nIndex, nIcons, sizeX, sizeY, pIconId, flags);
}

/***********************************************************************
 *           PrivateExtractIconsA			[USER32.@]
 */

UINT WINAPI PrivateExtractIconsA (
	LPCSTR lpstrFile,
	int nIndex,
	int sizeX,
	int sizeY,
	HICON * phicon, /* [out] pointer to array of nIcons HICON handles */
	UINT* piconid,  /* [out] pointer to array of nIcons icon identifiers or NULL */
	UINT nIcons,    /* [in] number of icons to retrieve */
	UINT flags )    /* [in] LR_* flags used by LoadImage */
{
    UINT ret;
    INT len = MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, NULL, 0);
    LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
#ifdef __REACTOS__
    if (lpwstrFile == NULL)
        return 0;
#endif

    MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, lpwstrFile, len);
    ret = PrivateExtractIconsW(lpwstrFile, nIndex, sizeX, sizeY, phicon, piconid, nIcons, flags);

    HeapFree(GetProcessHeap(), 0, lpwstrFile);
    return ret;
}

/***********************************************************************
 *           PrivateExtractIconExW			[USER32.@]
 * NOTES
 *  if nIndex == -1 it returns the number of icons in any case !!!
 */
UINT WINAPI PrivateExtractIconExW (
	LPCWSTR lpwstrFile,
	int nIndex,
	HICON * phIconLarge,
	HICON * phIconSmall,
	UINT nIcons )
{
	DWORD cyicon, cysmicon, cxicon, cxsmicon;
	UINT ret = 0;

	TRACE("%s %d %p %p %d\n",
	debugstr_w(lpwstrFile),nIndex,phIconLarge, phIconSmall, nIcons);

	if (nIndex == -1)
	  /* get the number of icons */
	  return ICO_ExtractIconExW(lpwstrFile, NULL, 0, 0, 0, 0, NULL, LR_DEFAULTCOLOR);

	if (nIcons == 1 && phIconSmall && phIconLarge)
	{
	  HICON hIcon[2];
	  cxicon = GetSystemMetrics(SM_CXICON);
	  cyicon = GetSystemMetrics(SM_CYICON);
	  cxsmicon = GetSystemMetrics(SM_CXSMICON);
	  cysmicon = GetSystemMetrics(SM_CYSMICON);

          ret = ICO_ExtractIconExW(lpwstrFile, hIcon, nIndex, 2, cxicon | (cxsmicon<<16),
	                           cyicon | (cysmicon<<16), NULL, LR_DEFAULTCOLOR);
	  *phIconLarge = hIcon[0];
	  *phIconSmall = hIcon[1];
 	  return ret;
	}

	if (phIconSmall)
	{
	  /* extract n small icons */
	  cxsmicon = GetSystemMetrics(SM_CXSMICON);
	  cysmicon = GetSystemMetrics(SM_CYSMICON);
	  ret = ICO_ExtractIconExW(lpwstrFile, phIconSmall, nIndex, nIcons, cxsmicon,
	                           cysmicon, NULL, LR_DEFAULTCOLOR);
	}
       if (phIconLarge)
	{
	  /* extract n large icons */
	  cxicon = GetSystemMetrics(SM_CXICON);
	  cyicon = GetSystemMetrics(SM_CYICON);
         ret = ICO_ExtractIconExW(lpwstrFile, phIconLarge, nIndex, nIcons, cxicon,
	                           cyicon, NULL, LR_DEFAULTCOLOR);
	}
	return ret;
}

/***********************************************************************
 *           PrivateExtractIconExA			[USER32.@]
 */
UINT WINAPI PrivateExtractIconExA (
	LPCSTR lpstrFile,
	int nIndex,
	HICON * phIconLarge,
	HICON * phIconSmall,
	UINT nIcons )
{
	UINT ret;
	INT len = MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, NULL, 0);
	LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
#ifdef __REACTOS__
    if (lpwstrFile == NULL)
        return 0;
#endif

	TRACE("%s %d %p %p %d\n", lpstrFile, nIndex, phIconLarge, phIconSmall, nIcons);

	MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, lpwstrFile, len);
	ret = PrivateExtractIconExW(lpwstrFile, nIndex, phIconLarge, phIconSmall, nIcons);
	HeapFree(GetProcessHeap(), 0, lpwstrFile);
	return ret;
}
