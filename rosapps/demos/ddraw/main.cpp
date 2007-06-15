#include <windows.h>
#include <ddrawi.h>
#include <ddraw.h>

LPDIRECTDRAW7 DirectDraw = NULL;
LPDIRECTDRAWSURFACE7 FrontBuffer = NULL;
LPDIRECTDRAWCLIPPER Clipper = NULL;
LPDDRAWI_DIRECTDRAW_INT This = NULL;

PCHAR DDErrorString (HRESULT hResult);
LONG WINAPI WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

bool Fullscreen, Running;
//#define USE_CLIPPER

bool Init (HWND hwnd)
{
	DDSURFACEDESC2 ddsd; 
	HRESULT hResult;

	// Create the main object
	OutputDebugString("=> DirectDrawCreateEx\n");
	hResult = DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL); 

	if (hResult != DD_OK)
	{
		MessageBox(0,DDErrorString(hResult), "DirectDrawCreateEx", 0);
		return 0;
	}

    This = (LPDDRAWI_DIRECTDRAW_INT)DirectDraw;

	// Set Fullscreen or windowed mode
	OutputDebugString("=> DDraw->SetCooperativeLevel\n");

	if(Fullscreen)
		hResult = DirectDraw->SetCooperativeLevel (hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	else
		hResult = DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL);
		
	if (hResult != DD_OK)
	{
		MessageBox(0,DDErrorString(hResult), "DDraw->SetCooperativeLevel", 0);
		return 0;
	}

	// Set the new resolution
	if(Fullscreen)
	{
		OutputDebugString("=> DDraw->SetDisplayMode\n");
		hResult = DirectDraw->SetDisplayMode (800, 600, 32, 0, 0);
			
		if (hResult != DD_OK)
		{
			MessageBox(0,DDErrorString(hResult), "DDraw->SetDisplayMode", 0);
			return 0;
		}
	}

	// Create the primary surface
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));

	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	OutputDebugString("=> DDraw->CreateSurface\n");
	hResult = DirectDraw->CreateSurface(&ddsd, &FrontBuffer, NULL);

	if (hResult != DD_OK)
	{
		MessageBox(0,DDErrorString(hResult), "DDraw->CreateSurface", 0);
		return 0;
	}

#ifdef USE_CLIPPER
	// Set up the clipper
	OutputDebugString("=> DDraw->CreateClipper\n");

    hResult = DirectDraw->CreateClipper(0,&Clipper,NULL);
	if (hResult != DD_OK)
	{
		MessageBox(0,DDErrorString(hResult), "DDraw->CreateSurface", 0);
		return 0;
	}

	OutputDebugString("=> Clipper->SetHWnd\n");
	hResult = Clipper->SetHWnd(0,hwnd);
    if (hResult != DD_OK)
    {
		MessageBox(0,DDErrorString(hResult), "Clipper->SetHWnd", 0);
        return  0;
    }    

	OutputDebugString("=> Suface->SetClipper\n");
    hResult = FrontBuffer->SetClipper(Clipper);
    if (hResult != DD_OK)
    {
		MessageBox(0,DDErrorString(hResult), "FrontBuffer->SetClipper", 0);
        return 0;
    }
#endif

	return true;
}

void Draw (HWND hwnd)
{
	// Make the frontbuffer pink
	RECT rect;
	GetWindowRect(hwnd, &rect); // this is not necessary when clippper is used

	DDBLTFX	 ddbltfx;
	ddbltfx.dwSize = sizeof(DDBLTFX);
	ddbltfx.dwFillColor = RGB(255, 0, 255); 

	OutputDebugString("=> Surface->Blt (DDBLT_COLORFILL)\n");
	
	FrontBuffer->Blt(&rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
}		

void CleanUp (void)
{
	if (Clipper != NULL)
	{
		OutputDebugString("=> Clipper->Release\n");
		Clipper->Release();
		Clipper = NULL;
	}

	if (FrontBuffer != NULL)
	{
		OutputDebugString("=> Surface->Release\n");
		FrontBuffer->Release();
		FrontBuffer = NULL;
	}
	if (DirectDraw != NULL)
	{
		OutputDebugString("=> DDraw->Release\n");
		DirectDraw->Release();
		DirectDraw = NULL;
	}
}

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, 
					LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;  
	WNDCLASS wndclass; 
	HWND hwnd;
	Fullscreen = MessageBox(0, "Do you want to me to run in Fullscreen ?", 0, MB_YESNO) == IDYES;

	// Create windnow
	wndclass.style         = CS_HREDRAW | CS_VREDRAW; 
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInst;
	wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW); 
	wndclass.hbrBackground = (HBRUSH)GetStockObject (LTGRAY_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = "DDrawDemo"; 

	RegisterClass(&wndclass);    

#ifdef USE_CLIPPER
	hwnd = CreateWindow("DDrawDemo", 
                         "ReactOS DirectDraw Demo", 
						 Fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,
						 CW_USEDEFAULT, 
						 CW_USEDEFAULT,
						 800, 
						 600,     
						 NULL, NULL, 
						 hInst, NULL);
#else
	hwnd = CreateWindow("DDrawDemo", 
                         "ReactOS DirectDraw Demo", 
                         WS_POPUP,
						 CW_USEDEFAULT, 
						 CW_USEDEFAULT,
						 800, 
						 600,     
						 NULL, NULL, 
						 hInst, NULL);
#endif

	// Inizalize Ddraw
	if(Init(hwnd))
	{
		Running = true;

		ShowWindow(hwnd, nCmdShow);  
		UpdateWindow(hwnd);
	}
    
	// Main loop
	while (Running)
	{
		if(Fullscreen)
			Draw(hwnd);

		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{ 
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} 

	CleanUp();

	return 0;
}


LONG WINAPI WndProc (HWND hwnd, UINT message, 
                         UINT wParam, LONG lParam) 
{ 
	switch (message)
	{
		case WM_PAINT:
		{
			if(!Fullscreen)
				Draw(hwnd);
		} break;

		case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
				Running=false;
				return 0;
			} break;
		}

		case WM_DESTROY:
		{
			PostQuitMessage (0); 
			return 0;
		} break;
	}

	return DefWindowProc (hwnd, message, wParam, lParam);
} 

PCHAR DDErrorString (HRESULT hResult)
{
	switch (hResult)
	{
		case DD_OK:								 return "DD_OK";
		case DDERR_ALREADYINITIALIZED:           return "DDERR_ALREADYINITIALIZED";
		case DDERR_CANNOTATTACHSURFACE:          return "DDERR_CANNOTATTACHSURFACE";
		case DDERR_CANNOTDETACHSURFACE:          return "DDERR_CANNOTDETACHSURFACE";
		case DDERR_CURRENTLYNOTAVAIL:            return "DDERR_CURRENTLYNOTAVAIL";
		case DDERR_EXCEPTION:                    return "DDERR_EXCEPTION";
		case DDERR_GENERIC:                      return "DDERR_GENERIC";
		case DDERR_HEIGHTALIGN:                  return "DDERR_HEIGHTALIGN";
		case DDERR_INCOMPATIBLEPRIMARY:          return "DDERR_INCOMPATIBLEPRIMARY";
		case DDERR_INVALIDCAPS:                  return "DDERR_INVALIDCAPS";
		case DDERR_INVALIDCLIPLIST:              return "DDERR_INVALIDCLIPLIST";
		case DDERR_INVALIDMODE:                  return "DDERR_INVALIDMODE";
		case DDERR_INVALIDOBJECT:                return "DDERR_INVALIDOBJECT";
		case DDERR_INVALIDPARAMS:                return "DDERR_INVALIDPARAMS";
		case DDERR_INVALIDPIXELFORMAT:           return "DDERR_INVALIDPIXELFORMAT";
		case DDERR_INVALIDRECT:                  return "DDERR_INVALIDRECT";
		case DDERR_LOCKEDSURFACES:               return "DDERR_LOCKEDSURFACES";
		case DDERR_NO3D:                         return "DDERR_NO3D";
		case DDERR_NOALPHAHW:                    return "DDERR_NOALPHAHW";
		case DDERR_NOCLIPLIST:                   return "DDERR_NOCLIPLIST";
		case DDERR_NOCOLORCONVHW:                return "DDERR_NOCOLORCONVHW";
		case DDERR_NOCOOPERATIVELEVELSET:        return "DDERR_NOCOOPERATIVELEVELSET";
		case DDERR_NOCOLORKEY:                   return "DDERR_NOCOLORKEY";
		case DDERR_NOCOLORKEYHW:                 return "DDERR_NOCOLORKEYHW";
		case DDERR_NODIRECTDRAWSUPPORT:          return "DDERR_NODIRECTDRAWSUPPORT";
		case DDERR_NOEXCLUSIVEMODE:              return "DDERR_NOEXCLUSIVEMODE";
		case DDERR_NOFLIPHW:                     return "DDERR_NOFLIPHW";
		case DDERR_NOGDI:                        return "DDERR_NOGDI";
		case DDERR_NOMIRRORHW:                   return "DDERR_NOMIRRORHW";
		case DDERR_NOTFOUND:                     return "DDERR_NOTFOUND";
		case DDERR_NOOVERLAYHW:                  return "DDERR_NOOVERLAYHW";
		case DDERR_NORASTEROPHW:                 return "DDERR_NORASTEROPHW";
		case DDERR_NOROTATIONHW:                 return "DDERR_NOROTATIONHW";
		case DDERR_NOSTRETCHHW:                  return "DDERR_NOSTRETCHHW";
		case DDERR_NOT4BITCOLOR:                 return "DDERR_NOT4BITCOLOR";
		case DDERR_NOT4BITCOLORINDEX:            return "DDERR_NOT4BITCOLORINDEX";
		case DDERR_NOT8BITCOLOR:                 return "DDERR_NOT8BITCOLOR";
		case DDERR_NOTEXTUREHW:                  return "DDERR_NOTEXTUREHW";
		case DDERR_NOVSYNCHW:                    return "DDERR_NOVSYNCHW";
		case DDERR_NOZBUFFERHW:                  return "DDERR_NOZBUFFERHW";
		case DDERR_NOZOVERLAYHW:                 return "DDERR_NOZOVERLAYHW";
		case DDERR_OUTOFCAPS:                    return "DDERR_OUTOFCAPS";
		case DDERR_OUTOFMEMORY:                  return "DDERR_OUTOFMEMORY";
		case DDERR_OUTOFVIDEOMEMORY:             return "DDERR_OUTOFVIDEOMEMORY";
		case DDERR_OVERLAYCANTCLIP:              return "DDERR_OVERLAYCANTCLIP";
		case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
		case DDERR_PALETTEBUSY:                  return "DDERR_PALETTEBUSY";
		case DDERR_COLORKEYNOTSET:               return "DDERR_COLORKEYNOTSET";
		case DDERR_SURFACEALREADYATTACHED:       return "DDERR_SURFACEALREADYATTACHED";
		case DDERR_SURFACEALREADYDEPENDENT:      return "DDERR_SURFACEALREADYDEPENDENT";
		case DDERR_SURFACEBUSY:                  return "DDERR_SURFACEBUSY";
		case DDERR_CANTLOCKSURFACE:              return "DDERR_CANTLOCKSURFACE";
		case DDERR_SURFACEISOBSCURED:            return "DDERR_SURFACEISOBSCURED";
		case DDERR_SURFACELOST:                  return "DDERR_SURFACELOST";
		case DDERR_SURFACENOTATTACHED:           return "DDERR_SURFACENOTATTACHED";
		case DDERR_TOOBIGHEIGHT:                 return "DDERR_TOOBIGHEIGHT";
		case DDERR_TOOBIGSIZE:                   return "DDERR_TOOBIGSIZE";
		case DDERR_TOOBIGWIDTH:                  return "DDERR_TOOBIGWIDTH";
		case DDERR_UNSUPPORTED:                  return "DDERR_UNSUPPORTED";
		case DDERR_UNSUPPORTEDFORMAT:            return "DDERR_UNSUPPORTEDFORMAT";
		case DDERR_UNSUPPORTEDMASK:              return "DDERR_UNSUPPORTEDMASK";
		case DDERR_VERTICALBLANKINPROGRESS:      return "DDERR_VERTICALBLANKINPROGRESS";
		case DDERR_WASSTILLDRAWING:              return "DDERR_WASSTILLDRAWING";
		case DDERR_XALIGN:                       return "DDERR_XALIGN";
		case DDERR_INVALIDDIRECTDRAWGUID:        return "DDERR_INVALIDDIRECTDRAWGUID";
		case DDERR_DIRECTDRAWALREADYCREATED:     return "DDERR_DIRECTDRAWALREADYCREATED";
		case DDERR_NODIRECTDRAWHW:               return "DDERR_NODIRECTDRAWHW";
		case DDERR_PRIMARYSURFACEALREADYEXISTS:  return "DDERR_PRIMARYSURFACEALREADYEXISTS";
		case DDERR_NOEMULATION:                  return "DDERR_NOEMULATION";
		case DDERR_REGIONTOOSMALL:               return "DDERR_REGIONTOOSMALL";
		case DDERR_CLIPPERISUSINGHWND:           return "DDERR_CLIPPERISUSINGHWND";
		case DDERR_NOCLIPPERATTACHED:            return "DDERR_NOCLIPPERATTACHED";
		case DDERR_NOHWND:                       return "DDERR_NOHWND";
		case DDERR_HWNDSUBCLASSED:               return "DDERR_HWNDSUBCLASSED";
		case DDERR_HWNDALREADYSET:               return "DDERR_HWNDALREADYSET";
		case DDERR_NOPALETTEATTACHED:            return "DDERR_NOPALETTEATTACHED";
		case DDERR_NOPALETTEHW:                  return "DDERR_NOPALETTEHW";
		case DDERR_BLTFASTCANTCLIP:              return "DDERR_BLTFASTCANTCLIP";
		case DDERR_NOBLTHW:                      return "DDERR_NOBLTHW";
		case DDERR_NODDROPSHW:                   return "DDERR_NODDROPSHW";
		case DDERR_OVERLAYNOTVISIBLE:            return "DDERR_OVERLAYNOTVISIBLE";
		case DDERR_NOOVERLAYDEST:                return "DDERR_NOOVERLAYDEST";
		case DDERR_INVALIDPOSITION:              return "DDERR_INVALIDPOSITION";
		case DDERR_NOTAOVERLAYSURFACE:           return "DDERR_NOTAOVERLAYSURFACE";
		case DDERR_EXCLUSIVEMODEALREADYSET:      return "DDERR_EXCLUSIVEMODEALREADYSET";
		case DDERR_NOTFLIPPABLE:                 return "DDERR_NOTFLIPPABLE";
		case DDERR_CANTDUPLICATE:                return "DDERR_CANTDUPLICATE";
		case DDERR_NOTLOCKED:                    return "DDERR_NOTLOCKED";
		case DDERR_CANTCREATEDC:                 return "DDERR_CANTCREATEDC";
		case DDERR_NODC:                         return "DDERR_NODC";
		case DDERR_WRONGMODE:                    return "DDERR_WRONGMODE";
		case DDERR_IMPLICITLYCREATED:            return "DDERR_IMPLICITLYCREATED";
		case DDERR_NOTPALETTIZED:                return "DDERR_NOTPALETTIZED";
		case DDERR_UNSUPPORTEDMODE:              return "DDERR_UNSUPPORTEDMODE";
		case DDERR_NOMIPMAPHW:                   return "DDERR_NOMIPMAPHW";
		case DDERR_INVALIDSURFACETYPE:           return "DDERR_INVALIDSURFACETYPE";
		case DDERR_DCALREADYCREATED:             return "DDERR_DCALREADYCREATED";
		case DDERR_CANTPAGELOCK:                 return "DDERR_CANTPAGELOCK";
		case DDERR_CANTPAGEUNLOCK:               return "DDERR_CANTPAGEUNLOCK";
		case DDERR_NOTPAGELOCKED:                return "DDERR_NOTPAGELOCKED";
		case DDERR_NOTINITIALIZED:               return "DDERR_NOTINITIALIZED";
	}
	return "Unknown Error";
}

