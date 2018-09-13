// CODE WHICH I REMOVED BUT THAT I'M AFRAID TO THROW AWAY IN CASE I NEED IT
//
// NOT IN THE BUILD
 
 
/*++ CPerfPage::DrawLegend

Routine Description:

    Draws the legend on the performance page

Arguments:

    lpdi    - LPDRAWITEMSTRUCT describing area we need to paint

Return Value:

Revision History:

      Jan-18-95 Davepl  Created

--*/

void CPerfPage::DrawLegend(LPDRAWITEMSTRUCT lpdi)
{
          int xPos    = 10;     // X pos for drawing
    const int yLine   = 6;      // Y pos for drawing the lines
    const int yText   = 0;      // Y pos for drawing the text
    const int LineLen = 10;     // Length of legend lines

    FillRect(lpdi->hDC, &lpdi->rcItem, (HBRUSH) GetStockObject(GRAPH_BRUSH));
    SetBkColor(lpdi->hDC, RGB(0,0,0));

    SetTextColor(lpdi->hDC, aColors[MEM_PEN]);
    SelectObject(lpdi->hDC, m_hPens[MEM_PEN]);

    MoveToEx(lpdi->hDC, xPos, yLine, (LPPOINT) NULL);
    xPos += LineLen;
    LineTo(lpdi->hDC, xPos, yLine);
    xPos += 5;

    xPos = TextToLegend(lpdi->hDC, xPos, yText, g_szMemUsage) + 10;

    {
        static const LPCTSTR pszLabels[2] = { g_szTotalCPU, g_szKernelCPU };

        for (int i = 0; i < 2; i++)
        {
            SetTextColor(lpdi->hDC, aColors[i]);
            SelectObject(lpdi->hDC, m_hPens[i]);

            MoveToEx(lpdi->hDC, xPos, yLine, (LPPOINT) NULL);
            xPos += LineLen;
            LineTo(lpdi->hDC, xPos, yLine);
            xPos += 5;

            xPos = TextToLegend(lpdi->hDC, xPos, yText, pszLabels[i]) + 10;

            // Don't both with the kernel legend unless needed

            if (FALSE == g_Options.m_fKernelTimes)
            {
                break;
            }
        }
    }
}
