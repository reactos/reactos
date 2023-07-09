
#define NO_CONSUME FALSE


extern EXPECTED_EVENT *eeList;
extern EXPECTED_EVENT masterEE;
extern DEBUG_EVENT64 falseSSEvent;

__inline EXPECTED_EVENT *
SaveEvents() {
    EXPECTED_EVENT* returnEE = eeList->next;

    eeList->next = NULL;
    return returnEE;
}

__inline void
RestoreEvents(EXPECTED_EVENT * ee)
{
    eeList->next = ee;
}


__inline BOOL DMSStep(HPID hpid, HTID htid, EXOP exop)
{
        assert(FALSE);
        return(FALSE);
}
