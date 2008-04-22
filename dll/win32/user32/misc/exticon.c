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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <user32.h>

#include <wine/debug.h>

/* Start of Hack section */

WINE_DEFAULT_DEBUG_CHANNEL(icon);

#include "pshpack1.h"

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

#include "poppack.h"
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
            return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].OffsetToDirectory);
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
    return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry->OffsetToDirectory);
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
	  if (mz_header->e_cblp == 1)	/* .ICO file ? */
	  {
	    *retptr = (LPBYTE)-1;	/* ICONHEADER.idType, must be 1 */
	    return 1;
	  }
	  else
	    return 0; /* failed */
	}
	if (mz_header->e_lfanew >= pesize) {
	    return 0; /* failed, happens with PKZIP DOS Exes for instance. */
	}
	if (*((DWORD*)(peimage + mz_header->e_lfanew)) == IMAGE_NT_SIGNATURE )
	  return IMAGE_NT_SIGNATURE;
#if 0
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
#endif
	return 0; /* failed */
}
#if 0
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
 * see http://www.microsoft.com/win32dev/ui/icons.htm
 */
#define HEADER_SIZE		(sizeof(CURSORICONDIR) - sizeof (CURSORICONDIRENTRY))
#define HEADER_SIZE_FILE	(sizeof(icoICONDIR) - sizeof (icoICONDIRENTRY))

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
	*uSize = lpcid->idCount * sizeof(CURSORICONDIRENTRY) + HEADER_SIZE;
	if( (lpID = (CURSORICONDIR*)HeapAlloc(GetProcessHeap(),0, *uSize) ))
	{
	  /* copy the header */
	  lpID->idReserved = lpcid->idReserved;
	  lpID->idType = lpcid->idType;
	  lpID->idCount = lpcid->idCount;

	  /* copy the entries */
	  for( i=0; i < lpcid->idCount; i++ )
	  {
	    memcpy((void*)&(lpID->idEntries[i]),(void*)&(lpcid->idEntries[i]), sizeof(CURSORICONDIRENTRY) - 2);
	    lpID->idEntries[i].wResId = i;
	  }

	  *lplpiID = (LPicoICONDIR)peimage;
	  return (BYTE *)lpID;
	}
	return 0;
}
#endif
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
	UINT16		iconDirCount = 0; //,iconCount = 0;
	LPBYTE		peimage;
	HANDLE		fmapping;
	//ULONG		uSize;
	DWORD		fsizeh,fsizel;
        WCHAR		szExePath[MAX_PATH];
        DWORD		dwSearchReturn;

	TRACE("%s, %d, %d %p 0x%08x\n", debugstr_w(lpszExeFileName), nIconIndex, nIcons, pIconId, flags);

        dwSearchReturn = SearchPathW(NULL, lpszExeFileName, NULL, sizeof(szExePath) / sizeof(szExePath[0]), szExePath, NULL);
        if ((dwSearchReturn == 0) || (dwSearchReturn > sizeof(szExePath) / sizeof(szExePath[0])))
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

	cx1 = LOWORD(cxDesired);
	cx2 = HIWORD(cxDesired) ? HIWORD(cxDesired) : cx1;
	cy1 = LOWORD(cyDesired);
	cy2 = HIWORD(cyDesired) ? HIWORD(cyDesired) : cy1;

	if (pIconId) /* Invalidate first icon identifier */
		*pIconId = 0xFFFFFFFF;

	if (!pIconId) /* if no icon identifier array present use the icon handle array as intermediate storage */
	  pIconId = (UINT*)RetPtr;

	sig = USER32_GetResourceTable(peimage, fsizel, &pData);

/* ico file or NE exe/dll*/
#if 0
	if (sig==IMAGE_OS2_SIGNATURE || sig==1) /* .ICO file */
	{
	  BYTE		*pCIDir = 0;
	  NE_TYPEINFO	*pTInfo = (NE_TYPEINFO*)(pData + 2);
	  NE_NAMEINFO	*pIconStorage = NULL;
	  NE_NAMEINFO	*pIconDir = NULL;
	  LPicoICONDIR	lpiID = NULL;

	  TRACE("-- OS2/icon Signature (0x%08lx)\n", sig);

	  if (pData == (BYTE*)-1)
	  {
	    pCIDir = ICO_GetIconDirectory(peimage, &lpiID, &uSize);	/* check for .ICO file */
	    if (pCIDir)
	    {
	      iconDirCount = 1; iconCount = lpiID->idCount;
	      TRACE("-- icon found %p 0x%08lx 0x%08x 0x%08x\n", pCIDir, uSize, iconDirCount, iconCount);
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
	      if (lpiID && pCIDir)	/* *.ico file, deallocate heap pointer*/
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
	        pIconId[i] = LookupIconIdFromDirectoryEx(pCIDir, TRUE, (i & 1) ? cx2 : cx1, (i & 1) ? cy2 : cy1, flags);
	      }
	      if (lpiID && pCIDir)	/* *.ico file, deallocate heap pointer*/
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
	          RetPtr[icon] = (HICON)CreateIconFromResourceEx(pCIDir, uSize, TRUE, 0x00030000,
	                                                         (icon & 1) ? cx2 : cx1, (icon & 1) ? cy2 : cy1, flags);
	        else
	          RetPtr[icon] = 0;
	      }
	      ret = icon;	/* return number of retrieved icons */
	    }
	  }
	}
/* end ico file */

/* exe/dll */
	else if( sig == IMAGE_NT_SIGNATURE )
#endif
	if( sig == IMAGE_NT_SIGNATURE )
	{
	  LPBYTE		idata,igdata;
	  PIMAGE_DOS_HEADER	dheader;
	  PIMAGE_NT_HEADERS	pe_header;
	  PIMAGE_SECTION_HEADER	pe_sections;
	  const IMAGE_RESOURCE_DIRECTORY *rootresdir,*iconresdir,*icongroupresdir;
	  const IMAGE_RESOURCE_DATA_ENTRY *idataent,*igdataent;
	  const IMAGE_RESOURCE_DIRECTORY_ENTRY *xresent;
	  UINT	i, j;

	  dheader = (PIMAGE_DOS_HEADER)peimage;
	  pe_header = (PIMAGE_NT_HEADERS)(peimage+dheader->e_lfanew);	  /* it is a pe header, USER32_GetResourceTable checked that */
	  pe_sections = (PIMAGE_SECTION_HEADER)(((char*)pe_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)
	                                        + pe_header->FileHeader.SizeOfOptionalHeader);
	  rootresdir = NULL;

	  /* search for the root resource directory */
	  for (i=0;i<pe_header->FileHeader.NumberOfSections;i++)
	  {
	    if (pe_sections[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	      continue;
	    if (fsizel < pe_sections[i].PointerToRawData+pe_sections[i].SizeOfRawData) {
	      FIXME("File %s too short (section is at %ld bytes, real size is %ld)\n",
		      debugstr_w(lpszExeFileName),
		      pe_sections[i].PointerToRawData+pe_sections[i].SizeOfRawData,
		      fsizel
	      );
	      goto end;
	    }
	    /* FIXME: doesn't work when the resources are not in a separate section */
	    if (pe_sections[i].VirtualAddress == pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress)
	    {
	      rootresdir = (PIMAGE_RESOURCE_DIRECTORY)(peimage+pe_sections[i].PointerToRawData);
	      break;
	    }
	  }

	  if (!rootresdir)
	  {
	    WARN("haven't found section for resource directory.\n");
	    goto end;		/* failure */
	  }

	  /* search for the group icon directory */
	  if (!(icongroupresdir = find_entry_by_id(rootresdir, LOWORD(RT_GROUP_ICON), rootresdir)))
	  {
	    WARN("No Icongroupresourcedirectory!\n");
	    goto end;		/* failure */
	  }
	  iconDirCount = icongroupresdir->NumberOfNamedEntries + icongroupresdir->NumberOfIdEntries;

	  /* only number of icons requested */
	  if( nIcons == 0 )
	  {
	    ret = iconDirCount;
	    goto end;		/* success */
	  }

	  if( nIconIndex < 0 )
	  {
	    /* search resource id */
	    int n = 0;
	    int iId = abs(nIconIndex);
	    PIMAGE_RESOURCE_DIRECTORY_ENTRY xprdeTmp = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1);

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
	  if( nIcons / 2 > iconDirCount - nIconIndex )
	    nIcons = 2 * (iconDirCount - nIconIndex);

	  /* starting from specified index */
	  xresent = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1) + nIconIndex;

	  for (i=0; i < nIcons; i++)
	  {
	    const IMAGE_RESOURCE_DIRECTORY *resdir;

	    /* go down this resource entry, name */
	    resdir = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)rootresdir+(xresent->OffsetToDirectory));

	    /* default language (0) */
	    resdir = find_entry_default(resdir,rootresdir);
	    igdataent = (PIMAGE_RESOURCE_DATA_ENTRY)resdir;

	    /* lookup address in mapped image for virtual address */
	    igdata = NULL;

	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++)
	    {
	      if (igdataent->OffsetToData < pe_sections[j].VirtualAddress)
	        continue;
	      if (igdataent->OffsetToData+igdataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
	        continue;

	      if (igdataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData+igdataent->Size > fsizel) {
	        FIXME("overflow in PE lookup (%s has len %ld, have offset %ld), short file?\n", debugstr_w(lpszExeFileName), fsizel,
	        	   igdataent->OffsetToData - pe_sections[j].VirtualAddress + pe_sections[j].PointerToRawData + igdataent->Size);
	        goto end; /* failure */
	      }
	      igdata = peimage+(igdataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }

	    if (!igdata)
	    {
	      FIXME("no matching real address for icongroup!\n");
	      goto end;	/* failure */
	    }
	    pIconId[i] = LookupIconIdFromDirectoryEx(igdata, TRUE, (i & 1) ? cx2 : cx1, (i & 1) ? cy2 : cy1, flags);
	    if (i & 1) xresent++;
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
	    if (!xresdir)
	    {
	      WARN("find_entry_by_id failed\n");
	      ret = 0xFFFFFFFF;
	      goto end;
	    }
	    xresdir = find_entry_default(xresdir, rootresdir);
	    if (!xresdir)
	    {
	      WARN("find_entry_default failed\n");
	      ret = 0xFFFFFFFF;
	      goto end;
	    }
	    idataent = (PIMAGE_RESOURCE_DATA_ENTRY)xresdir;
	    idata = NULL;

	    /* map virtual to address in image */
	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++)
	    {
	      if (idataent->OffsetToData < pe_sections[j].VirtualAddress)
	        continue;
	      if (idataent->OffsetToData+idataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
	        continue;
	      idata = peimage+(idataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }
	    if (!idata)
	    {
	      WARN("no matching real address found for icondata!\n");
	      RetPtr[i]=0;
	      continue;
	    }
	    RetPtr[i] = (HICON) CreateIconFromResourceEx(idata,idataent->Size,TRUE,0x00030000,
	                                                 (i & 1) ? cx2 : cx1, (i & 1) ? cy2 : cy1, flags);
	  }
	  ret = i;	/* return number of retrieved icons */
	}			/* if(sig == IMAGE_NT_SIGNATURE) */

end:
	UnmapViewOfFile(peimage);	/* success */
	return ret;
}

/***********************************************************************
 *           PrivateExtractIconsW			[USER32.@]
 * @implemented
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
	  WARN("Uneven number %d of icons requested for small and large icons!", nIcons);
	}
	return ICO_ExtractIconExW(lpwstrFile, phicon, nIndex, nIcons, sizeX, sizeY, pIconId, flags);
}

/***********************************************************************
 *           PrivateExtractIconsA			[USER32.@]
 * @implemented
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

    MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, lpwstrFile, len);
    ret = PrivateExtractIconsW(lpwstrFile, nIndex, sizeX, sizeY, phicon, piconid, nIcons, flags);

    HeapFree(GetProcessHeap(), 0, lpwstrFile);
    return ret;
}

/***********************************************************************
 *           PrivateExtractIconExW			[USER32.@]
 * @implemented
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
	INT ret = 0;

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

	  ret = ICO_ExtractIconExW(lpwstrFile, (HICON*) &hIcon, nIndex, 2, cxicon | (cxsmicon<<16),
	                           cyicon | (cysmicon<<16), NULL, LR_DEFAULTCOLOR);
	  *phIconLarge = (1 <= ret ? hIcon[0] : NULL);
	  *phIconSmall = (2 <= ret ? hIcon[1] : NULL);
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
 * @implemented
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

	TRACE("%s %d %p %p %d\n", lpstrFile, nIndex, phIconLarge, phIconSmall, nIcons);

	MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, lpwstrFile, len);
	ret = PrivateExtractIconExW(lpwstrFile, nIndex, phIconLarge, phIconSmall, nIcons);
	HeapFree(GetProcessHeap(), 0, lpwstrFile);
	return ret;
}
