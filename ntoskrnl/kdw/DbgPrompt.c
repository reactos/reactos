#if 0
ULONG
NTAPI
DebugPrompt(IN PSTRING Output,
            IN PSTRING Input)
{
    /* Call the Debug Service */
    return DebugService(BREAKPOINT_PROMPT,
                        Output->Buffer,
                        UlongToPtr(Output->Length),
                        Input->Buffer,
                        UlongToPtr(Input->MaximumLength));
}
#endif

ULONG
NTAPI
DbgPrompt(IN PCCH Prompt,
          OUT PCH Response,
          IN ULONG MaximumResponseLength)
{
    STRING Output;
    STRING Input;

    /* Setup the input string */
    Input.MaximumLength = (USHORT)MaximumResponseLength;
    Input.Buffer = Response;

    /* Setup the output string */
    Output.Length = strlen(Prompt);
    Output.Buffer = (PCHAR)Prompt;

    /* Call the system service */
    return DebugService(BREAKPOINT_PROMPT,
                        Output.Buffer,
                        UlongToPtr(Output.Length),
                        Input.Buffer,
                        UlongToPtr(Input.MaximumLength));
}

