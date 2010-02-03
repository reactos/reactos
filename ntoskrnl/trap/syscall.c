
VOID
FASTCALL
// KiSystemCall(IN ULONG SystemCallNumber, IN PVOID Arguments)
KiSystemCall(KTRAP_FRAME *TrapFrame)
{
    PVOID Arguments;
	PKTHREAD Thread;
    PKTRAP_FRAME TrapFrame;
    PKSERVICE_TABLE_DESCRIPTOR DescriptorTable;
    ULONG Id, Offset, StackBytes;
    PVOID Handler;
	ULONG Result;

	CpuSetFs(KGDT_R0_PCR);
        
	// get arguments
	Arguments = TrapFrame->Edx;

	/* Chain trap frames !!! */
    TrapFrame->Edx = (ULONG_PTR)Thread->TrapFrame;
    
	/* Decode the system call number */
    Offset = (TrapFrame->Eax >> SERVICE_TABLE_SHIFT) & SERVICE_TABLE_MASK;
    Id = TrapFrame->Eax & SERVICE_NUMBER_MASK;

	/* Loop because we might need to try this twice in case of a GUI call */
    for(;;)
    {
        Thread = KeGetCurrentThread();
		DescriptorTable = (PVOID)((ULONG_PTR)Thread->ServiceTable + Offset);
    
        /* Validate the system call number */
		if (id <= DescriptorTable->limit)
			break;

        /* Check if this is a GUI call */
        if (!(Offset & SERVICE_TABLE_TEST))
        {
            /* Fail the call */
            Result = STATUS_INVALID_SYSTEM_SERVICE;
            goto ExitCall;
        }

        /* Convert us to a GUI thread -- must wrap in ASM to get new EBP */        
        Result = KiConvertToGuiThread();
        if (!NT_SUCCESS(Result))
        {
            /* Figure out how we should fail to the user */
			DPRINT1("KiConvertToGuiThread failed\n");
            for(;;);
		}
    }
    
    /* Check if this is a GUI call */
    if (Offset & SERVICE_TABLE_TEST)
    {
        /* Get the batch count and flush if necessary */
        if (NtCurrentTeb()->GdiBatchCount) KeGdiFlushUserBatch();
    }
    
    /* Increase system call count */
    KeGetCurrentPrcb()->KeSystemCalls++;
    
    /* FIXME: Increase individual counts on debug systems */
    //KiIncreaseSystemCallCount(DescriptorTable, Id);
    
    /* Get stack bytes */
    StackBytes = DescriptorTable->Number[Id];
    
    /* Probe caller stack */
    if ( (Arguments < (PVOID)MmUserProbeAddress) && !(KiUserTrap(TrapFrame)) )
    {
        /* Access violation */
        DPRINT1("Probe caller stack failed\n");
        for(;;);
    }
    
    /* Get the handler and make the system call */
    Handler = (PVOID)DescriptorTable->Base[Id];
    Result = KiSystemCallTrampoline(Handler, Arguments, StackBytes);
    
    /* Make sure we're exiting correctly */
    KiExitSystemCallDebugChecks(Id, TrapFrame);
    
    /* Restore the old trap frame */
ExitCall:
    Thread->TrapFrame = (PKTRAP_FRAME)TrapFrame->Edx;

    /* Disable interrupts until we return */
    CpuIntDisable();
    
    /* Check for APC delivery */
    KiCheckForApcDelivery(TrapFrame);
    
	/* Exit from system call */
	TrapFrame->Eax = Result;

	/* Now exit the trap for real */
    // KiExitTrap(TrapFrame, 0);
}

VOID
FASTCALL
KiSystemCallHandler(IN PKTRAP_FRAME TrapFrame)
                    IN KPROCESSOR_MODE PreviousMode,
                    IN KPROCESSOR_MODE PreviousPreviousMode,
{
	// DPRINTT("\n");
    /* No error code */
    TrapFrame->ErrCode = 0;
    
    /* Clear direction flag */
    CpuCld();
    
	/* Save previous mode and FS segment */
    TrapFrame->PreviousPreviousMode = PreviousPreviousMode;
        
    /* Save the SEH chain and terminate it for now */    
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;
    KeGetPcr()->Tib.ExceptionList = EXCEPTION_CHAIN_END;
        
    /* Clear DR7 and check for debugging */
    TrapFrame->Dr7 = 0;
    if (__builtin_expect(Thread->DispatcherHeader.DebugActive & 0xFF, 0))
    {
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Set thread fields */
    Thread->TrapFrame = TrapFrame;
    Thread->PreviousMode = PreviousMode;
    
    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);
    
    /* Enable interrupts and make the call */
    CpuIntEnable();
    KiSystemCall(ServiceNumber, Arguments);   
}

VOID
_FASTCALL
KiFastCallEntryHandler(IN PKTRAP_FRAME TrapFrame)
{
	DPRINTT("\n");

	/* Set up a fake INT Stack and enable interrupts */
    TrapFrame->HardwareSegSs = KGDT_R3_DATA | RPL_MASK;
    TrapFrame->HardwareEsp = (ULONG_PTR)Arguments - 8; // Stack is 2 frames down
    TrapFrame->EFlags |= EFLAGS_INTERRUPT_MASK;		// ???
    TrapFrame->SegCs = KGDT_R3_CODE | RPL_MASK;
    TrapFrame->Eip = SharedUserData->SystemCallReturn;
    __writeeflags(0x2);		// !!! sure? shouldn't be necessary
    
    /* Call the shared handler (inline) */
    KiSystemCallHandler(TrapFrame);
}

VOID
_FASTCALL
KiSystemServiceHandler(IN PKTRAP_FRAME TrapFrame)
{
	// DPRINTT("\n");

    /* Call the shared handler (inline) */
    KiSystemCallHandler(TrapFrame);,
}


