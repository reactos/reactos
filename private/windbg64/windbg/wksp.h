
//
// A workspace consists of a collection of workspace sections
//

class WORKSPACE {

    //
    // This class contains no dynamic data.
    //
    // It contains a static list head, and some member functions
    // to manage the list.
    //

public:

    WORKSPACE();

private:

    VOID
    AddMemberToSection(
        PVOID Section,
        PVOID Member
        );

    static LIST_ENTRY WorkspaceSectionList;


    struct SECTION_ENTRY {

        //
        // A pointer to the section we are registering
        //
        PVOID Object;

        //
        // Membership in the global list of all sections
        //
        LIST_ENTRY SectionList;

        //
        // The list of all of the members of this section
        //
        LIST_ENTRY MemberList;
    };

    typedef SECTION_ENTRY * PSECTION_ENTRY;

    struct ITEM_ENTRY {

        //
        // A pointer to the item we are registering
        //
        PVOID Object;

        //
        // The parent section's list of members
        //
        LIST_ENTRY MemberList;
    };

    typedef ITEM_ENTRY * PITEM_ENTRY;

};

WORKSPACE::
WORKSPACE() {
    if (WorkspaceSectionList.Flink == 0) {
        InitializeListHead(WorkspaceSectionList);
    }
}


WORKSPACE::
AddMemberToSection(
    PVOID Section,
    PVOID Member
    )
{
    PSECTION_ENTRY ThisSection;
    PSECTION_ENTRY FoundSection = NULL;
    PITEM_ENTRY NewMember;

    Next = WorkspaceSectionList.Flink;

    while (Next != WorkspaceSectionList) {

        ThisSection = CONTAINING_RECORD(Next, SECTION_ENTRY, ListOfSections);
        if (ThisSection->Object == Section) {
            FoundSection = ThisSection;
            break;
        }

        Next = Next->Flink;
    }

    if (!FoundSection) {
        FoundSection = malloc(sizeof(WORKSPACE_SECTION_ENTRY));
        FoundSection->Object = Section;
        InitializeListHead(FoundSection->MemberList);
        if (WorkspaceSectionList.Flink == 0) {
            InitializeListHead(&WorkspaceSectionList);
        }
        InsertTailList(&WorkspaceSectionList, &FoundSection->SectionList);
    }

    NewMember = malloc(sizeof(*NewMember));
    NewMember->Object = Member;
    InsertTailList(&FoundSection->MemberList, &NewMember->MemberList);
}


//
// A workspace section collects together related workspace items
// and sections.  It inherits no data from WORKSPACE except for
// the master list which lets us walk the workspace tree from
// any section node.
//
class WORKSPACE_SECTION : WORKSPACE {

public:
    Save();
    Restore();

    //
    // This register function will be called when a workspace
    // item is instantiated in a child section class.
    //
    // There might be no need to ever unregister member objects
    //
    RegisterMember(PVOID Object) { AddMemberToSection((PVOID)this, Object); }

    HKEY GetRegistryKey();

private:

};



//
// Parent class for a registry value.
//
// Each item will be instantiated in a WORKSPACE_SECTION,
// and will register itself with the parent as it is constructed.
//

class WORKSPACE_ITEM {

private:
    PSTR RegistryNameString;
    PWORKSPACE_SECTION ParentSection;

public:
    //WORKSPACE_ITEM(PSTR Name);
    WORKSPACE_ITEM(WORKSPACE_SECTION *, PSTR);

    ~WORKSPACE_ITEM();

    //
    // To save or restore, it needs to obtain a registry
    // key from the parent section.
    // If a key is not provided, the method must call the
    // parent to obtain a key.
    //

    virtual Save(HKEY);
    virtual Save();

    virtual Restore(HKEY);
    virtual Restore();

};

WORKSPACE_ITEM::
WORKSPACE_ITEM(
    WORKSPACE_SECTION *Parent,
    PSTR Name
    )
{
    ParentSection = Parent;
    ParentSection->RegisterMember(this);
}

//
// This class destructor is probably not neccessary...
//
WORKSPACE_ITEM::
~WORKSPACE_ITEM(
    void
    )
{
}



//
// Integer workspace item
//
class INT_WORKSPACE_ITEM : WORKSPACE_ITEM {

private:
    int Data;

public:

    INT_WORKSPACE_ITEM( PSTR Name, int Value = 0 );

    ~INT_WORKSPACE_ITEM() {};

    Save(HKEY Key = NULL);
    Save();

    Restore();
};

INT_WORKSPACE_ITEM::
INT_WORKSPACE_ITEM(
    PSTR Name,
    int Value
    )
{
    RegistryNameString = Name;
    Data = Value;
}

INT_WORKSPACE_ITEM::Save(HKEY Key)
{
    if (Key == NULL) {
        //
        // Get a registry key from somewhere
        //
        Key = ParentSection->GetRegistryKey();
    }

    //
    // Write out the value
    //

    RegSaveValue(Key, RegistryNameString, Data, REG_INTEGER);

}




//
// An instance of a workspace section
//
class LOCAL_WORKSPACE : WORKSPACE_SECTION {

    HKEY GetRegistryKey();

    INT_WORKSPACE_ITEM DefaultFormat(this, "Default Format" );
    INT_WORKSPACE_ITEM PanePosition(this, "Pane Position" );

};

WORKSPACE_SECTION::Save()
{
    //
    // iterate over the items and save them
    //
}

WORKSPACE_SECTION::Restore()
{
    //
    // iterate over the items and read them
    //
}


