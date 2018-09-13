#include "priv.h"
#if 0
#include "strbuf.h"


#define USE_ATOM_TABLE
#define ATOM_TABLE_PRIME    101
#define ATOM_TABLE_GROWBY   50
BOOL g_fAtomTableInited = FALSE;


// BUGBUG: really bad out-of-memory handling

CStringBuf::CStringBuf(int cGrowBy)
{
#ifdef USE_ATOM_TABLE

    m_atomTable = NULL;
    m_nAtoms = 0;
    m_currentAtom = 0;
    m_bufSize = 0;

    if (!g_fAtomTableInited)
    {
        InitAtomTable(ATOM_TABLE_PRIME);
        g_fAtomTableInited = TRUE;
    }

#else

    m_growBy = cGrowBy;
    m_buf = 0;
    m_len = 0;
    m_pos = 0;

#endif
}

CStringBuf::~CStringBuf()
{
#ifdef USE_ATOM_TABLE

    if(m_atomTable)
        LocalFree((HANDLE)m_atomTable);

#else

    if(m_buf)
        LocalFree((HANDLE)m_buf);

#endif
}

char*   CStringBuf::DetachBuf()
{
#ifdef USE_ATOM_TABLE

    LPSTR buf = NULL;
    PWORD_ATOM pAtom = m_atomTable;

    if(m_currentAtom)
    {
        buf = (LPSTR)LocalAlloc(LPTR, m_bufSize);
        if(buf)
        {
            LPSTR pPos = buf;
            UINT uiWordLen;
            TCHAR szWord[MAX_WORD_LENGTH];

            *pPos = '\0';

            // go through the atom table.
            // and delete the atoms as we get the names.

            for(int i = 0; i < m_currentAtom; i++, pAtom++)
            {
                if(uiWordLen = GetAtomName(pAtom->atom, szWord, ARRAYSIZE(szWord)))
                {
                    // get the word.

                    memcpy(pPos, szWord, uiWordLen);
                    pPos += uiWordLen;
                    *pPos++ = '+';

                    // we don't put the number of hits in the query yet.

                    // make sure the atom is deleted.
                    // note that DeleteAtom returns zero when the ref count is not zero.
                    // while (!DeleteAtom(pAtom->atom)) ;

                    DeleteAtom(pAtom->atom);
                }
            }

            // free the atom table.

            LocalFree((HANDLE)m_atomTable);
            m_atomTable = NULL;
            m_nAtoms = 0;
            m_currentAtom = 0;
            m_bufSize = 0;

            // remove the last '+' character, and null terminate.

            if(pPos > buf)
                *--pPos = '\0';

            // note that caller of DeatchBuf is responsible for freeing the returned buffer.
        }
    }


#else

    char* buf = m_buf;
    m_buf = 0;
    m_len = 0;
    m_pos = 0;

#endif

    return buf;
}

int     CStringBuf::Grow(int cAdd)
{
#ifdef USE_ATOM_TABLE

    int newSize = m_nAtoms + cAdd;

    PWORD_ATOM newBuf;

    if(!m_atomTable)
        newBuf = (PWORD_ATOM)LocalAlloc(LPTR, newSize * sizeof(WORD_ATOM));
    else
        newBuf = (PWORD_ATOM)LocalReAlloc((HANDLE)m_atomTable, newSize * sizeof(WORD_ATOM), LMEM_MOVEABLE);

    if(!newBuf)
        return 0;

    m_atomTable = newBuf;
    m_nAtoms = newSize;

    return 1;

#else

    int newSize = m_pos + cAdd + m_growBy;

    // char* newBuf = (char*) realloc(m_buf, newSize);
    char* newBuf;

    if(!m_buf)
        newBuf = (char *)LocalAlloc(LPTR, newSize);
    else
        newBuf = (char *)LocalReAlloc((HANDLE)m_buf, newSize, LMEM_MOVEABLE);

    if(!newBuf)
        return 0;
    m_buf = newBuf;
    m_len = newSize;
    return 1;

#endif
}

void    CStringBuf::AddChar(char ch)
{
#ifndef USE_ATOM_TABLE

    if(m_pos == m_len)
    {
        if(!Grow(1))
            return;
    }
    m_buf[m_pos++] = ch;

#endif
}

BOOL    CStringBuf::AddSZ(char* sz)
{
#ifdef USE_ATOM_TABLE

    int iStrLen = lstrlen(sz);

    // ignore single character words

    if(iStrLen <= 1)
        return FALSE;

    // probably other kind of strings strings that we should also ignore.
    // 1. two character words
    // 2. numbers

    if(m_currentAtom >= m_nAtoms)
        if(!Grow(ATOM_TABLE_GROWBY))
            return FALSE;

    // haven't implement the hits yet.
    // if we want the atom functions to handle the hits for us,
    // we'll just keep calling AddAtom and count the hits when we DeleteAtom.

    ATOM atom = FindAtom(sz);

    if(!atom)
    {
        atom = AddAtom(sz);

        if(atom)
        {
            m_atomTable[m_currentAtom].atom = atom;

            m_currentAtom++;
            m_bufSize += iStrLen + 1;

            return TRUE;
        }
    }

    return FALSE;

#else

    int szLen = strlen(sz);
    if(m_pos + szLen >= m_len)
    {
        if(!Grow(szLen))
            return FALSE;
    }
    memcpy(m_buf + m_pos, sz, szLen + 1);
    m_pos += szLen;
    return TRUE;

#endif
}

void    CStringBuf::SetSize(int cNew)
{
#ifndef USE_ATOM_TABLE

    ASSERT(cNew >= 0 && cNew <= m_pos);

    if(m_buf && (cNew >= 0))
    {
        m_pos = cNew;
        m_buf[m_pos] = '\0';
    }

#endif
}

#endif