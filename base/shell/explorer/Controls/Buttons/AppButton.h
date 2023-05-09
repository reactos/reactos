class StartButton : public CWindowImpl<StartButton>
{
    HIMAGELIST m_ImageList;
    SIZE m_Size;
    HFONT m_Font;
    HBITMAP hBmp;

  public:
    StartButton() : m_ImageList(NULL), m_Font(NULL)
    {
        m_Size.cx = 0;
        m_Size.cy = 0;
    }

    virtual ~StartButton()
    {
        if (m_ImageList != NULL)
            ImageList_Destroy(m_ImageList);

        if (m_Font != NULL)
            DeleteObject(m_Font);
    }

    SIZE
    GetSize()
    {
        return m_Size;
    }

    VOID
    UpdateSize()
    {
        SIZE Size = {0, 0};

        if (m_ImageList == NULL || !SendMessageW(BCM_GETIDEALSIZE, 0, (LPARAM)&Size))
        {
            Size.cx = 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CYCAPTION) * 3;
        }

        Size.cy = max(Size.cy, GetSystemMetrics(SM_CYCAPTION));
        Size.cx = max(Size.cy, GetSystemMetrics(SM_CYCAPTION));
        /* Save the size of the start button */
        m_Size = Size;
    }

    VOID
    UpdateFont()
    {
        ///* Get the system fonts, we use the caption font, always bold, though. */
        //NONCLIENTMETRICS ncm = {sizeof(ncm)};
        //if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
        //    return;

        //if (m_Font)
        //    DeleteObject(m_Font);

        //ncm.lfCaptionFont.lfWeight = FW_BOLD;
        //m_Font = CreateFontIndirect(&ncm.lfCaptionFont);

        //SetFont(m_Font, FALSE);
    }

    VOID
    Initialize()
    {
        // HACK & FIXME: CORE-18016
        HWND hWnd = m_hWnd;
        m_hWnd = NULL;
        SubclassWindow(hWnd);

        SetWindowTheme(m_hWnd, L"Start", NULL);

       // m_ImageList = ImageList_LoadImageW(
          //  hExplorerInstance, MAKEINTRESOURCEW(IDB_START), 0, 0, 0, IMAGE_BITMAP,
          //  LR_LOADTRANSPARENT | LR_CREATEDIBSECTION);
       // ImageList_AddMasked(hImageList, mappedBitmap, RGB(0, 0, 0));
       // SendMessage(m_hWnd, TB_SETIMAGELIST, 0, (LPARAM)m_ImageList);  
        BUTTON_IMAGELIST bil = {m_ImageList, {1, 1, 1, 1}, BUTTON_IMAGELIST_ALIGN_CENTER};
        SendMessageW(BCM_SETIMAGELIST, 0, (LPARAM)&bil);



      
        if (hBmp == NULL)
        {
           // MessageBox(NULL, "Error while loading image", "Error", MB_OK | MB_ICONERROR);
        }
        else
        {
           // SendMessage(m_hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);

        }
        

         UpdateSize();
    }

    HWND
    Create(HWND hwndParent, HIMAGELIST imgList, HBITMAP hBmp2)
    {
        // WCHAR szStartCaption[0];
        /* if (!LoadStringW(hExplorerInstance,
                          IDS_START,
                          szStartCaption,
                          _countof(szStartCaption)))
         {
             wcscpy(szStartCaption, L"START");
         }*/
         m_ImageList = imgList;
         hBmp = hBmp2;
         DWORD dwStyle =
             WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_CENTER | BS_BITMAP | BS_VCENTER | BS_FLAT;

        // HACK & FIXME: CORE-18016
        m_hWnd = CreateWindowEx(
            0, WC_BUTTON, nullptr, dwStyle, 0, 0, 40, 40, hwndParent, (HMENU)1, hExplorerInstance, NULL);

        if (m_hWnd)
            Initialize();

        return m_hWnd;
    }

    LRESULT
    OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (uMsg == WM_KEYUP && wParam != VK_SPACE)
            return 0;

        GetParent().PostMessage(TWM_OPENSTARTMENU);
        return 0;
    }
    //LRESULT
    //OnLWM_DRAWITEM(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    //{
    // 
    //    LPDRAWITEMSTRUCT Item = (LPDRAWITEMSTRUCT)lParam;

    //   
    //        FillRect(Item->hDC, &Item->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));

    //   
    //    return 0;
    //}

    BEGIN_MSG_MAP(StartButton)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
 /*   MESSAGE_HANDLER(WM_DRAWITEM, OnLWM_DRAWITEM)*/
    END_MSG_MAP()
};
