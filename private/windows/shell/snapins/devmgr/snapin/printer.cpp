/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    printer.cpp

Abstract:

    This module implements CPrinter -- class that support printing

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "printer.h"
#include "cdriver.h"
#include "sysinfo.h"

const TCHAR* const g_BlankLine = TEXT("");
const TCHAR* const g_NewLine = TEXT("\r\n");
const CHAR*  const g_NewLineA = "\r\n";
//
// CPrinter Class implementation
//

BOOL CPrinter::s_UserAborted = FALSE;
HWND CPrinter::s_hCancelDlg = NULL;


void
CPrintCancelDialog::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BN_CLICKED == HIWORD(wParam) && IDCANCEL == LOWORD(wParam))
        CPrinter::s_UserAborted = TRUE;
}

CPrinter::CPrinter(
    HWND hwndOwner,
    HDC hDC
    )
{
    m_hwndOwner = hwndOwner;
    m_hDC = hDC;
    ASSERT(hDC);
    m_CurLine = 0;
    m_CurPage = 0;
    m_Indent = 0;
    m_Status = 1;
    TEXTMETRIC tm;
    GetTextMetrics(m_hDC, &tm);
    m_yChar = tm.tmHeight + tm.tmExternalLeading;
    m_xChar = tm.tmAveCharWidth;

    //  Give a little room for dot matrix printers.
    m_xMargin = GetDeviceCaps(m_hDC, LOGPIXELSX) * 3 / 4;
    DWORD LinesPerPage;

    LinesPerPage = GetDeviceCaps(m_hDC, VERTRES) / m_yChar;
    m_yBottomMargin = LinesPerPage - 3; // Bottom Margin 3 lines from bottom of page.
    m_CancelDlg.DoModaless(hwndOwner, (LPARAM)&m_CancelDlg);
    s_hCancelDlg = m_CancelDlg.m_hDlg;

    // Set the abort proc to allow cancel
    SetAbortProc(m_hDC, AbortPrintProc);
    
    // four lines for top margin
    m_yTopMargin = 4;
    m_hLogFile = INVALID_HANDLE_VALUE;
}

int
CPrinter::StartDoc(
    LPCTSTR DocTitle
    )
{
    if (m_hDC)
    {
        if (m_hwndOwner)
            ::EnableWindow(m_hwndOwner, FALSE);

        // initialize DOCINFO
        DOCINFO DocInfo;
        DocInfo.cbSize = sizeof(DocInfo);
        TCHAR Temp[MAX_PATH];
        lstrcpy(Temp, DocTitle);
        DocInfo.lpszDocName = Temp;
        //
        DocInfo.lpszOutput = NULL;
        DocInfo.lpszDatatype = NULL;
        DocInfo.fwType = 0;
        m_CurPage = 1;
        m_CurLine = 0;
        m_Status = ::StartDoc(m_hDC, &DocInfo);
    }
    
    else
    {
        if (!DocTitle || _T('\0') == *DocTitle)
            m_Status = 0;
        
        else
        {
            m_hLogFile = CreateFile(DocTitle,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            m_Status = (INVALID_HANDLE_VALUE == m_hLogFile) ? 0 : 1;
        }
    }

    return m_Status;
}

int
CPrinter::EndDoc()
{

    if (m_hDC)
    {
        if (m_hwndOwner)
            ::EnableWindow(m_hwndOwner, TRUE);
        
        if (s_hCancelDlg)
        {
            DestroyWindow(s_hCancelDlg);
            s_hCancelDlg = NULL;
        }
        
        m_Status = ::EndDoc(m_hDC);
    }

    else
    {
        m_Status = 1;
        
        if (INVALID_HANDLE_VALUE != m_hLogFile)
            CloseHandle(m_hLogFile);
        
        else
            m_Status = 0;
        
        m_hLogFile = INVALID_HANDLE_VALUE;
    }

    return m_Status;
}

int
CPrinter::AbortDoc()
{
    if (m_hDC)
    {
        if (m_hwndOwner)
            ::EnableWindow(m_hwndOwner, TRUE);
        
        if (s_hCancelDlg)
        {
            DestroyWindow(s_hCancelDlg);
            s_hCancelDlg = NULL;
        }
        
        m_Status = ::AbortDoc(m_hDC);
    }

    else
    {
        m_Status = 1;
        
        if (INVALID_HANDLE_VALUE != m_hLogFile)
            CloseHandle(m_hLogFile);
        
        else
            m_Status = 0;
        
        m_hLogFile = INVALID_HANDLE_VALUE;
    }

    return m_Status;
}

int
CPrinter::FlushPage()
{
    return PrintLine(NULL);
}

int
CPrinter::PrintLine(
    LPCTSTR LineText
    )
{
    if (INVALID_HANDLE_VALUE != m_hLogFile)
    {
        if (LineText)
        {
            DWORD BytesWritten;
            if (m_Indent)
            {
                int Count = m_Indent * 2;
                CHAR Blanks[MAX_PATH];
                for (int i = 0; i < Count; i++)
                    Blanks[i] = _T(' ');
                WriteFile(m_hLogFile, Blanks, Count * sizeof(CHAR), &BytesWritten, NULL);
            }
#ifdef UNICODE
            int LenW = wcslen(LineText);
            CHAR LineTextA[MAX_PATH];
            int LenA;
            LenA = WideCharToMultiByte(CP_ACP, 0, LineText, LenW, LineTextA, ARRAYLEN(LineTextA), NULL, NULL);
            WriteFile(m_hLogFile, LineTextA, LenA * sizeof(CHAR), &BytesWritten, NULL);
            WriteFile(m_hLogFile, g_NewLineA, strlen(g_NewLineA) * sizeof(CHAR), &BytesWritten, NULL);
#else
            WriteFile(m_hLogFile, LineText, lstrlen(LineText) * sizeof(CHAR), &BytesWritten, NULL);
            WriteFike(m_hLogFile, g_NewLine, lstrlen(g_NewLine) * sizeof(CHAR), &BytesWritten, NULL);
#endif
        }
    }
    
    else
    {
        // !LineText means flush the page
        if ((!LineText && m_CurLine) || (m_CurLine > m_yBottomMargin))
        {
            m_CurLine = 0;
            if (m_Status)
                m_Status = ::EndPage(m_hDC);
        }
        
        if (LineText)
        {
            //if this is the first line and we are still in good shape,
            //start a new page
            if (!m_CurLine && m_Status)
            {
                m_Status = ::StartPage(m_hDC);
                if (m_Status)
                {
                    TCHAR PageTitle[MAX_PATH];
                    wsprintf(PageTitle, (LPCTSTR)m_strPageTitle, m_CurPage);
                    m_CurLine = m_yTopMargin;
                    TextOut(m_hDC, m_xMargin, m_yChar*m_CurLine, PageTitle, lstrlen(PageTitle));
                    // have one blank line right after page title
                    LineFeed();
                    m_CurLine++;
                    m_CurPage++;
                }
            }
            
            if (m_Status)
                TextOut(m_hDC, m_xMargin + m_xChar*m_Indent*2, m_yChar*m_CurLine, LineText, lstrlen(LineText));
            
            m_CurLine++;
        }
    }

    return m_Status;
}

inline
void
CPrinter::LineFeed()
{
    PrintLine(g_BlankLine);
}

// the abort procedure
BOOL CALLBACK
AbortPrintProc(
    HDC hDC,
    int nCode
    )
{
    MSG msg;

    while (!CPrinter::s_UserAborted && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (!IsDialogMessage(CPrinter::s_hCancelDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return !CPrinter::s_UserAborted;
}

//
// This function prints system summary.
// INPUT:
//      Machine -- the machine
// OUTPUT:
//      0 -- failed else succeeded.
//
//

int
CPrinter::PrintSystemSummary(
    CMachine& Machine
    )
{
    CSystemInfo SysInfo;
    TCHAR Line[MAX_PATH];
    TCHAR Buffer[MAX_PATH];
    TCHAR Format[MAX_PATH];
    TCHAR Unknown[30];
    DWORD Size, BufferSize;
    BufferSize = ARRAYLEN(Buffer);
    // preload the "Unknown" string which will be used as default when
    // the corresponding value can not found
    LoadString(g_hInstance, IDS_PRINT_UNKNOWN, Unknown, ARRAYLEN(Unknown));


    //
    // print System summary heading
    //
    LoadString(g_hInstance, IDS_PRINT_HEADING_SYSSUMMARY, Buffer, ARRAYLEN(Buffer));
    LoadString(g_hInstance, IDS_PRINT_BANNER, Format, ARRAYLEN(Format));
    wsprintf(Line, Format, Buffer);
    PrintLine(Line);
    LineFeed();

    //
    // Windows version
    //
    LoadString(g_hInstance, IDS_PRINT_WINVER, Line, ARRAYLEN(Line));
    Size = SysInfo.WindowsVersion(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);
    
    //
    // Registered Owner
    //
    LoadString(g_hInstance, IDS_PRINT_OWNER, Line, ARRAYLEN(Line));
    Size = SysInfo.RegisteredOwner(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);
    
    //
    // Registered Organization
    //
    LoadString(g_hInstance, IDS_PRINT_ORGANIZATION, Line, ARRAYLEN(Line));
    Size = SysInfo.RegisteredOrganization(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);
    
    //
    // Computer name
    //
    LoadString(g_hInstance, IDS_PRINT_COMPUTERNAME, Line, ARRAYLEN(Line));
    lstrcat(Line, SysInfo.ComputerName());
    PrintLine(Line);
    
    //
    // Machine Type
    //
    LoadString(g_hInstance, IDS_PRINT_MACHINE_TYPE, Line, ARRAYLEN(Line));
    Size = SysInfo.MachineType(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);
    
    //
    // System BIOS Name
    //
    LoadString(g_hInstance, IDS_PRINT_SYSBIOS_NAME, Line, ARRAYLEN(Line));
    Size = SysInfo.SystemBiosName(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);
    
    //
    // System BIOS Date
    //
    LoadString(g_hInstance, IDS_PRINT_SYSBIOS_DATE, Line, ARRAYLEN(Line));
    Size = SysInfo.SystemBiosDate(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);
    
    //
    // System BIOS Version
    //
    LoadString(g_hInstance, IDS_PRINT_SYSBIOS_VERSION, Line, ARRAYLEN(Line));
    Size = SysInfo.SystemBiosVersion(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);

    //
    // Processor type
    //
    LoadString(g_hInstance, IDS_PRINT_PROCESSOR_TYPE, Line, ARRAYLEN(Line));
    Size = SysInfo.ProcessorType(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);

    //
    // Processor vendor
    //
    LoadString(g_hInstance, IDS_PRINT_PROCESSOR_VENDOR, Line, ARRAYLEN(Line));
    Size = SysInfo.ProcessorVendor(Buffer, BufferSize);
    lstrcat(Line, Size ? Buffer : Unknown);
    PrintLine(Line);

    //
    // Number of processors
    //
    LoadString(g_hInstance, IDS_PRINT_PROCESSORS, Line, ARRAYLEN(Line));
    DWORD NumProcessors = SysInfo.NumberOfProcessors();
    if (NumProcessors)
    {
        wsprintf(Buffer, TEXT("%u"), NumProcessors);
        lstrcat(Line, Buffer);
    }
    else
    {
        lstrcat(Line, Unknown);
    }
    PrintLine(Line);
    
    //
    // Total physical memory
    //
    ULARGE_INTEGER MemorySize;
    SysInfo.TotalPhysicalMemory(MemorySize);
    LoadString(g_hInstance, IDS_PRINT_PHY_MEMORY, Line, ARRAYLEN(Line));
    if (MemorySize.QuadPart)
    {
        LoadString(g_hInstance, IDS_PRINT_MEMORY_UNIT, Format, ARRAYLEN(Format));
        MemorySize.QuadPart += 1024*1024 - 1;
        wsprintf(Buffer, Format, Int64ShrlMod32(MemorySize.QuadPart, 20));
        lstrcat(Line, Buffer);
    }
    else
    {
        lstrcat(Line, Unknown);
    }
    PrintLine(Line);
    LineFeed();
    
    //
    //  Local disk drive information
    //
    // Print Disk info summary heading
    LoadString(g_hInstance, IDS_PRINT_HEADING_DISKINFO, Buffer, ARRAYLEN(Buffer));
    LoadString(g_hInstance, IDS_PRINT_BANNER, Format, ARRAYLEN(Format));
    wsprintf(Line, Format, Buffer);
    PrintLine(Line);
    LineFeed();

    DISK_INFO DiskInfo;
    DiskInfo.cbSize = sizeof(DiskInfo);
    for(int Drive = 0; Drive < 25; Drive++)
    {
        // information we want to report on the drive:
        // (1). drive letter and type
        // (2). Total space
        // (3). Free space(if available)
        // (4). Cylinders
        // (5). Heads
        // (6). Sectors per track
        // (7). Bytes per sector
        Indent();
        if(SysInfo.GetDiskInfo(Drive, DiskInfo))
        {
            TCHAR DriveLetter;
            DriveLetter = Drive + _T('A');
            LoadString(g_hInstance, IDS_PRINT_DRIVE_LETTER, Format, ARRAYLEN(Format));
            wsprintf(Line, Format, DriveLetter);
            PrintLine(Line);
            Indent();
            
            //
            // Drive type
            //
            LoadString(g_hInstance, IDS_PRINT_DRIVE_TYPE, Format, ARRAYLEN(Format));
            int StringId;
            LoadString(g_hInstance,
                       IDS_MEDIA_BASE + (int)DiskInfo.MediaType,
                       Buffer, ARRAYLEN(Buffer));
            wsprintf(Line, Format, Buffer);
            PrintLine(Line);
            
            //
            //Total and free space
            //
            if (DiskInfo.TotalSpace.HighPart)
            {
                // we have a big disk
                LoadString(g_hInstance, IDS_PRINT_TOTAL_SPACE_XL, Format, ARRAYLEN(Format));
                wsprintf(Line, Format, DiskInfo.TotalSpace.HighPart,
                         DiskInfo.TotalSpace.LowPart);
                PrintLine(Line);
                if (-1 != DiskInfo.FreeSpace.QuadPart)
                {
                    LoadString(g_hInstance, IDS_PRINT_FREE_SPACE_XL, Format, ARRAYLEN(Format));
                    wsprintf(Line, Format, DiskInfo.FreeSpace.HighPart,
                             DiskInfo.FreeSpace.LowPart);
                    PrintLine(Line);
                }
            }
            
            else
            {
                LoadString(g_hInstance, IDS_PRINT_TOTAL_SPACE, Format, ARRAYLEN(Format));
                wsprintf(Line, Format, DiskInfo.TotalSpace.LowPart);
                PrintLine(Line);
                if (-1 != DiskInfo.FreeSpace.QuadPart)
                {
                    LoadString(g_hInstance, IDS_PRINT_FREE_SPACE, Format, ARRAYLEN(Format));
                    wsprintf(Line, Format, DiskInfo.FreeSpace.LowPart);
                    PrintLine(Line);
                }
            }
            
            //
            // Disk physical dimension
            // skip CD-ROM because the dimension it reports is bogus
            if (DRIVE_CDROM != DiskInfo.DriveType)
            {
                //
                // Heads
                //
                LoadString(g_hInstance, IDS_PRINT_HEADS, Format, ARRAYLEN(Format));
                wsprintf(Line, Format, DiskInfo.Heads);
                PrintLine(Line);
                
                //
                // Cylinders
                //
                if (DiskInfo.Cylinders.HighPart)
                {
                    LoadString(g_hInstance, IDS_PRINT_CYLINDERS_XL, Format, ARRAYLEN(Format));
                    wsprintf(Line, Format, DiskInfo.Cylinders.HighPart,
                             DiskInfo.Cylinders.LowPart);
                    PrintLine(Line);
                }
                
                else
                {
                    LoadString(g_hInstance, IDS_PRINT_CYLINDERS, Format, ARRAYLEN(Format));
                    wsprintf(Line, Format, DiskInfo.Cylinders.LowPart);
                    PrintLine(Line);
                }
                
                //
                // Sectors per track
                //
                LoadString(g_hInstance, IDS_PRINT_TRACKSIZE, Format, ARRAYLEN(Format));
                wsprintf(Line, Format, DiskInfo.SectorsPerTrack);
                PrintLine(Line);
                
                //
                // Bytes per sector
                //
                LoadString(g_hInstance, IDS_PRINT_SECTORSIZE, Format, ARRAYLEN(Format));
                wsprintf(Line, Format, DiskInfo.BytesPerSector);
                PrintLine(Line);
            }

            UnIndent();
            LineFeed();
        }

        UnIndent();
    }

    return 1;
}

int
CPrinter::PrintResourceSummary(
    CMachine& Machine
    )
{
    TCHAR Temp[MAX_PATH];
    CResource* pResource;
    String str;
    String strBanner;

    PrintSystemSummary(Machine);

    //
    // print IRQ summary heading
    //
    str.LoadString(g_hInstance, IDS_PRINT_HEADING_IRQSUMMARY);
    strBanner.LoadString(g_hInstance, IDS_PRINT_BANNER);
    wsprintf(Temp, (LPCTSTR)strBanner, (LPCTSTR)str);
    PrintLine(Temp);
    LineFeed();
    CResourceList IrqSummary(&Machine, ResType_IRQ);
    if (IrqSummary.GetCount())
    {
        CResource* pResRoot;
        IrqSummary.CreateResourceTree(&pResRoot);
        str.LoadString(g_hInstance, IDS_PRINT_IRQSUM);
        PrintLine(str);
        Indent();
        PrintResourceSubtree(pResRoot);
        UnIndent();
        LineFeed();
    }

    //
    // print DMA summary heading
    //
    str.LoadString(g_hInstance, IDS_PRINT_HEADING_DMASUMMARY);
    wsprintf(Temp, (LPCTSTR)strBanner, (LPCTSTR)str);
    PrintLine(Temp);
    LineFeed();
    CResourceList DmaSummary(&Machine, ResType_DMA);
    if (DmaSummary.GetCount())
    {
        CResource* pResRoot;
        DmaSummary.CreateResourceTree(&pResRoot);
        str.LoadString(g_hInstance, IDS_PRINT_DMASUM);
        PrintLine(str);
        Indent();
        PrintResourceSubtree(pResRoot);
        UnIndent();
        LineFeed();
    }

    //
    // print MEM summary heading
    //
    str.LoadString(g_hInstance, IDS_PRINT_HEADING_MEMSUMMARY);
    wsprintf(Temp, (LPCTSTR)strBanner, (LPCTSTR)str);
    PrintLine(Temp);
    LineFeed();
    CResourceList MemSummary(&Machine, ResType_Mem);
    if (MemSummary.GetCount())
    {
        CResource* pResRoot;
        MemSummary.CreateResourceTree(&pResRoot);
        str.LoadString(g_hInstance, IDS_PRINT_MEMSUM);
        PrintLine(str);
        Indent();
        PrintResourceSubtree(pResRoot);
        UnIndent();
        LineFeed();
    }

    //
    // print IO summary heading
    //
    str.LoadString(g_hInstance, IDS_PRINT_HEADING_IOSUMMARY);
    wsprintf(Temp, (LPCTSTR)strBanner, (LPCTSTR)str);
    PrintLine(Temp);
    LineFeed();
    CResourceList IoSummary(&Machine, ResType_IO);
    if (IoSummary.GetCount())
    {
        CResource* pResRoot;
        IoSummary.CreateResourceTree(&pResRoot);
        str.LoadString(g_hInstance, IDS_PRINT_IOSUM);
        PrintLine(str);
        Indent();
        PrintResourceSubtree(pResRoot);
        UnIndent();
        LineFeed();
    }

    return 1;
}

int
CPrinter::PrintResourceSubtree(
    CResource* pResRoot
    )
{
    while (pResRoot)
    {
        DWORD Status, Problem;
        
        if (pResRoot->m_pDevice->GetStatus(&Status, &Problem) && Problem ||
            pResRoot->m_pDevice->IsDisabled())
        {
            TCHAR Temp[MAX_PATH];
            Temp[0] = _T('*');
            lstrcpy(&Temp[1], pResRoot->GetDisplayName());
            PrintLine(Temp);
        }

        else
        {
            PrintLine(pResRoot->GetViewName());
        }

        if (pResRoot->GetChild())
        {
            if ((ResType_IO == pResRoot->ResType()) ||
                (ResType_Mem == pResRoot->ResType())) {
            
                Indent();
            }

            PrintResourceSubtree(pResRoot->GetChild());
            
            if ((ResType_IO == pResRoot->ResType()) ||
                (ResType_Mem == pResRoot->ResType())) {
            
                UnIndent();
            }
        }

        pResRoot = pResRoot->GetSibling();
    }

    return 1;
}

int
CPrinter::PrintAllClassAndDevice(
    CMachine* pMachine
    )
{

    if (!pMachine)
        return 0;
    
    String strHeading;
    String strBanner;
    TCHAR       Temp[MAX_PATH];
    strHeading.LoadString(g_hInstance, IDS_PRINT_HEADING_SYSDEVINFO);
    strBanner.LoadString(g_hInstance, IDS_PRINT_BANNER);
    wsprintf(Temp, (LPCTSTR)strBanner, (LPCTSTR)strHeading);
    PrintLine(Temp);
    LineFeed();

    CClass* pClass;
    PVOID Context;
    
    if (pMachine->GetFirstClass(&pClass, Context))
    {
        do
        {
            PrintClass(pClass, FALSE);
        }while (pMachine->GetNextClass(&pClass, Context));
    }

    return 1;
}

int
CPrinter::PrintClass(
    CClass* pClass,
    BOOL PrintBanner
    )
{
    PVOID Context;
    CDevice* pDevice;

    if (!pClass)
        return 0;

    if (PrintBanner)
    {
        String strHeading;
        String strBanner;
        TCHAR  Temp[MAX_PATH];
        strHeading.LoadString(g_hInstance, IDS_PRINT_HEADING_SYSDEVCLASS);
        strBanner.LoadString(g_hInstance, IDS_PRINT_BANNER);
        wsprintf(Temp, (LPCTSTR)strBanner, (LPCTSTR)strHeading);
        PrintLine(Temp);
        LineFeed();
    }
    
    if (pClass && pClass->GetFirstDevice(&pDevice, Context))
    {
        do
        {
            // do print banner on the device
            PrintDevice(pDevice, FALSE);
        }while (pClass->GetNextDevice(&pDevice, Context));
    }
    
    return 1;
}

int
CPrinter::PrintDevice(
    CDevice* pDevice,
    BOOL PrintBanner
    )
{

    if (!pDevice)
        return 0;
    
    String str;
    TCHAR Temp[MAX_PATH];

    if (PrintBanner)
    {
        String strBanner;
        str.LoadString(g_hInstance, IDS_PRINT_HEADING_SYSDEVICE);
        strBanner.LoadString(g_hInstance, IDS_PRINT_BANNER);
        wsprintf(Temp, (LPCTSTR)strBanner, (LPCTSTR)str);
        PrintLine(Temp);
        LineFeed();
    }
    
    DWORD Status, Problem;
    
    if (pDevice->GetStatus(&Status, &Problem) && Problem ||
        pDevice->IsDisabled())
    {
        TCHAR Temp[MAX_PATH];
        LoadString(g_hInstance, IDS_PRINT_DEVICE_DISABLED, Temp, ARRAYLEN(Temp));
        PrintLine(Temp);
    }
    
    str.LoadString(g_hInstance, IDS_PRINT_CLASS);
    wsprintf(Temp, (LPCTSTR)str, pDevice->GetClassDisplayName());
    PrintLine(Temp);
    str.LoadString(g_hInstance, IDS_PRINT_DEVICE);
    wsprintf(Temp, (LPCTSTR)str, pDevice->GetDisplayName());
    PrintLine(Temp);
    PrintDeviceResource(pDevice);
    PrintDeviceDriver(pDevice);
    return 1;
}

int
CPrinter::PrintAll(
    CMachine& Machine
    )
{
    PrintResourceSummary(Machine);
    PrintAllClassAndDevice(&Machine);
    return 1;
}

//
// This function prints the given device's resource summary
//
int
CPrinter::PrintDeviceResource(
    CDevice* pDevice
    )
{

    if (!pDevice)
        return 0;

    CResourceList IrqSummary(pDevice, ResType_IRQ);
    CResourceList DmaSummary(pDevice, ResType_DMA);
    CResourceList MemSummary(pDevice, ResType_Mem);
    CResourceList IoSummary(pDevice, ResType_IO);

    String str;
    TCHAR Temp[MAX_PATH];

    // if the device has any kind of resources, print it
    if (IrqSummary.GetCount() || DmaSummary.GetCount() ||
        MemSummary.GetCount() || IoSummary.GetCount())
    {
        str.LoadString(g_hInstance, IDS_PRINT_RESOURCE);
        PrintLine(str);
        // start printing individual resources
        Indent();
        PVOID Context;
        CResource* pResource;
        DWORDLONG dlBase, dlLen;
        TCHAR Format[MAX_PATH];
        if (IrqSummary.GetFirst(&pResource, Context))
        {
            LoadString(g_hInstance, IDS_PRINT_IRQ_FORMAT, Temp, ARRAYLEN(Temp));
            do
            {
                pResource->GetValue(&dlBase, &dlLen);
                str.Format(Temp, (ULONG)dlBase);
                PrintLine((LPCTSTR)str);
            }while (IrqSummary.GetNext(&pResource, Context));
        }

        if (DmaSummary.GetFirst(&pResource, Context))
        {
            LoadString(g_hInstance, IDS_PRINT_DMA_FORMAT, Temp, ARRAYLEN(Temp));
            do
            {
                pResource->GetValue(&dlBase, &dlLen);
                str.Format(Temp, (ULONG)dlBase);
                PrintLine((LPCTSTR)str);
            }while (DmaSummary.GetNext(&pResource, Context));
        }

        if (MemSummary.GetFirst(&pResource, Context))
        {
            LoadString(g_hInstance, IDS_PRINT_MEM_FORMAT, Temp, ARRAYLEN(Temp));
            do
            {
                pResource->GetValue(&dlBase, &dlLen);
                str.Format(Temp, (ULONG)dlBase, (ULONG)(dlBase + dlLen - 1));
                PrintLine((LPCTSTR)str);
            }while (MemSummary.GetNext(&pResource, Context));
        }

        if (IoSummary.GetFirst(&pResource, Context))
        {
            LoadString(g_hInstance, IDS_PRINT_IO_FORMAT, Temp, ARRAYLEN(Temp));
            do
            {
                pResource->GetValue(&dlBase, &dlLen);
                str.Format(Temp, (ULONG)dlBase, (ULONG)(dlBase + dlLen -1));
                PrintLine((LPCTSTR)str);
            }while (IoSummary.GetNext(&pResource, Context));
        }

        UnIndent();
    }

    else
    {
        str.LoadString(g_hInstance, IDS_PRINT_NORES);
        PrintLine(str);
    }

    return 1;
}


//
// This function prints the given device's driver information
// INPUT:
//      pDevice  -- the device
// OUTPUT:
//      >0 if the function succeeded.
//      0 if the function failed.
//
int
CPrinter::PrintDeviceDriver(
    CDevice* pDevice
    )
{
    if (!pDevice)
        return 0;

    String str;
    TCHAR Temp[MAX_PATH];

    CDriver* pDriver;
    pDriver = pDevice->CreateDriver();
    SafePtr<CDriver> DrvPtr;
    
    if (pDriver)
    {
        DrvPtr.Attach(pDriver);
        str.LoadString(g_hInstance, IDS_PRINT_DRVINFO);
        PrintLine(str);
        PVOID Context;
        CDriverFile* pDrvFile;
        Indent();
        
        if (pDriver->GetFirstDriverFile(&pDrvFile, Context))
        {
            do
            {
                PrintLine(pDrvFile->GetFullPathName());
                HANDLE hFile;
                Indent();
                hFile = CreateFile(pDrvFile->GetFullPathName(),
                                   GENERIC_READ,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL |
                                   FILE_ATTRIBUTE_READONLY |
                                   FILE_ATTRIBUTE_SYSTEM |
                                   FILE_ATTRIBUTE_HIDDEN,
                                   NULL
                                   );
                
                if (INVALID_HANDLE_VALUE != hFile)
                {
                    DWORD FileSize;
                    FileSize = ::GetFileSize(hFile, NULL);
                    CloseHandle(hFile);
                    LoadString(g_hInstance, IDS_PRINT_FILESIZE, Temp, ARRAYLEN(Temp));
                    str.Format(Temp, FileSize);
                    PrintLine(str);
                    // print the driver version infomation
                    TCHAR Unknown[MAX_PATH];
                    LoadString(g_hInstance, IDS_PRINT_UNKNOWN, Unknown, ARRAYLEN(Unknown));
                    if (pDrvFile->HasVersionInfo())
                    {
                        LoadString(g_hInstance, IDS_PRINT_FILEVERSION, Temp, ARRAYLEN(Temp));
                        if (pDrvFile->GetVersion())
                        {
                            str.Format(Temp, pDrvFile->GetVersion());
                        }
                        
                        else
                        {
                            str.Format(Temp, Unknown);
                        }
                        
                        PrintLine(str);

                        LoadString(g_hInstance, IDS_PRINT_FILEMFG, Temp, ARRAYLEN(Temp));
                        
                        if (pDrvFile->GetProvider())
                        {
                            str.Format(Temp, pDrvFile->GetProvider());
                        }
                        
                        else
                        {
                            str.Format(Temp, Unknown);
                        }
                        
                        PrintLine(str);

                        LoadString(g_hInstance, IDS_PRINT_FILECOPYRIGHT, Temp, ARRAYLEN(Temp));
                        
                        if (pDrvFile->GetCopyright())
                        {
                            str.Format(Temp, pDrvFile->GetCopyright());
                        }
                        
                        else
                        {
                            str.Format(Temp, Unknown);
                        }
                        
                        PrintLine(str);
                    }

                    else
                    {
                        str.LoadString(g_hInstance, IDS_PRINT_NOVERSION);
                        PrintLine(str);
                    }
                }

                else
                {
                    str.LoadString(g_hInstance, IDS_PRINT_DRVMISSING);
                    PrintLine(str);
                }
                
                UnIndent();
            }while (pDriver->GetNextDriverFile(&pDrvFile, Context));
        }

        UnIndent();
    }

    LineFeed();
    
    return 1;
}
