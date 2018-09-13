//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       imgxbm.cxx
//
//  Contents:   Image filter for .xbm files
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_IMGBITS_HXX_
#define X_IMGBITS_HXX_
#include "imgbits.hxx"
#endif

#define XX_DMsg(x, y)

#define MAX_LINE 512

unsigned const char nibMask[8] =
{
    1, 2, 4, 8, 16, 32, 64, 128
};

MtDefine(CImgTaskXbm, Dwn, "CImgTaskXbm")

class CImgTaskXbm : public CImgTask
{

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskXbm))

    typedef CImgTask super;
    virtual void Decode(BOOL *pfNonProgressive);
};

static
char * next_token(char ** ppsz)
{
    char * pszBeg = *ppsz;
    char * pszEnd;

    // Skip leading whitespace, if any

    while (*pszBeg == ' ' || *pszBeg == '\t')
        ++pszBeg;

    if (*pszBeg == 0)
    {
        *ppsz = NULL;
        return(NULL);
    }

    // Find end of token
    
    pszEnd = pszBeg;

    while (*pszEnd && *pszEnd != ' ' && *pszEnd != '\t')
        ++pszEnd;

    if (*pszEnd)
        *pszEnd++ = 0;
        
    *ppsz = pszEnd;

    return(pszBeg);
}

void CImgTaskXbm::Decode(BOOL *pfNonProgressive)
{
    char line[MAX_LINE+2], *name_and_type;
    char *t;
    unsigned char *ptr;
    long bytes_per_line, version10p, raster_length, padding, win_extra_bytes_per_line;
    int bytes, temp = 0, value;
    int Ncolors, charspp, xpmformat;
    int line_idx = 0;
    char lookahead = 0;
    int n;
    char *tok;
    char *end;
    char *pszTok;
    CImgBitsDIB *pibd;

    *pfNonProgressive = TRUE;

    _xWid = 0;
    _yHei = 0;
    Ncolors = 0;
    charspp = 0;
    xpmformat = 0;
    for (;;)
    {
        line_idx = 0;
        if (lookahead)
        {
            line[0] = lookahead;
            lookahead = 0;
            line_idx = 1;
        }

        for (; line_idx < MAX_LINE; line_idx++)
        {
            if (!Read(&line[line_idx], 1))
                return;
            if (line[line_idx] == '\n' || line[line_idx] == '\r' || line[line_idx] == '{')
            {
                if (!Read(&line[line_idx], 1))
                    return;
                if (line[line_idx] != '\n' && line[line_idx] != '\r')
                    lookahead = line[line_idx];
                break;
            }
        }
        line[line_idx] = '\0';

        pszTok = line;
        tok = next_token(&pszTok);
        if (!tok)
            return;
        if (!strcmp(tok, "#define"))
        {
            name_and_type = next_token(&pszTok);
            if (!name_and_type)
                continue;
            if (NULL == (t = strrchr(name_and_type, '_')))
                t = name_and_type;
            else
                t++;

            tok = next_token(&pszTok);
            if (!tok)
                continue;
            value = strtol(tok, &end, 10);

            if (!strcmp("width", t))
                _xWid = value;
            else if (!strcmp("height", t))
                _yHei = value;
            else if (!strcmp("ncolors", t))
                Ncolors = value;
            else if (!strcmp("pixel", t))
                charspp = value;
            continue;
        }
        if (!strcmp(tok, "static"))
        {
            t = next_token(&pszTok);
            if (!t)
                continue;
            if (!strcmp(t, "unsigned"))
            {
                t = next_token(&pszTok);
                if (!t)
                    continue;
            }
            if (!strcmp(t, "short"))
            {
                version10p = 1;
                break;
            }
            else if (!strcmp(t, "char"))
            {
                version10p = 0;
                t = next_token(&pszTok);
                if (*t == '*')
                    xpmformat = 1;
                break;
            }
        }
        else
            continue;
    }
    if (version10p)
    {
        XX_DMsg(DBG_IMAGE, ("Don't do old version 10p xbm images!\n"));
        return;
    }
    if (xpmformat)
    {
        XX_DMsg(DBG_IMAGE, ("Can't Handle XPM format inlined images!\n"));
        return;
    }
    if (_xWid == 0)
    {
        XX_DMsg(DBG_IMAGE, ("Can't read image w = 0!\n"));
        return;
    }
    if (_yHei == 0)
    {
        XX_DMsg(DBG_IMAGE, ("Can't read image h = 0!\n"));
        return;
    }
    padding = 0;
    if (((_xWid % 16) >= 1) && ((_xWid % 16) <= 8) && version10p)
    {
        padding = 1;
    }
    bytes_per_line = ((_xWid + 7) / 8) + padding;
    if (bytes_per_line % 4)
        win_extra_bytes_per_line = (4 - (bytes_per_line % 4)) % 4;    // 0-3, extra padding for long boundaries.

    else
        win_extra_bytes_per_line = 0;
    raster_length = bytes_per_line * _yHei;

    pibd = new CImgBitsDIB();
    if (!pibd)
        return;

    if (pibd->AllocMaskOnly(_xWid, _yHei))
    {
        goto abort;
    }

    OnSize(_xWid, _yHei, -1);

    line_idx = 0;
    if (lookahead)
    {
        line_idx = 1;
        line[0] = lookahead;
    }

    ptr = (BYTE *)pibd->GetMaskBits();
    
    {
        /* TODO UNIX  gui/x_xbm.c has some bReverseBitmap stuff in it 
        **  that might have to be moved in here.  It only affects code 
        ** in this block.
        */
        long cnt = 0;
#ifndef _MAC
        ptr += (_yHei - 1) * (bytes_per_line + win_extra_bytes_per_line);
#endif
        for (bytes = 0; bytes < raster_length; bytes++)
        {
            if (line_idx == 0)
            {
                for (;;)
                {
                    if (!Read(&line[0], 1))
                        goto abort;
                    if (line[0] != '\r' && line[0] != '\n')
                    {
                        line_idx = 1;
                        break;
                    }
                }
            }
            for (;;)
            {
                if (!Read(&line[line_idx], 1))
                {
                    if (line_idx == 0)
                        goto abort;
                    break;
                }
                if (line[line_idx] == ',' || line[line_idx] == '}')
                {
                    break;
                }
                if (line_idx < MAX_LINE) line_idx++;
            }
            line[line_idx] = '\0';
            value = strtol(line, &end, 16);
            line_idx = 0;

            for (n = 0, temp = 0; n < 8; n++)
            {
                temp += (value & 0x01) << (7 - n);
                value = value >> 1;
            }
            value = temp & 0xff;
            *ptr++ = (unsigned char) value;
            if (++cnt == bytes_per_line)
            {
                for (cnt = 0; cnt < win_extra_bytes_per_line; cnt++)
                    *ptr++ = (unsigned char) 0;
#ifndef _MAC
                ptr -= 2 * (bytes_per_line + win_extra_bytes_per_line);
#endif
                cnt = 0;
            }
        }
    }

#ifdef _MAC
    pibd->ReleaseMaskBits();
#endif

    _ySrcBot = -1;
    _pImgBits = pibd;
    return;

abort:
    delete pibd;
    return;
}

CImgTask * NewImgTaskXbm()
{
    return(new CImgTaskXbm);
}
