#ifndef __WS_DEFS_H__
#define __WS_DEFS_H__


#define WORKSPACE_REVISION_NUMBER "0032"

//
//
// Common base classes for all workspace items & containers.
//  Item & Container always have pointers to parent containers
//  and a pointer to a string indicating the name of the registry key.
//
// The class will duplicate the name of the registry key.
//
//  CBASE_WKSP wksp(pParent, "RegKey");         // ok
//
//  {
//      char sz[100];
//      strcpy(sz, "RegEntry";
//      CBASE_WKSP wksp(pParent, sz);           // ok
//  }
//
//  CBASE_WKSP wksp(pParent, _strdup("Reg"));   // Error: The string is
//                                              // duplicated twice, causing
//                                              // a mem leak. Without a reference
//                                              // to the first duplicate.
// NOTE: pParent & pszRegistryName can be NULL.
//
//
// USAGE: When a class is instantiated, it automatically adds itself
//  to the parent's list of items/containers. To avoid this, do the following:
//  
//
//  CBASE_WKSP wksp(NULL, "Reg1");              // ok
//  CBASE_WKSP wksp(NULL, NULL);                // ok
//  CBASE_WKSP wksp();                          // ok
//
//  wksp.SetRegistryName("Reg3");
//  wksp.SetParent(Foo);
//  



//
//
// The class that encapsulates a workspace. It may contain data items
//  and/or other workspaces. The data members it contains can be accessed
//  through the list or directly as member variables. There is a 1 to 1
//  correlation between the data members and the items in the list.
//
// The contents of a container can be mirrored to the root of the tree.
//
// Only an entire branch downward can be mirrored.
//  + not mirrrored
//    + mirrored
//      + mirrored  <- Since the parent is mirrored, the child must also be mirrored
//
    // The entries and the items that are pointed to by each entries
    // must have been allocated on the heap.
    //
    // Dynamic list of child containers
    //CCONTAINER_WKSP_ENTRY m_listConts;
    // Dynamic list of child items
    //CITEM_WKSP_ENTRY m_listItems;


// Abstract class that simply defines the interface that all derived classes
//  will share.
class CGenInterface_WKSP {
public:
    // Indicates whether a workspace is mirrored as a MRU default.
    BOOL m_bMirror;

    // Indicates that the number of workspaces varies, an example
    //  of this is a container with a list of breakpoints.
    BOOL m_bDynamicList;

    // Never modify this
    const char * const m_pszRegistryName;
    CGenInterface_WKSP * const m_pParent;

protected:
    // Key to the mirrored registry entry.
    HKEY m_hkeyMirror;
    // Key to the main registry entry.
    HKEY m_hkeyRegistry;

public:
    CGenInterface_WKSP();
    virtual ~CGenInterface_WKSP();

    void Init(CGenInterface_WKSP * const pParent, const char * const pszRegistryName, 
        BOOL bMirror = FALSE, BOOL bDynamic = FALSE);

    virtual void SetMirrorFlagForChildren() = 0;

    void SetRegistryName(const char * const pszRegistryName);
    void SetParent(CGenInterface_WKSP * const pParent);

    CGenInterface_WKSP * GetRootParent();

    virtual void AddToContainerList(CGenInterface_WKSP * p) = 0;
    virtual CItemInterface_WKSP * GetDataItemInterface(PVOID pv) const = 0;

public:
    virtual void CloseRegistryKeys() = 0;
    virtual HKEY GetRegistryKey(PBOOL pbRegKeyCreated = NULL) = 0;
    virtual HKEY GetMirrorKey(PBOOL pbRegKeyCreated = NULL, 
                              PSTR pszSubstituteRegistryName = NULL
                              ) = 0;

    virtual void Restore(BOOL bOnlyItems) = 0;
    virtual void Save(BOOL bOnlySaveMirror, BOOL bOnlyItems) = 0;

    virtual void Duplicate(const CGenInterface_WKSP & arg) = 0;
};


class CDoNothingStub_WKSP : public CGenInterface_WKSP 
{
public:
    CDoNothingStub_WKSP()                                               {}

    virtual void AddToContainerList(CGenInterface_WKSP * p)             { Assert(0); }
    virtual CItemInterface_WKSP * GetDataItemInterface(PVOID pv) const  { Assert(0); return NULL; }
    virtual void CloseRegistryKeys()                                    { Assert(0); }
    virtual HKEY GetRegistryKey(PBOOL pbRegKeyCreated = NULL)           { Assert(0); return NULL; }
    
    virtual HKEY GetMirrorKey(PBOOL pbRegKeyCreated = NULL, 
                              PSTR pszSubstituteRegistryName = NULL
                              )                                         { Assert(0); return NULL; }

    virtual void Restore(BOOL bOnlyItems)                               { Assert(0); }
    virtual void Save(BOOL bOnlySaveMirror, BOOL bOnlyItems)            { Assert(0); }

    // Included to make it compile.
    virtual void Duplicate(const CGenInterface_WKSP & arg)              { Assert(0); }

    virtual void SetMirrorFlagForChildren()                             { Assert(0); }

};


template<class TCont, class TItem>
class CGen_WKSP : public CDoNothingStub_WKSP {
public:
    void Containers_DeleteList();
    void Items_DeleteList();

    // List of child containers
    TList<TCont *> m_listConts;
    // List of owned items
    TList<TItem *> m_listItems;

    virtual void AddToContainerList(CGenInterface_WKSP * p);
    virtual CItemInterface_WKSP * GetDataItemInterface(PVOID pv) const;

public:
    virtual void CloseRegistryKeys();
    virtual HKEY GetRegistryKey(PBOOL pbRegKeyCreated = NULL);
    virtual HKEY GetMirrorKey(PBOOL pbRegKeyCreated = NULL, 
                              PSTR pszSubstituteRegistryName = NULL
                              );

public:
    virtual void Duplicate(const CGenInterface_WKSP & arg);

    virtual void SetMirrorFlagForChildren();

protected:
    // Helper function that actually reads the values from the registry.
    virtual void Restore_Read(const HKEY hkey, BOOL bOnlyItems);
    //virtual void Save_Read(const HKEY hkey);
public:
    virtual void Restore(BOOL bOnlyItems);
    virtual void Save(BOOL bOnlySaveMirror, BOOL bOnlyItems);
};






#endif
