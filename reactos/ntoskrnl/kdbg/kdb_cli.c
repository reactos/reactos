/*
 *  ReactOS kernel
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb_cli.c
 * PURPOSE:         Kernel debugger command line interface
 * PROGRAMMER:      Gregor Anich (blight@blight.eu.org)
 *                  Hervé Poussineau
 * UPDATE HISTORY:
 *                  Created 16/01/2005
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* DEFINES *******************************************************************/

#define KEY_BS          8
#define KEY_ESC         27
#define KEY_DEL         127

#define KEY_SCAN_UP     72
#define KEY_SCAN_DOWN   80

#define KDB_ENTER_CONDITION_TO_STRING(cond)                               \
                   ((cond) == KdbDoNotEnter ? "never" :                   \
                   ((cond) == KdbEnterAlways ? "always" :                 \
                   ((cond) == KdbEnterFromKmode ? "kmode" : "umode")))

#define KDB_ACCESS_TYPE_TO_STRING(type)                                   \
                   ((type) == KdbAccessRead ? "read" :                    \
                   ((type) == KdbAccessWrite ? "write" :                  \
                   ((type) == KdbAccessReadWrite ? "rdwr" : "exec")))

#define NPX_STATE_TO_STRING(state)                                        \
                   ((state) == NPX_STATE_LOADED ? "Loaded" :              \
                   ((state) == NPX_STATE_NOT_LOADED ? "Not loaded" : "Unknown"))

/* PROTOTYPES ****************************************************************/

STATIC BOOLEAN KdbpCmdEvalExpression(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdDisassembleX(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdRegs(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdBackTrace(ULONG Argc, PCHAR Argv[]);

STATIC BOOLEAN KdbpCmdContinue(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdStep(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdBreakPointList(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdEnableDisableClearBreakPoint(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdBreakPoint(ULONG Argc, PCHAR Argv[]);

STATIC BOOLEAN KdbpCmdThread(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdProc(ULONG Argc, PCHAR Argv[]);

STATIC BOOLEAN KdbpCmdMod(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdGdtLdtIdt(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdPcr(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdTss(ULONG Argc, PCHAR Argv[]);

STATIC BOOLEAN KdbpCmdBugCheck(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdFilter(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdSet(ULONG Argc, PCHAR Argv[]);
STATIC BOOLEAN KdbpCmdHelp(ULONG Argc, PCHAR Argv[]);

/* GLOBALS *******************************************************************/

STATIC BOOLEAN KdbUseIntelSyntax = FALSE; /* Set to TRUE for intel syntax */
STATIC BOOLEAN KdbBreakOnModuleLoad = FALSE; /* Set to TRUE to break into KDB when a module is loaded */

STATIC CHAR KdbCommandHistoryBuffer[2048]; /* Command history string ringbuffer */
STATIC PCHAR KdbCommandHistory[sizeof(KdbCommandHistoryBuffer) / 8] = { NULL }; /* Command history ringbuffer */
STATIC LONG KdbCommandHistoryBufferIndex = 0;
STATIC LONG KdbCommandHistoryIndex = 0;

STATIC ULONG KdbNumberOfRowsPrinted = 0;
STATIC ULONG KdbNumberOfColsPrinted = 0;
STATIC BOOLEAN KdbOutputAborted = FALSE;
STATIC LONG KdbNumberOfRowsTerminal = -1;
STATIC LONG KdbNumberOfColsTerminal = -1;

PCHAR KdbInitFileBuffer = NULL; /* Buffer where KDBinit file is loaded into during initialization */

STATIC CONST struct
{
   PCHAR Name;
   PCHAR Syntax;
   PCHAR Help;
   BOOLEAN (*Fn)(ULONG Argc, PCHAR Argv[]);
} KdbDebuggerCommands[] = {
   /* Data */
   { NULL, NULL, "Data", NULL },
   { "?", "? expression", "Evaluate expression.", KdbpCmdEvalExpression },
   { "disasm", "disasm [address] [L count]", "Disassemble count instructions at address.", KdbpCmdDisassembleX },
   { "x", "x [address] [L count]", "Display count dwords, starting at addr.", KdbpCmdDisassembleX },
   { "regs", "regs", "Display general purpose registers.", KdbpCmdRegs },
   { "cregs", "cregs", "Display control registers.", KdbpCmdRegs },
   { "sregs", "sregs", "Display status registers.", KdbpCmdRegs },
   { "dregs", "dregs", "Display debug registers.", KdbpCmdRegs },
   { "bt", "bt [*frameaddr|thread id]", "Prints current backtrace or from given frame addr", KdbpCmdBackTrace },

   /* Flow control */
   { NULL, NULL, "Flow control", NULL },
   { "cont", "cont", "Continue execution (leave debugger)", KdbpCmdContinue },
   { "step", "step [count]", "Execute single instructions, stepping into interrupts.", KdbpCmdStep },
   { "next", "next [count]", "Execute single instructions, skipping calls and reps.", KdbpCmdStep },
   { "bl", "bl", "List breakpoints.", KdbpCmdBreakPointList },
   { "be", "be [breakpoint]", "Enable breakpoint.", KdbpCmdEnableDisableClearBreakPoint },
   { "bd", "bd [breakpoint]", "Disable breakpoint.", KdbpCmdEnableDisableClearBreakPoint },
   { "bc", "bc [breakpoint]", "Clear breakpoint.", KdbpCmdEnableDisableClearBreakPoint },
   { "bpx", "bpx [address] [IF condition]", "Set software execution breakpoint at address.", KdbpCmdBreakPoint },
   { "bpm", "bpm [r|w|rw|x] [byte|word|dword] [address] [IF condition]", "Set memory breakpoint at address.", KdbpCmdBreakPoint },

   /* Process/Thread */
   { NULL, NULL, "Process/Thread", NULL },
   { "thread", "thread [list[ pid]|[attach ]tid]", "List threads in current or specified process, display thread with given id or attach to thread.", KdbpCmdThread },
   { "proc", "proc [list|[attach ]pid]", "List processes, display process with given id or attach to process.", KdbpCmdProc },

   /* System information */
   { NULL, NULL, "System info", NULL },
   { "mod", "mod [address]", "List all modules or the one containing address.", KdbpCmdMod },
   { "gdt", "gdt", "Display global descriptor table.", KdbpCmdGdtLdtIdt },
   { "ldt", "ldt", "Display local descriptor table.", KdbpCmdGdtLdtIdt },
   { "idt", "idt", "Display interrupt descriptor table.", KdbpCmdGdtLdtIdt },
   { "pcr", "pcr", "Display processor control region.", KdbpCmdPcr },
   { "tss", "tss", "Display task state segment.", KdbpCmdTss },

   /* Others */
   { NULL, NULL, "Others", NULL },
   { "bugcheck", "bugcheck", "Bugchecks the system.", KdbpCmdBugCheck },
   { "filter", "filter [error|warning|trace|info|level]+|-[componentname|default]", "Enable/disable debug channels", KdbpCmdFilter },
   { "set", "set [var] [value]", "Sets var to value or displays value of var.", KdbpCmdSet },
   { "help", "help", "Display help screen.", KdbpCmdHelp }
};

/* FUNCTIONS *****************************************************************/

/*!\brief Transform a component name to an integer
 *
 * \param ComponentName  The name of the component.
 * \param ComponentId    Receives the component id on success.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 */
STATIC BOOLEAN
KdbpGetComponentId(
   IN  PCCH ComponentName,
   OUT PULONG ComponentId)
{
   ULONG i;

   static struct {
      PCCH Name;
      ULONG Id;
   } ComponentTable[] = {
       { "DEFAULT", DPFLTR_DEFAULT_ID },
       { "SYSTEM", DPFLTR_SYSTEM_ID },
       { "SMSS", DPFLTR_SMSS_ID },
       { "SETUP", DPFLTR_SETUP_ID },
       { "NTFS", DPFLTR_NTFS_ID },
       { "FSTUB", DPFLTR_FSTUB_ID },
       { "CRASHDUMP", DPFLTR_CRASHDUMP_ID },
       { "CDAUDIO", DPFLTR_CDAUDIO_ID },
       { "CDROM", DPFLTR_CDROM_ID },
       { "CLASSPNP", DPFLTR_CLASSPNP_ID },
       { "DISK", DPFLTR_DISK_ID },
       { "REDBOOK", DPFLTR_REDBOOK_ID },
       { "STORPROP", DPFLTR_STORPROP_ID },
       { "SCSIPORT", DPFLTR_SCSIPORT_ID },
       { "SCSIMINIPORT", DPFLTR_SCSIMINIPORT_ID },
       { "CONFIG", DPFLTR_CONFIG_ID },
       { "I8042PRT", DPFLTR_I8042PRT_ID },
       { "SERMOUSE", DPFLTR_SERMOUSE_ID },
       { "LSERMOUS", DPFLTR_LSERMOUS_ID },
       { "KBDHID", DPFLTR_KBDHID_ID },
       { "MOUHID", DPFLTR_MOUHID_ID },
       { "KBDCLASS", DPFLTR_KBDCLASS_ID },
       { "MOUCLASS", DPFLTR_MOUCLASS_ID },
       { "TWOTRACK", DPFLTR_TWOTRACK_ID },
       { "WMILIB", DPFLTR_WMILIB_ID },
       { "ACPI", DPFLTR_ACPI_ID },
       { "AMLI", DPFLTR_AMLI_ID },
       { "HALIA64", DPFLTR_HALIA64_ID },
       { "VIDEO", DPFLTR_VIDEO_ID },
       { "SVCHOST", DPFLTR_SVCHOST_ID },
       { "VIDEOPRT", DPFLTR_VIDEOPRT_ID },
       { "TCPIP", DPFLTR_TCPIP_ID },
       { "DMSYNTH", DPFLTR_DMSYNTH_ID },
       { "NTOSPNP", DPFLTR_NTOSPNP_ID },
       { "FASTFAT", DPFLTR_FASTFAT_ID },
       { "SAMSS", DPFLTR_SAMSS_ID },
       { "PNPMGR", DPFLTR_PNPMGR_ID },
       { "NETAPI", DPFLTR_NETAPI_ID },
       { "SCSERVER", DPFLTR_SCSERVER_ID },
       { "SCCLIENT", DPFLTR_SCCLIENT_ID },
       { "SERIAL", DPFLTR_SERIAL_ID },
       { "SERENUM", DPFLTR_SERENUM_ID },
       { "UHCD", DPFLTR_UHCD_ID },
       { "BOOTOK", DPFLTR_BOOTOK_ID },
       { "BOOTVRFY", DPFLTR_BOOTVRFY_ID },
       { "RPCPROXY", DPFLTR_RPCPROXY_ID },
       { "AUTOCHK", DPFLTR_AUTOCHK_ID },
       { "DCOMSS", DPFLTR_DCOMSS_ID },
       { "UNIMODEM", DPFLTR_UNIMODEM_ID },
       { "SIS", DPFLTR_SIS_ID },
       { "FLTMGR", DPFLTR_FLTMGR_ID },
       { "WMICORE", DPFLTR_WMICORE_ID },
       { "BURNENG", DPFLTR_BURNENG_ID },
       { "IMAPI", DPFLTR_IMAPI_ID },
       { "SXS", DPFLTR_SXS_ID },
       { "FUSION", DPFLTR_FUSION_ID },
       { "IDLETASK", DPFLTR_IDLETASK_ID },
       { "SOFTPCI", DPFLTR_SOFTPCI_ID },
       { "TAPE", DPFLTR_TAPE_ID },
       { "MCHGR", DPFLTR_MCHGR_ID },
       { "IDEP", DPFLTR_IDEP_ID },
       { "PCIIDE", DPFLTR_PCIIDE_ID },
       { "FLOPPY", DPFLTR_FLOPPY_ID },
       { "FDC", DPFLTR_FDC_ID },
       { "TERMSRV", DPFLTR_TERMSRV_ID },
       { "W32TIME", DPFLTR_W32TIME_ID },
       { "PREFETCHER", DPFLTR_PREFETCHER_ID },
       { "RSFILTER", DPFLTR_RSFILTER_ID },
       { "FCPORT", DPFLTR_FCPORT_ID },
       { "PCI", DPFLTR_PCI_ID },
       { "DMIO", DPFLTR_DMIO_ID },
       { "DMCONFIG", DPFLTR_DMCONFIG_ID },
       { "DMADMIN", DPFLTR_DMADMIN_ID },
       { "WSOCKTRANSPORT", DPFLTR_WSOCKTRANSPORT_ID },
       { "VSS", DPFLTR_VSS_ID },
       { "PNPMEM", DPFLTR_PNPMEM_ID },
       { "PROCESSOR", DPFLTR_PROCESSOR_ID },
       { "DMSERVER", DPFLTR_DMSERVER_ID },
       { "SR", DPFLTR_SR_ID },
       { "INFINIBAND", DPFLTR_INFINIBAND_ID },
       { "IHVDRIVER", DPFLTR_IHVDRIVER_ID },
       { "IHVVIDEO", DPFLTR_IHVVIDEO_ID },
       { "IHVAUDIO", DPFLTR_IHVAUDIO_ID },
       { "IHVNETWORK", DPFLTR_IHVNETWORK_ID },
       { "IHVSTREAMING", DPFLTR_IHVSTREAMING_ID },
       { "IHVBUS", DPFLTR_IHVBUS_ID },
       { "HPS", DPFLTR_HPS_ID },
       { "RTLTHREADPOOL", DPFLTR_RTLTHREADPOOL_ID },
       { "LDR", DPFLTR_LDR_ID },
       { "TCPIP6", DPFLTR_TCPIP6_ID },
       { "ISAPNP", DPFLTR_ISAPNP_ID },
       { "SHPC", DPFLTR_SHPC_ID },
       { "STORPORT", DPFLTR_STORPORT_ID },
       { "STORMINIPORT", DPFLTR_STORMINIPORT_ID },
       { "PRINTSPOOLER", DPFLTR_PRINTSPOOLER_ID },
   };

   for (i = 0; i < sizeof(ComponentTable) / sizeof(ComponentTable[0]); i++)
       if (_stricmp(ComponentName, ComponentTable[i].Name) == 0)
       {
           *ComponentId = ComponentTable[i].Id;
           return TRUE;
       }

   return FALSE;
}

/*!\brief Evaluates an expression...
 *
 * Much like KdbpRpnEvaluateExpression, but prints the error message (if any)
 * at the given offset.
 *
 * \param Expression  Expression to evaluate.
 * \param ErrOffset   Offset (in characters) to print the error message at.
 * \param Result      Receives the result on success.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 */
STATIC BOOLEAN
KdbpEvaluateExpression(
   IN  PCHAR Expression,
   IN  LONG ErrOffset,
   OUT PULONGLONG Result)
{
   STATIC CHAR ErrMsgBuffer[130] = "^ ";
   LONG ExpressionErrOffset = -1;
   PCHAR ErrMsg = ErrMsgBuffer;
   BOOLEAN Ok;

   Ok = KdbpRpnEvaluateExpression(Expression, KdbCurrentTrapFrame, Result,
                                  &ExpressionErrOffset, ErrMsgBuffer + 2);
   if (!Ok)
   {
      if (ExpressionErrOffset >= 0)
         ExpressionErrOffset += ErrOffset;
      else
         ErrMsg += 2;
      KdbpPrint("%*s%s\n", ExpressionErrOffset, "", ErrMsg);
   }

   return Ok;
}

/*!\brief Evaluates an expression and displays the result.
 */
STATIC BOOLEAN
KdbpCmdEvalExpression(ULONG Argc, PCHAR Argv[])
{
   ULONG i, len;
   ULONGLONG Result = 0;
   ULONG ul;
   LONG l = 0;
   BOOLEAN Ok;

   if (Argc < 2)
   {
      KdbpPrint("?: Argument required\n");
      return TRUE;
   }

   /* Put the arguments back together */
   Argc--;
   for (i = 1; i < Argc; i++)
   {
      len = strlen(Argv[i]);
      Argv[i][len] = ' ';
   }

   /* Evaluate the expression */
   Ok = KdbpEvaluateExpression(Argv[1], sizeof("kdb:> ")-1 + (Argv[1]-Argv[0]), &Result);
   if (Ok)
   {
      if (Result > 0x00000000ffffffffLL)
      {
         if (Result & 0x8000000000000000LL)
            KdbpPrint("0x%016I64x  %20I64u  %20I64d\n", Result, Result, Result);
         else
            KdbpPrint("0x%016I64x  %20I64u\n", Result, Result);
      }
      else
      {
         ul = (ULONG)Result;
         if (ul <= 0xff && ul >= 0x80)
            l = (LONG)((CHAR)ul);
         else if (ul <= 0xffff && ul >= 0x8000)
            l = (LONG)((SHORT)ul);
         else
            l = (LONG)ul;
         if (l < 0)
            KdbpPrint("0x%08lx  %10lu  %10ld\n", ul, ul, l);
         else
            KdbpPrint("0x%08lx  %10lu\n", ul, ul);
      }
   }

   return TRUE;
}

/*!\brief Display list of active debug channels
 */
STATIC BOOLEAN
KdbpCmdFilter(ULONG Argc, PCHAR Argv[])
{
    ULONG i, j, ComponentId, Level;
    ULONG set = DPFLTR_MASK, clear = DPFLTR_MASK;
    PCHAR pend;
    LPCSTR opt, p;
    static struct {
        LPCSTR Name;
        ULONG Level;
    } debug_classes[] = {
        { "error", 1 << DPFLTR_ERROR_LEVEL },
        { "warning", 1 << DPFLTR_WARNING_LEVEL },
        { "trace", 1 << DPFLTR_TRACE_LEVEL },
        { "info", 1 << DPFLTR_INFO_LEVEL },
    };

    for (i = 1; i < Argc; i++)
    {
        opt = Argv[i];
        p = opt + strcspn(opt, "+-");
        if (!p[0]) p = opt; /* assume it's a debug channel name */

        if (p > opt)
        {
            for (j = 0; j < sizeof(debug_classes) / sizeof(debug_classes[0]); j++)
            {
                SIZE_T len = strlen(debug_classes[j].Name);
                if (len != (p - opt))
                    continue;
                if (_strnicmp(opt, debug_classes[j].Name, len) == 0) /* found it */
                {
                    if (*p == '+')
                        set |= debug_classes[j].Level;
                    else
                        clear |= debug_classes[j].Level;
                    break;
                }
            }
            if (j == sizeof(debug_classes) / sizeof(debug_classes[0]))
            {
                Level = strtoul(opt, &pend, 0);
                if (pend != p)
                {
                    KdbpPrint("filter: bad class name '%.*s'\n", p - opt, opt);
                    continue;
                }
                if (*p == '+')
                    set |= Level;
                else
                    clear |= Level;
            }
        }
        else
        {
            if (*p == '-')
                clear = ~0;
            else
                set = ~0;
        }
        if (*p == '+' || *p == '-')
            p++;

        if (!KdbpGetComponentId(p, &ComponentId))
        {
            KdbpPrint("filter: '%s' is not a valid component name!\n", p);
            return TRUE;
        }

        /* Get current mask value */
        NtSetDebugFilterState(ComponentId, set, TRUE);
        NtSetDebugFilterState(ComponentId, clear, FALSE);
    }

    return TRUE;
}

/*!\brief Disassembles 10 instructions at eip or given address or
 *        displays 16 dwords from memory at given address.
 */
STATIC BOOLEAN
KdbpCmdDisassembleX(ULONG Argc, PCHAR Argv[])
{
   ULONG Count;
   ULONG ul;
   INT i;
   ULONGLONG Result = 0;
   ULONG_PTR Address = KdbCurrentTrapFrame->Tf.Eip;
   LONG InstLen;

   if (Argv[0][0] == 'x') /* display memory */
      Count = 16;
   else /* disassemble */
      Count = 10;

   if (Argc >= 2)
   {
      /* Check for [L count] part */
      ul = 0;
      if (strcmp(Argv[Argc-2], "L") == 0)
      {
         ul = strtoul(Argv[Argc-1], NULL, 0);
         if (ul > 0)
         {
            Count = ul;
            Argc -= 2;
         }
      }
      else if (Argv[Argc-1][0] == 'L')
      {
         ul = strtoul(Argv[Argc-1] + 1, NULL, 0);
         if (ul > 0)
         {
            Count = ul;
            Argc--;
         }
      }

      /* Put the remaining arguments back together */
      Argc--;
      for (ul = 1; ul < Argc; ul++)
      {
         Argv[ul][strlen(Argv[ul])] = ' ';
      }
      Argc++;
   }

   /* Evaluate the expression */
   if (Argc > 1)
   {
      if (!KdbpEvaluateExpression(Argv[1], sizeof("kdb:> ")-1 + (Argv[1]-Argv[0]), &Result))
         return TRUE;
      if (Result > (ULONGLONG)(~((ULONG_PTR)0)))
         KdbpPrint("Warning: Address %I64x is beeing truncated\n");
      Address = (ULONG_PTR)Result;
   }
   else if (Argv[0][0] == 'x')
   {
      KdbpPrint("x: Address argument required.\n");
      return TRUE;
   }

   if (Argv[0][0] == 'x')
   {
      /* Display dwords */
      ul = 0;
      while (Count > 0)
      {
         if (!KdbSymPrintAddress((PVOID)Address))
            KdbpPrint("<%x>:", Address);
         else
            KdbpPrint(":");
         i = min(4, Count);
         Count -= i;
         while (--i >= 0)
         {
            if (!NT_SUCCESS(KdbpSafeReadMemory(&ul, (PVOID)Address, sizeof(ul))))
               KdbpPrint(" ????????");
            else
               KdbpPrint(" %08x", ul);
            Address += sizeof(ul);
         }
         KdbpPrint("\n");
      }
   }
   else
   {
      /* Disassemble */
      while (Count-- > 0)
      {
         if (!KdbSymPrintAddress((PVOID)Address))
            KdbpPrint("<%08x>: ", Address);
         else
            KdbpPrint(": ");
         InstLen = KdbpDisassemble(Address, KdbUseIntelSyntax);
         if (InstLen < 0)
         {
            KdbpPrint("<INVALID>\n");
            return TRUE;
         }
         KdbpPrint("\n");
         Address += InstLen;
      }
   }

   return TRUE;
}

/*!\brief Displays CPU registers.
 */
STATIC BOOLEAN
KdbpCmdRegs(ULONG Argc, PCHAR Argv[])
{
   PKTRAP_FRAME Tf = &KdbCurrentTrapFrame->Tf;
   INT i;
   STATIC CONST PCHAR EflagsBits[32] = { " CF", NULL, " PF", " BIT3", " AF", " BIT5",
                                         " ZF", " SF", " TF", " IF", " DF", " OF",
                                         NULL, NULL, " NT", " BIT15", " RF", " VF",
                                         " AC", " VIF", " VIP", " ID", " BIT22",
                                         " BIT23", " BIT24", " BIT25", " BIT26",
                                         " BIT27", " BIT28", " BIT29", " BIT30",
                                         " BIT31" };

   if (Argv[0][0] == 'r') /* regs */
   {
      KdbpPrint("CS:EIP  0x%04x:0x%08x\n"
                "SS:ESP  0x%04x:0x%08x\n"
                "   EAX  0x%08x   EBX  0x%08x\n"
                "   ECX  0x%08x   EDX  0x%08x\n"
                "   ESI  0x%08x   EDI  0x%08x\n"
                "   EBP  0x%08x\n",
                Tf->SegCs & 0xFFFF, Tf->Eip,
                Tf->HardwareSegSs, Tf->HardwareEsp,
                Tf->Eax, Tf->Ebx,
                Tf->Ecx, Tf->Edx,
                Tf->Esi, Tf->Edi,
                Tf->Ebp);
      KdbpPrint("EFLAGS  0x%08x ", Tf->EFlags);
      for (i = 0; i < 32; i++)
      {
         if (i == 1)
         {
            if ((Tf->EFlags & (1 << 1)) == 0)
               KdbpPrint(" !BIT1");
         }
         else if (i == 12)
         {
            KdbpPrint(" IOPL%d", (Tf->EFlags >> 12) & 3);
         }
         else if (i == 13)
         {
         }
         else if ((Tf->EFlags & (1 << i)) != 0)
            KdbpPrint(EflagsBits[i]);
      }
      KdbpPrint("\n");
   }
   else if (Argv[0][0] == 'c') /* cregs */
   {
      ULONG Cr0, Cr2, Cr3, Cr4;
      KDESCRIPTOR Gdtr = {0}, Ldtr = {0}, Idtr = {0};
      ULONG Tr = 0;
      STATIC CONST PCHAR Cr0Bits[32] = { " PE", " MP", " EM", " TS", " ET", " NE", NULL, NULL,
                                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                         " WP", NULL, " AM", NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL, NULL, NULL, " NW", " CD", " PG" };
      STATIC CONST PCHAR Cr4Bits[32] = { " VME", " PVI", " TSD", " DE", " PSE", " PAE", " MCE", " PGE",
                                         " PCE", " OSFXSR", " OSXMMEXCPT", NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

      Cr0 = KdbCurrentTrapFrame->Cr0;
      Cr2 = KdbCurrentTrapFrame->Cr2;
      Cr3 = KdbCurrentTrapFrame->Cr3;
      Cr4 = KdbCurrentTrapFrame->Cr4;

      /* Get descriptor table regs */
      Ke386GetGlobalDescriptorTable(*(PKDESCRIPTOR)&Gdtr.Limit);
      Ke386GetLocalDescriptorTable(Ldtr.Limit);
      Ke386GetInterruptDescriptorTable(*(PKDESCRIPTOR)&Idtr.Limit);

      /* Get the task register */
      Ke386GetTr(Tr);

      /* Display the control registers */
      KdbpPrint("CR0  0x%08x ", Cr0);
      for (i = 0; i < 32; i++)
      {
         if (Cr0Bits[i] == NULL)
            continue;
         if ((Cr0 & (1 << i)) != 0)
            KdbpPrint(Cr0Bits[i]);
      }
      KdbpPrint("\nCR2  0x%08x\n", Cr2);
      KdbpPrint("CR3  0x%08x  Pagedir-Base 0x%08x %s%s\n", Cr3, (Cr3 & 0xfffff000),
                (Cr3 & (1 << 3)) ? " PWT" : "", (Cr3 & (1 << 4)) ? " PCD" : "" );
      KdbpPrint("CR4  0x%08x ", Cr4);
      for (i = 0; i < 32; i++)
      {
         if (Cr4Bits[i] == NULL)
            continue;
         if ((Cr4 & (1 << i)) != 0)
            KdbpPrint(Cr4Bits[i]);
      }

      /* Display the descriptor table regs */
      KdbpPrint("\nGDTR  Base 0x%08x  Size 0x%04x\n", Gdtr.Base, Gdtr.Limit);
      KdbpPrint("LDTR  Base 0x%08x  Size 0x%04x\n", Ldtr.Base, Ldtr.Limit);
      KdbpPrint("IDTR  Base 0x%08x  Size 0x%04x\n", Idtr.Base, Idtr.Limit);
   }
   else if (Argv[0][0] == 's') /* sregs */
   {
      KdbpPrint("CS  0x%04x  Index 0x%04x  %cDT RPL%d\n",
                Tf->SegCs & 0xffff, (Tf->SegCs & 0xffff) >> 3,
                (Tf->SegCs & (1 << 2)) ? 'L' : 'G', Tf->SegCs & 3);
      KdbpPrint("DS  0x%04x  Index 0x%04x  %cDT RPL%d\n",
                Tf->SegDs, Tf->SegDs >> 3, (Tf->SegDs & (1 << 2)) ? 'L' : 'G', Tf->SegDs & 3);
      KdbpPrint("ES  0x%04x  Index 0x%04x  %cDT RPL%d\n",
                Tf->SegEs, Tf->SegEs >> 3, (Tf->SegEs & (1 << 2)) ? 'L' : 'G', Tf->SegEs & 3);
      KdbpPrint("FS  0x%04x  Index 0x%04x  %cDT RPL%d\n",
                Tf->SegFs, Tf->SegFs >> 3, (Tf->SegFs & (1 << 2)) ? 'L' : 'G', Tf->SegFs & 3);
      KdbpPrint("GS  0x%04x  Index 0x%04x  %cDT RPL%d\n",
                Tf->SegGs, Tf->SegGs >> 3, (Tf->SegGs & (1 << 2)) ? 'L' : 'G', Tf->SegGs & 3);
      KdbpPrint("SS  0x%04x  Index 0x%04x  %cDT RPL%d\n",
                Tf->HardwareSegSs, Tf->HardwareSegSs >> 3, (Tf->HardwareSegSs & (1 << 2)) ? 'L' : 'G', Tf->HardwareSegSs & 3);
   }
   else /* dregs */
   {
      ASSERT(Argv[0][0] == 'd');
      KdbpPrint("DR0  0x%08x\n"
                "DR1  0x%08x\n"
                "DR2  0x%08x\n"
                "DR3  0x%08x\n"
                "DR6  0x%08x\n"
                "DR7  0x%08x\n",
                Tf->Dr0, Tf->Dr1, Tf->Dr2, Tf->Dr3,
                Tf->Dr6, Tf->Dr7);
   }
   return TRUE;
}

/*!\brief Displays a backtrace.
 */
STATIC BOOLEAN
KdbpCmdBackTrace(ULONG Argc, PCHAR Argv[])
{
   ULONG Count;
   ULONG ul;
   ULONGLONG Result = 0;
   ULONG_PTR Frame = KdbCurrentTrapFrame->Tf.Ebp;
   ULONG_PTR Address;

   if (Argc >= 2)
   {
      /* Check for [L count] part */
      ul = 0;
      if (strcmp(Argv[Argc-2], "L") == 0)
      {
         ul = strtoul(Argv[Argc-1], NULL, 0);
         if (ul > 0)
         {
            Count = ul;
            Argc -= 2;
         }
      }
      else if (Argv[Argc-1][0] == 'L')
      {
         ul = strtoul(Argv[Argc-1] + 1, NULL, 0);
         if (ul > 0)
         {
            Count = ul;
            Argc--;
         }
      }

      /* Put the remaining arguments back together */
      Argc--;
      for (ul = 1; ul < Argc; ul++)
      {
         Argv[ul][strlen(Argv[ul])] = ' ';
      }
      Argc++;
   }

   /* Check if frame addr or thread id is given. */
   if (Argc > 1)
   {
      if (Argv[1][0] == '*')
      {
         Argv[1]++;
         /* Evaluate the expression */
         if (!KdbpEvaluateExpression(Argv[1], sizeof("kdb:> ")-1 + (Argv[1]-Argv[0]), &Result))
            return TRUE;
         if (Result > (ULONGLONG)(~((ULONG_PTR)0)))
            KdbpPrint("Warning: Address %I64x is beeing truncated\n");
         Frame = (ULONG_PTR)Result;
      }
      else
      {

         KdbpPrint("Thread backtrace not supported yet!\n");
         return TRUE;
      }
   }
   else
   {
      KdbpPrint("Eip:\n");
      /* Try printing the function at EIP */
      if (!KdbSymPrintAddress((PVOID)KdbCurrentTrapFrame->Tf.Eip))
         KdbpPrint("<%08x>\n", KdbCurrentTrapFrame->Tf.Eip);
      else
         KdbpPrint("\n");
   }

   KdbpPrint("Frames:\n");
   for (;;)
   {
      if (Frame == 0)
         break;
      if (!NT_SUCCESS(KdbpSafeReadMemory(&Address, (PVOID)(Frame + sizeof(ULONG_PTR)), sizeof (ULONG_PTR))))
      {
         KdbpPrint("Couldn't access memory at 0x%p!\n", Frame + sizeof(ULONG_PTR));
         break;
      }
      if (!KdbSymPrintAddress((PVOID)Address))
         KdbpPrint("<%08x>\n", Address);
      else
         KdbpPrint("\n");
      if (Address == 0)
         break;
      if (!NT_SUCCESS(KdbpSafeReadMemory(&Frame, (PVOID)Frame, sizeof (ULONG_PTR))))
      {
         KdbpPrint("Couldn't access memory at 0x%p!\n", Frame);
         break;
      }
   }

   return TRUE;
}

/*!\brief Continues execution of the system/leaves KDB.
 */
STATIC BOOLEAN
KdbpCmdContinue(ULONG Argc, PCHAR Argv[])
{
   /* Exit the main loop */
   return FALSE;
}

/*!\brief Continues execution of the system/leaves KDB.
 */
STATIC BOOLEAN
KdbpCmdStep(ULONG Argc, PCHAR Argv[])
{
   ULONG Count = 1;

   if (Argc > 1)
   {
      Count = strtoul(Argv[1], NULL, 0);
      if (Count == 0)
      {
         KdbpPrint("%s: Integer argument required\n", Argv[0]);
         return TRUE;
      }
   }

   if (Argv[0][0] == 'n')
      KdbSingleStepOver = TRUE;
   else
      KdbSingleStepOver = FALSE;

   /* Set the number of single steps and return to the interrupted code. */
   KdbNumSingleSteps = Count;

   return FALSE;
}

/*!\brief Lists breakpoints.
 */
STATIC BOOLEAN
KdbpCmdBreakPointList(ULONG Argc, PCHAR Argv[])
{
   LONG l;
   ULONG_PTR Address = 0;
   KDB_BREAKPOINT_TYPE Type = 0;
   KDB_ACCESS_TYPE AccessType = 0;
   UCHAR Size = 0;
   UCHAR DebugReg = 0;
   BOOLEAN Enabled = FALSE;
   BOOLEAN Global = FALSE;
   PEPROCESS Process = NULL;
   PCHAR str1, str2, ConditionExpr, GlobalOrLocal;
   CHAR Buffer[20];

   l = KdbpGetNextBreakPointNr(0);
   if (l < 0)
   {
      KdbpPrint("No breakpoints.\n");
      return TRUE;
   }

   KdbpPrint("Breakpoints:\n");
   do
   {
      if (!KdbpGetBreakPointInfo(l, &Address, &Type, &Size, &AccessType, &DebugReg,
                                 &Enabled, &Global, &Process, &ConditionExpr))
      {
         continue;
      }

      if (l == KdbLastBreakPointNr)
      {
         str1 = "\x1b[1m*";
         str2 = "\x1b[0m";
      }
      else
      {
         str1 = " ";
         str2 = "";
      }

      if (Global)
         GlobalOrLocal = "  global";
      else
      {
         GlobalOrLocal = Buffer;
         sprintf(Buffer, "  PID 0x%08lx",
                 (ULONG)(Process ? Process->UniqueProcessId : INVALID_HANDLE_VALUE));
      }

      if (Type == KdbBreakPointSoftware || Type == KdbBreakPointTemporary)
      {
         KdbpPrint(" %s%03d  BPX  0x%08x%s%s%s%s%s\n",
                   str1, l, Address,
                   Enabled ? "" : "  disabled",
                   GlobalOrLocal,
                   ConditionExpr ? "  IF " : "",
                   ConditionExpr ? ConditionExpr : "",
                   str2);
      }
      else if (Type == KdbBreakPointHardware)
      {
         if (!Enabled)
         {
            KdbpPrint(" %s%03d  BPM  0x%08x  %-5s %-5s  disabled%s%s%s%s\n", str1, l, Address,
                      KDB_ACCESS_TYPE_TO_STRING(AccessType),
                      Size == 1 ? "byte" : (Size == 2 ? "word" : "dword"),
                      GlobalOrLocal,
                      ConditionExpr ? "  IF " : "",
                      ConditionExpr ? ConditionExpr : "",
                      str2);
         }
         else
         {
            KdbpPrint(" %s%03d  BPM  0x%08x  %-5s %-5s  DR%d%s%s%s%s\n", str1, l, Address,
                      KDB_ACCESS_TYPE_TO_STRING(AccessType),
                      Size == 1 ? "byte" : (Size == 2 ? "word" : "dword"),
                      DebugReg,
                      GlobalOrLocal,
                      ConditionExpr ? "  IF " : "",
                      ConditionExpr ? ConditionExpr : "",
                      str2);
         }
      }
   }
   while ((l = KdbpGetNextBreakPointNr(l+1)) >= 0);

   return TRUE;
}

/*!\brief Enables, disables or clears a breakpoint.
 */
STATIC BOOLEAN
KdbpCmdEnableDisableClearBreakPoint(ULONG Argc, PCHAR Argv[])
{
   PCHAR pend;
   ULONG BreakPointNr;

   if (Argc < 2)
   {
      KdbpPrint("%s: argument required\n", Argv[0]);
      return TRUE;
   }

   pend = Argv[1];
   BreakPointNr = strtoul(Argv[1], &pend, 0);
   if (pend == Argv[1] || *pend != '\0')
   {
      KdbpPrint("%s: integer argument required\n", Argv[0]);
      return TRUE;
   }

   if (Argv[0][1] == 'e') /* enable */
   {
      KdbpEnableBreakPoint(BreakPointNr, NULL);
   }
   else if (Argv [0][1] == 'd') /* disable */
   {
      KdbpDisableBreakPoint(BreakPointNr, NULL);
   }
   else /* clear */
   {
      ASSERT(Argv[0][1] == 'c');
      KdbpDeleteBreakPoint(BreakPointNr, NULL);
   }

   return TRUE;
}

/*!\brief Sets a software or hardware (memory) breakpoint at the given address.
 */
STATIC BOOLEAN
KdbpCmdBreakPoint(ULONG Argc, PCHAR Argv[])
{
   ULONGLONG Result = 0;
   ULONG_PTR Address;
   KDB_BREAKPOINT_TYPE Type;
   UCHAR Size = 0;
   KDB_ACCESS_TYPE AccessType = 0;
   ULONG AddressArgIndex, i;
   LONG ConditionArgIndex;
   BOOLEAN Global = TRUE;

   if (Argv[0][2] == 'x') /* software breakpoint */
   {
      if (Argc < 2)
      {
         KdbpPrint("bpx: Address argument required.\n");
         return TRUE;
      }

      AddressArgIndex = 1;
      Type = KdbBreakPointSoftware;
   }
   else /* memory breakpoint */
   {
      ASSERT(Argv[0][2] == 'm');

      if (Argc < 2)
      {
         KdbpPrint("bpm: Access type argument required (one of r, w, rw, x)\n");
         return TRUE;
      }

      if (_stricmp(Argv[1], "x") == 0)
         AccessType = KdbAccessExec;
      else if (_stricmp(Argv[1], "r") == 0)
         AccessType = KdbAccessRead;
      else if (_stricmp(Argv[1], "w") == 0)
         AccessType = KdbAccessWrite;
      else if (_stricmp(Argv[1], "rw") == 0)
         AccessType = KdbAccessReadWrite;
      else
      {
         KdbpPrint("bpm: Unknown access type '%s'\n", Argv[1]);
         return TRUE;
      }

      if (Argc < 3)
      {
         KdbpPrint("bpm: %s argument required.\n", AccessType == KdbAccessExec ? "Address" : "Memory size");
         return TRUE;
      }
      AddressArgIndex = 3;
      if (_stricmp(Argv[2], "byte") == 0)
         Size = 1;
      else if (_stricmp(Argv[2], "word") == 0)
         Size = 2;
      else if (_stricmp(Argv[2], "dword") == 0)
         Size = 4;
      else if (AccessType == KdbAccessExec)
      {
         Size = 1;
         AddressArgIndex--;
      }
      else
      {
         KdbpPrint("bpm: Unknown memory size '%s'\n", Argv[2]);
         return TRUE;
      }

      if (Argc <= AddressArgIndex)
      {
         KdbpPrint("bpm: Address argument required.\n");
         return TRUE;
      }

      Type = KdbBreakPointHardware;
   }

   /* Put the arguments back together */
   ConditionArgIndex = -1;
   for (i = AddressArgIndex; i < (Argc-1); i++)
   {
      if (strcmp(Argv[i+1], "IF") == 0) /* IF found */
      {
         ConditionArgIndex = i + 2;
         if (ConditionArgIndex >= Argc)
         {
            KdbpPrint("%s: IF requires condition expression.\n", Argv[0]);
            return TRUE;
         }
         for (i = ConditionArgIndex; i < (Argc-1); i++)
            Argv[i][strlen(Argv[i])] = ' ';
         break;
      }
      Argv[i][strlen(Argv[i])] = ' ';
   }

   /* Evaluate the address expression */
   if (!KdbpEvaluateExpression(Argv[AddressArgIndex],
                               sizeof("kdb:> ")-1 + (Argv[AddressArgIndex]-Argv[0]),
                               &Result))
   {
      return TRUE;
   }
   if (Result > (ULONGLONG)(~((ULONG_PTR)0)))
      KdbpPrint("%s: Warning: Address %I64x is beeing truncated\n", Argv[0]);
   Address = (ULONG_PTR)Result;

   KdbpInsertBreakPoint(Address, Type, Size, AccessType,
                        (ConditionArgIndex < 0) ? NULL : Argv[ConditionArgIndex],
                        Global, NULL);

   return TRUE;
}

/*!\brief Lists threads or switches to another thread context.
 */
STATIC BOOLEAN
KdbpCmdThread(ULONG Argc, PCHAR Argv[])
{
   PLIST_ENTRY Entry;
   PETHREAD Thread = NULL;
   PEPROCESS Process = NULL;
   PULONG Esp;
   PULONG Ebp;
   ULONG Eip;
   ULONG ul = 0;
   PCHAR State, pend, str1, str2;
   STATIC CONST PCHAR ThreadStateToString[DeferredReady+1] =
                                          { "Initialized", "Ready", "Running",
                                            "Standby", "Terminated", "Waiting",
                                            "Transition", "DeferredReady" };
   ASSERT(KdbCurrentProcess != NULL);

   if (Argc >= 2 && _stricmp(Argv[1], "list") == 0)
   {
      Process = KdbCurrentProcess;

      if (Argc >= 3)
      {
         ul = strtoul(Argv[2], &pend, 0);
         if (Argv[2] == pend)
         {
            KdbpPrint("thread: '%s' is not a valid process id!\n", Argv[2]);
            return TRUE;
         }
         if (!NT_SUCCESS(PsLookupProcessByProcessId((PVOID)ul, &Process)))
         {
            KdbpPrint("thread: Invalid process id!\n");
            return TRUE;
         }
      }

      Entry = Process->ThreadListHead.Flink;
      if (Entry == &Process->ThreadListHead)
      {
         if (Argc >= 3)
            KdbpPrint("No threads in process 0x%08x!\n", ul);
         else
            KdbpPrint("No threads in current process!\n");
         return TRUE;
      }

      KdbpPrint("  TID         State        Prior.  Affinity    EBP         EIP\n");
      do
      {
         Thread = CONTAINING_RECORD(Entry, ETHREAD, ThreadListEntry);

         if (Thread == KdbCurrentThread)
         {
            str1 = "\x1b[1m*";
            str2 = "\x1b[0m";
         }
         else
         {
            str1 = " ";
            str2 = "";
         }

         if (Thread->Tcb.TrapFrame != NULL)
         {
            if (Thread->Tcb.TrapFrame->PreviousPreviousMode == KernelMode)
               Esp = (PULONG)Thread->Tcb.TrapFrame->TempEsp;
            else
               Esp = (PULONG)Thread->Tcb.TrapFrame->HardwareEsp;
            Ebp = (PULONG)Thread->Tcb.TrapFrame->Ebp;
            Eip = Thread->Tcb.TrapFrame->Eip;
         }
         else
         {
            Esp = (PULONG)Thread->Tcb.KernelStack;
            Ebp = (PULONG)Esp[4];
            Eip = 0;
            if (Ebp != NULL) /* FIXME: Should we attach to the process to read Ebp[1]? */
               KdbpSafeReadMemory(&Eip, Ebp + 1, sizeof (Eip));
         }
         if (Thread->Tcb.State < (DeferredReady + 1))
            State = ThreadStateToString[Thread->Tcb.State];
         else
            State = "Unknown";

         KdbpPrint(" %s0x%08x  %-11s  %3d     0x%08x  0x%08x  0x%08x%s\n",
                   str1,
                   Thread->Cid.UniqueThread,
                   State,
                   Thread->Tcb.Priority,
                   Thread->Tcb.Affinity,
                   Ebp,
                   Eip,
                   str2);

         Entry = Entry->Flink;
      }
      while (Entry != &Process->ThreadListHead);
   }
   else if (Argc >= 2 && _stricmp(Argv[1], "attach") == 0)
   {
      if (Argc < 3)
      {
         KdbpPrint("thread attach: thread id argument required!\n");
         return TRUE;
      }

      ul = strtoul(Argv[2], &pend, 0);
      if (Argv[2] == pend)
      {
         KdbpPrint("thread attach: '%s' is not a valid thread id!\n", Argv[2]);
         return TRUE;
      }
      if (!KdbpAttachToThread((PVOID)ul))
      {
         return TRUE;
      }
      KdbpPrint("Attached to thread 0x%08x.\n", ul);
   }
   else
   {
      Thread = KdbCurrentThread;

      if (Argc >= 2)
      {
         ul = strtoul(Argv[1], &pend, 0);
         if (Argv[1] == pend)
         {
            KdbpPrint("thread: '%s' is not a valid thread id!\n", Argv[1]);
            return TRUE;
         }
         if (!NT_SUCCESS(PsLookupThreadByThreadId((PVOID)ul, &Thread)))
         {
            KdbpPrint("thread: Invalid thread id!\n");
            return TRUE;
         }
      }

      if (Thread->Tcb.State < (DeferredReady + 1))
         State = ThreadStateToString[Thread->Tcb.State];
      else
         State = "Unknown";
      KdbpPrint("%s"
                "  TID:            0x%08x\n"
                "  State:          %s (0x%x)\n"
                "  Priority:       %d\n"
                "  Affinity:       0x%08x\n"
                "  Initial Stack:  0x%08x\n"
                "  Stack Limit:    0x%08x\n"
                "  Stack Base:     0x%08x\n"
                "  Kernel Stack:   0x%08x\n"
                "  Trap Frame:     0x%08x\n"
                "  NPX State:      %s (0x%x)\n",
                (Argc < 2) ? "Current Thread:\n" : "",
                Thread->Cid.UniqueThread,
                State, Thread->Tcb.State,
                Thread->Tcb.Priority,
                Thread->Tcb.Affinity,
                Thread->Tcb.InitialStack,
                Thread->Tcb.StackLimit,
                Thread->Tcb.StackBase,
                Thread->Tcb.KernelStack,
                Thread->Tcb.TrapFrame,
                NPX_STATE_TO_STRING(Thread->Tcb.NpxState), Thread->Tcb.NpxState);

   }

   return TRUE;
}

/*!\brief Lists processes or switches to another process context.
 */
STATIC BOOLEAN
KdbpCmdProc(ULONG Argc, PCHAR Argv[])
{
   PLIST_ENTRY Entry;
   PEPROCESS Process;
   PCHAR State, pend, str1, str2;
   ULONG ul;
   extern LIST_ENTRY PsActiveProcessHead;

   if (Argc >= 2 && _stricmp(Argv[1], "list") == 0)
   {
      Entry = PsActiveProcessHead.Flink;
      if (!Entry || Entry == &PsActiveProcessHead)
      {
         KdbpPrint("No processes in the system!\n");
         return TRUE;
      }

      KdbpPrint("  PID         State       Filename\n");
      do
      {
         Process = CONTAINING_RECORD(Entry, EPROCESS, ActiveProcessLinks);

         if (Process == KdbCurrentProcess)
         {
            str1 = "\x1b[1m*";
            str2 = "\x1b[0m";
         }
         else
         {
            str1 = " ";
            str2 = "";
         }

         State = ((Process->Pcb.State == ProcessInMemory) ? "In Memory" :
                 ((Process->Pcb.State == ProcessOutOfMemory) ? "Out of Memory" : "In Transition"));

         KdbpPrint(" %s0x%08x  %-10s  %s%s\n",
                   str1,
                   Process->UniqueProcessId,
                   State,
                   Process->ImageFileName,
                   str2);

         Entry = Entry->Flink;
      }
      while(Entry != &PsActiveProcessHead);
   }
   else if (Argc >= 2 && _stricmp(Argv[1], "attach") == 0)
   {
      if (Argc < 3)
      {
         KdbpPrint("process attach: process id argument required!\n");
         return TRUE;
      }

      ul = strtoul(Argv[2], &pend, 0);
      if (Argv[2] == pend)
      {
         KdbpPrint("process attach: '%s' is not a valid process id!\n", Argv[2]);
         return TRUE;
      }
      if (!KdbpAttachToProcess((PVOID)ul))
      {
         return TRUE;
      }
      KdbpPrint("Attached to process 0x%08x, thread 0x%08x.\n", (ULONG)ul,
                (ULONG)KdbCurrentThread->Cid.UniqueThread);
   }
   else
   {
      Process = KdbCurrentProcess;

      if (Argc >= 2)
      {
         ul = strtoul(Argv[1], &pend, 0);
         if (Argv[1] == pend)
         {
            KdbpPrint("proc: '%s' is not a valid process id!\n", Argv[1]);
            return TRUE;
         }
         if (!NT_SUCCESS(PsLookupProcessByProcessId((PVOID)ul, &Process)))
         {
            KdbpPrint("proc: Invalid process id!\n");
            return TRUE;
         }
      }

      State = ((Process->Pcb.State == ProcessInMemory) ? "In Memory" :
              ((Process->Pcb.State == ProcessOutOfMemory) ? "Out of Memory" : "In Transition"));
      KdbpPrint("%s"
                "  PID:             0x%08x\n"
                "  State:           %s (0x%x)\n"
                "  Image Filename:  %s\n",
                (Argc < 2) ? "Current process:\n" : "",
                Process->UniqueProcessId,
                State, Process->Pcb.State,
                Process->ImageFileName);
   }

   return TRUE;
}

/*!\brief Lists loaded modules or the one containing the specified address.
 */
STATIC BOOLEAN
KdbpCmdMod(ULONG Argc, PCHAR Argv[])
{
   ULONGLONG Result = 0;
   ULONG_PTR Address;
   KDB_MODULE_INFO Info;
   BOOLEAN DisplayOnlyOneModule = FALSE;
   INT i = 0;

   if (Argc >= 2)
   {
      /* Put the arguments back together */
      Argc--;
      while (--Argc >= 1)
         Argv[Argc][strlen(Argv[Argc])] = ' ';

      /* Evaluate the expression */
      if (!KdbpEvaluateExpression(Argv[1], sizeof("kdb:> ")-1 + (Argv[1]-Argv[0]), &Result))
      {
         return TRUE;
      }
      if (Result > (ULONGLONG)(~((ULONG_PTR)0)))
         KdbpPrint("%s: Warning: Address %I64x is beeing truncated\n", Argv[0]);
      Address = (ULONG_PTR)Result;

      if (!KdbpSymFindModuleByAddress((PVOID)Address, &Info))
      {
         KdbpPrint("No module containing address 0x%p found!\n", Address);
         return TRUE;
      }
      DisplayOnlyOneModule = TRUE;
   }
   else
   {
      if (!KdbpSymFindModuleByIndex(0, &Info))
      {
		  ULONG_PTR ntoskrnlBase = ((ULONG_PTR)KdbpCmdMod) & 0xfff00000;
		  KdbpPrint("  Base      Size      Name\n");
		  KdbpPrint("  %08x  %08x  %s\n", ntoskrnlBase, 0, "ntoskrnl.exe");
		  return TRUE;
      }
      i = 1;
   }

   KdbpPrint("  Base      Size      Name\n");
   for (;;)
   {
      KdbpPrint("  %08x  %08x  %ws\n", Info.Base, Info.Size, Info.Name);

      if ((!DisplayOnlyOneModule && !KdbpSymFindModuleByIndex(i++, &Info)) ||
          DisplayOnlyOneModule)
      {
         break;
      }
   }

   return TRUE;
}

/*!\brief Displays GDT, LDT or IDTd.
 */
STATIC BOOLEAN
KdbpCmdGdtLdtIdt(ULONG Argc, PCHAR Argv[])
{
   KDESCRIPTOR Reg = {0};
   ULONG SegDesc[2];
   ULONG SegBase;
   ULONG SegLimit;
   PCHAR SegType;
   USHORT SegSel;
   UCHAR Type, Dpl;
   INT i;
   ULONG ul;

   if (Argv[0][0] == 'i')
   {
      /* Read IDTR */
      Ke386GetInterruptDescriptorTable(*(PKDESCRIPTOR)&Reg.Limit);

      if (Reg.Limit < 7)
      {
         KdbpPrint("Interrupt descriptor table is empty.\n");
         return TRUE;
      }
      KdbpPrint("IDT Base: 0x%08x  Limit: 0x%04x\n", Reg.Base, Reg.Limit);
      KdbpPrint("  Idx  Type        Seg. Sel.  Offset      DPL\n");
      for (i = 0; (i + sizeof(SegDesc) - 1) <= Reg.Limit; i += 8)
      {
         if (!NT_SUCCESS(KdbpSafeReadMemory(SegDesc, (PVOID)(Reg.Base + i), sizeof(SegDesc))))
         {
            KdbpPrint("Couldn't access memory at 0x%08x!\n", Reg.Base + i);
            return TRUE;
         }

         Dpl = ((SegDesc[1] >> 13) & 3);
         if ((SegDesc[1] & 0x1f00) == 0x0500)        /* Task gate */
            SegType = "TASKGATE";
         else if ((SegDesc[1] & 0x1fe0) == 0x0e00)   /* 32 bit Interrupt gate */
            SegType = "INTGATE32";
         else if ((SegDesc[1] & 0x1fe0) == 0x0600)   /* 16 bit Interrupt gate */
            SegType = "INTGATE16";
         else if ((SegDesc[1] & 0x1fe0) == 0x0f00)   /* 32 bit Trap gate */
            SegType = "TRAPGATE32";
         else if ((SegDesc[1] & 0x1fe0) == 0x0700)   /* 16 bit Trap gate */
            SegType = "TRAPGATE16";
         else
            SegType = "UNKNOWN";

         if ((SegDesc[1] & (1 << 15)) == 0) /* not present */
         {
            KdbpPrint("  %03d  %-10s  [NP]       [NP]        %02d\n",
                      i / 8, SegType, Dpl);
         }
         else if ((SegDesc[1] & 0x1f00) == 0x0500) /* Task gate */
         {
            SegSel = SegDesc[0] >> 16;
            KdbpPrint("  %03d  %-10s  0x%04x                 %02d\n",
                      i / 8, SegType, SegSel, Dpl);
         }
         else
         {
            SegSel = SegDesc[0] >> 16;
            SegBase = (SegDesc[1] & 0xffff0000) | (SegDesc[0] & 0x0000ffff);
            KdbpPrint("  %03d  %-10s  0x%04x     0x%08x  %02d\n",
                      i / 8, SegType, SegSel, SegBase, Dpl);
         }
      }
   }
   else
   {
      ul = 0;
      if (Argv[0][0] == 'g')
      {
         /* Read GDTR */
         Ke386GetGlobalDescriptorTable(*(PKDESCRIPTOR)&Reg.Limit);
         i = 8;
      }
      else
      {
         ASSERT(Argv[0][0] == 'l');
         /* Read LDTR */
         Ke386GetLocalDescriptorTable(Reg.Limit);
         i = 0;
         ul = 1 << 2;
      }

      if (Reg.Limit < 7)
      {
         KdbpPrint("%s descriptor table is empty.\n",
                   Argv[0][0] == 'g' ? "Global" : "Local");
         return TRUE;
      }
      KdbpPrint("%cDT Base: 0x%08x  Limit: 0x%04x\n",
                Argv[0][0] == 'g' ? 'G' : 'L', Reg.Base, Reg.Limit);
      KdbpPrint("  Idx  Sel.    Type         Base        Limit       DPL  Attribs\n");
      for ( ; (i + sizeof(SegDesc) - 1) <= Reg.Limit; i += 8)
      {
         if (!NT_SUCCESS(KdbpSafeReadMemory(SegDesc, (PVOID)(Reg.Base + i), sizeof(SegDesc))))
         {
            KdbpPrint("Couldn't access memory at 0x%08x!\n", Reg.Base + i);
            return TRUE;
         }
         Dpl = ((SegDesc[1] >> 13) & 3);
         Type = ((SegDesc[1] >> 8) & 0xf);

         SegBase = SegDesc[0] >> 16;
         SegBase |= (SegDesc[1] & 0xff) << 16;
         SegBase |= SegDesc[1] & 0xff000000;
         SegLimit = SegDesc[0] & 0x0000ffff;
         SegLimit |= (SegDesc[1] >> 16) & 0xf;
         if ((SegDesc[1] & (1 << 23)) != 0)
         {
            SegLimit *= 4096;
            SegLimit += 4095;
         }
         else
         {
            SegLimit++;
         }

         if ((SegDesc[1] & (1 << 12)) == 0) /* System segment */
         {
            switch (Type)
            {
            case 1: SegType = "TSS16(Avl)"; break;
            case 2: SegType = "LDT"; break;
            case 3: SegType = "TSS16(Busy)"; break;
            case 4: SegType = "CALLGATE16"; break;
            case 5: SegType = "TASKGATE"; break;
            case 6: SegType = "INTGATE16"; break;
            case 7: SegType = "TRAPGATE16"; break;
            case 9: SegType = "TSS32(Avl)"; break;
            case 11: SegType = "TSS32(Busy)"; break;
            case 12: SegType = "CALLGATE32"; break;
            case 14: SegType = "INTGATE32"; break;
            case 15: SegType = "INTGATE32"; break;
            default: SegType = "UNKNOWN"; break;
            }
            if (!(Type >= 1 && Type <= 3) &&
                Type != 9 && Type != 11)
            {
               SegBase = 0;
               SegLimit = 0;
            }
         }
         else if ((SegDesc[1] & (1 << 11)) == 0) /* Data segment */
         {
            if ((SegDesc[1] & (1 << 22)) != 0)
               SegType = "DATA32";
            else
               SegType = "DATA16";

         }
         else /* Code segment */
         {
            if ((SegDesc[1] & (1 << 22)) != 0)
               SegType = "CODE32";
            else
               SegType = "CODE16";
         }

         if ((SegDesc[1] & (1 << 15)) == 0) /* not present */
         {
            KdbpPrint("  %03d  0x%04x  %-11s  [NP]        [NP]        %02d   NP\n",
                      i / 8, i | Dpl | ul, SegType, Dpl);
         }
         else
         {
            KdbpPrint("  %03d  0x%04x  %-11s  0x%08x  0x%08x  %02d  ",
                      i / 8, i | Dpl | ul, SegType, SegBase, SegLimit, Dpl);
            if ((SegDesc[1] & (1 << 12)) == 0) /* System segment */
            {
               /* FIXME: Display system segment */
            }
            else if ((SegDesc[1] & (1 << 11)) == 0) /* Data segment */
            {
               if ((SegDesc[1] & (1 << 10)) != 0) /* Expand-down */
                  KdbpPrint(" E");
               KdbpPrint((SegDesc[1] & (1 << 9)) ? " R/W" : " R");
               if ((SegDesc[1] & (1 << 8)) != 0)
                  KdbpPrint(" A");
            }
            else /* Code segment */
            {
               if ((SegDesc[1] & (1 << 10)) != 0) /* Conforming */
                  KdbpPrint(" C");
               KdbpPrint((SegDesc[1] & (1 << 9)) ? " R/X" : " X");
               if ((SegDesc[1] & (1 << 8)) != 0)
                  KdbpPrint(" A");
            }
            if ((SegDesc[1] & (1 << 20)) != 0)
               KdbpPrint(" AVL");
            KdbpPrint("\n");
         }
      }
   }

   return TRUE;
}

/*!\brief Displays the KPCR
 */
STATIC BOOLEAN
KdbpCmdPcr(ULONG Argc, PCHAR Argv[])
{
   PKIPCR Pcr = (PKIPCR)KeGetPcr();

   KdbpPrint("Current PCR is at 0x%08x.\n", (INT)Pcr);
   KdbpPrint("  Tib.ExceptionList:         0x%08x\n"
             "  Tib.StackBase:             0x%08x\n"
             "  Tib.StackLimit:            0x%08x\n"
             "  Tib.SubSystemTib:          0x%08x\n"
             "  Tib.FiberData/Version:     0x%08x\n"
             "  Tib.ArbitraryUserPointer:  0x%08x\n"
             "  Tib.Self:                  0x%08x\n"
             "  Self:                      0x%08x\n"
             "  PCRCB:                     0x%08x\n"
             "  Irql:                      0x%02x\n"
             "  IRR:                       0x%08x\n"
             "  IrrActive:                 0x%08x\n"
             "  IDR:                       0x%08x\n"
             "  KdVersionBlock:            0x%08x\n"
             "  IDT:                       0x%08x\n"
             "  GDT:                       0x%08x\n"
             "  TSS:                       0x%08x\n"
             "  MajorVersion:              0x%04x\n"
             "  MinorVersion:              0x%04x\n"
             "  SetMember:                 0x%08x\n"
             "  StallScaleFactor:          0x%08x\n"
             "  Number:                    0x%02x\n"
             "  L2CacheAssociativity:      0x%02x\n"
             "  VdmAlert:                  0x%08x\n"
             "  L2CacheSize:               0x%08x\n"
             "  InterruptMode:             0x%08x\n",
             Pcr->NtTib.ExceptionList, Pcr->NtTib.StackBase, Pcr->NtTib.StackLimit,
             Pcr->NtTib.SubSystemTib, Pcr->NtTib.FiberData, Pcr->NtTib.ArbitraryUserPointer,
             Pcr->NtTib.Self, Pcr->Self, Pcr->Prcb, Pcr->Irql, Pcr->IRR, Pcr->IrrActive,
             Pcr->IDR, Pcr->KdVersionBlock, Pcr->IDT, Pcr->GDT, Pcr->TSS,
             Pcr->MajorVersion, Pcr->MinorVersion, Pcr->SetMember, Pcr->StallScaleFactor,
             Pcr->Number, Pcr->L2CacheAssociativity,
             Pcr->VdmAlert, Pcr->SecondLevelCacheSize, Pcr->InterruptMode);

   return TRUE;
}

/*!\brief Displays the TSS
 */
STATIC BOOLEAN
KdbpCmdTss(ULONG Argc, PCHAR Argv[])
{
   KTSS *Tss = KeGetPcr()->TSS;

   KdbpPrint("Current TSS is at 0x%08x.\n", (INT)Tss);
   KdbpPrint("  Eip:           0x%08x\n"
             "  Es:            0x%04x\n"
             "  Cs:            0x%04x\n"
             "  Ss:            0x%04x\n"
             "  Ds:            0x%04x\n"
             "  Fs:            0x%04x\n"
             "  Gs:            0x%04x\n"
             "  IoMapBase:     0x%04x\n",
             Tss->Eip, Tss->Es, Tss->Cs, Tss->Ds, Tss->Fs, Tss->Gs, Tss->IoMapBase);
   return TRUE;
}

/*!\brief Bugchecks the system.
 */
STATIC BOOLEAN
KdbpCmdBugCheck(ULONG Argc, PCHAR Argv[])
{
   KeBugCheck(0xDEADDEAD);
   return TRUE;
}

/*!\brief Sets or displays a config variables value.
 */
STATIC BOOLEAN
KdbpCmdSet(ULONG Argc, PCHAR Argv[])
{
   LONG l;
   BOOLEAN First;
   PCHAR pend = 0;
   KDB_ENTER_CONDITION ConditionFirst = KdbDoNotEnter;
   KDB_ENTER_CONDITION ConditionLast = KdbDoNotEnter;
   STATIC CONST PCHAR ExceptionNames[21] =
      { "ZERODEVIDE", "DEBUGTRAP", "NMI", "INT3", "OVERFLOW", "BOUND", "INVALIDOP",
        "NOMATHCOP", "DOUBLEFAULT", "RESERVED(9)", "INVALIDTSS", "SEGMENTNOTPRESENT",
        "STACKFAULT", "GPF", "PAGEFAULT", "RESERVED(15)", "MATHFAULT", "ALIGNMENTCHECK",
        "MACHINECHECK", "SIMDFAULT", "OTHERS" };

   if (Argc == 1)
   {
      KdbpPrint("Available settings:\n");
      KdbpPrint("  syntax [intel|at&t]\n");
      KdbpPrint("  condition [exception|*] [first|last] [never|always|kmode|umode]\n");
      KdbpPrint("  break_on_module_load [true|false]\n");
   }
   else if (strcmp(Argv[1], "syntax") == 0)
   {
      if (Argc == 2)
         KdbpPrint("syntax = %s\n", KdbUseIntelSyntax ? "intel" : "at&t");
      else if (Argc >= 3)
      {
         if (_stricmp(Argv[2], "intel") == 0)
            KdbUseIntelSyntax = TRUE;
         else if (_stricmp(Argv[2], "at&t") == 0)
            KdbUseIntelSyntax = FALSE;
         else
            KdbpPrint("Unknown syntax '%s'.\n", Argv[2]);
      }
   }
   else if (strcmp(Argv[1], "condition") == 0)
   {
      if (Argc == 2)
      {
         KdbpPrint("Conditions:                 (First)  (Last)\n");
         for (l = 0; l < RTL_NUMBER_OF(ExceptionNames) - 1; l++)
         {
            if (ExceptionNames[l] == NULL)
               continue;
            if (!KdbpGetEnterCondition(l, TRUE, &ConditionFirst))
               ASSERT(0);
            if (!KdbpGetEnterCondition(l, FALSE, &ConditionLast))
               ASSERT(0);
            KdbpPrint("  #%02d  %-20s %-8s %-8s\n", l, ExceptionNames[l],
                      KDB_ENTER_CONDITION_TO_STRING(ConditionFirst),
                      KDB_ENTER_CONDITION_TO_STRING(ConditionLast));
         }
         ASSERT(l == (RTL_NUMBER_OF(ExceptionNames) - 1));
         KdbpPrint("       %-20s %-8s %-8s\n", ExceptionNames[l],
                   KDB_ENTER_CONDITION_TO_STRING(ConditionFirst),
                   KDB_ENTER_CONDITION_TO_STRING(ConditionLast));
      }
      else
      {
         if (Argc >= 5 && strcmp(Argv[2], "*") == 0) /* Allow * only when setting condition */
            l = -1;
         else
         {
            l = strtoul(Argv[2], &pend, 0);
            if (Argv[2] == pend)
            {
               for (l = 0; l < RTL_NUMBER_OF(ExceptionNames); l++)
               {
                  if (ExceptionNames[l] == NULL)
                     continue;
                  if (_stricmp(ExceptionNames[l], Argv[2]) == 0)
                     break;
               }
            }
            if (l >= RTL_NUMBER_OF(ExceptionNames))
            {
               KdbpPrint("Unknown exception '%s'.\n", Argv[2]);
               return TRUE;
            }
         }
         if (Argc > 4)
         {
            if (_stricmp(Argv[3], "first") == 0)
               First = TRUE;
            else if (_stricmp(Argv[3], "last") == 0)
               First = FALSE;
            else
            {
               KdbpPrint("set condition: second argument must be 'first' or 'last'\n");
               return TRUE;
            }
            if (_stricmp(Argv[4], "never") == 0)
               ConditionFirst = KdbDoNotEnter;
            else if (_stricmp(Argv[4], "always") == 0)
               ConditionFirst = KdbEnterAlways;
            else if (_stricmp(Argv[4], "umode") == 0)
               ConditionFirst = KdbEnterFromUmode;
            else if (_stricmp(Argv[4], "kmode") == 0)
               ConditionFirst = KdbEnterFromKmode;
            else
            {
               KdbpPrint("set condition: third argument must be 'never', 'always', 'umode' or 'kmode'\n");
               return TRUE;
            }
            if (!KdbpSetEnterCondition(l, First, ConditionFirst))
            {
               if (l >= 0)
                  KdbpPrint("Couldn't change condition for exception #%02d\n", l);
               else
                  KdbpPrint("Couldn't change condition for all exceptions\n", l);
            }
         }
         else /* Argc >= 3 */
         {
            if (!KdbpGetEnterCondition(l, TRUE, &ConditionFirst))
               ASSERT(0);
            if (!KdbpGetEnterCondition(l, FALSE, &ConditionLast))
               ASSERT(0);
            if (l < (RTL_NUMBER_OF(ExceptionNames) - 1))
            {
               KdbpPrint("Condition for exception #%02d (%s): FirstChance %s  LastChance %s\n",
                         l, ExceptionNames[l],
                         KDB_ENTER_CONDITION_TO_STRING(ConditionFirst),
                         KDB_ENTER_CONDITION_TO_STRING(ConditionLast));
            }
            else
            {
               KdbpPrint("Condition for all other exceptions: FirstChance %s  LastChance %s\n",
                         KDB_ENTER_CONDITION_TO_STRING(ConditionFirst),
                         KDB_ENTER_CONDITION_TO_STRING(ConditionLast));
            }
         }
      }
   }
   else if (strcmp(Argv[1], "break_on_module_load") == 0)
   {
      if (Argc == 2)
         KdbpPrint("break_on_module_load = %s\n", KdbBreakOnModuleLoad ? "enabled" : "disabled");
      else if (Argc >= 3)
      {
         if (_stricmp(Argv[2], "enable") == 0 || _stricmp(Argv[2], "enabled") == 0 ||
             _stricmp(Argv[2], "true") == 0)
            KdbBreakOnModuleLoad = TRUE;
         else if (_stricmp(Argv[2], "disable") == 0 || _stricmp(Argv[2], "disabled") == 0 ||
                  _stricmp(Argv[2], "false") == 0)
            KdbBreakOnModuleLoad = FALSE;
         else
            KdbpPrint("Unknown setting '%s'.\n", Argv[2]);
      }
   }
   else
      KdbpPrint("Unknown setting '%s'.\n", Argv[1]);

   return TRUE;
}

/*!\brief Displays help screen.
 */
STATIC BOOLEAN
KdbpCmdHelp(ULONG Argc, PCHAR Argv[])
{
   ULONG i;

   KdbpPrint("Kernel debugger commands:\n");
   for (i = 0; i < RTL_NUMBER_OF(KdbDebuggerCommands); i++)
   {
      if (KdbDebuggerCommands[i].Syntax == NULL) /* Command group */
      {
         if (i > 0)
            KdbpPrint("\n");
         KdbpPrint("\x1b[7m* %s:\x1b[0m\n", KdbDebuggerCommands[i].Help);
         continue;
      }

      KdbpPrint("  %-20s - %s\n",
                KdbDebuggerCommands[i].Syntax,
                KdbDebuggerCommands[i].Help);
   }

   return TRUE;
}

/*!\brief Prints the given string with printf-like formatting.
 *
 * \param Format  Format of the string/arguments.
 * \param ...     Variable number of arguments matching the format specified in \a Format.
 *
 * \note Doesn't correctly handle \\t and terminal escape sequences when calculating the
 *       number of lines required to print a single line from the Buffer in the terminal.
 */
VOID
KdbpPrint(
   IN PCHAR Format,
   IN ...  OPTIONAL)
{
   STATIC CHAR Buffer[4096];
   STATIC BOOLEAN TerminalInitialized = FALSE;
   STATIC BOOLEAN TerminalConnected = FALSE;
   STATIC BOOLEAN TerminalReportsSize = TRUE;
   CHAR c = '\0';
   PCHAR p, p2;
   ULONG Length;
   ULONG i, j;
   LONG RowsPrintedByTerminal;
   ULONG ScanCode;
   va_list ap;

   /* Check if the user has aborted output of the current command */
   if (KdbOutputAborted)
      return;

   /* Initialize the terminal */
   if (!TerminalInitialized)
   {
      DbgPrint("\x1b[7h");      /* Enable linewrap */

      /* Query terminal type */
      /*DbgPrint("\x1b[Z");*/
      DbgPrint("\x05");

      TerminalInitialized = TRUE;
      Length = 0;
	  KeStallExecutionProcessor(100000);

      for (;;)
      {
         c = KdbpTryGetCharSerial(5000);
         if (c == -1)
            break;
         Buffer[Length++] = c;
         if (Length >= (sizeof (Buffer) - 1))
            break;
      }
      Buffer[Length] = '\0';
      if (Length > 0)
         TerminalConnected = TRUE;
   }

   /* Get number of rows and columns in terminal */
   if ((KdbNumberOfRowsTerminal < 0) || (KdbNumberOfColsTerminal < 0) ||
       (KdbNumberOfRowsPrinted) == 0) /* Refresh terminal size each time when number of rows printed is 0 */
   {
      if ((KdbDebugState & KD_DEBUG_KDSERIAL) && TerminalConnected && TerminalReportsSize)
      {
         /* Try to query number of rows from terminal. A reply looks like "\x1b[8;24;80t" */
         TerminalReportsSize = FALSE;
		 KeStallExecutionProcessor(100000);
         DbgPrint("\x1b[18t");
         c = KdbpTryGetCharSerial(5000);
         if (c == KEY_ESC)
         {
            c = KdbpTryGetCharSerial(5000);
            if (c == '[')
            {
               Length = 0;
               for (;;)
               {
                  c = KdbpTryGetCharSerial(5000);
                  if (c == -1)
                     break;
                  Buffer[Length++] = c;
                  if (isalpha(c) || Length >= (sizeof (Buffer) - 1))
                     break;
               }
               Buffer[Length] = '\0';
               if (Buffer[0] == '8' && Buffer[1] == ';')
               {
                  for (i = 2; (i < Length) && (Buffer[i] != ';'); i++);
                  if (Buffer[i] == ';')
                  {
                     Buffer[i++] = '\0';
                     /* Number of rows is now at Buffer + 2 and number of cols at Buffer + i */
                     KdbNumberOfRowsTerminal = strtoul(Buffer + 2, NULL, 0);
                     KdbNumberOfColsTerminal = strtoul(Buffer + i, NULL, 0);
                     TerminalReportsSize = TRUE;
                  }
               }
            }
			/* Clear further characters */
			while ((c = KdbpTryGetCharSerial(5000)) != -1);
         }
      }

      if (KdbNumberOfRowsTerminal <= 0)
      {
         /* Set number of rows to the default. */
         KdbNumberOfRowsTerminal = 24;
      }
      else if (KdbNumberOfColsTerminal <= 0)
      {
         /* Set number of cols to the default. */
         KdbNumberOfColsTerminal = 80;
      }
   }

   /* Get the string */
   va_start(ap, Format);
   Length = _vsnprintf(Buffer, sizeof (Buffer) - 1, Format, ap);
   Buffer[Length] = '\0';
   va_end(ap);

   p = Buffer;
   while (p[0] != '\0')
   {
      i = strcspn(p, "\n");

      /* Calculate the number of lines which will be printed in the terminal
       * when outputting the current line
       */
      if (i > 0)
         RowsPrintedByTerminal = (i + KdbNumberOfColsPrinted - 1) / KdbNumberOfColsTerminal;
      else
         RowsPrintedByTerminal = 0;
      if (p[i] == '\n')
         RowsPrintedByTerminal++;

      /*DbgPrint("!%d!%d!%d!%d!", KdbNumberOfRowsPrinted, KdbNumberOfColsPrinted, i, RowsPrintedByTerminal);*/

      /* Display a prompt if we printed one screen full of text */
      if (KdbNumberOfRowsTerminal > 0 &&
          (LONG)(KdbNumberOfRowsPrinted + RowsPrintedByTerminal) >= KdbNumberOfRowsTerminal)
      {
         if (KdbNumberOfColsPrinted > 0)
            DbgPrint("\n");
         DbgPrint("--- Press q to abort, any other key to continue ---");
         if (KdbDebugState & KD_DEBUG_KDSERIAL)
            c = KdbpGetCharSerial();
         else
            c = KdbpGetCharKeyboard(&ScanCode);
         if (c == '\r')
         {
            /* Try to read '\n' which might follow '\r' - if \n is not received here
             * it will be interpreted as "return" when the next command should be read.
             */
            if (KdbDebugState & KD_DEBUG_KDSERIAL)
               c = KdbpTryGetCharSerial(5);
            else
               c = KdbpTryGetCharKeyboard(&ScanCode, 5);
         }
         DbgPrint("\n");
         if (c == 'q')
         {
            KdbOutputAborted = TRUE;
            return;
         }
         KdbNumberOfRowsPrinted = 0;
         KdbNumberOfColsPrinted = 0;
      }

      /* Insert a NUL after the line and print only the current line. */
      if (p[i] == '\n' && p[i + 1] != '\0')
      {
         c = p[i + 1];
         p[i + 1] = '\0';
      }
      else
      {
         c = '\0';
      }

      /* Remove escape sequences from the line if there's no terminal connected */
      if (!TerminalConnected)
      {
         while ((p2 = strrchr(p, '\x1b')) != NULL) /* Look for escape character */
         {
            if (p2[1] == '[')
            {
               j = 2;
               while (!isalpha(p2[j++]));
               strcpy(p2, p2 + j);
            }
            else
            {
                strcpy(p2, p2 + 1);
            }
         }
      }

	  DbgPrint("%s", p);

      if (c != '\0')
         p[i + 1] = c;

      /* Set p to the start of the next line and
       * remember the number of rows/cols printed
       */
      p += i;
      if (p[0] == '\n')
      {
         p++;
         KdbNumberOfColsPrinted = 0;
      }
      else
      {
         ASSERT(p[0] == '\0');
         KdbNumberOfColsPrinted += i;
      }
      KdbNumberOfRowsPrinted += RowsPrintedByTerminal;
   }
}

/*!\brief Appends a command to the command history
 *
 * \param Command  Pointer to the command to append to the history.
 */
STATIC VOID
KdbpCommandHistoryAppend(
   IN PCHAR Command)
{
   ULONG Length1 = strlen(Command) + 1;
   ULONG Length2 = 0;
   INT i;
   PCHAR Buffer;

   ASSERT(Length1 <= RTL_NUMBER_OF(KdbCommandHistoryBuffer));

   if (Length1 <= 1 ||
       (KdbCommandHistory[KdbCommandHistoryIndex] != NULL &&
        strcmp(KdbCommandHistory[KdbCommandHistoryIndex], Command) == 0))
   {
      return;
   }

   /* Calculate Length1 and Length2 */
   Buffer = KdbCommandHistoryBuffer + KdbCommandHistoryBufferIndex;
   KdbCommandHistoryBufferIndex += Length1;
   if (KdbCommandHistoryBufferIndex >= (LONG)RTL_NUMBER_OF(KdbCommandHistoryBuffer))
   {
      KdbCommandHistoryBufferIndex -= RTL_NUMBER_OF(KdbCommandHistoryBuffer);
      Length2 = KdbCommandHistoryBufferIndex;
      Length1 -= Length2;
   }

   /* Remove previous commands until there is enough space to append the new command */
   for (i = KdbCommandHistoryIndex; KdbCommandHistory[i] != NULL;)
   {
      if ((Length2 > 0 &&
           (KdbCommandHistory[i] >= Buffer ||
            KdbCommandHistory[i] < (KdbCommandHistoryBuffer + KdbCommandHistoryBufferIndex))) ||
          (Length2 <= 0 &&
           (KdbCommandHistory[i] >= Buffer &&
            KdbCommandHistory[i] < (KdbCommandHistoryBuffer + KdbCommandHistoryBufferIndex))))
      {
         KdbCommandHistory[i] = NULL;
      }
      i--;
      if (i < 0)
         i = RTL_NUMBER_OF(KdbCommandHistory) - 1;
      if (i == KdbCommandHistoryIndex)
         break;
   }

   /* Make sure the new command history entry is free */
   KdbCommandHistoryIndex++;
   KdbCommandHistoryIndex %= RTL_NUMBER_OF(KdbCommandHistory);
   if (KdbCommandHistory[KdbCommandHistoryIndex] != NULL)
   {
      KdbCommandHistory[KdbCommandHistoryIndex] = NULL;
   }

   /* Append command */
   KdbCommandHistory[KdbCommandHistoryIndex] = Buffer;
   ASSERT((KdbCommandHistory[KdbCommandHistoryIndex] + Length1) <= KdbCommandHistoryBuffer + RTL_NUMBER_OF(KdbCommandHistoryBuffer));
   memcpy(KdbCommandHistory[KdbCommandHistoryIndex], Command, Length1);
   if (Length2 > 0)
   {
      memcpy(KdbCommandHistoryBuffer, Command + Length1, Length2);
   }
}

/*!\brief Reads a line of user-input.
 *
 * \param Buffer  Buffer to store the input into. Trailing newlines are removed.
 * \param Size    Size of \a Buffer.
 *
 * \note Accepts only \n newlines, \r is ignored.
 */
STATIC VOID
KdbpReadCommand(
   OUT PCHAR Buffer,
   IN  ULONG Size)
{
   CHAR Key;
   PCHAR Orig = Buffer;
   ULONG ScanCode = 0;
   BOOLEAN EchoOn;
   STATIC CHAR LastCommand[1024] = "";
   STATIC CHAR NextKey = '\0';
   INT CmdHistIndex = -1;
   INT i;

   EchoOn = !((KdbDebugState & KD_DEBUG_KDNOECHO) != 0);

   for (;;)
   {
      if (KdbDebugState & KD_DEBUG_KDSERIAL)
      {
         Key = (NextKey == '\0') ? KdbpGetCharSerial() : NextKey;
         NextKey = '\0';
         ScanCode = 0;
         if (Key == KEY_ESC) /* ESC */
         {
            Key = KdbpGetCharSerial();
            if (Key == '[')
            {
               Key = KdbpGetCharSerial();
               switch (Key)
               {
               case 'A':
                  ScanCode = KEY_SCAN_UP;
                  break;
               case 'B':
                  ScanCode = KEY_SCAN_DOWN;
                  break;
               case 'C':
                  break;
               case 'D':
                  break;
               }
            }
         }
      }
      else
      {
         ScanCode = 0;
         Key = (NextKey == '\0') ? KdbpGetCharKeyboard(&ScanCode) : NextKey;
         NextKey = '\0';
      }

      if ((ULONG)(Buffer - Orig) >= (Size - 1))
      {
         /* Buffer is full, accept only newlines */
         if (Key != '\n')
            continue;
      }

      if (Key == '\r')
      {
         /* Read the next char - this is to throw away a \n which most clients should
          * send after \r.
          */
		 KeStallExecutionProcessor(100000);
         if (KdbDebugState & KD_DEBUG_KDSERIAL)
            NextKey = KdbpTryGetCharSerial(5);
         else
            NextKey = KdbpTryGetCharKeyboard(&ScanCode, 5);
         if (NextKey == '\n' || NextKey == -1) /* \n or no response at all */
            NextKey = '\0';
         KdbpPrint("\n");
	 /*
	  * Repeat the last command if the user presses enter. Reduces the
	  * risk of RSI when single-stepping.
	  */
	 if (Buffer == Orig)
	 {
            strncpy(Buffer, LastCommand, Size);
            Buffer[Size - 1] = '\0';
	 }
	 else
	 {
            *Buffer = '\0';
            strncpy(LastCommand, Orig, sizeof (LastCommand));
            LastCommand[sizeof (LastCommand) - 1] = '\0';
         }
         return;
      }
      else if (Key == KEY_BS || Key == KEY_DEL)
      {
         if (Buffer > Orig)
         {
            Buffer--;
            *Buffer = 0;
            if (EchoOn)
               KdbpPrint("%c %c", KEY_BS, KEY_BS);
	    else
               KdbpPrint(" %c", KEY_BS);
         }
      }
      else if (ScanCode == KEY_SCAN_UP)
      {
         BOOLEAN Print = TRUE;
         if (CmdHistIndex < 0)
            CmdHistIndex = KdbCommandHistoryIndex;
         else
         {
            i = CmdHistIndex - 1;
            if (i < 0)
               CmdHistIndex = RTL_NUMBER_OF(KdbCommandHistory) - 1;
            if (KdbCommandHistory[i] != NULL && i != KdbCommandHistoryIndex)
               CmdHistIndex = i;
            else
               Print = FALSE;
         }
         if (Print && KdbCommandHistory[CmdHistIndex] != NULL)
         {
            while (Buffer > Orig)
            {
               Buffer--;
               *Buffer = 0;
               if (EchoOn)
                  KdbpPrint("%c %c", KEY_BS, KEY_BS);
               else
                  KdbpPrint(" %c", KEY_BS);
            }
            i = min(strlen(KdbCommandHistory[CmdHistIndex]), Size - 1);
            memcpy(Orig, KdbCommandHistory[CmdHistIndex], i);
            Orig[i] = '\0';
            Buffer = Orig + i;
            KdbpPrint("%s", Orig);
         }
      }
      else if (ScanCode == KEY_SCAN_DOWN)
      {
         if (CmdHistIndex > 0 && CmdHistIndex != KdbCommandHistoryIndex)
         {
            i = CmdHistIndex + 1;
            if (i >= (INT)RTL_NUMBER_OF(KdbCommandHistory))
               i = 0;
            if (KdbCommandHistory[i] != NULL)
            {
               CmdHistIndex = i;
               while (Buffer > Orig)
               {
                  Buffer--;
                  *Buffer = 0;
                  if (EchoOn)
                     KdbpPrint("%c %c", KEY_BS, KEY_BS);
                  else
                     KdbpPrint(" %c", KEY_BS);
               }
               i = min(strlen(KdbCommandHistory[CmdHistIndex]), Size - 1);
               memcpy(Orig, KdbCommandHistory[CmdHistIndex], i);
               Orig[i] = '\0';
               Buffer = Orig + i;
               KdbpPrint("%s", Orig);
            }
         }
      }
      else
      {
         if (EchoOn)
            KdbpPrint("%c", Key);

         *Buffer = Key;
         Buffer++;
      }
   }
}

/*!\brief Parses command line and executes command if found
 *
 * \param Command    Command line to parse and execute if possible.
 *
 * \retval TRUE   Don't continue execution.
 * \retval FALSE  Continue execution (leave KDB)
 */
STATIC BOOLEAN
KdbpDoCommand(
   IN PCHAR Command)
{
   ULONG i;
   PCHAR p;
   ULONG Argc;
   STATIC PCH Argv[256];
   STATIC CHAR OrigCommand[1024];

   strncpy(OrigCommand, Command, sizeof(OrigCommand) - 1);
   OrigCommand[sizeof(OrigCommand) - 1] = '\0';

   Argc = 0;
   p = Command;
   for (;;)
   {
      while (*p == '\t' || *p == ' ')
         p++;
      if (*p == '\0')
         break;

      i = strcspn(p, "\t ");
      Argv[Argc++] = p;
      p += i;
      if (*p == '\0')
         break;
      *p = '\0';
      p++;
   }
   if (Argc < 1)
      return TRUE;

   for (i = 0; i < RTL_NUMBER_OF(KdbDebuggerCommands); i++)
   {
      if (KdbDebuggerCommands[i].Name == NULL)
         continue;

      if (strcmp(KdbDebuggerCommands[i].Name, Argv[0]) == 0)
      {
         return KdbDebuggerCommands[i].Fn(Argc, Argv);
      }
   }

   KdbpPrint("Command '%s' is unknown.\n", OrigCommand);
   return TRUE;
}

/*!\brief KDB Main Loop.
 *
 * \param EnteredOnSingleStep  TRUE if KDB was entered on single step.
 */
VOID
KdbpCliMainLoop(
   IN BOOLEAN EnteredOnSingleStep)
{
   STATIC CHAR Command[1024];
   BOOLEAN Continue;

   if (EnteredOnSingleStep)
   {
      if (!KdbSymPrintAddress((PVOID)KdbCurrentTrapFrame->Tf.Eip))
      {
         KdbpPrint("<%x>", KdbCurrentTrapFrame->Tf.Eip);
      }
      KdbpPrint(": ");
      if (KdbpDisassemble(KdbCurrentTrapFrame->Tf.Eip, KdbUseIntelSyntax) < 0)
      {
         KdbpPrint("<INVALID>");
      }
      KdbpPrint("\n");
   }

   /* Flush the input buffer */
   if (KdbDebugState & KD_DEBUG_KDSERIAL)
   {
      while (KdbpTryGetCharSerial(1) != -1);
   }
   else
   {
      ULONG ScanCode;
      while (KdbpTryGetCharKeyboard(&ScanCode, 1) != -1);
   }

   /* Main loop */
   do
   {
      /* Print the prompt */
      KdbpPrint("kdb:> ");

      /* Read a command and remember it */
      KdbpReadCommand(Command, sizeof (Command));
      KdbpCommandHistoryAppend(Command);

      /* Reset the number of rows/cols printed and output aborted state */
      KdbNumberOfRowsPrinted = KdbNumberOfColsPrinted = 0;
      KdbOutputAborted = FALSE;

      /* Call the command */
      Continue = KdbpDoCommand(Command);
   } while (Continue);
}

/*!\brief Called when a module is loaded.
 *
 * \param Name  Filename of the module which was loaded.
 */
VOID
KdbpCliModuleLoaded(IN PUNICODE_STRING Name)
{
   if (!KdbBreakOnModuleLoad)
      return;

   KdbpPrint("Module %wZ loaded.\n", Name);
   DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
}

/*!\brief This function is called by KdbEnterDebuggerException...
 *
 * Used to interpret the init file in a context with a trapframe setup
 * (KdbpCliInit call KdbEnter which will call KdbEnterDebuggerException which will
 * call this function if KdbInitFileBuffer is not NULL.
 */
VOID
KdbpCliInterpretInitFile()
{
   PCHAR p1, p2;
   INT i;
   CHAR c;

   /* Execute the commands in the init file */
   DPRINT("KDB: Executing KDBinit file...\n");
   p1 = KdbInitFileBuffer;
   while (p1[0] != '\0')
   {
      i = strcspn(p1, "\r\n");
      if (i > 0)
      {
         c = p1[i];
         p1[i] = '\0';

         /* Look for "break" command and comments */
         p2 = p1;
         while (isspace(p2[0]))
            p2++;
         if (strncmp(p2, "break", sizeof("break")-1) == 0 &&
             (p2[sizeof("break")-1] == '\0' || isspace(p2[sizeof("break")-1])))
         {
            /* break into the debugger */
            KdbpCliMainLoop(FALSE);
         }
         else if (p2[0] != '#' && p2[0] != '\0') /* Ignore empty lines and comments */
         {
            KdbpDoCommand(p1);
         }

         p1[i] = c;
      }
      p1 += i;
      while (p1[0] == '\r' || p1[0] == '\n')
         p1++;
   }
   DPRINT("KDB: KDBinit executed\n");
}

/*!\brief Called when KDB is initialized
 *
 * Reads the KDBinit file from the SystemRoot\system32\drivers\etc directory and executes it.
 */
VOID
KdbpCliInit()
{
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING FileName;
   IO_STATUS_BLOCK Iosb;
   FILE_STANDARD_INFORMATION FileStdInfo;
   HANDLE hFile = NULL;
   INT FileSize;
   PCHAR FileBuffer;
   ULONG OldEflags = 0;

   /* Initialize the object attributes */
   RtlInitUnicodeString(&FileName, L"\\SystemRoot\\system32\\drivers\\etc\\KDBinit");
   InitializeObjectAttributes(&ObjectAttributes, &FileName, 0, NULL, NULL);

   /* Open the file */
   Status = ZwOpenFile(&hFile, FILE_READ_DATA, &ObjectAttributes, &Iosb, 0,
                       FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                       FILE_NO_INTERMEDIATE_BUFFERING);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Could not open \\SystemRoot\\system32\\drivers\\etc\\KDBinit (Status 0x%x)", Status);
      return;
   }

   /* Get the size of the file */
   Status = ZwQueryInformationFile(hFile, &Iosb, &FileStdInfo, sizeof (FileStdInfo),
                                   FileStandardInformation);
   if (!NT_SUCCESS(Status))
   {
      ZwClose(hFile);
      DPRINT("Could not query size of \\SystemRoot\\system32\\drivers\\etc\\KDBinit (Status 0x%x)", Status);
      return;
   }
   FileSize = FileStdInfo.EndOfFile.u.LowPart;

   /* Allocate memory for the file */
   FileBuffer = ExAllocatePool(PagedPool, FileSize + 1); /* add 1 byte for terminating '\0' */
   if (FileBuffer == NULL)
   {
      ZwClose(hFile);
      DPRINT("Could not allocate %d bytes for KDBinit file\n", FileSize);
      return;
   }

   /* Load file into memory */
   Status = ZwReadFile(hFile, 0, 0, 0, &Iosb, FileBuffer, FileSize, 0, 0);
   ZwClose(hFile);
   if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
   {
      ExFreePool(FileBuffer);
      DPRINT("Could not read KDBinit file into memory (Status 0x%lx)\n", Status);
      return;
   }
   FileSize = min(FileSize, (INT)Iosb.Information);
   FileBuffer[FileSize] = '\0';

   /* Enter critical section */
   Ke386SaveFlags(OldEflags);
   _disable();

   /* Interpret the init file... */
   KdbInitFileBuffer = FileBuffer;
   KdbEnter();
   KdbInitFileBuffer = NULL;

   /* Leave critical section */
   Ke386RestoreFlags(OldEflags);

   ExFreePool(FileBuffer);
}

