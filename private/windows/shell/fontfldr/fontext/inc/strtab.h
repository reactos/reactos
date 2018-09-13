#ifndef __STRING_TABLE_H
#define __STRING_TABLE_H
///////////////////////////////////////////////////////////////////////////////
// Class: StringTable
//
// This class implements a simple hash table for storing text strings.
// The purpose of the table is to store strings and then verify later
// if the table contains a given string.  Since there is no data associated
// with the string, the stored strings act as both key and data.  Therefore,
// there is no requirement for string retrieval.  Only existence checks
// are required.
// The structure maintains a fixed-length array of pointers, each pointing
// to a linked list structure (List).  These lists are used to handle the
// problem of hash collisions (sometimes known as "separate chaining").
//
// Note that these classes do not contain all the stuff that is usually
// considered necessary in C++ classes.  Things like copy constructors,
// assignment operator, type conversion etc are excluded.  The classes
// are very specialized for the Font Folder application and these things
// would be considered "fat".  Should this hash table class be later used 
// in a situation where these things are needed, they can be added then.
//
// The public interfaces to the table are:
//
//      Initialize  - Initialize a new string table.
//      Add         - Add a new string to a table.
//      Exists      - Determine if a string exists in a table.
//      Count       - Return the number of strings in a table.
//
// Destruction of the object automatically releases all memory associated
// with the table.
//
// BrianAu - 4/11/96
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <tchar.h>

//
// Hash table containing text strings.
// 
class StringTable {
    private:
        //
        // Linked list for hash collisions.
        //
        class List {
            private:
                //
                // Element in hash collision list.
                //
                class Element {
                    public:
                        LPTSTR m_pszText;   // Pointer to string text.
                        Element *m_pNext;   // Pointer to next in list.

                        Element(void);
                        ~Element(void);
                        BOOL Initialize(LPCTSTR pszItem);
                        BOOL operator == (const Element& ele) const;
                        BOOL operator != (const Element& ele) const;
#ifdef DEBUG
                        void DebugOut(void) const;
#endif
                };

                Element *m_pHead;  // Ptr to head of list;
                DWORD   m_dwCount; // Count of elements in list.

            public:
                List(void);
                ~List(void);
                BOOL Add(LPCTSTR pszText, BOOL bAllowDuplicates = TRUE);
                BOOL Exists(LPCTSTR pszText) const;
                DWORD Count(void) const { return m_dwCount; }
#ifdef DEBUG
                void DebugOut(void) const;
#endif
        };

        CRITICAL_SECTION m_cs;    // Protect from concurrent access.
        List **m_apLists;         // Array of ptrs to collision lists.
        DWORD m_dwItemCount;      // Number of items in table.
        DWORD m_dwHashBuckets;    // Number of pointers in hash array.
        BOOL  m_bCaseSensitive;   // Key strings treated case-sensitive?
        BOOL  m_bAllowDuplicates; // Allow duplicate strings?

        DWORD Hash(LPCTSTR pszText) const;
        LPTSTR StringTable::CreateUpperCaseString(LPCTSTR pszText) const;
        BOOL Exists(DWORD dwHashCode, LPCTSTR pszText);

    public:
        StringTable(void);
        ~StringTable(void);
        BOOL Initialize(DWORD dwHashBuckets, 
                        BOOL bCaseSensitive   = TRUE,
                        BOOL bAllowDuplicates = FALSE);

        BOOL IsInitialized(void);
        void Destroy(void);
        BOOL Add(LPCTSTR pszText);
        BOOL Exists(LPCTSTR pszText);
        DWORD Count(void) const { return m_dwItemCount; }
#ifdef DEBUG
        void DebugOut(void) const;
#endif
};

#endif