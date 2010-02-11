VOID NTAPI DbgCommandString(IN PCCH Name, IN PCCH Command)
{
    STRING NameString, CommandString;

    /* Setup the strings */
    NameString.Buffer = (PCHAR)Name;
    NameString.Length = strlen(Name);
    CommandString.Buffer = (PCHAR)Command;
    CommandString.Length = strlen(Command);

    /* Send them to the debugger */
    DebugService2(&NameString, &CommandString, BREAKPOINT_COMMAND_STRING);
}
