#include "precomp.h"
#pragma hdrstop


//
// Internal function definitions
//

ARC_STATUS
AlpFreeComponents(
    IN PCHAR *EnvVarComponents
    );

BOOLEAN
AlpMatchComponent(
    IN PCHAR Value1,
    IN PCHAR Value2
    );

//
// Function implementations
//


ARC_STATUS
AlGetEnvVarComponents (
    IN  PCHAR  EnvValue,
    OUT PCHAR  **EnvVarComponents,
    OUT PULONG PNumComponents
    )

/*++

Routine Description:

    This routine takes an environment variable string and turns it into
    the constituent value strings:

    Example EnvValue = "Value1;Value2;Value3" is turned into:

    "Value1", "Value2", "Value3"

    The following are valid value strings:

    1. "     "                                      :one null value is found
    2. ";;;;    "                                   :five null values are found
    3. " ;Value1    ;   Value2;Value3;;;;;;;   ;"   :12 value strings are found,
                                                    :9 of which are null

    If an invalid component (contains embedded white space) is found in the
    string then this routine attempts to resynch to the next value, no error
    is returned, and a the first part of the invalid value is returned for the
    bad component.

    1.  "    Value1;Bad   Value2; Value3"           : 2 value strings are found

    The value strings returned suppress all whitespace before and after the
    value.


Arguments:

    EnvValue:  ptr to zero terminated environment value string

    EnvVarComponents: ptr to a PCHAR * variable to receive the buffer of
                      ptrs to the constituent value strings.

    PNumComponents: ptr to a ULONG to receive the number of value strings found

Return Value:

    The function returns the following error codes:
         EACCES if EnvValue is NULL
         ENOMEM if the memory allocation fails


    The function returns the following success codes:
         ESUCCESS.

    When the function returns ESUCCESS:
         - *PNumComponent field gets the number of value strings found
         - if the number is non zero the *EnvVarComponents field gets the
           ptr to the buffer containing ptrs to value strings

--*/


{
    PCHAR pchStart, pchEnd, pchNext;
    PCHAR pchComponents[MAX_COMPONENTS + 1];
    ULONG NumComponents, i;
    PCHAR pch;
    ULONG size;

    //
    // Validate the EnvValue
    //
    if (EnvValue == NULL) {
        return (EACCES);
    }

    //
    // Initialise the ptr array with nulls
    //
    for (i = 0; i < (MAX_COMPONENTS+1); i++) {
        pchComponents[i] = NULL;
    }

    //
    // Initialise ptrs to search components
    //
    pchStart      = EnvValue;
    NumComponents = 0;


    //
    // search till either pchStart reaches the end or till max components
    // is reached, the below has been programmed from a dfsa.
    //
    while (*pchStart && NumComponents < MAX_COMPONENTS) {

        //
        // STATE 1: find the beginning of next variable value
        //
        while (*pchStart!=0 && isspace(*pchStart)) {
            pchStart++;
        }


        if (*pchStart == 0) {
            break;
        }

        //
        // STATE 2: In the midst of a value
        //
        pchEnd = pchStart;
        while (*pchEnd!=0 && !isspace(*pchEnd) && *pchEnd!=';') {
            pchEnd++;
        }

        //
        // STATE 3: spit out the value found
        //

        size = pchEnd - pchStart;
        if ((pch = AlAllocateHeap(size+1)) == NULL) {
            AlpFreeComponents(pchComponents);
            return (ENOMEM);
        }
        strncpy (pch, pchStart, size);
        pch[size]=0;
        pchComponents[NumComponents++]=pch;

        //
        // STATE 4: variable value end has been reached, find the beginning
        // of the next value
        //
        if ((pchNext = strchr(pchEnd, ';')) == NULL) {
            break; // out of the big while loop because we are done
        }

        //
        // Advance beyond the semicolon.
        //

        pchNext++;

        //
        // reinitialise to begin STATE 1
        //
        pchStart = pchNext;

    } // end while.

    //
    // Get memory to hold an environment pointer and return that
    //

    if ( NumComponents!=0 ) {
        PCHAR *pch;

        if ((pch = (PCHAR *)AlAllocateHeap((NumComponents+1)*sizeof(PCHAR))) == NULL) {
            AlpFreeComponents(pchComponents);
            return (ENOMEM);
        }

        //
        // the last one is NULL because we initialised the array with NULLs
        //

        for ( i = 0; i <= NumComponents; i++) {
            pch[i] = pchComponents[i];
        }


        *EnvVarComponents = pch;
    }

    //
    // Update the number of elements field and return success
    //
    *PNumComponents = NumComponents;
    return (ESUCCESS);
}


ARC_STATUS
AlFreeEnvVarComponents (
    IN PCHAR *EnvVarComponents
    )
/*++

Routine Description:

    This routine frees up all the components in the ptr array and frees
    up the storage for the ptr array itself too

Arguments:

    EnvVarComponents: the ptr to the PCHAR * Buffer

Return Value:

    ESUCCESS if freeing successful
    EACCES   if memory ptr invalid



--*/


{
    ARC_STATUS Status;

    //
    // if the pointer is NULL just return success
    //
    if (EnvVarComponents == NULL) {
        return (ESUCCESS);
    }

    //
    // free all the components first, if error in freeing return
    //
    Status = AlpFreeComponents(EnvVarComponents);
    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // free the component holder too
    //
    if( AlDeallocateHeap(EnvVarComponents) != NULL) {
        return (EACCES);
    }
    else {
        return (ESUCCESS);
    }

}


ARC_STATUS
AlpFreeComponents(
    IN PCHAR *EnvVarComponents
    )

/*++

Routine Description:

   This routine frees up only the components in the ptr array, but doesn't
   free the ptr array storage itself.

Arguments:

    EnvVarComponents: the ptr to the PCHAR * Buffer

Return Value:

    ESUCCESS if freeing successful
    EACCES   if memory ptr invalid

--*/

{

    //
    // get all the components and free them
    //
    while (*EnvVarComponents != NULL) {
        if(AlDeallocateHeap(*EnvVarComponents++) != NULL) {
            return(EACCES);
        }
    }

    return(ESUCCESS);
}


BOOLEAN
AlpMatchComponent(
    IN PCHAR Value1,
    IN PCHAR Value2
    )

/*++

Routine Description:

    This routine compares two components to see if they are equal.  This is
    essentially comparing strings except that leading zeros are stripped from
    key values.

Arguments:

    Value1 - Supplies a pointer to the first value to match.

    Value2 - Supplies a pointer to the second value to match.


Return Value:

    If the components match, TRUE is returned, otherwise FALSE is returned.

--*/

{
    while ((*Value1 != 0) && (*Value2 != 0)) {
        if (tolower(*Value1) != tolower(*Value2)) {
            return FALSE;
        }

        if (*Value1 == '(') {
            do {
                *Value1++;
            } while (*Value1 == '0');
        } else {
            *Value1++;
        }

        if (*Value2 == '(') {
            do {
                *Value2++;
            } while (*Value2 == '0');
        } else {
            *Value2++;
        }
    }

    if ((*Value1 == 0) && (*Value2 == 0)) {
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
AlFindNextMatchComponent(
    IN PCHAR EnvValue,
    IN PCHAR MatchValue,
    IN ULONG StartComponent,
    OUT PULONG MatchComponent OPTIONAL
    )

/*++

Routine Description:

    This routine compares each component of EnvValue, starting with
    StartComponent, until a match is found or there are no more components.

Arguments:

    EnvValue - Supplies a pointer to the environment variable value.

    MatchValue - Supplies a pointer to the value to match.

    StartComponent - Supplies the component number to start the match.

    MatchComponent - Supplies an optional pointer to a variable to receive
                     the number of the component that matched.

Return Value:

    If a match is found, TRUE is returned, otherwise FALSE is returned.

--*/

{
    ARC_STATUS Status;
    PCHAR *EnvVarComponents;
    ULONG NumComponents;
    ULONG Index;
    BOOLEAN Match;


    Status = AlGetEnvVarComponents(EnvValue, &EnvVarComponents, &NumComponents);

    if (Status != ESUCCESS) {
        return FALSE;
    }

    Match = FALSE;
    for (Index = StartComponent ; Index < NumComponents ; Index++ ) {
        if (AlpMatchComponent(EnvVarComponents[Index], MatchValue)) {
            Match = TRUE;
            break;
        }
    }

    if (ARGUMENT_PRESENT(MatchComponent)) {
        *MatchComponent = Index;
    }

    AlFreeEnvVarComponents(EnvVarComponents);
    return Match;
}


ARC_STATUS
AlAddSystemPartition(
    IN PCHAR NewSystemPartition
    )

/*++

Routine Description:

    This routine adds a system partition to the SystemPartition environment
    variable, and updates the Osloader, OsloadPartition, OsloadFilename,
    and OsloadOptions variables.

Arguments:

    SystemPartition - Supplies a pointer to the pathname of the system
                      partition to add.

Return Value:

    If the system partition was successfully added, ESUCCESS is returned,
    otherwise an error code is returned.

    BUGBUG - This program is simplistic and doesn't attempt to make sure all
    the variables are consistent.  It also doesn't fail gracefully.

--*/

{
    ARC_STATUS Status;
    PCHAR SystemPartition;
    CHAR TempValue[MAXIMUM_ENVIRONMENT_VALUE];
    //PCHAR Osloader;
    //PCHAR OsloadPartition;
    //PCHAR OsloadFilename;
    //PCHAR OsloadOptions;

    //
    // Get the system partition environment variable.
    //

    SystemPartition = ArcGetEnvironmentVariable("SystemPartition");

    //
    // If the variable doesn't exist, add it and exit.
    //

    if (SystemPartition == NULL) {
        if(strlen(NewSystemPartition) < MAXIMUM_ENVIRONMENT_VALUE) {
            Status = ArcSetEnvironmentVariable("SystemPartition",
                                               NewSystemPartition);
        } else {
            Status = E2BIG;
        }
        return Status;
    }

    //
    // If the variable exists, add the new partition to the end.
    //
    if(strlen(SystemPartition)+strlen(NewSystemPartition)+2 > MAXIMUM_ENVIRONMENT_VALUE) {
        return(E2BIG);
    }

    strcpy(TempValue, SystemPartition);
    strcat(TempValue, ";");
    strcat(TempValue, NewSystemPartition);
    Status = ArcSetEnvironmentVariable("SystemPartition",
                                       TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

#if 0
    //
    // Add semicolons to the end of each of the associated variables.
    // If they don't exist add them.
    //

    //
    // Get the Osloader environment variable and add a semicolon to the end.
    //

    Osloader = ArcGetEnvironmentVariable("Osloader");
    if (Osloader == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, Osloader);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("Osloader",TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Get the OsloadPartition environment variable and add a semicolon to the end.
    //

    OsloadPartition = ArcGetEnvironmentVariable("OsloadPartition");
    if (OsloadPartition == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, OsloadPartition);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("OsloadPartition",TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Get the OsloadFilename environment variable and add a semicolon to the end.
    //

    OsloadFilename = ArcGetEnvironmentVariable("OsloadFilename");
    if (OsloadFilename == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, OsloadFilename);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("OsloadFilename",TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Get the OsloadOptions environment variable and add a semicolon to the end.
    //

    OsloadOptions = ArcGetEnvironmentVariable("OsloadOptions");
    if (OsloadOptions == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, OsloadOptions);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("OsloadOptions",TempValue);
#endif
    return Status;
}


typedef struct _tagMENUITEM {
    PCHAR Text;
    ULONG AssociatedData;
} MENUITEM,*PMENUITEM;

typedef struct _tagMENUCOOKIE {
    ULONG     ItemCount;
    PMENUITEM Items;
} MENUCOOKIE,*PMENUCOOKIE;


// indent for menus, status, etc.

char MARGIN[] = "          ";
char MSGMARGIN[] = " ";

// special constants used when fetching keystrokes

#define KEY_UP 1
#define KEY_DOWN 2


VOID
MarkLine(
    ULONG   Line,
    BOOLEAN Selected,
    PCHAR String
    );

BOOLEAN
CommonMenuDisplay(
    PMENUCOOKIE Menu,
    BOOLEAN     StaticMenu,
    PCHAR       Items[],
    ULONG       ItemCount,
    BOOLEAN     PrintOnly,
    ULONG       AssociatedDataOfDefaultChoice,
    ULONG      *AssociatedDataOfChoice,
    PCHAR       MenuName,
    ULONG       Row
    );

char
GetChar(
    VOID
    );


BOOLEAN
AlInitializeMenuPackage(
    VOID
    )
{
    return(TRUE);
}


ULONG
AlGetMenuNumberItems(
    PVOID MenuID
    )
{
    return(((PMENUCOOKIE)MenuID)->ItemCount);
}


ULONG
AlGetMenuAssociatedData(
    PVOID MenuID,
    ULONG n
    )
{
    return(((PMENUCOOKIE)MenuID)->Items[n].AssociatedData);
}

BOOLEAN
AlNewMenu(
    PVOID *MenuID
    )
{
    PMENUCOOKIE p;

    if(!(p = AlAllocateHeap(sizeof(MENUCOOKIE)))) {
        return(FALSE);
    }
    p->ItemCount = 0;
    p->Items = NULL;
    *MenuID = p;
    return(TRUE);
}


VOID
AlFreeMenu(
    PVOID MenuID
    )
{
    PMENUCOOKIE p = MenuID;
    ULONG       i;

    for(i=0; i<p->ItemCount; i++) {
        if(p->Items[i].Text != NULL) {
            AlDeallocateHeap(p->Items[i].Text);
        }
    }
    if(p->Items) {
        AlDeallocateHeap(p->Items);
    }
    AlDeallocateHeap(p);
}


BOOLEAN
AlAddMenuItem(
    PVOID MenuID,
    PCHAR Text,
    ULONG AssociatedData,
    ULONG Attributes                // unused
    )
{
    PMENUCOOKIE Menu = MenuID;
    PMENUITEM   p;

    DBG_UNREFERENCED_PARAMETER(Attributes);

    if(!Menu->ItemCount) {
        if((Menu->Items = AlAllocateHeap(sizeof(MENUITEM))) == NULL) {
            return(FALSE);
        }
        Menu->ItemCount = 1;
        p = Menu->Items;
    } else {
        if((p = AlReallocateHeap(Menu->Items,sizeof(MENUITEM)*(Menu->ItemCount+1))) == NULL) {
            return(FALSE);
        }
        Menu->Items = p;
        p = &Menu->Items[Menu->ItemCount++];
    }

    if((p->Text = AlAllocateHeap(strlen(Text)+1)) == NULL) {
        return(FALSE);
    }
    strcpy(p->Text,Text);
    p->AssociatedData = AssociatedData;
    return(TRUE);
}


BOOLEAN
AlAddMenuItems(
    PVOID MenuID,
    PCHAR Text[],
    ULONG ItemCount
    )
{
    ULONG base,i;

    base = AlGetMenuNumberItems(MenuID);

    for(i=0; i<ItemCount; i++) {
    if(!AlAddMenuItem(MenuID,Text[i],i+base,0)) {
            return(FALSE);
        }
    }
    return(TRUE);
}


BOOLEAN
AlDisplayMenu(
    PVOID   MenuID,
    BOOLEAN PrintOnly,
    ULONG   AssociatedDataOfDefaultChoice,
    ULONG  *AssociatedDataOfChoice,
    ULONG   Row,
    PCHAR   MenuName
    )
{
    return(CommonMenuDisplay((PMENUCOOKIE)MenuID,
                             FALSE,
                             NULL,
                             ((PMENUCOOKIE)MenuID)->ItemCount,
                             PrintOnly,
                             AssociatedDataOfDefaultChoice,
                             AssociatedDataOfChoice,
                             MenuName,
                             Row
                            )
          );
}


BOOLEAN
AlDisplayStaticMenu(
    PCHAR  Items[],
    ULONG  ItemCount,
    ULONG  DefaultChoice,
    ULONG  Row,
    ULONG *IndexOfChoice
    )
{
    return(CommonMenuDisplay(NULL,
                             TRUE,
                             Items,
                             ItemCount,
                             FALSE,
                             DefaultChoice,
                             IndexOfChoice,
                             NULL,
                             Row
                            )
          );
}



BOOLEAN
CommonMenuDisplay(
    PMENUCOOKIE Menu,
    BOOLEAN     StaticMenu,
    PCHAR       Items[],
    ULONG       ItemCount,
    BOOLEAN     PrintOnly,
    ULONG       AssociatedDataOfDefaultChoice,
    ULONG      *AssociatedDataOfChoice,
    PCHAR       MenuName,
    ULONG       Row
    )
{
//    ULONG x;
    ULONG i,MenuBaseLine,Selection;
    char  c;
    PCHAR String;

    AlSetPosition(Row,0);
    AlPrint("%cJ",ASCI_CSI);            // clear to end of screen.
    MenuBaseLine = Row;

    AlSetScreenColor(7,4);              // white on blue

//    if(MenuName) {
//        AlPrint("%s%s\r\n%s",MARGIN,MenuName,MARGIN);
//        x = strlen(MenuName);
//        for(i=0; i<x; i++) {
//            AlPrint("-");
//        }
//        AlPrint("\r\n\r\n");
//        MenuBaseLine += 3;
//    }

    for(i=0; i<ItemCount; i++) {
        AlSetScreenAttributes(1,0,0);   // hi intensity
        AlPrint("%s%s\r\n",MARGIN,StaticMenu ? Items[i] : Menu->Items[i].Text);
    }

    if(PrintOnly) {

        char dummy;
        AlPrint("\r\nPress any key to continue.");
        AlGetString(&dummy,0);

    } else {

//        AlPrint("\r\n%sMake Selection using arrow keys and return,\r\n%sor escape to cancel",MARGIN,MARGIN);

        Selection = 0;
        if(StaticMenu) {
            Selection = AssociatedDataOfDefaultChoice;
        } else {
            for(i=0; i<ItemCount; i++) {
                if(Menu->Items[i].AssociatedData == AssociatedDataOfDefaultChoice) {
                    Selection = i;
                    break;
                }
            }
        }

        String = StaticMenu ? Items[Selection] : Menu->Items[Selection].Text;
        MarkLine(MenuBaseLine+Selection,TRUE, String);

        while(((c = GetChar()) != ASCI_ESC) && (c != ASCI_LF) && (c != ASCI_CR)) {

            String = StaticMenu ? Items[Selection] : Menu->Items[Selection].Text;
            MarkLine(MenuBaseLine+Selection,FALSE,String);

            if(c == KEY_UP) {
                if(!Selection--) {
                    Selection = ItemCount - 1;
                }
            } else if(c == KEY_DOWN) {
                if(++Selection == ItemCount) {
                    Selection = 0;
                }
            }

            String = StaticMenu ? Items[Selection] : Menu->Items[Selection].Text;
            MarkLine(MenuBaseLine+Selection,TRUE,String);
        }

        // set cursor to a free place on the screen.
        AlSetPosition(MenuBaseLine + ItemCount + 4,0);

        if(c == ASCI_ESC) {
            return(FALSE);
        }

        *AssociatedDataOfChoice = StaticMenu ? Selection : Menu->Items[Selection].AssociatedData;
    }
    return(TRUE);
}



VOID
MarkLine(
    ULONG Line,
    BOOLEAN Selected,
    PCHAR String
    )
{
    AlSetPosition(Line,sizeof(MARGIN));
    if (Selected) {
        AlSetScreenAttributes(1,0,1);   // hi intensity, Reverse Video
    }
    AlPrint("%s\r\n", String);
    AlSetScreenAttributes(1,0,0);       // hi intensity
}



char
GetChar(
    VOID
    )
{
    UCHAR c;
    ULONG count;

    ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);
    switch(c) {
//  case ASCI_ESC:
//      ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);
//      if(c != '[') {
//          break;
//      }
    case ASCI_CSI:
        ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);
        switch(c) {
        case 'A':
        case 'D':
            return(KEY_UP);
        case 'B':
        case 'C':
            return(KEY_DOWN);
        }
    default:
        return(c);
    }
}



VOID
AlWaitKey(
    PCHAR Prompt
    )
{
    char buff[1];

    AlPrint(MSGMARGIN);
    AlPrint(Prompt ? Prompt : "Press any key to continue...");
    AlGetString(buff,0);
}


VOID
vAlStatusMsg(
    IN ULONG   Row,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    IN va_list ArgumentList
    )
{
    char  text[256];
    ULONG Length,Count;

    AlSetPosition(Row,0);
    AlPrint(MSGMARGIN);
    Length = vsprintf(text,FormatString,ArgumentList);
    if(Error) {
        AlSetScreenColor(1,4);         // red on blue
    } else {
        AlSetScreenColor(3,4);         // yellow on blue
    }
    AlSetScreenAttributes(1,0,0);      // hi intensity
    ArcWrite(ARC_CONSOLE_OUTPUT,text,Length,&Count);
    AlPrint("\r\n");
    AlSetScreenColor(7,4);             // white on blue
    AlSetScreenAttributes(1,0,0);      // hi intensity
}


VOID
AlStatusMsg(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    )
{
    va_list ArgList;

    va_start(ArgList,FormatString);
    vAlStatusMsg(TopRow,Error,FormatString,ArgList);

    AlWaitKey(NULL);
    AlClearStatusArea(TopRow,BottomRow);
}


VOID
AlStatusMsgNoWait(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    )
{
    va_list ArgList;

    AlClearStatusArea(TopRow,BottomRow);
    va_start(ArgList,FormatString);
    vAlStatusMsg(TopRow,Error,FormatString,ArgList);
}


VOID
AlClearStatusArea(
    IN ULONG TopRow,
    IN ULONG BottomRow
    )
{
    ULONG i;

    for(i=BottomRow; i>=TopRow; --i) {
        AlSetPosition(i,0);
        AlClearLine();
    }
}


ARC_STATUS
AlGetMenuSelection(

    IN  PCHAR   szTitle,
    IN  PCHAR   *rgszSelections,
    IN  ULONG   crgsz,
    IN  ULONG   crow,
    IN  ULONG   irgszDefault,
    OUT PULONG  pirgsz,
    OUT PCHAR   *pszSelection
    )
/*++

Routine Description:

    This routine takes an array of strings, turns them into a menu
    and gets a selection. If ESC hit then ESUCCESS is returned with
    the *pszSelection NULL.

    crgsz is assume to be 1 or greater.


Arguments:

    szTitle - Pointer to menu title to pass to AlDisplayMenu
    prgszSelection - pointer to an array of strings for menu
    crgsz - count of strings
    irgszDefault - index in rgszSelection to use as default selection

Return Value:

    irgsz - index to selection
    pszSelection - pointer int rgszSelection for selection. Note that
                   this is not a dup and should not be freed seperately
                   then rgszSelections.

    Note: if ARC_STATUS == ESUCCESS and pszSelection == NULL then the
          menu was successfully displayed but the user hit ESC to select
          nothing from the menu.

--*/


{

    PVOID  hdMenuId;

    *pszSelection = NULL;
    if (!AlNewMenu(&hdMenuId)) {

        return( ENOMEM );

    }

    //
    // BUGBUG for now 1 selection will also build a menu, in the
    // future once this is working we should just return that selection
    //

    if (!AlAddMenuItems(hdMenuId, rgszSelections, crgsz)) {

        AlFreeMenu(hdMenuId);
        return( ENOMEM );

    }

    if (!AlDisplayMenu(hdMenuId,
                       FALSE,
                       irgszDefault,
                       pirgsz,
                       crow,
                       szTitle)) {

        //
        // User did not pick a system partition. return NULL
        // can caller should abort
        //
        AlFreeMenu(hdMenuId);
        return( ESUCCESS );

    }

    AlFreeMenu(hdMenuId);
    *pszSelection = rgszSelections[*pirgsz];
    return( ESUCCESS );

}

PCHAR
AlStrDup(

    IN  PCHAR   szString
    )

/*++

Routine Description:

    This routine makes a copy of the passed in string. I do not use
    the CRT strdup since it uses malloc.

Arguments:

    szString - pointer of string to dup.

Return Value:

    pointer to dup'd string. NULL if could not allocate

--*/
{

    PCHAR   szT;

    if (szT = AlAllocateHeap(strlen(szString) + 1)) {

        strcpy(szT, szString);
        return(szT);

    }
    return( NULL );

}


PCHAR
AlCombinePaths (

    IN  PCHAR   szPath1,
    IN  PCHAR   szPath2
    )

/*++

Routine Description:

    This routine combines to strings. It allocate a new string
    to hold both strings.

Arguments:

    pointer to combined path. NULL if failed to allocate.

Return Value:


--*/
{

    PCHAR   szT;

    if (szT = AlAllocateHeap(strlen(szPath1) + strlen(szPath2) + 1)) {

        strcpy(szT, szPath1);
        strcat(szT, szPath2);
        return( szT );

    } else {

        return ( NULL );

    }

}

VOID
AlFreeArray (

    IN  BOOLEAN fFreeArray,
    IN  PCHAR   *rgsz,
    IN  ULONG   csz
    )
/*++

Routine Description:

    This routine iterates through an array of pointers to strings freeing
    each string and finally the array itself.

Arguments:

    fFreeArray - flag wither to free the array itself.
    rgsz - pointer to array of strings.
    csz - size of array.

Return Value:


--*/

{

    ULONG   irgsz;

    if (!csz) {

        return;

    }

    for( irgsz = 0; irgsz < csz; irgsz++ ) {

        if (rgsz[irgsz]) {

            AlDeallocateHeap(rgsz[irgsz]);

        } else {

            break;

        }

    }
    if (fFreeArray) {
        AlDeallocateHeap( rgsz );
    }

}

ARC_STATUS
AlGetBase (
    IN  PCHAR   szPath,
    OUT PCHAR   *pszBase
    )

/*++

Routine Description:


    This routine strips the filename off a path.

Arguments:

    szPath - path to strip.

Return Value:

    pszBaseh - pointer to buffer holding new base. (this is a copy)

--*/

{

    PCHAR   szPathT;

    //
    // Make local copy of szArcInstPath so we can alter it
    //
    *pszBase = AlStrDup(szPath);
    if ( *pszBase == NULL ) {

        return( ENOMEM );
    }

    //
    // The start of the path part should be either a \ or a ) where
    // ) is the end of the arc name
    //
    if ((szPathT = strrchr(*pszBase,'\\')) == 0) {
        if ((szPathT = strrchr(*pszBase, ')')) == 0) {

            AlDeallocateHeap(*pszBase);
            return( EBADSYNTAX );
        }
    }


    //
    // Cut filename out
    //
    // szPath points to either ')' or '\' so need to move over that
    // onto actual name
    //
    *(szPathT + 1) = 0;
    return( ESUCCESS );


}

//
// Define static data.
//


PCHAR AdapterTypes[AdapterMaximum + 1] = {"eisa","scsi", "multi", NULL};

PCHAR ControllerTypes[ControllerMaximum + 1] = {"cdrom", "disk", NULL};

PCHAR PeripheralTypes[PeripheralMaximum + 1] = {"rdisk", "fdisk", NULL};



PCHAR
AlGetNextArcNamToken (
    IN PCHAR TokenString,
    OUT PCHAR OutputToken,
    OUT PULONG UnitNumber
    )

/*++

Routine Description:

    This routine scans the specified token string for the next token and
    unit number. The token format is:

        name[(unit)]

Arguments:

    TokenString - Supplies a pointer to a zero terminated token string.

    OutputToken - Supplies a pointer to a variable that receives the next
        token.

    UnitNumber - Supplies a pointer to a variable that receives the unit
        number.

Return Value:

    If another token exists in the token string, then a pointer to the
    start of the next token is returned. Otherwise, a value of NULL is
    returned.

--*/

{

    //
    // If there are more characters in the token string, then parse the
    // next token. Otherwise, return a value of NULL.
    //

    if (*TokenString == '\0') {
        return NULL;

    } else {
        while ((*TokenString != '\0') && (*TokenString != '(')) {
            *OutputToken++ = *TokenString++;
        }

        *OutputToken = '\0';

        //
        // If a unit number is specified, then convert it to binary.
        // Otherwise, default the unit number to zero.
        //

        *UnitNumber = 0;
        if (*TokenString == '(') {
            TokenString += 1;
            while ((*TokenString != '\0') && (*TokenString != ')')) {
                *UnitNumber = (*UnitNumber * 10) + (*TokenString++ - '0');
            }

            if (*TokenString == ')') {
                TokenString += 1;
            }
        }
    }

    return TokenString;
}


ULONG
AlMatchArcNamToken (
    IN PCHAR TokenValue,
    IN TOKEN_TYPE TokenType
    )

/*++

Routine Description:

    This routine attempts to match a token with an array of possible
    values.

Arguments:

    TokenValue - Supplies a pointer to a zero terminated token value.

    TokenType  - Indicates which type of token we are dealing with
                 (AdapterType/ControllerType/PeripheralType)

Return Value:

    If the token type is invalid, INVALID_TOKEN_TYPE is returned.

    If a token match is not located, then a value INVALID_TOKEN_VALUE
    is returned.

    If a token match is located, then the ENUM value of the token is
    returned.

--*/

{

    ULONG   Index;
    PCHAR   MatchString;
    PCHAR   TokenString;
    PCHAR   *TokenArray;
    BOOLEAN Found;

    //
    // Depending on token type choose the appropriate token string array
    //
    switch (TokenType) {
        case AdapterType:
            TokenArray = AdapterTypes;
            break;

        case ControllerType:
            TokenArray =  ControllerTypes;
            break;

        case PeripheralType:
            TokenArray = PeripheralTypes;
            break;

        default:
            return ((ULONG)INVALID_TOKEN_TYPE);
    }

    //
    // Scan the match array until either a match is found or all of
    // the match strings have been scanned.
    //
    // BUGBUG** The code below can be easily implemented using _strcmpi.
    //

    Index = 0;
    Found = FALSE;
    while (TokenArray[Index] != NULL) {
        MatchString = TokenArray[Index];
        TokenString = TokenValue;
        while ((*MatchString != '\0') && (*TokenString != '\0')) {
            if (toupper(*MatchString) != toupper(*TokenString)) {
                break;
            }

            MatchString += 1;
            TokenString += 1;
        }

        if ((*MatchString == '\0') && (*TokenString == '\0')) {
            Found = TRUE;
            break;
        }

        Index += 1;
    }

    return (Found ? Index : INVALID_TOKEN_VALUE);
}

ULONG
AlPrint (
    PCHAR Format,
    ...
    )

{

    va_list arglist;
    UCHAR Buffer[256];
    ULONG Count;
    ULONG Length;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    Length = vsprintf(Buffer, Format, arglist);
    ArcWrite( ARC_CONSOLE_OUTPUT, Buffer, Length, &Count);
    va_end(arglist);
    return 0;
}


BOOLEAN
AlGetString(
    OUT PCHAR String,
    IN  ULONG StringLength
    )

/*++

Routine Description:

    This routine reads a string from standardin until a
    carriage return or escape is found or StringLength is reached.

Arguments:

    String - Supplies a pointer to where the string will be stored.

    StringLength - Supplies the Max Length to read.

Return Value:

    FALSE if user pressed esc, TRUE otherwise.

--*/

{
    CHAR    c;
    ULONG   Count;
    PCHAR   Buffer;

    Buffer = String;
    while (ArcRead(ARC_CONSOLE_INPUT,&c,1,&Count)==ESUCCESS) {
        if(c == ASCI_ESC) {
            return(FALSE);
        }
        if ((c=='\r') || (c=='\n') || ((ULONG)(Buffer-String) == StringLength)) {
            *Buffer='\0';
            ArcWrite(ARC_CONSOLE_OUTPUT,"\r\n",2,&Count);
            return(TRUE);
        }
        //
        // Check for backspace;
        //
        if (c=='\b') {
            if (((ULONG)Buffer > (ULONG)String)) {
                Buffer--;
                ArcWrite(ARC_CONSOLE_OUTPUT,"\b \b",3,&Count);
            }
        } else {
            //
            // If it's a printable char store it and display it.
            //
            if (isprint(c)) {
                *Buffer++ = c;
                ArcWrite(ARC_CONSOLE_OUTPUT,&c,1,&Count);
            }
        }
    }
}
