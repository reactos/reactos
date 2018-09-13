#include "headers.hxx"

#ifndef X_CLRNGPRS_HXX_
#define X_CLRNGPRS_HXX_
#include <clrngprs.hxx>
#endif


MtDefine(CCellRangeParser, ObjectModel, "CCellRangeParser")


CCellRangeParser::CCellRangeParser(LPCTSTR szRange)
{
    int nCurIndex;

    _fFailed = FALSE;
    _fOneValue = FALSE;

    if(szRange == NULL || *szRange == 0)
        goto Fail;
    
    // Remove all the spaces, convert to uppercase, replace .. with : and
    // store in _strNormString
    Normalize(szRange);

    nCurIndex = 0;

    if(!GetColumn(&nCurIndex, &(_RangeRect.left)))
        goto Fail;
    if(!GetNumber(&nCurIndex, &(_RangeRect.top)))
        goto Fail;
    
    if(_strNormString[nCurIndex] == 0)
    {
        _fOneValue = TRUE;
        _RangeRect.right = _RangeRect.left;
        _RangeRect.bottom = _RangeRect.top;
        goto Done;
    }

    if(((LPCTSTR)_strNormString)[nCurIndex++] != _T(':'))
        goto Fail;
    
    if(!GetColumn(&nCurIndex, &(_RangeRect.right)))
        goto Fail;
    if(!GetNumber(&nCurIndex, &(_RangeRect.bottom)))
        goto Fail;
Done:
    return;

Fail:
    _fFailed = TRUE;
    goto Done;
}


#define eStart       0
#define eSeenLetter1 1
#define eSeenNumber1 2
#define eSeenSepar   3
#define eSeenLetter2 4
#define eSeenNumber2 5
        
void 
CCellRangeParser::Normalize(LPCTSTR szRange)
{
    TCHAR       c, cAppnd;
    int         i;
    int         nLen = _tcslen(szRange);
    int         nState;
    int         nDigit;

    _strNormString.ReAlloc(nLen + 1); 
    _strNormString.SetLengthNoAlloc(0);
    nState = eStart;

    for(i = 0; i < nLen; i++)
    {
        c = szRange[i];
        if(_istspace(c))
            continue;
        else if((c == _T('.') && szRange[i+1] == _T('.')) || c == _T(':'))
        {
            if(nState == eSeenNumber1)
            {
                cAppnd = _T(':');
                if(c == _T('.')) i++;
                nState++;
            }
            else
                goto Fail;
        }
        else if(_istalpha(c))
        {
            if(nState == eSeenSepar || nState == eStart)
            {
                cAppnd = c;
                if(_istalpha(szRange[i+1]))
                {
                    _strNormString.Append(&cAppnd, 1);
                    i++;
                    cAppnd = szRange[i];
                }
                nState++;
            }
            else
                goto Fail;
        } else if(_istdigit(c))
        {
            if(nState == eSeenLetter1 || nState == eSeenLetter2)
            {
                // Remove heading 0's
                while(c == _T('0'))
                {
                    i++;
                    c = szRange[i];
                }
                if(!_istdigit(c))
                    goto Fail;
                cAppnd = c;
                // make sure that no more then 5 digits are processed
                nDigit = 0;
                while(_istdigit(szRange[i+1]))
                {
                    if(nDigit > 5)
                        goto Fail;
                    nDigit++;
                    _strNormString.Append(&cAppnd, 1);
                    i++;
                    cAppnd = szRange[i];
                }
                nState++;
            }
            else
                goto Fail;
        }
        _strNormString.Append(&cAppnd, 1);
    } /* for */


    if(nState != eSeenNumber1 && nState != eSeenNumber2)
        goto Fail;

    if(nState == eSeenNumber1)
        _fOneValue = TRUE;

    CharUpper(_strNormString);

    return;
Fail:
    _fFailed = TRUE;
    return;
}

BOOL 
CCellRangeParser::GetColumn(int *pnCurIndex, long *pnCol)
{
    int nVal;

    if(_fFailed)
        return FALSE;

    if(_strNormString.Length() == 0 || !_istalpha(_strNormString[*pnCurIndex]))
    {
        _fFailed = TRUE;
        return FALSE;
    }

  nVal = _strNormString[*pnCurIndex] - _T('A');
  
  (*pnCurIndex)++;

  if(_istalpha(_strNormString[*pnCurIndex]))
  {
      // A translates to 0 but AA to 1*26+0 so we need 26*(nVal+1)
      nVal = 26 * (nVal + 1) + (_strNormString[*pnCurIndex] - _T('A'));
      (*pnCurIndex)++;
  }

  *pnCol = nVal;

  return TRUE;
}

BOOL 
CCellRangeParser::GetNumber(int *pnCurIndex, long *pnRow)
{
    int nVal;

    if(_fFailed)
        return FALSE;

    if(_strNormString.Length() == 0 || !_istdigit(_strNormString[*pnCurIndex]))
    {
        _fFailed = TRUE;
        return FALSE;
    }

  nVal = _strNormString[*pnCurIndex] - _T('0');
  
  (*pnCurIndex)++;

  while(_istdigit(_strNormString[*pnCurIndex]))
  {
      nVal = 10 * nVal + (_strNormString[*pnCurIndex] - _T('0'));
      (*pnCurIndex)++;
  }

  // Adjust for the fact that first row is row 1 and return the value
  *pnRow = nVal - 1;
  
  return TRUE;
}
