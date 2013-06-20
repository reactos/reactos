INT
Test_NtUserGetTitleBarInfo(PTESTINFO pti)
{
   HWND hWnd;
   TITLEBARINFO tbi;

   hWnd = CreateWindowA("BUTTON",
      "Test",
      BS_PUSHBUTTON | WS_VISIBLE,
      0,
      0,
      50,
      30,
      NULL,
      NULL,
      g_hInstance,
      0);

   ASSERT(hWnd);

   /* FALSE case */
   /* no windows handle */
   TEST(NtUserGetTitleBarInfo(NULL, &tbi) == FALSE);
   /* no TITLEBARINFO struct */
   TEST(NtUserGetTitleBarInfo(hWnd, NULL) == FALSE);
   /* nothing */
   TEST(NtUserGetTitleBarInfo(NULL, NULL) == FALSE);
   /* wrong size */
   tbi.cbSize = 0;
   TEST(NtUserGetTitleBarInfo(hWnd, &tbi) == FALSE);

   /* TRUE case */
   tbi.cbSize = sizeof(TITLEBARINFO);
   TEST(NtUserGetTitleBarInfo(hWnd, &tbi) == TRUE);

   DestroyWindow(hWnd);

   return APISTATUS_NORMAL;
}

