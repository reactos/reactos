//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ace.h
//
//  This file contains definitions and prototypes for the ACE abstraction
//  class (CAce)
//
//--------------------------------------------------------------------------

#ifndef _ACE_H_
#define _ACE_H_

class CAce : public ACE_HEADER
{
public:
  //UCHAR           AceType;        // Inherited from ACE_HEADER
  //UCHAR           AceFlags;
  //USHORT          AceSize;
    ACCESS_MASK     Mask;
    ULONG           Flags;          // ACE_OBJECT_TYPE_PRESENT, etc.
    GUID            ObjectType;
    GUID            InheritedObjectType;
    PSID            psid;
    SID_NAME_USE    sidType;
private:
    LPTSTR          pszName;
    LPTSTR          pszType;
    LPTSTR          pszAccessType;
    LPTSTR          pszInheritType;
    BOOL            bPropertyAce;

public:
    CAce(PACE_HEADER pAceHeader = NULL);
    ~CAce();

    LPTSTR GetName()        const { return pszName;         }
    LPTSTR GetType()        const { return pszType;         }
    LPTSTR GetAccessType()  const { return pszAccessType;   }
    LPTSTR GetInheritType() const { return pszInheritType;  }
    BOOL   IsPropertyAce()  const { return bPropertyAce;    }

    LPTSTR LookupName(LPCTSTR pszServer = NULL, LPSECURITYINFO2 psi2 = NULL);

    void SetName(LPCTSTR pszN, LPCTSTR pszL = NULL);
    void SetType(LPCTSTR psz)        { SetString(&pszType, psz);        }
    void SetAccessType(LPCTSTR psz)  { SetString(&pszAccessType, psz);  }
    void SetInheritType(LPCTSTR psz) { SetString(&pszInheritType, psz); }
    void SetPropertyAce(BOOL b)      { bPropertyAce = b;                }
    void SetSid(PSID p, LPCTSTR pszName, LPCTSTR pszLogonName, SID_NAME_USE type);
    PACE_HEADER Copy() const;
    void CopyTo(PACE_HEADER pAceDest) const;
    int  CompareType(const CAce *pAceCompare) const;
    DWORD Merge(const CAce *pAce2);

private:
    void SetString(LPTSTR *ppszDest, LPCTSTR pszSrc);
};
typedef CAce *PACE;

#define AllFlagsOn(dw1, dw2)        (((dw1) & (dw2)) == (dw2))  // equivalent to ((dw1 | dw2) == dw1)
#define IsAuditAlarmACE(type) ( ((type) == SYSTEM_AUDIT_ACE_TYPE)        || \
                                ((type) == SYSTEM_AUDIT_OBJECT_ACE_TYPE) || \
                                ((type) == SYSTEM_ALARM_ACE_TYPE)        || \
                                ((type) == SYSTEM_ALARM_OBJECT_ACE_TYPE) )

BOOL
IsEqualACEType(DWORD dwType1, DWORD dwType2);

DWORD
MergeAceHelper(DWORD dwAceFlags1,
               DWORD dwMask1,
               DWORD dwAceFlags2,
               DWORD dwMask2,
               DWORD dwMergeFlags,
               LPDWORD pdwResult);

// CAce::Merge and MergeAceHelper return values
#define MERGE_FAIL              0   // Unable to merge ACEs
#define MERGE_OK_1              1   // ACE 1 (this) implies ACE 2
#define MERGE_OK_2              2   // ACE 2 implies ACE 1 (this)
#define MERGE_MODIFIED_FLAGS    3   // ACEs can be merged by modifying flags (new flags in *pdwResult)
#define MERGE_MODIFIED_MASK     4   // ACEs can be merged by modifying mask (new mask in *pdwResult)

// Values for MergeAceHelper dwMergeFlags parameter
#define MF_OBJECT_TYPE_1_PRESENT    1
#define MF_OBJECT_TYPE_2_PRESENT    2
#define MF_OBJECT_TYPE_EQUAL        4
#define MF_AUDIT_ACE_TYPE           8


#endif  // _ACE_H_
