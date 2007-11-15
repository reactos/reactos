/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/wgl.c
 * PURPOSE:              OpenGL32 lib, rosglXXX functions
 * PROGRAMMER:           Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 2, 2004: Created
 */

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <windows.h>

#define NTOS_MODE_USER
#include <ddraw.h>
#include <ddrawi.h>
#include <winddi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "teb.h"

#define OPENGL32_GL_FUNC_PROTOTYPES
#include "opengl32.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(UNIMPLEMENTED)
# define UNIMPLEMENTED DBGPRINT( "UNIMPLEMENTED" )
#endif


typedef struct _OPENGL_INFO
{
	DWORD Version;          /*!< Driver interface version */
	DWORD DriverVersion;    /*!< Driver version */
	WCHAR DriverName[256];  /*!< Driver name */
} OPENGL_INFO, *POPENGL_INFO;


/*! \brief Append OpenGL Rendering Context (GLRC) to list
 *
 * \param glrc [IN] Pointer to GLRC to append to list
 */
static
void
ROSGL_AppendContext( GLRC *glrc )
{
	/* synchronize */
	if (WaitForSingleObject( OPENGL32_processdata.glrc_mutex, INFINITE ) ==
	    WAIT_FAILED)
	{
		DBGPRINT( "Error: WaitForSingleObject() failed (%d)", GetLastError() );
		return; /* FIXME: do we have to expect such an error and handle it? */
	}

	if (OPENGL32_processdata.glrc_list == NULL)
		OPENGL32_processdata.glrc_list = glrc;
	else
	{
		GLRC *p = OPENGL32_processdata.glrc_list;
		while (p->next != NULL)
			p = p->next;
		p->next = glrc;
	}

	/* release mutex */
	if (!ReleaseMutex( OPENGL32_processdata.glrc_mutex ))
		DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );
}


/*! \brief Remove OpenGL Rendering Context (GLRC) from list
 *
 * \param glrc [IN] Pointer to GLRC to remove from list
 */
static
void
ROSGL_RemoveContext( GLRC *glrc )
{
	/* synchronize */
	if (WaitForSingleObject( OPENGL32_processdata.glrc_mutex, INFINITE ) ==
	    WAIT_FAILED)
	{
		DBGPRINT( "Error: WaitForSingleObject() failed (%d)", GetLastError() );
		return; /* FIXME: do we have to expect such an error and handle it? */
	}

	if (glrc == OPENGL32_processdata.glrc_list)
		OPENGL32_processdata.glrc_list = glrc->next;
	else
	{
		GLRC *p = OPENGL32_processdata.glrc_list;
		while (p != NULL)
		{
			if (p->next == glrc)
			{
				p->next = glrc->next;
				break;
			}
			p = p->next;
		}
		if (p == NULL)
			DBGPRINT( "Error: GLRC 0x%08x not found in list!", glrc );
	}

	/* release mutex */
	if (!ReleaseMutex( OPENGL32_processdata.glrc_mutex ))
		DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );
}


/*! \brief Create a new GL Context (GLRC) and append it to the list
 *
 * \return Pointer to new GLRC on success
 * \retval NULL Returned on failure (i.e. Out of memory)
 */
static
GLRC *
ROSGL_NewContext(void)
{
	GLRC *glrc;

	/* allocate GLRC */
	glrc = (GLRC*)HeapAlloc( GetProcessHeap(),
	              HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, sizeof (GLRC) );

	/* append to list */
	ROSGL_AppendContext( glrc );

	return glrc;
}

/*! \brief Delete all GLDCDATA with this IDC
 *
 * \param icd [IN] Pointer to a ICD
 */
static
VOID
ROSGL_DeleteDCDataForICD( GLDRIVERDATA *icd )
{
	GLDCDATA *p, **pptr;

	/* synchronize */
	if (WaitForSingleObject( OPENGL32_processdata.dcdata_mutex, INFINITE ) ==
	    WAIT_FAILED)
	{
		DBGPRINT( "Error: WaitForSingleObject() failed (%d)", GetLastError() );
		return;
	}

	p = OPENGL32_processdata.dcdata_list;
	pptr = &OPENGL32_processdata.dcdata_list;
	while (p != NULL)
	{
		if (p->icd == icd)
		{
			*pptr = p->next;
			OPENGL32_UnloadICD( p->icd );

			if (!HeapFree( GetProcessHeap(), 0, p ))
				DBGPRINT( "Warning: HeapFree() on GLDCDATA failed (%d)",
				          GetLastError() );

			p = *pptr;
		}
		else
		{
			pptr = &p->next;
			p = p->next;
		}
	}

	/* release mutex */
	if (!ReleaseMutex( OPENGL32_processdata.dcdata_mutex ))
		DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );
}


/*! \brief Delete a GL Context (GLRC) and remove it from the list
 *
 * \param glrc [IN] Pointer to GLRC to delete
 *
 * \retval TRUE  Success
 * \retval FALSE Failure
 */
static
BOOL
ROSGL_DeleteContext( GLRC *glrc )
{
	/* unload icd */
	if (glrc->icd != NULL)
		ROSGL_DeleteDCDataForICD( glrc->icd );

	/* remove from list */
	ROSGL_RemoveContext( glrc );

	/* free memory */
	HeapFree( GetProcessHeap(), 0, glrc );

	return TRUE;
}


/*! \brief Check wether a GLRC is in the list
 *
 * \param glrc [IN] Pointer to GLRC to look for in the list
 *
 * \retval TRUE  GLRC was found
 * \retval FALSE GLRC was not found
 */
static
BOOL
ROSGL_ContainsContext( GLRC *glrc )
{
	GLRC *p;
        BOOL found = FALSE;

	/* synchronize */
	if (WaitForSingleObject( OPENGL32_processdata.glrc_mutex, INFINITE ) ==
	    WAIT_FAILED)
	{
		DBGPRINT( "Error: WaitForSingleObject() failed (%d)", GetLastError() );
		return FALSE; /* FIXME: do we have to expect such an error and handle it? */
	}

	p = OPENGL32_processdata.glrc_list;
	while (p != NULL)
	{
		if (p == glrc)
		{
			found = TRUE;
			break;
		}
		p = p->next;
	}

	/* release mutex */
	if (!ReleaseMutex( OPENGL32_processdata.glrc_mutex ))
		DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );

	return found;
}


/*! \brief Get GL private DC data.
 *
 * This function adds an empty GLDCDATA to the list if there is no data for the
 * given DC yet.
 *
 * \param hdc [IN] Handle to a Device Context for which to get the data
 *
 * \return Pointer to GLDCDATA on success
 * \retval NULL on failure
 */
static
GLDCDATA *
ROSGL_GetPrivateDCData( HDC hdc )
{
	GLDCDATA *data;

	/* check hdc */
	if (GetObjectType( hdc ) != OBJ_DC && GetObjectType( hdc ) != OBJ_MEMDC)
	{
		DBGPRINT( "Error: hdc is not a DC handle!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	/* synchronize */
	if (WaitForSingleObject( OPENGL32_processdata.dcdata_mutex, INFINITE ) ==
	    WAIT_FAILED)
	{
		DBGPRINT( "Error: WaitForSingleObject() failed (%d)", GetLastError() );
		return NULL; /* FIXME: do we have to expect such an error and handle it? */
	}

	/* look for data in list */
	data = OPENGL32_processdata.dcdata_list;
	while (data != NULL)
	{
		if (data->hdc == hdc) /* found */
			break;
		data = data->next;
	}

	/* allocate new data if not found in list */
	if (data == NULL)
	{
		data = HeapAlloc( GetProcessHeap(),
		                  HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
		                  sizeof (GLDCDATA) );
		if (data == NULL)
		{
			DBGPRINT( "Error: HeapAlloc() failed (%d)", GetLastError() );
		}
		else
		{
			data->hdc = hdc;

			/* append data to list */
			if (OPENGL32_processdata.dcdata_list == NULL)
				OPENGL32_processdata.dcdata_list = data;
			else
			{
				GLDCDATA *p = OPENGL32_processdata.dcdata_list;
				while (p->next != NULL)
					p = p->next;
				p->next = data;
			}
		}
	}

	/* release mutex */
	if (!ReleaseMutex( OPENGL32_processdata.dcdata_mutex ))
		DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );

	return data;
}


/*! \brief Get ICD from HDC.
 *
 * This function asks the display driver which OpenGL ICD to load for the given
 * HDC, loads it and returns a pointer to a GLDRIVERDATA struct on success.
 *
 * \param hdc [IN] Handle for DC for which to load/get the ICD
 *
 * \return Pointer to GLDRIVERDATA
 * \retval NULL Failure.
 */
static
GLDRIVERDATA *
ROSGL_ICDForHDC( HDC hdc )
{
	GLDCDATA *dcdata;
	GLDRIVERDATA *drvdata;

	dcdata = ROSGL_GetPrivateDCData( hdc );
	if (dcdata == NULL)
		return NULL;

	if (dcdata->icd == NULL)
	{
		LPCWSTR driverName;
		OPENGL_INFO info;

		/* NOTE: This might be done by multiple threads simultaneously, but only the fastest
		         actually gets to set the ICD! */

		driverName = _wgetenv( L"OPENGL32_DRIVER" );
		if (driverName == NULL)
		{
			DWORD dwInput;
			LONG ret;

			/* get driver name */
			dwInput = OPENGL_GETINFO;
			ret = ExtEscape( hdc, QUERYESCSUPPORT, sizeof (dwInput), (LPCSTR)&dwInput, 0, NULL );
			if (ret > 0)
			{
				dwInput = 0;
				ret = ExtEscape( hdc, OPENGL_GETINFO, sizeof (dwInput),
				                 (LPCSTR)&dwInput, sizeof (OPENGL_INFO),
				                 (LPSTR)&info );
			}
			if (ret <= 0)
			{
				HKEY hKey;
				DWORD type, size;

				if (ret < 0)
				{
					DBGPRINT( "Warning: ExtEscape to get the drivername failed! (%d)", GetLastError() );
					if (MessageBox( WindowFromDC( hdc ), L"Couldn't get installable client driver name!\nUsing default driver.",
					                L"OPENGL32.dll: Warning", MB_OKCANCEL | MB_ICONWARNING ) == IDCANCEL)
					{
						return NULL;
					}
				}

				/* open registry key */
				ret = RegOpenKeyExW( HKEY_LOCAL_MACHINE, OPENGL_DRIVERS_SUBKEY, 0, KEY_QUERY_VALUE, &hKey );
				if (ret != ERROR_SUCCESS)
				{
					DBGPRINT( "Error: Couldn't open registry key '%ws'", OPENGL_DRIVERS_SUBKEY );
					SetLastError( ret );
					return NULL;
				}

				/* query value */
				size = sizeof (info.DriverName);
				ret = RegQueryValueExW( hKey, L"DefaultDriver", 0, &type, (LPBYTE)info.DriverName, &size );
				RegCloseKey( hKey );
				if (ret != ERROR_SUCCESS || type != REG_SZ)
				{
					DBGPRINT( "Error: Couldn't query DefaultDriver value or not a string" );
					SetLastError( ret );
					return NULL;
				}
			}
		}
		else
		{
			wcsncpy( info.DriverName, driverName, sizeof (info.DriverName) / sizeof (info.DriverName[0]) );
		}
		/* load driver (or get a reference) */
		drvdata = OPENGL32_LoadICD( info.DriverName );
		if (drvdata == NULL)
		{
			WCHAR Buffer[256];
			snwprintf(Buffer, sizeof(Buffer)/sizeof(WCHAR),
			          L"Couldn't load driver \"%s\".", driverName);
			MessageBox(WindowFromDC( hdc ), Buffer,
			           L"OPENGL32.dll: Warning",
			           MB_OK | MB_ICONWARNING);
		}
		else
		{
			/* Atomically set the ICD!!! */
			if (InterlockedCompareExchangePointer((PVOID*)&dcdata->icd,
			                                      (PVOID)drvdata,
			                                      NULL) != NULL)
			{
				/* Too bad, somebody else was faster... */
				OPENGL32_UnloadICD(drvdata);
			}
		}
	}

	return dcdata->icd;
}


/*! \brief SetContextCallBack passed to DrvSetContext.
 *
 * This function gets called by the OpenGL driver whenever the current GL
 * context (dispatch table) is to be changed.
 *
 * \param table [IN] Function pointer table (first DWORD is number of functions)
 *
 * \return unkown (maybe void? ERROR_SUCCESS at the moment)
 */
DWORD
CALLBACK
ROSGL_SetContextCallBack( const ICDTable *table )
{
	TEB *teb;
	PROC *tebTable, *tebDispatchTable;
	INT size;

	teb = NtCurrentTeb();
	tebTable = (PROC *)teb->glTable;
	tebDispatchTable = (PROC *)teb->glDispatchTable;

	DBGTRACE( "Called!" );

	if (table != NULL)
	{
		DBGPRINT( "Function count: %d\n", table->num_funcs );

		/* save table */
		size = sizeof (PROC) * table->num_funcs;
		memcpy( tebTable, table->dispatch_table, size );
		memset( tebTable + table->num_funcs, 0,
		        sizeof (table->dispatch_table) - size );
	}
	else
	{
		DBGPRINT( "Unsetting current context" );
		memset( tebTable, 0, sizeof (table->dispatch_table) );
	}

	/* put in empty functions as long as we dont have a fallback */
	#define X(func, ret, typeargs, args, icdidx, tebidx, stack)                 \
		if (tebTable[icdidx] == NULL)                                       \
		{                                                                   \
			if (table != NULL)                                          \
				DBGPRINT( "Warning: GL proc '%s' is NULL", #func ); \
			tebTable[icdidx] = (PROC)glEmptyFunc##stack;                \
		}
	GLFUNCS_MACRO
	#undef X

	/* fill teb->glDispatchTable for fast calls */
	#define X(func, ret, typeargs, args, icdidx, tebidx, stack)         \
		if (tebidx >= 0)                                            \
			tebDispatchTable[tebidx] = tebTable[icdidx];
	GLFUNCS_MACRO
	#undef X

	DBGPRINT( "Done." );

	return ERROR_SUCCESS;
}


/*! \brief Attempts to find the best matching pixel format for HDC
 *
 * This function is comparing each available format with the preferred one
 * and returns the one which is closest to it.
 * If PFD_DOUBLEBUFFER, PFD_STEREO or one of PFD_DRAW_TO_WINDOW,
 * PFD_DRAW_TO_BITMAP, PFD_SUPPORT_GDI and PDF_SUPPORT_OPENGL is given then
 * only formats which also support those will be enumerated (unless
 * PFD_DOUBLEBUFFER_DONTCARE or PFD_STEREO_DONTCARE is also set)
 *
 * \param hdc [IN] Handle to DC for which to get a pixel format index
 * \param pfd [IN] PFD describing what kind of format you want
 *
 * \return Pixel format index
 * \retval 0 Failed to find a suitable format
 */
#define BUFFERDEPTH_SCORE(want, have) \
	((want == 0) ? (0) : ((want < have) ? (1) : ((want > have) ? (3) : (0))))
int
APIENTRY
rosglChoosePixelFormat( HDC hdc, CONST PIXELFORMATDESCRIPTOR *pfd )
{
	GLDRIVERDATA *icd;
	PIXELFORMATDESCRIPTOR icdPfd;
	int i;
	int best = 0;
	int score, bestScore = 0x7fff; /* used to choose a pfd if no exact match */
	int icdNumFormats;
	const DWORD compareFlags = PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP |
	                           PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL;

	DBGTRACE( "Called!" );

	/* load ICD */
	icd = ROSGL_ICDForHDC( hdc );
	if (icd == NULL)
		return 0;

	/* check input */
	if (pfd->nSize != sizeof (PIXELFORMATDESCRIPTOR) || pfd->nVersion != 1)
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return 0;
	}

	/* get number of formats */
	icdNumFormats = icd->DrvDescribePixelFormat( hdc, 1,
	                                 sizeof (PIXELFORMATDESCRIPTOR), &icdPfd );
	if (icdNumFormats == 0)
	{
		DBGPRINT( "Error: DrvDescribePixelFormat failed (%d)", GetLastError() );
		return 0;
	}
	DBGPRINT( "Info: Enumerating %d pixelformats", icdNumFormats );

	/* try to find best format */
	for (i = 0; i < icdNumFormats; i++)
	{
		if (icd->DrvDescribePixelFormat( hdc, i + 1,
		                         sizeof (PIXELFORMATDESCRIPTOR), &icdPfd ) == 0)
		{
			DBGPRINT( "Warning: DrvDescribePixelFormat failed (%d)",
			          GetLastError() );
			break;
		}

		if ((pfd->dwFlags & PFD_GENERIC_ACCELERATED) != 0) /* we do not support such kind of drivers */
		{
			continue;
		}

		/* compare flags */
		if ((pfd->dwFlags & compareFlags) != (icdPfd.dwFlags & compareFlags))
			continue;
		if (!(pfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
		    ((pfd->dwFlags & PFD_DOUBLEBUFFER) != (icdPfd.dwFlags & PFD_DOUBLEBUFFER)))
			continue;
		if (!(pfd->dwFlags & PFD_STEREO_DONTCARE) &&
		    ((pfd->dwFlags & PFD_STEREO) != (icdPfd.dwFlags & PFD_STEREO)))
			continue;

		/* check other attribs */
		score = 0; /* higher is worse */
		if (pfd->iPixelType != icdPfd.iPixelType)
			score += 5; /* this is really bad i think */
		if (pfd->iLayerType != icdPfd.iLayerType)
			score += 15; /* this is very very bad ;) */

		score += BUFFERDEPTH_SCORE(pfd->cAlphaBits, icdPfd.cAlphaBits);
		score += BUFFERDEPTH_SCORE(pfd->cAccumBits, icdPfd.cAccumBits);
		score += BUFFERDEPTH_SCORE(pfd->cDepthBits, icdPfd.cDepthBits);
		score += BUFFERDEPTH_SCORE(pfd->cStencilBits, icdPfd.cStencilBits);
		score += BUFFERDEPTH_SCORE(pfd->cAuxBuffers, icdPfd.cAuxBuffers);

		/* check score */
		if (score < bestScore)
		{
			bestScore = score;
			best = i + 1;
			if (bestScore == 0)
				break;
		}
	}

	if (best == 0)
		SetLastError( 0 ); /* FIXME: set appropriate error */

	DBGPRINT( "Info: Suggesting pixelformat %d", best );
	return best;
}


/*! \brief Copy data specified by mask from one GLRC to another.
 *
 * \param src  [IN] Source GLRC
 * \param src  [OUT] Destination GLRC
 * \param mask [IN] Bitfield like given to glPushAttrib()
 *
 * \retval TRUE  Success
 * \retval FALSE Failure
 */
BOOL
APIENTRY
rosglCopyContext( HGLRC hsrc, HGLRC hdst, UINT mask )
{
	GLRC *src = (GLRC *)hsrc;
	GLRC *dst = (GLRC *)hdst;

	/* check glrcs */
	if (!ROSGL_ContainsContext( src ))
	{
		DBGPRINT( "Error: src GLRC not found!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}
	if (!ROSGL_ContainsContext( dst ))
	{
		DBGPRINT( "Error: dst GLRC not found!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	/* I think this is only possible within one ICD */
	if (src->icd != dst->icd)
	{
		DBGPRINT( "Error: src and dst GLRC use different ICDs!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	/* copy data (call ICD) */
	return src->icd->DrvCopyContext( src->hglrc, dst->hglrc, mask );
}


/*! \brief Create a new GL Rendering Context.
 *
 * This function can create over- or underlay surfaces.
 *
 * \param hdc [IN] Handle for DC for which to create context
 * \param layer [IN] Layer number to bind (draw?) to
 *
 * \return Handle for the created GLRC
 * \retval NULL Failure
 */
HGLRC
APIENTRY
rosglCreateLayerContext( HDC hdc, int layer )
{
	GLDRIVERDATA *icd = NULL;
	GLRC *glrc;
	HGLRC drvHglrc = NULL;

	DBGTRACE( "Called!" );

/*	if (GetObjectType( hdc ) != OBJ_DC)
	{
		DBGPRINT( "Error: hdc is not a DC handle!" );
		return NULL;
	}
*/
	/* create new GLRC */
	glrc = ROSGL_NewContext();
	if (glrc == NULL)
		return NULL;

	/* load ICD */
	icd = ROSGL_ICDForHDC( hdc );
	if (icd == NULL)
	{
		ROSGL_DeleteContext( glrc );
		DBGPRINT( "Couldn't get ICD by HDC :-(" );
		/* FIXME: fallback? */
		return NULL;
	}

	/* create context */
	if (icd->DrvCreateLayerContext != NULL)
		drvHglrc = icd->DrvCreateLayerContext( hdc, layer );
	if (drvHglrc == NULL)
	{
		if (layer == 0 && icd->DrvCreateContext != NULL)
			drvHglrc = icd->DrvCreateContext( hdc );
		else
			DBGPRINT( "Warning: CreateLayerContext not supported by ICD!" );
	}

	if (drvHglrc == NULL)
	{
		/* FIXME: fallback to mesa? */
		DBGPRINT( "Error: DrvCreate[Layer]Context failed! (%d)", GetLastError() );
		ROSGL_DeleteContext( glrc );
		return NULL;
	}

	/* we have our GLRC in glrc and the ICD's GLRC in drvHglrc */
	glrc->hglrc = drvHglrc;
	glrc->icd = icd;

	return (HGLRC)glrc;
}


/*! \brief Create a new GL Rendering Context.
 *
 * \param hdc [IN] Handle for DC for which to create context
 *
 * \return Handle for the created GLRC
 * \retval NULL Failure
 */
HGLRC
APIENTRY
rosglCreateContext( HDC hdc )
{
	return rosglCreateLayerContext( hdc, 0 );
}


/*! \brief Delete an OpenGL context
 *
 * \param hglrc [IN] Handle to GLRC to delete; must not be a threads current RC!
 *
 * \retval TRUE  Success
 * \retval FALSE Failure (i.e. GLRC is current for a thread)
 */
BOOL
APIENTRY
rosglDeleteContext( HGLRC hglrc )
{
	GLRC *glrc = (GLRC *)hglrc;

	/* check if we know about this context */
	if (!ROSGL_ContainsContext( glrc ))
	{
		DBGPRINT( "Error: hglrc not found!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	/* make sure GLRC is not current for some thread */
	if (glrc->is_current)
	{
		DBGPRINT( "Error: GLRC is current for DC 0x%08x", glrc->hdc );
		SetLastError( ERROR_INVALID_FUNCTION );
		return FALSE;
	}

	/* release ICD's context */
	if (glrc->hglrc != NULL)
	{
		if (!glrc->icd->DrvDeleteContext( glrc->hglrc ))
		{
			DBGPRINT( "Warning: DrvDeleteContext() failed (%d)", GetLastError() );
			return FALSE;
		}
	}

	/* free resources */
	return ROSGL_DeleteContext( glrc );
}


BOOL
APIENTRY
rosglDescribeLayerPlane( HDC hdc, int iPixelFormat, int iLayerPlane,
                         UINT nBytes, LPLAYERPLANEDESCRIPTOR plpd )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return FALSE;
}


/*! \brief Gets information about a pixelformat.
 *
 * \param hdc     [IN]  Handle to DC
 * \param iFormat [IN]  Pixelformat index
 * \param nBytes  [IN]  sizeof (pfd) - at most nBytes are copied into pfd
 * \param pfd     [OUT] Pointer to a PIXELFORMATDESCRIPTOR
 *
 * \return Maximum pixelformat index/number of formats
 * \retval 0 Failure
 */
int
APIENTRY
rosglDescribePixelFormat( HDC hdc, int iFormat, UINT nBytes,
                          LPPIXELFORMATDESCRIPTOR pfd )
{
	int ret = 0;
	GLDRIVERDATA *icd = ROSGL_ICDForHDC( hdc );

	if (icd != NULL)
	{
		ret = icd->DrvDescribePixelFormat( hdc, iFormat, nBytes, pfd );
		if (ret == 0)
			DBGPRINT( "Error: DrvDescribePixelFormat(format=%d) failed (%d)", iFormat, GetLastError() );
	}
	else
	{
		SetLastError( ERROR_INVALID_FUNCTION );
	}

	return ret;
}


/*! \brief Return the thread's current GLRC
 *
 * \return Handle for thread's current GLRC
 * \retval NULL No current GLRC set
 */
HGLRC
APIENTRY
rosglGetCurrentContext()
{
	return (HGLRC)(OPENGL32_threaddata->glrc);
}


/*! \brief Return the thread's current DC
 *
 * \return Handle for thread's current DC
 * \retval NULL No current DC/GLRC set
 */
HDC
APIENTRY
rosglGetCurrentDC()
{
	/* FIXME: is it correct to return NULL when there is no current GLRC or
	   is there another way to find out the wanted HDC? */
	if (OPENGL32_threaddata->glrc == NULL)
		return NULL;
	return (HDC)(OPENGL32_threaddata->glrc->hdc);
}


int
APIENTRY
rosglGetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart,
                             int cEntries, COLORREF *pcr )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return 0;
}


/*! \brief Returns the current pixelformat.
 *
 * \param hdc [IN] Handle to DC to get the pixelformat from
 *
 * \return Pixelformat index
 * \retval 0 Failure
 */
int
WINAPI
rosglGetPixelFormat( HDC hdc )
{
	GLDCDATA *dcdata;

	DBGTRACE( "Called!" );

	dcdata = ROSGL_GetPrivateDCData( hdc );
	if (dcdata == NULL)
	{
		DBGPRINT( "Error: ROSGL_GetPrivateDCData failed!" );
		return 0;
	}

	return dcdata->pixel_format;
}


/*! \brief Get the address for an OpenGL extension function.
 *
 * The addresses this function returns are only valid within the same thread
 * which it was called from.
 *
 * \param proc [IN] Name of the function to look for
 *
 * \return The address of the proc
 * \retval NULL Failure
 */
PROC
APIENTRY
rosglGetProcAddress( LPCSTR proc )
{
	PROC func;
	GLDRIVERDATA *icd;

	if (OPENGL32_threaddata->glrc == NULL)
	{
		DBGPRINT( "Error: No current GLRC!" );
		SetLastError( ERROR_INVALID_FUNCTION );
		return NULL;
	}

	icd = OPENGL32_threaddata->glrc->icd;
	func = icd->DrvGetProcAddress( proc );
	if (func != NULL)
	{
		DBGPRINT( "Info: Proc \"%s\" loaded from ICD.", proc );
		return func;
	}

	/* FIXME: Should we return wgl/gl 1.1 functions? */
	SetLastError( ERROR_PROC_NOT_FOUND );
	return NULL;
}


/*! \brief Make the given GLRC the threads current GLRC for hdc
 *
 * \param hdc   [IN] Handle for a DC to be drawn on
 * \param hglrc [IN] Handle for a GLRC to make current
 *
 * \retval TRUE  Success
 * \retval FALSE Failure
 */
BOOL
APIENTRY
rosglMakeCurrent( HDC hdc, HGLRC hglrc )
{
	GLRC *glrc = (GLRC *)hglrc;
	ICDTable *icdTable = NULL;

	DBGTRACE( "Called!" );

	/* flush current context */
	if (OPENGL32_threaddata->glrc != NULL)
	{
		glFlush();
	}

	/* check if current context is unset */
	if (glrc == NULL)
	{
		if (OPENGL32_threaddata->glrc != NULL)
		{
			glrc = OPENGL32_threaddata->glrc;
			glrc->icd->DrvReleaseContext( glrc->hglrc );
			glrc->is_current = FALSE;
			OPENGL32_threaddata->glrc = NULL;
		}
	}
	else
	{
		/* check hdc */
		if (GetObjectType( hdc ) != OBJ_DC && GetObjectType( hdc ) != OBJ_MEMDC)
		{
			DBGPRINT( "Error: hdc is not a DC handle!" );
			SetLastError( ERROR_INVALID_HANDLE );
			return FALSE;
		}

		/* check if we know about this glrc */
		if (!ROSGL_ContainsContext( glrc ))
		{
			DBGPRINT( "Error: hglrc not found!" );
			SetLastError( ERROR_INVALID_HANDLE );
			return FALSE;
		}

		/* check if it is available */
		if (glrc->is_current && glrc->thread_id != GetCurrentThreadId()) /* used by another thread */
		{
			DBGPRINT( "Error: hglrc is current for thread 0x%08x", glrc->thread_id );
			SetLastError( ERROR_INVALID_HANDLE );
			return FALSE;
		}

		/* call the ICD */
		if (glrc->hglrc != NULL)
		{
			DBGPRINT( "Info: Calling DrvSetContext!" );
			SetLastError( ERROR_SUCCESS );
			icdTable = glrc->icd->DrvSetContext( hdc, glrc->hglrc,
			                                     ROSGL_SetContextCallBack );
			if (icdTable == NULL)
			{
				DBGPRINT( "Error: DrvSetContext failed (%d)\n", GetLastError() );
				return FALSE;
			}
			DBGPRINT( "Info: DrvSetContext succeeded!" );
		}

		/* make it current */
		if (OPENGL32_threaddata->glrc != NULL)
			OPENGL32_threaddata->glrc->is_current = FALSE;
		glrc->is_current = TRUE;
		glrc->thread_id = GetCurrentThreadId();
		glrc->hdc = hdc;
		OPENGL32_threaddata->glrc = glrc;
	}

	if (ROSGL_SetContextCallBack( icdTable ) != ERROR_SUCCESS && icdTable == NULL)
	{
		DBGPRINT( "Warning: ROSGL_SetContextCallBack failed!" );
	}

	return TRUE;
}


BOOL
APIENTRY
rosglRealizeLayerPalette( HDC hdc, int iLayerPlane, BOOL bRealize )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return FALSE;
}


int
APIENTRY
rosglSetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart,
                             int cEntries, CONST COLORREF *pcr )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return 0;
}


/*! \brief Set a DCs pixelformat
 *
 * \param hdc     [IN] Handle to DC for which to set the format
 * \param iFormat [IN] Index of the pixelformat to set
 * \param pfd     [IN] Not sure what this is for
 *
 * \retval TRUE  Success
 * \retval FALSE Failure
 */
BOOL
WINAPI
rosglSetPixelFormat( HDC hdc, int iFormat, CONST PIXELFORMATDESCRIPTOR *pfd )
{
	GLDRIVERDATA *icd;
	GLDCDATA *dcdata;

	DBGTRACE( "Called!" );

	/* load ICD */
	icd = ROSGL_ICDForHDC( hdc );
	if (icd == NULL)
	{
		DBGPRINT( "Warning: ICDForHDC() failed" );
		return FALSE;
	}

	/* call ICD */
	if (!icd->DrvSetPixelFormat( hdc, iFormat, pfd ))
	{
		DBGPRINT( "Warning: DrvSetPixelFormat(format=%d) failed (%d)",
		          iFormat, GetLastError() );
		return FALSE;
	}

	/* store format in private DC data */
	dcdata = ROSGL_GetPrivateDCData( hdc );
	if (dcdata == NULL)
	{
		DBGPRINT( "Error: ROSGL_GetPrivateDCData() failed!" );
		return FALSE;
	}
	dcdata->pixel_format = iFormat;

	return TRUE;
}


/*! \brief Enable display-list sharing between multiple GLRCs
 *
 * This will only work if both GLRCs are from the same driver.
 *
 * \param hglrc1 [IN] GLRC number 1
 * \param hglrc2 [IN] GLRC number 2
 *
 * \retval TRUE  Success
 * \retval FALSE Failure
 */
BOOL
APIENTRY
rosglShareLists( HGLRC hglrc1, HGLRC hglrc2 )
{
	GLRC *glrc1 = (GLRC *)hglrc1;
	GLRC *glrc2 = (GLRC *)hglrc2;

	/* check glrcs */
	if (!ROSGL_ContainsContext( glrc1 ))
	{
		DBGPRINT( "Error: hglrc1 not found!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}
	if (!ROSGL_ContainsContext( glrc2 ))
	{
		DBGPRINT( "Error: hglrc2 not found!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	/* I think this is only possible within one ICD */
	if (glrc1->icd != glrc2->icd)
	{
		DBGPRINT( "Error: hglrc1 and hglrc2 use different ICDs!" );
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	/* share lists (call ICD) */
	return glrc1->icd->DrvShareLists( glrc1->hglrc, glrc2->hglrc );
}


/*! \brief Flushes GL and swaps front/back buffer if appropriate
 *
 * \param hdc [IN] Handle to device context to swap buffers for
 *
 * \retval TRUE  Success
 * \retval FALSE Failure
 */
BOOL
APIENTRY
rosglSwapBuffers( HDC hdc )
{
	GLDRIVERDATA *icd = ROSGL_ICDForHDC( hdc );
	DBGTRACE( "Called!" );
	if (icd != NULL)
	{
		DBGPRINT( "Swapping buffers!" );
		if (!icd->DrvSwapBuffers( hdc ))
		{
			DBGPRINT( "Error: DrvSwapBuffers failed (%d)", GetLastError() );
			return FALSE;
		}
		return TRUE;
	}

	/* FIXME: implement own functionality? */
	SetLastError( ERROR_INVALID_FUNCTION );
	return FALSE;
}


BOOL
APIENTRY
rosglSwapLayerBuffers( HDC hdc, UINT fuPlanes )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return FALSE;
}


BOOL
APIENTRY
rosglUseFontBitmapsA( HDC hdc, DWORD  first, DWORD count, DWORD listBase )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return FALSE;
}


BOOL
APIENTRY
rosglUseFontBitmapsW( HDC hdc, DWORD  first, DWORD count, DWORD listBase )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return FALSE;
}


BOOL
APIENTRY
rosglUseFontOutlinesA( HDC hdc, DWORD first, DWORD count, DWORD listBase,
                       FLOAT deviation, FLOAT extrusion, int format,
                       GLYPHMETRICSFLOAT *pgmf )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return FALSE;
}


BOOL
APIENTRY
rosglUseFontOutlinesW( HDC hdc, DWORD first, DWORD count, DWORD listBase,
                       FLOAT deviation, FLOAT extrusion, int format,
                       GLYPHMETRICSFLOAT *pgmf )
{
	UNIMPLEMENTED;
	SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
	return FALSE;
}

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

/* EOF */
