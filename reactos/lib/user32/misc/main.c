
#include <windows.h>
#include <stdio.h>


LRESULT CALLBACK WindowFunc(HWND,UINT,WPARAM, LPARAM);


int main(int argc, char **argv)
{
	

	WINPOS_CreateInternalPosAtom();
	SYSCOLOR_Init();
        WIDGETS_Init();
	ICONTITLE_Init();
	DIALOG_Init();
	COMBO_Init();
	MENU_Init();

	MessageBox(NULL,"xxx","yyyy",MB_OK);

	return 0;

}


	



LRESULT CALLBACK WindowFunc(HWND hwnd,UINT message,WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
