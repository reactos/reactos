#include "ppl.h"


CRITICAL_SECTION g_cs;
POUR_PROP_PARAMS Head = NULL;

#define Enter() EnterCriticalSection(&g_cs);
#define Leave() LeaveCriticalSection(&g_cs);

/*++

Routine Description: PplConstruct

    Constructs and initializes the Prop Params linked list

--*/
void PplConstruct(void)
{
    InitializeCriticalSection(&g_cs);
    Head = NULL;
}

/*++

Routine Description: PplDestruct

    Destroys the linked list and deallocates any remaining nodes in the list

--*/
void PplDestruct(void)
{
    POUR_PROP_PARAMS current, tmp;

    Enter();

    current = Head;
    while (current) {
        tmp = current;
        current = current->Next;

        LocalFree(tmp->pAdvancedData);
        LocalFree(tmp);
    }

    Head = NULL;

    Leave();
}

/*++

Routine Description: FindParams

    Finds the device instance in the linked list.  If the device is not in the list,
    then a new node is allocated, inserted into the head of the list and returned
    to the caller.

Arguments:

    DevInst - Unique Number identifying the device

Return Value:

    POUR_PROP_PARAMS - The node in the list that it represented by the DevInst
                        or NULL if not enough memory is available

--*/
POUR_PROP_PARAMS FindParams(DWORD DevInst)
{
    POUR_PROP_PARAMS tmp, current;

    // 
    // No need for Enter()/Leave() because this func is called from within 
    // PplAddParams exclusively
    //

    //
    // Iterate through the list and try to find the node with the same
    // DevInst
    //
    for (current = Head; current; current = current->Next) {
        if (current->DeviceInfoData->DevInst == DevInst) {
            OutputDebugString(_T("found duplicate!"));

            //
            // This node in the list represents this DevInst.  Clear out all previous
            // information, keeping the following in minde
            //  1  Save the next node in the list so that the list will remain intact.  
            //  2  Freeing the now defunct pAdvancedData 
            //
            tmp = current->Next;
            if (current->pAdvancedData) {
                LocalFree(current->pAdvancedData);
            }

            ZeroMemory(current, sizeof(OUR_PROP_PARAMS));
            current->Next = tmp;

            return current;
        }
    }

    //
    // We didn't find that particular DevInst in the list, so we create one 
    //   and insert it at the head of the list
    //
    tmp = (POUR_PROP_PARAMS) LocalAlloc(LMEM_FIXED, sizeof(OUR_PROP_PARAMS)); 
    if (!tmp) {
        return NULL;
    }

    ZeroMemory(tmp, sizeof(OUR_PROP_PARAMS));
    tmp->Next = Head;
    Head =tmp;

    return tmp;
}

/*++

Routine Description: PplAddParams

    Returns a node associated with DevInst

Arguments:

    DevInst - Unique Number identifying the device

Return Value:

    POUR_PROP_PARAMS - The node in the list that it represented by the DevInst
                        or NULL if not enough memory is available

--*/
POUR_PROP_PARAMS PplAddParams(DWORD DevInst)
{
    POUR_PROP_PARAMS popp;

    Enter();
    popp = FindParams(DevInst);
    Leave();

    return popp;
}

/*++

Routine Description: PplRemoveParams

    Removes Params from the linked list.  Params is no longer valid after the
    function returns.

Arguments:

    Params - The node to remove from the linked list

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
BOOL PplRemoveParams(POUR_PROP_PARAMS Params)
{
    POUR_PROP_PARAMS * current;
    BOOL retVal = TRUE;

    Enter();

    for (current = &Head; *current; current = &((*current)->Next)) { 
        if (*current == Params) {
            break;
        }
    }

    if (*current) {
        *current = Params->Next;
    
        LocalFree(Params->pAdvancedData);
        LocalFree(Params);
    }
    else {
        retVal = FALSE;
    }
    
    Leave();

    return retVal;
}
