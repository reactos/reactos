/*
 * PROJECT:         ReactOS Sound Record Application
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/sndrec32.cpp
 * PURPOSE:         Application Startup
 * PROGRAMMERS:     Marco Pagliaricci <ms_blue (at) hotmail (dot) it>
 */

#include "stdafx.h"
#include "sndrec32.h"




//#pragma comment(lib, "comctl32.lib")


HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];


ATOM			MyRegisterClass(HINSTANCE hInstance);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);



HWND main_win;
HWND slider;
HWND buttons[5];
HBITMAP butbmps[5];
HBITMAP butbmps_dis[5];
WNDPROC buttons_std_proc;

BOOL butdisabled[5];
BOOL stopped_flag;
BOOL isnew;


DWORD slider_pos;
WORD slider_min;
WORD slider_max;

DWORD samples_max;

OPENFILENAME ofn;
TCHAR file_path[MAX_PATH];
BOOL path_set;

snd::audio_membuffer * AUD_BUF;
snd::audio_waveout * AUD_OUT;
snd::audio_wavein * AUD_IN;


BOOL s_recording;



int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	MSG msg;
	HACCEL hAccelTable;

	InitCommonControls();


	butbmps[0] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_START ));
	butbmps[1] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_END ));
	butbmps[2] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_PLAY ));
	butbmps[3] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_STOP ));
	butbmps[4] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_REC ));

	butbmps_dis[0] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_START_DIS ));
	butbmps_dis[1] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_END_DIS ));
	butbmps_dis[2] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_PLAY_DIS ));
	butbmps_dis[3] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_STOP_DIS ));
	butbmps_dis[4] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2_REC_DIS ));


	snd::audio_membuffer AUD_buffer( snd::A44100_16BIT_STEREO );
	snd::audio_waveout AUD_waveout( snd::A44100_16BIT_STEREO, AUD_buffer );
	snd::audio_wavein AUD_wavein( snd::A44100_16BIT_STEREO, AUD_buffer );

	AUD_buffer.play_finished = l_play_finished;
	AUD_buffer.audio_arrival = l_audio_arrival;
	AUD_buffer.buffer_resized = l_buffer_resized;

	AUD_buffer.alloc_seconds( INITIAL_BUFREC_SECONDS );

	AUD_IN = &AUD_wavein;
	AUD_OUT = &AUD_waveout;
	AUD_BUF = &AUD_buffer;



	slider_pos = 0;
	slider_min = 0;
	slider_max = 32767;


	stopped_flag = FALSE;
	path_set = FALSE;
	isnew = TRUE;


	samples_max = AUD_buffer.total_samples();


	LoadString(hInstance,
		IDS_APP_TITLE, szTitle, MAX_LOADSTRING);



	LoadString(hInstance,
		IDC_REACTOS_SNDREC32, szWindowClass, MAX_LOADSTRING);



	MyRegisterClass(hInstance);


	if (!InitInstance (hInstance, nCmdShow))
	{
		MessageBox(0, 0, TEXT("CreateWindow() Error!"), 0);
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance,
				MAKEINTRESOURCE( IDC_REACTOS_SNDREC32 ));






	s_recording = false;




	AUD_wavein.open();
	AUD_waveout.open();






	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	AUD_waveout.close();
	AUD_wavein.close();

	AUD_buffer.clear();


	return (int) msg.wParam;
}




ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon( hInstance, MAKEINTRESOURCE( IDI_SNDREC32 ));
	wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH)( 16 );
	wcex.lpszMenuName	= MAKEINTRESOURCE( IDR_MENU1 );
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SNDREC32 ));


	return RegisterClassEx( &wcex );
}

BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
   HWND hWnd;

   hInst = hInstance;

   hWnd = CreateWindow(
					szWindowClass,
					szTitle,
					WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					MAINWINDOW_W,
					MAINWINDOW_H,
					NULL, NULL,
					hInstance, NULL
				);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   main_win = hWnd;


   return TRUE;
}


//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	RECT rect;
	PAINTSTRUCT ps;
	HDC hdc;


	//
	// Checking for global pointers to buffer and
	// io audio devices.
	//

	if (( !AUD_IN ) || ( !AUD_OUT ) || ( !AUD_BUF ))
	{
		MessageBox( 0, TEXT("Buffer Error"), 0, 0 );
		return 1;
	}


	switch (message)
	{


	case WM_CREATE:



		//
		// Creating ALL the buttons
		//

		for ( int i = 0; i < 5; ++ i )
		{

			buttons[i] = CreateWindow(
								TEXT("button"),
								TEXT(""),
								WS_CHILD|WS_VISIBLE|BS_BITMAP,
								BUTTONS_CX + ( i * (BUTTONS_W+((i == 0)?0:BUTTONS_SPACE))),
								BUTTONS_CY, BUTTONS_W, BUTTONS_H, hWnd,
								(HMENU)i, hInst, 0
							);

			if ( !buttons[i] )
			{
				MessageBox(0, 0, TEXT("CreateWindow() Error!"), 0);
				return FALSE;

			}


			//
			// Realize the button bmp image
			//

			SendMessage(buttons[i], BM_SETIMAGE, ( WPARAM )IMAGE_BITMAP, ( LPARAM )butbmps[i]);

			UpdateWindow( buttons[i] );

			disable_but( i );

		}


		//
		// Creating the SLIDER window
		//

		slider = CreateWindow(
						TRACKBAR_CLASS,
						TEXT(""),
						WS_CHILD|WS_VISIBLE|TBS_NOTICKS|TBS_HORZ|TBS_ENABLESELRANGE,
						SLIDER_CX, SLIDER_CY, SLIDER_W, SLIDER_H, hWnd,
						(HMENU)SLIDER_ID, hInst, 0
					);


		if ( !slider )
		{
			MessageBox(0, 0, TEXT("CreateWindow() Error!"), 0);
			return FALSE;

		}


		//
		// Sets slider limits
		//


		SendMessage(
				slider,
				TBM_SETRANGE,
				(WPARAM)TRUE,
				(LPARAM)MAKELONG(slider_min,slider_max)
			);


		UpdateWindow( slider );

		enable_but( BUTREC_ID );

		EnableWindow( slider, FALSE );




		break;



	//
	// Implements slider logic
	//

	case WM_HSCROLL :
	{
		switch( LOWORD( wParam ))
		{

		case SB_ENDSCROLL:
			break;

		case SB_PAGERIGHT:
		case SB_PAGELEFT:
		case TB_THUMBTRACK:


			//
			// If the user touch the slider bar,
			// set the audio start position properly
			//


			slider_pos = SendMessage( slider, TBM_GETPOS, 0, 0 );


			AUD_BUF->set_position(
					AUD_BUF->audinfo().bytes_in_samples(
									(( slider_pos * samples_max ) / slider_max )
								)
				);


			break;

		}

		break;
	}






	case WM_COMMAND:

		wmId    = LOWORD( wParam );
		wmEvent = HIWORD( wParam );

		if (( wmId >= 0 ) && ( wmId < 5 ) && ( butdisabled[wmId] == TRUE ))
			break;

		switch (wmId)
		{

		case ID_NEW:

			if ( !isnew )
			{

				if ( AUD_IN->current_status() == snd::WAVEIN_RECORDING )
					AUD_IN->stop_recording();


				if (( AUD_OUT->current_status() == snd::WAVEOUT_PLAYING ) ||
					( AUD_OUT->current_status() == snd::WAVEOUT_PAUSED ))
					AUD_OUT->stop();


				AUD_BUF->reset();

				enable_but( BUTREC_ID );
				disable_but( BUTSTART_ID );
				disable_but( BUTEND_ID );
				disable_but( BUTSTOP_ID );
				disable_but( BUTPLAY_ID );


				samples_max = AUD_BUF->total_samples();
				slider_pos = 0;

				SendMessage(slider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) slider_pos);

				EnableMenuItem( GetMenu( hWnd ), ID_FILE_SAVEAS, MF_GRAYED );
				EnableMenuItem( GetMenu( hWnd ), ID_FILE_SAVE, MF_GRAYED );

				isnew = TRUE;

				ZeroMemory( file_path, MAX_PATH );

				EnableWindow( slider, FALSE );

			}



			break;




		case ID_FILE_OPEN:

			ZeroMemory( &ofn, sizeof( ofn ));

			ofn.lStructSize = sizeof( ofn );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = TEXT("Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0");
			ofn.lpstrFile = file_path;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt = TEXT("wav");

			if( GetOpenFileName( &ofn ))
			{
				open_wav( file_path );
				EnableMenuItem( GetMenu( hWnd ), ID_FILE_SAVE, MF_ENABLED );
				EnableMenuItem( GetMenu( hWnd ), ID_FILE_SAVEAS, MF_ENABLED );

				EnableWindow( slider, TRUE );

			}

			break;




		case ID__ABOUT:



			break;


		case ID_FILE_SAVEAS:

			ZeroMemory( &ofn, sizeof( ofn ));

			ofn.lStructSize = sizeof( ofn );
			ofn.hwndOwner         = hWnd ;
			ofn.Flags             = OFN_OVERWRITEPROMPT;
			ofn.lpstrFilter = TEXT("Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0");
			ofn.lpstrFile = file_path;
			ofn.nMaxFile = MAX_PATH;

			ofn.lpstrDefExt = TEXT("wav");

			if ( GetSaveFileName ( &ofn ))
			{
				write_wav( file_path );

				EnableMenuItem( GetMenu( hWnd ), ID_FILE_SAVE, MF_ENABLED );
			}

			break;

		case ID_EXIT:
			DestroyWindow( hWnd );
			break;


			//
			// Sndrec32 buttons routines
			//

		case BUTSTART_ID:

			AUD_BUF->set_position_start();

			slider_pos = 0;

			SendMessage( slider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) slider_pos );

			break;


		case BUTEND_ID:
			//Beep(300,200);
			break;

		case BUTPLAY_ID:

			AUD_OUT->play();

			disable_but( BUTSTART_ID );
			disable_but( BUTEND_ID );
			disable_but( BUTREC_ID );
			disable_but( BUTPLAY_ID );


			SetTimer( hWnd, 1, 250, 0 );

			break;

		case BUTSTOP_ID:
			if ( s_recording )
			{
				s_recording = FALSE;

				AUD_IN->stop_recording();



				//
				// Resetting slider position
				//

				slider_pos = 0;
				SendMessage(slider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) slider_pos);


				samples_max = AUD_BUF->samples_received();

				EnableMenuItem((HMENU)IDR_MENU1, ID_FILE_SAVEAS, MF_ENABLED );


				enable_but( BUTSTART_ID );
				enable_but( BUTEND_ID );
				enable_but( BUTREC_ID );
				enable_but( BUTPLAY_ID );

				EnableMenuItem( GetMenu( hWnd ), ID_FILE_SAVEAS, MF_ENABLED );
				EnableWindow( slider, TRUE );



			} else {

				AUD_OUT->pause();

				enable_but( BUTSTART_ID );
				enable_but( BUTEND_ID );
				enable_but( BUTREC_ID );
				enable_but( BUTPLAY_ID );

			}

			KillTimer( hWnd, 1 );

			break;

		case BUTREC_ID:

			s_recording = TRUE;

			samples_max = AUD_BUF->total_samples();

			AUD_IN->start_recording();

			enable_but( BUTSTOP_ID );

			disable_but( BUTSTART_ID );
			disable_but( BUTEND_ID );
			disable_but( BUTREC_ID );
			disable_but( BUTPLAY_ID );

			isnew = FALSE;

			EnableWindow( slider, FALSE );

			SetTimer( hWnd, 1, 150, 0 );

			break;


		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;


	case WM_TIMER:

		if ( stopped_flag )
		{
			KillTimer(hWnd, 1);
			slider_pos = 0;

			enable_but( BUTPLAY_ID );

			stopped_flag = FALSE;
		}

		SendMessage( slider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) slider_pos );



		break;



	case WM_PAINT:

		InvalidateRect( hWnd, &rect, TRUE );
		hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}





void l_play_finished ( void )
{

	stopped_flag = true;


	enable_but( BUTSTART_ID );
	enable_but( BUTEND_ID );
	enable_but( BUTREC_ID );
	enable_but( BUTPLAY_ID );



}

void l_audio_arrival ( unsigned int samples_arrival )
{


	slider_pos += (DWORD) (( slider_max * samples_arrival ) / samples_max );


}

void l_buffer_resized ( unsigned int new_size )
{





}

VOID enable_but( DWORD id )
{

	butdisabled[ id ] = FALSE;

	SendMessage(buttons[ id ], BM_SETIMAGE, ( WPARAM )IMAGE_BITMAP, ( LPARAM )butbmps[ id ]);


}
VOID disable_but( DWORD id )
{

	butdisabled[ id ] = TRUE;

	SendMessage(buttons[ id ], BM_SETIMAGE, ( WPARAM )IMAGE_BITMAP, ( LPARAM )butbmps_dis[ id ]);

}
















BOOL open_wav( TCHAR * f )
{


	HANDLE file;

	riff_hdr r;
	wave_hdr w;
	data_chunk d;

	BOOL b;

	DWORD bytes_recorded_in_wav = 0;
	DWORD is_read = 0;


	file = CreateFile(
					f,
					GENERIC_READ,
					0, 0,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					0
				);



	if ( !file )
	{
		MessageBox(
					main_win,
					TEXT("Cannot open file. CreateFile() error."),
					TEXT("ERROR"),
					MB_OK|MB_ICONERROR
				);

		return FALSE;
	}


	b = ReadFile( file, ( LPVOID ) &r, sizeof ( r ), &is_read, 0 );

	if ( !b )
	{
		MessageBox(
					main_win,
					TEXT("Cannot read RIFF header."),
					TEXT("ERROR"),
					MB_OK|MB_ICONERROR
				);

		CloseHandle( file );
		return FALSE;
	}


	b = ReadFile( file, ( LPVOID ) &w, sizeof ( w ), &is_read, 0 );


	if ( !b )
	{
		MessageBox(
					main_win,
					TEXT("Cannot read WAVE header."),
					TEXT("ERROR"),
					MB_OK|MB_ICONERROR
				);

		CloseHandle( file );
		return FALSE;
	}



	b = ReadFile( file, ( LPVOID ) &d, sizeof ( d ), &is_read, 0 );

	if ( !b )
	{
		MessageBox(
					main_win,
					TEXT("Cannot read WAVE subchunk."),
					TEXT("ERROR"),
					MB_OK|MB_ICONERROR
				);

		CloseHandle( file );
		return FALSE;
	}

	bytes_recorded_in_wav = r.chunksize - 36;


	if ( bytes_recorded_in_wav == 0 )
	{
		MessageBox(
					main_win,
					TEXT("Cannot read file. No audio data."),
					TEXT("ERROR"),
					MB_OK|MB_ICONERROR
				);

		CloseHandle( file );
		return FALSE;
	}


	snd::audio_format openfmt
		( w.SampleRate, w.BitsPerSample, w.NumChannels );



	AUD_BUF->clear();


	AUD_BUF->alloc_bytes( bytes_recorded_in_wav );


	b = ReadFile(
				file,
				( LPVOID ) AUD_BUF->audio_buffer(),
				bytes_recorded_in_wav,
				&is_read,
				0
			);


	AUD_BUF->set_b_received( bytes_recorded_in_wav );


	if (( !b ) || ( is_read != bytes_recorded_in_wav ))
	{
		MessageBox(
					main_win,
					TEXT("Cannot read file. Error reading audio data."),
					TEXT("ERROR"),
					MB_OK|MB_ICONERROR
				);

		CloseHandle( file );

		AUD_BUF->reset();
		return FALSE;
	}

	CloseHandle( file );

	enable_but( BUTPLAY_ID );
	enable_but( BUTSTOP_ID );
	enable_but( BUTSTART_ID );
	enable_but( BUTEND_ID );
	enable_but( BUTREC_ID );


	samples_max = AUD_BUF->samples_received();

	isnew = FALSE;

	return TRUE;

}


BOOL
write_wav( TCHAR * f )
{

	HANDLE file;


	DWORD written;
	BOOL is_writ;
	int i;
	riff_hdr r;
	wave_hdr w;
	data_chunk d;



	file = CreateFile(
					f,
					GENERIC_WRITE,
					0, 0,
					CREATE_NEW,
					FILE_ATTRIBUTE_NORMAL,
					0
				);



	if ( !file )
	{
		i = MessageBox(
					main_win,
					TEXT("File already exist. Overwrite it?"),
					TEXT("Warning"),
					MB_YESNO|MB_ICONQUESTION
				);

		if ( i == IDYES )
		{

			file = CreateFile(
							f,
							GENERIC_WRITE,
							0, 0,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							0
						);

			if ( !file )
			{
				MessageBox(
						main_win,
						TEXT("File Error, CreateFile() failed."),
						TEXT("ERROR"),
						MB_OK|MB_ICONERROR
					);


				return FALSE;

			}


		} else
			return FALSE;
	}




	r.magic = 0x46464952;


	r.format = 0x45564157;
	r.chunksize = 36 + AUD_BUF->bytes_recorded();


	w.Subchunkid = 0x20746d66;

	w.Subchunk1Size = 16;
	w.AudioFormat = 1;
	w.NumChannels = AUD_BUF->audinfo().channels();
	w.SampleRate = AUD_BUF->audinfo().sample_rate();
	w.ByteRate = AUD_BUF->audinfo().byte_rate();
	w.BlockAlign = AUD_BUF->audinfo().block_align();
	w.BitsPerSample = AUD_BUF->audinfo().bits();


	d.subc = 0x61746164;
	d.subc_size = AUD_BUF->bytes_recorded();



	//
	// Writing headers
	//


	is_writ = WriteFile( file, ( LPCVOID ) &r, sizeof ( r ), &written, 0 );

	if ( !is_writ )
	{
		MessageBox(
				main_win,
				TEXT("File Error, WriteFile() failed."),
				TEXT("ERROR"),
				MB_OK|MB_ICONERROR
			);

		CloseHandle( file );

		return FALSE;

	}


	is_writ = WriteFile( file, ( LPCVOID ) &w, sizeof ( w ), &written, 0 );

	if ( !is_writ )
	{
		MessageBox(
				main_win,
				TEXT("File Error, WriteFile() failed."),
				TEXT("ERROR"),
				MB_OK|MB_ICONERROR
			);

		CloseHandle( file );

		return FALSE;

	}


	is_writ = WriteFile( file, ( LPCVOID ) &d, sizeof ( d ), &written, 0 );


	if ( !is_writ )
	{
		MessageBox(
				main_win,
				TEXT("File Error, WriteFile() failed."),
				TEXT("ERROR"),
				MB_OK|MB_ICONERROR
			);

		CloseHandle( file );

		return FALSE;

	}



	is_writ = WriteFile(
					file,
					( LPCVOID ) AUD_BUF->audio_buffer(),
					AUD_BUF->bytes_recorded(),
					&written,
					0
				);

	if ( !is_writ )
	{
		MessageBox(
				main_win,
				TEXT("File Error, WriteFile() failed."),
				TEXT("ERROR"),
				MB_OK|MB_ICONERROR
			);

		CloseHandle( file );

		return FALSE;

	}


	CloseHandle( file );

	return TRUE;
}
