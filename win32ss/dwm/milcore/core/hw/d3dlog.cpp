// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Debug dump per-frame info.
//
//      Output file[s]: c:\d3dlogN.xaml (N=0,1,2...)
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#if DBG

static char s_dump_filename[] = "c:\\d3dlog0.txt";

DeclareTagEx(tagD3DLog, "MIL-HW", "Dump D3D Log", FALSE);

char const* const
CD3DLog::m_frameNames[D3DLogFrame::size] =
{
#define D3DLOG_NAME(t,d) d,
        D3DLOG_FIELDS(D3DLOG_NAME)

};

void CD3DLog::Dump()
{
    if (!IsTagEnabled(tagD3DLog)) return;
    errno_t err;
    err = fopen_s(&m_pFile, s_dump_filename, "wt");
    if (err != 0) return;
    s_dump_filename[9]++;
    DumpHead();
    for (D3DLogFrame* pFrame = m_data; pFrame != m_pCurrentFrame; pFrame++)
    {
        DumpRow(pFrame);
    }
    DumpTail();
    fclose(m_pFile);
}

void CD3DLog::DumpHead()
{
    fprintf(m_pFile, "%s\n", "milrender dbg d3dlog dump");

    static const int N = D3DLogFrame::size;

    int counts[N];
    int lengths[N];

    {
        for (int i = 0; i < N; i++)
        {
            counts[i] = 0;
            lengths[i] = static_cast<int>(strlen(CD3DLog::m_frameNames[i]));
        }
    }
    
    for (bool fNeedMoreLines = true; fNeedMoreLines;)
    {
        fNeedMoreLines = false;

        for (int i = 0; i < N; i++)
        {
            int &count = counts[i];
            int length = lengths[i];
            for (int j = 0; j < 9; j++)
            {
                fprintf(m_pFile, "%c", count < length ? CD3DLog::m_frameNames[i][count++] : ' ');
            }

            fprintf(m_pFile, "%c", ' ');
            fNeedMoreLines = fNeedMoreLines || (count < length);
        }
        fprintf(m_pFile, "%c", '\n');
    }
}

void CD3DLog::DumpTail()
{
    fprintf(m_pFile, "%s\n", "--------------------------------");
}


void CD3DLog::DumpRow(__in_ecount(1) const D3DLogFrame* pFrame)
{
    static const int N = D3DLogFrame::size;

    for (int i = 0; i < N; i++)
    {
        fprintf(m_pFile, "%-9d ", pFrame->data[i]);

    }
    fprintf(m_pFile, "%c", '\n');
}


#endif //DBG


