BOOLEAN
NTAPI
RtlpCaptureStackLimits(IN ULONG_PTR Ebp,
                       IN ULONG_PTR *StackBegin,
                       IN ULONG_PTR *StackEnd)
{
	PKTHREAD Thread = KeGetCurrentThread();

	/* Don't even try at ISR level or later */
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		return FALSE;

	/* Start with defaults */
	*StackBegin = Thread->StackLimit;
	*StackEnd = (ULONG_PTR)Thread->StackBase;

	/* Check if EBP is inside the stack */
	if ((*StackBegin <= Ebp) && (Ebp <= *StackEnd))
	{
		/* Then make the stack start at EBP */
		*StackBegin = Ebp;
	}
	else
	{
		/* Now we're going to assume we're on the DPC stack */
		*StackEnd = (ULONG_PTR)(KeGetPcr()->Prcb->DpcStack);
		*StackBegin = *StackEnd - KERNEL_STACK_SIZE;

		/* Check if we seem to be on the DPC stack */
		if ((*StackEnd) && (*StackBegin < Ebp) && (Ebp <= *StackEnd))
		{
			/* We're on the DPC stack */
			*StackBegin = Ebp;
		}
		else
		{
			/* We're somewhere else entirely... use EBP for safety */
			*StackBegin = Ebp;
			*StackEnd = (ULONG_PTR)PAGE_ALIGN(*StackBegin);
		}
	}
	/* Return success */
	return TRUE;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    ULONG_PTR Stack, NewStack, StackBegin, StackEnd = 0;
    ULONG Eip;
    BOOLEAN Result, StopSearch = FALSE;
    ULONG i = 0;
    PETHREAD Thread = PsGetCurrentThread();
    PTEB Teb;
    PKTRAP_FRAME TrapFrame;

    /* Get current EBP */
	CpuGetEbp_m(Stack);

    /* Set it as the stack begin limit as well */
    StackBegin = (ULONG_PTR)Stack;

    /* Check if we're called for non-logging mode */
    if (!Flags)
    {
        /* Get the actual safe limits */
        Result = RtlpCaptureStackLimits((ULONG_PTR)Stack,
                                        &StackBegin,
                                        &StackEnd);
        if (!Result) return 0;
    }

    /* Use a SEH block for maximum protection */
    _SEH2_TRY
    {
        /* Check if we want the user-mode stack frame */
        if (Flags == 1)
        {
            /* Get the trap frame and TEB */
            TrapFrame = KeGetTrapFrame(&Thread->Tcb);
            Teb = Thread->Tcb.Teb;

            /* Make sure we can trust the TEB and trap frame */
            if (!(Teb) ||
                !(Thread->SystemThread) ||
                (KeIsAttachedProcess()) ||
                (KeGetCurrentIrql() >= DISPATCH_LEVEL))
            {
                /* Invalid or unsafe attempt to get the stack */
                return 0;
            }

            /* Get the stack limits */
            StackBegin = (ULONG_PTR)Teb->NtTib.StackLimit;
            StackEnd = (ULONG_PTR)Teb->NtTib.StackBase;
#ifdef _M_IX86
            Stack = TrapFrame->Ebp;
#elif defined(_M_PPC)
            Stack = TrapFrame->Gpr1;
#else
#error Unknown architecture
#endif

            /* Validate them */
            if (StackEnd <= StackBegin) return 0;
            ProbeForRead((PVOID)StackBegin,
                         StackEnd - StackBegin,
                         sizeof(CHAR));
        }

        /* Loop the frames */
        for (i = 0; i < Count; i++)
        {
            /*
             * Leave if we're past the stack,
             * if we're before the stack,
             * or if we've reached ourselves.
             */
            if ((Stack >= StackEnd) ||
                (!i ? (Stack < StackBegin) : (Stack <= StackBegin)) ||
                ((StackEnd - Stack) < (2 * sizeof(ULONG_PTR))))
            {
                /* We're done or hit a bad address */
                break;
            }

            /* Get new stack and EIP */
            NewStack = *(PULONG_PTR)Stack;
            Eip = *(PULONG_PTR)(Stack + sizeof(ULONG_PTR));

            /* Check if the new pointer is above the oldone and past the end */
            if (!((Stack < NewStack) && (NewStack < StackEnd)))
            {
                /* Stop searching after this entry */
                StopSearch = TRUE;
            }

            /* Also make sure that the EIP isn't a stack address */
            if ((StackBegin < Eip) && (Eip < StackEnd)) break;

            /* Check if we reached a user-mode address */
            if (!(Flags) && !(Eip & 0x80000000)) break; // FIXME: 3GB breakage

            /* Save this frame */
            Callers[i] = (PVOID)Eip;

            /* Check if we should continue */
            if (StopSearch)
            {
                /* Return the next index */
                i++;
                break;
            }

            /* Move to the next stack */
            Stack = NewStack;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* No index */
        i = 0;
    }
    _SEH2_END;

    /* Return frames parsed */
    return i;
}

