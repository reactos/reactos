
DllMain()
{
  /* %%% handle GDI initialization here  */
}

HDC GDICreateDC(LPCWSTR Driver,
                LPCWSTR Device,
                CONST PDEVMODE InitData)
{
  /* %%% initialize device driver here on first call for display DC.  */
}

