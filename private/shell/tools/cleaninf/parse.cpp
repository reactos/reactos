
#include "priv.h"       



#define IS_WHITESPACE(ch)   (' ' == ch || '\t' == ch)
#define IS_NEWLINE(ch)      ('\n' == ch)


// Flags for _ReadChar
#define RCF_NEXTLINE        0x0001      // skip to next line
#define RCF_NEXTNWS         0x0002      // skip to next non-whitespace
#define RCF_SKIPTRAILING    0x0004      // skip trailing whitespace


// constructor
CParseFile::CParseFile()
{
}



/*-------------------------------------------------------------------------
Purpose: Parse the given file according to the provided flags.
*/
void CParseFile::Parse(FILE * pfileSrc, FILE * pfileDest, DWORD dwFlags)
{
    _bSkipWhitespace = BOOLIFY(dwFlags & PFF_WHITESPACE);
    
    _pfileSrc = pfileSrc;
    _pfileDest = pfileDest;
    _ichRead = 0;
    _cchRead = 0;
    
    _ichWrite = 0;
    
    _ch = 0;

    if (dwFlags & PFF_HTML)
        _ParseHtml();
    else if (dwFlags & PFF_HTC)
        _ParseHtc();
    else if (dwFlags & PFF_JS)
        _ParseJS();
    else
        _ParseInf();

    _FlushWriteBuffer();
}


/*-------------------------------------------------------------------------
Purpose: Read the next character in the file.  Sets _ch.

*/
char CParseFile::_ReadChar(DWORD dwFlags)
{
    BOOL bFirstCharSav = _bFirstChar;
    
    do 
    {
        _ichRead++;
        _bFirstChar = FALSE;

        // Are we past the buffer, or do we skip to next line?
        if (_ichRead >= _cchRead || dwFlags & RCF_NEXTLINE)
        {
            // Yes; read in more
            if (fgets(_szReadBuf, SIZECHARS(_szReadBuf), _pfileSrc))
            {
                _ichRead = 0;
                _cchRead = strlen(_szReadBuf);
                _bFirstChar = TRUE;
            }
            else
            {
                _ichRead = 0;
                _cchRead = 0;
            }
        }

        if (_ichRead < _cchRead)
            _ch = _szReadBuf[_ichRead];
        else
            _ch = CHAR_EOF;
    } while ((dwFlags & RCF_NEXTNWS) && IS_WHITESPACE(_ch));

    // Are we supposed to skip to the next non-whitespace?
    if (dwFlags & RCF_NEXTNWS)
    {
        // Yes; then retain the "first character" state
        _bFirstChar = bFirstCharSav;
    }
        
    return _ch;
}


/*-------------------------------------------------------------------------
Purpose: Read ahead to the next character in the buffer and return its
         value, but don't set _ch or increment the read pointer.
*/
char CParseFile::_SniffChar(int ichAhead)
{
    if (_ichRead + ichAhead < _cchRead)
        return _szReadBuf[_ichRead + ichAhead];
        
    return 0;
}



/*-------------------------------------------------------------------------
Purpose: Write the character to the file
*/
void CParseFile::_WriteChar(char ch)
{
    _szWriteBuf[_ichWrite++] = ch;
    _szWriteBuf[_ichWrite] = 0;

    if ('\n' == ch || SIZECHARS(_szWriteBuf)-1 == _ichWrite)
    {
        fputs(_szWriteBuf, _pfileDest);
        _ichWrite = 0;
    }
}


/*-------------------------------------------------------------------------
Purpose: Flushes the write buffer to the file
*/
void CParseFile::_FlushWriteBuffer(void)
{
    if (_ichWrite > 0)
    {
        fputs(_szWriteBuf, _pfileDest);
        _ichWrite = 0;
    }
}



/*-------------------------------------------------------------------------
Purpose: Parse a .inf file.
*/
void CParseFile::_ParseInf(void)
{
    _ReadChar(0);
    
    while (CHAR_EOF != _ch)
    {
        if (_bFirstChar)
        {
            // Is this a comment?
            if (';' == _ch)
            {
                // Yes; skip to next line
                _ReadChar(RCF_NEXTLINE);
                continue;
            }

            if (_SkipWhitespace())
                continue;
        }
        
        _WriteChar(_ch);
        _ReadChar(0);
    }
}    


/*-------------------------------------------------------------------------
Purpose: Write the current character and the rest of the tag.  Assumes
         _ch is the beginning of the tag ('<').

         There are some parts of the tag which may be compacted if _bSkipWhitespace
         is TRUE.  The general rule is only one space is required between attributes,
         and newlines are converted to spaces if necessary.  Anything in quotes 
         (single or double) are left alone.
*/
void CParseFile::_WriteTag(void)
{
    BOOL bSingleQuotes = FALSE;
    BOOL bDblQuotes = FALSE;
    BOOL bWrittenSpace = FALSE;
    
    // The end of the tag is the next '>' that is not in single or double-quotes.

    while (CHAR_EOF != _ch)
    {
        if (_bSkipWhitespace && !bSingleQuotes && !bDblQuotes)
        {
            // Is this a space or a newline?
            if (IS_WHITESPACE(_ch) || IS_NEWLINE(_ch))
            {
                // Yes; Have we written a space yet?
                if (!bWrittenSpace)
                {
                    // No; write a space and skip to the next non-whitespace
                    _WriteChar(' ');
                    bWrittenSpace = TRUE;
                }
                
                _ReadChar(RCF_NEXTNWS);
                continue;
            }
        }

        // End of tag?
        if ('>' == _ch && !bSingleQuotes && !bDblQuotes)
        {
            // Yes
            _WriteChar(_ch);
            break;
        }
        
        bWrittenSpace = FALSE;
        _WriteChar(_ch);
        _ReadChar(0);

        if ('\'' == _ch)
            bSingleQuotes ^= TRUE;
        else if ('"' == _ch)
            bDblQuotes ^= TRUE;
    }
}


/*-------------------------------------------------------------------------
Purpose: Skip the current comment tag.  Assumes _ch is the beginning of
         the tag ('<').
*/
void CParseFile::_SkipCommentTag(void)
{
    // The end of the tag is the next '-->'

    while (CHAR_EOF != _ch)
    {
        // Is the end of the comment coming up?
        if ('-' == _ch && _SniffChar(1) == '-' && _SniffChar(2) == '>')
        {
            // Yes
            _ReadChar(0);   // skip '-'
            _ReadChar(0);   // skip '>'
            break;
        }
        
        _ReadChar(0);
    }
}


/*-------------------------------------------------------------------------
Purpose: Skip leading whitespace.

         Returns TRUE if anything was skipped
*/
BOOL CParseFile::_SkipWhitespace(void)
{
    BOOL bRet = FALSE;
    
    if (_bSkipWhitespace)
    {
        if (IS_WHITESPACE(_ch))
        {
            // Skip leading whitespace in line
            _ReadChar(RCF_NEXTNWS);
            bRet = TRUE;
        }
        else if (IS_NEWLINE(_ch))
        {
            _ReadChar(RCF_NEXTLINE);
            bRet = TRUE;
        }
    }
    return bRet;
}


/*-------------------------------------------------------------------------
Purpose: Skip a C or C++ style comment

         Returns TRUE if a comment boundary was encountered.
*/
BOOL CParseFile::_SkipComment(int * pcNestedComment)
{
    BOOL bRet = FALSE;
    
    if ('/' == _ch)
    {
        // Is this a C++ comment?
        if ('/' == _SniffChar(1))
        {
            // Yes; skip it to end of line
            if (!_bFirstChar || !_bSkipWhitespace)
                _WriteChar('\n');
                
            _ReadChar(RCF_NEXTLINE);
            bRet = TRUE;
        }
        // Is this a C comment?
        else if ('*' == _SniffChar(1))
        {
            // Yes; skip to respective '*/'
            _ReadChar(0);       // skip '/'
            _ReadChar(0);       // skip '*'
            (*pcNestedComment)++;
            bRet = TRUE;
        }
    }
    else if ('*' == _ch)
    {
        // Is this the end of a C comment?
        if ('/' == _SniffChar(1))
        {
            // Yes
            _ReadChar(0);       // skip '*'
            _ReadChar(0);       // skip '/'
            (*pcNestedComment)--;
            
            // Prevent writing an unnecessary '\n'
            _bFirstChar = TRUE;
            bRet = TRUE;
        }
    }
    return bRet;
}


/*-------------------------------------------------------------------------
Purpose: Parse the innertext of the STYLE tag, remove any comments
*/
void CParseFile::_ParseInnerStyle(void)
{
    int cNestedComment = 0;
    
    // The end of the tag is the next '</STYLE>'

    while (CHAR_EOF != _ch)
    {
        if (_bFirstChar && _SkipWhitespace())
            continue;

        // Is the end of the styletag section coming up?
        if ('<' == _ch && _IsTagEqual("/STYLE"))
        {
            // Yes
            break;
        }

        if (_SkipComment(&cNestedComment))
            continue;

        if (0 == cNestedComment && !IS_NEWLINE(_ch))
            _WriteChar(_ch);
            
        _ReadChar(0);
    }
}


/*-------------------------------------------------------------------------
Purpose: Returns TRUE if the given tagname matches the currently parsed token
*/
BOOL CParseFile::_IsTagEqual(LPSTR pszTag)
{
    int ich = 1;

    while (*pszTag)
    {
        if (_SniffChar(ich++) != *pszTag++)
            return FALSE;
    }

    // We should verify we've come to the end of the tagName
    char chEnd = _SniffChar(ich);
    
    return (' ' == chEnd || '>' == chEnd || '<' == chEnd);
}


/*-------------------------------------------------------------------------
Purpose: Parse a .htm or .hta file.
*/
void CParseFile::_ParseHtml(void)
{
    BOOL bFollowingTag = FALSE;
    
    _ReadChar(0);
    
    while (CHAR_EOF != _ch)
    {
        if (_bFirstChar && _SkipWhitespace())
            continue;

        // Is this a tag?
        if ('<' == _ch)
        {
            // Yes; looks like it
            
            if (_IsTagEqual("!--"))
            {
                // Comment; skip it
                _SkipCommentTag();
                _ReadChar(0);
                bFollowingTag = TRUE;
                continue;
            }
            else if (_IsTagEqual("SCRIPT"))
            {
                // Parse the script 
                _WriteTag();        // write the <SCRIPT> tag
                
                // BUGBUG (scotth): we always assume javascript
                _ParseJS();

                _WriteTag();        // write the </SCRIPT> tag

                _ReadChar(0);
                bFollowingTag = TRUE;
                continue;
            }
            else if (_IsTagEqual("STYLE"))
            {
                _WriteTag();        // write the <STYLE> tag
                _ParseInnerStyle();
                _WriteTag();        // write the </STYLE> tag
                
                _ReadChar(0);
                bFollowingTag = TRUE;
                continue;
            }
            else
            {
                // Any other tag: write the tag and go to the next one
                _WriteTag();
                _ReadChar(0);
                bFollowingTag = TRUE;
                continue;
            }
        }

        if (bFollowingTag && _bSkipWhitespace)
        {
            bFollowingTag = FALSE;

            if (_SkipWhitespace())
                continue;
        }
        
        _WriteChar(_ch);
        _ReadChar(0);
    }
}


/*-------------------------------------------------------------------------
Purpose: Parse a .js file.
*/
void CParseFile::_ParseJS(void)
{
    BOOL bDblQuotes = FALSE;
    BOOL bSingleQuotes = FALSE;
    int cNestedComment = 0;
    
    _ReadChar(0);
    
    while (CHAR_EOF != _ch)
    {
        // Are we in a comment?
        if (0 == cNestedComment)
        {
            // No; (we only pay attention to strings when they're not in comments)
            if ('\'' == _ch)
                bSingleQuotes ^= TRUE;
            else if ('"' == _ch)
                bDblQuotes ^= TRUE;

            if (_bSkipWhitespace && !bDblQuotes && !bSingleQuotes)
            {
                if (IS_WHITESPACE(_ch))
                {
                    // Skip whitespace
                    if (!_bFirstChar)
                        _WriteChar(' ');
                        
                    _ReadChar(RCF_NEXTNWS);
                    continue;
                }
                else if (IS_NEWLINE(_ch))
                {
                    // Since javascript doesn't require a ';' at the end of a statement,
                    // we should at least replace the newline with a space so tokens don't
                    // get appended accidentally.

                    // The javascript engine has a line-length limit.  So don't replace
                    // a newline with a space.
                    if (!_bFirstChar)
                        _WriteChar('\n');
                        
                    _ReadChar(RCF_NEXTLINE);
                    continue;
                }
            }

            // Are we in a string?
            if (!bDblQuotes && !bSingleQuotes)
            {
                // No; look for the terminating SCRIPT tag
                if ('<' == _ch)
                {
                    if (_IsTagEqual("/SCRIPT"))
                    {
                        // We've reached the end of the script block
                        break;
                    }
                }
            }
        }

        // Are we in a string?
        if (!bDblQuotes && !bSingleQuotes)
        {
            // No; look for comments...
            if (_SkipComment(&cNestedComment))
                continue;
        }
        
        if (0 == cNestedComment)
            _WriteChar(_ch);
            
        _ReadChar(0);
    }
}


/*-------------------------------------------------------------------------
Purpose: Parse a .htc file.
*/
void CParseFile::_ParseHtc(void)
{
    BOOL bFollowingTag = FALSE;
    int cNestedComment = 0;
    
    _ReadChar(0);
    
    while (CHAR_EOF != _ch)
    {
        if (_bFirstChar && _SkipWhitespace())
            continue;

        // Is this a tag?
        if ('<' == _ch)
        {
            // Yes; is it a script tag?
            if (_IsTagEqual("SCRIPT"))
            {
                // Yes; parse the script 
                _WriteTag();        // write the <SCRIPT> tag
                
                // BUGBUG (scotth): we always assume javascript
                _ParseJS();

                _WriteTag();        // write the </SCRIPT> tag

                _ReadChar(0);
                bFollowingTag = TRUE;
                continue;
            }
            else
            {
                _WriteTag();
                _ReadChar(0);
                bFollowingTag = TRUE;
                continue;

            }
        }

        // Look for comments outside the SCRIPT block...
        if (_SkipComment(&cNestedComment))
            continue;
            
        if (bFollowingTag && _bSkipWhitespace)
        {
            bFollowingTag = FALSE;

            if (_SkipWhitespace())
                continue;
        }
        
        if (0 == cNestedComment)
             _WriteChar(_ch);
             
        _ReadChar(0);
    }
}




