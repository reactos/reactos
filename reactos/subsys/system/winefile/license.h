VOID WineLicense(HWND hWnd, LPCSTR lpszLanguage);
VOID WineWarranty(HWND hWnd, LPCSTR language);

typedef struct
{
  LPCSTR License, LicenseCaption;
  LPCSTR Warranty, WarrantyCaption;
} LICENSE;

extern LICENSE WineLicense_Cz;
extern LICENSE WineLicense_Da;
extern LICENSE WineLicense_De;
extern LICENSE WineLicense_En;
extern LICENSE WineLicense_Eo;
extern LICENSE WineLicense_Es;
extern LICENSE WineLicense_Fi;
extern LICENSE WineLicense_Fr;
extern LICENSE WineLicense_Hu;
extern LICENSE WineLicense_It;
extern LICENSE WineLicense_Ko;
extern LICENSE WineLicense_No;
extern LICENSE WineLicense_Pl;
extern LICENSE WineLicense_Po;
extern LICENSE WineLicense_Va;
