/******************************************************************************
 *
 * Name: acmacros.h - C macros for the entire subsystem.
 *       $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ACMACROS_H__
#define __ACMACROS_H__

/*
 * Data manipulation macros
 */

#ifndef LODWORD
#define LODWORD(l)                      ((u32)(UINT64)(l))
#endif

#ifndef HIDWORD
#define HIDWORD(l)                      ((u32)((((UINT64)(l)) >> 32) & 0xFFFFFFFF))
#endif

#ifndef LOWORD
#define LOWORD(l)                       ((u16)(NATIVE_UINT)(l))
#endif

#ifndef HIWORD
#define HIWORD(l)                       ((u16)((((NATIVE_UINT)(l)) >> 16) & 0xFFFF))
#endif

#ifndef LOBYTE
#define LOBYTE(l)                       ((u8)(u16)(l))
#endif

#ifndef HIBYTE
#define HIBYTE(l)                       ((u8)((((u16)(l)) >> 8) & 0xFF))
#endif

#define BIT0(x)                         ((((x) & 0x01) > 0) ? 1 : 0)
#define BIT1(x)                         ((((x) & 0x02) > 0) ? 1 : 0)
#define BIT2(x)                         ((((x) & 0x04) > 0) ? 1 : 0)

#define BIT3(x)                         ((((x) & 0x08) > 0) ? 1 : 0)
#define BIT4(x)                         ((((x) & 0x10) > 0) ? 1 : 0)
#define BIT5(x)                         ((((x) & 0x20) > 0) ? 1 : 0)
#define BIT6(x)                         ((((x) & 0x40) > 0) ? 1 : 0)
#define BIT7(x)                         ((((x) & 0x80) > 0) ? 1 : 0)

#define LOW_BASE(w)                     ((u16) ((w) & 0x0000FFFF))
#define MID_BASE(b)                     ((u8) (((b) & 0x00FF0000) >> 16))
#define HI_BASE(b)                      ((u8) (((b) & 0xFF000000) >> 24))
#define LOW_LIMIT(w)                    ((u16) ((w) & 0x0000FFFF))
#define HI_LIMIT(b)                     ((u8) (((b) & 0x00FF0000) >> 16))


#ifdef _IA16
/*
 * For 16-bit addresses, we have to assume that the upper 32 bits
 * are zero.
 */
#define ACPI_GET_ADDRESS(a)             ((a).lo)
#define ACPI_STORE_ADDRESS(a,b)         {(a).hi=0;(a).lo=(b);}
#define ACPI_VALID_ADDRESS(a)           ((a).hi | (a).lo)

#else
/*
 * Full 64-bit address on 32-bit and 64-bit platforms
 */
#define ACPI_GET_ADDRESS(a)             (a)
#define ACPI_STORE_ADDRESS(a,b)         ((a)=(b))
#define ACPI_VALID_ADDRESS(a)           (a)
#endif
 /*
  * Extract a byte of data using a pointer.  Any more than a byte and we
  * get into potential aligment issues -- see the STORE macros below
  */
#define GET8(addr)                      (*(u8*)(addr))


/*
 * Macros for moving data around to/from buffers that are possibly unaligned.
 * If the hardware supports the transfer of unaligned data, just do the store.
 * Otherwise, we have to move one byte at a time.
 */

#ifdef _HW_ALIGNMENT_SUPPORT

/* The hardware supports unaligned transfers, just do the move */

#define MOVE_UNALIGNED16_TO_16(d,s)     *(u16*)(d) = *(u16*)(s)
#define MOVE_UNALIGNED32_TO_32(d,s)     *(u32*)(d) = *(u32*)(s)
#define MOVE_UNALIGNED16_TO_32(d,s)     *(u32*)(d) = *(u16*)(s)

#else
/*
 * The hardware does not support unaligned transfers.  We must move the
 * data one byte at a time.  These macros work whether the source or
 * the destination (or both) is/are unaligned.
 */

#define MOVE_UNALIGNED16_TO_16(d,s)     {((u8 *)(d))[0] = ((u8 *)(s))[0];\
	 ((u8 *)(d))[1] = ((u8 *)(s))[1];}

#define MOVE_UNALIGNED32_TO_32(d,s)     {((u8 *)(d))[0] = ((u8 *)(s))[0];\
			  ((u8 *)(d))[1] = ((u8 *)(s))[1];\
			  ((u8 *)(d))[2] = ((u8 *)(s))[2];\
			  ((u8 *)(d))[3] = ((u8 *)(s))[3];}

#define MOVE_UNALIGNED16_TO_32(d,s)     {(*(u32*)(d)) = 0; MOVE_UNALIGNED16_TO_16(d,s);}

#endif


/*
 * Fast power-of-two math macros for non-optimized compilers
 */

#define _DIV(value,power_of2)           ((u32) ((value) >> (power_of2)))
#define _MUL(value,power_of2)           ((u32) ((value) << (power_of2)))
#define _MOD(value,divisor)             ((u32) ((value) & ((divisor) -1)))

#define DIV_2(a)                        _DIV(a,1)
#define MUL_2(a)                        _MUL(a,1)
#define MOD_2(a)                        _MOD(a,2)

#define DIV_4(a)                        _DIV(a,2)
#define MUL_4(a)                        _MUL(a,2)
#define MOD_4(a)                        _MOD(a,4)

#define DIV_8(a)                        _DIV(a,3)
#define MUL_8(a)                        _MUL(a,3)
#define MOD_8(a)                        _MOD(a,8)

#define DIV_16(a)                       _DIV(a,4)
#define MUL_16(a)                       _MUL(a,4)
#define MOD_16(a)                       _MOD(a,16)

/*
 * Divide and Modulo
 */
#define ACPI_DIVIDE(n,d)                ((n) / (d))
#define ACPI_MODULO(n,d)                ((n) % (d))

/*
 * Rounding macros (Power of two boundaries only)
 */

#define ROUND_DOWN(value,boundary)      ((value) & (~((boundary)-1)))
#define ROUND_UP(value,boundary)        (((value) + ((boundary)-1)) & (~((boundary)-1)))

#define ROUND_DOWN_TO_32_BITS(a)        ROUND_DOWN(a,4)
#define ROUND_DOWN_TO_64_BITS(a)        ROUND_DOWN(a,8)
#define ROUND_DOWN_TO_NATIVE_WORD(a)    ROUND_DOWN(a,ALIGNED_ADDRESS_BOUNDARY)

#define ROUND_UP_TO_32_bITS(a)          ROUND_UP(a,4)
#define ROUND_UP_TO_64_bITS(a)          ROUND_UP(a,8)
#define ROUND_UP_TO_NATIVE_WORD(a)      ROUND_UP(a,ALIGNED_ADDRESS_BOUNDARY)

#define ROUND_PTR_UP_TO_4(a,b)          ((b *)(((NATIVE_UINT)(a) + 3) & ~3))
#define ROUND_PTR_UP_TO_8(a,b)          ((b *)(((NATIVE_UINT)(a) + 7) & ~7))

#define ROUND_UP_TO_1_k(a)              (((a) + 1023) >> 10)

#ifdef DEBUG_ASSERT
#undef DEBUG_ASSERT
#endif


/* Macros for GAS addressing */

#define ACPI_PCI_DEVICE_MASK            (UINT64) 0x0000FFFF00000000
#define ACPI_PCI_FUNCTION_MASK          (UINT64) 0x00000000FFFF0000
#define ACPI_PCI_REGISTER_MASK          (UINT64) 0x000000000000FFFF

#define ACPI_PCI_FUNCTION(a)            (u32) ((((a) & ACPI_PCI_FUNCTION_MASK) >> 16))
#define ACPI_PCI_DEVICE(a)              (u32) ((((a) & ACPI_PCI_DEVICE_MASK) >> 32))

#ifndef _IA16
#define ACPI_PCI_REGISTER(a)            (u32) (((a) & ACPI_PCI_REGISTER_MASK))
#define ACPI_PCI_DEVFUN(a)              (u32) ((ACPI_PCI_DEVICE(a) << 16) | ACPI_PCI_FUNCTION(a))

#else
#define ACPI_PCI_REGISTER(a)            (u32) (((a) & 0x0000FFFF))
#define ACPI_PCI_DEVFUN(a)              (u32) ((((a) & 0xFFFF0000) >> 16))

#endif

/*
 * An ACPI_HANDLE (which is actually an ACPI_NAMESPACE_NODE *) can appear in some contexts,
 * such as on ap_obj_stack, where a pointer to an ACPI_OPERAND_OBJECT can also
 * appear.  This macro is used to distinguish them.
 *
 * The Data_type field is the first field in both structures.
 */

#define VALID_DESCRIPTOR_TYPE(d,t)      (((ACPI_NAMESPACE_NODE *)d)->data_type == t)


/* Macro to test the object type */

#define IS_THIS_OBJECT_TYPE(d,t)        (((ACPI_OPERAND_OBJECT  *)d)->common.type == (u8)t)

/* Macro to check the table flags for SINGLE or MULTIPLE tables are allowed */

#define IS_SINGLE_TABLE(x)              (((x) & 0x01) == ACPI_TABLE_SINGLE ? 1 : 0)

/*
 * Macro to check if a pointer is within an ACPI table.
 * Parameter (a) is the pointer to check.  Parameter (b) must be defined
 * as a pointer to an ACPI_TABLE_HEADER.  (b+1) then points past the header,
 * and ((u8 *)b+b->Length) points one byte past the end of the table.
 */

#ifndef _IA16
#define IS_IN_ACPI_TABLE(a,b)           (((u8 *)(a) >= (u8 *)(b + 1)) &&\
					  ((u8 *)(a) < ((u8 *)b + b->length)))

#else
#define IS_IN_ACPI_TABLE(a,b)           (_segment)(a) == (_segment)(b) &&\
							   (((u8 *)(a) >= (u8 *)(b + 1)) &&\
							   ((u8 *)(a) < ((u8 *)b + b->length)))
#endif

/*
 * Macros for the master AML opcode table
 */

#ifdef ACPI_DEBUG
#define OP_INFO_ENTRY(flags,name,Pargs,Iargs)     {flags,Pargs,Iargs,name}
#else
#define OP_INFO_ENTRY(flags,name,Pargs,Iargs)     {flags,Pargs,Iargs}
#endif

#define ARG_TYPE_WIDTH                  5
#define ARG_1(x)                        ((u32)(x))
#define ARG_2(x)                        ((u32)(x) << (1 * ARG_TYPE_WIDTH))
#define ARG_3(x)                        ((u32)(x) << (2 * ARG_TYPE_WIDTH))
#define ARG_4(x)                        ((u32)(x) << (3 * ARG_TYPE_WIDTH))
#define ARG_5(x)                        ((u32)(x) << (4 * ARG_TYPE_WIDTH))
#define ARG_6(x)                        ((u32)(x) << (5 * ARG_TYPE_WIDTH))

#define ARGI_LIST1(a)                   (ARG_1(a))
#define ARGI_LIST2(a,b)                 (ARG_1(b)|ARG_2(a))
#define ARGI_LIST3(a,b,c)               (ARG_1(c)|ARG_2(b)|ARG_3(a))
#define ARGI_LIST4(a,b,c,d)             (ARG_1(d)|ARG_2(c)|ARG_3(b)|ARG_4(a))
#define ARGI_LIST5(a,b,c,d,e)           (ARG_1(e)|ARG_2(d)|ARG_3(c)|ARG_4(b)|ARG_5(a))
#define ARGI_LIST6(a,b,c,d,e,f)         (ARG_1(f)|ARG_2(e)|ARG_3(d)|ARG_4(c)|ARG_5(b)|ARG_6(a))

#define ARGP_LIST1(a)                   (ARG_1(a))
#define ARGP_LIST2(a,b)                 (ARG_1(a)|ARG_2(b))
#define ARGP_LIST3(a,b,c)               (ARG_1(a)|ARG_2(b)|ARG_3(c))
#define ARGP_LIST4(a,b,c,d)             (ARG_1(a)|ARG_2(b)|ARG_3(c)|ARG_4(d))
#define ARGP_LIST5(a,b,c,d,e)           (ARG_1(a)|ARG_2(b)|ARG_3(c)|ARG_4(d)|ARG_5(e))
#define ARGP_LIST6(a,b,c,d,e,f)         (ARG_1(a)|ARG_2(b)|ARG_3(c)|ARG_4(d)|ARG_5(e)|ARG_6(f))

#define GET_CURRENT_ARG_TYPE(list)      (list & ((u32) 0x1F))
#define INCREMENT_ARG_LIST(list)        (list >>= ((u32) ARG_TYPE_WIDTH))


/*
 * Reporting macros that are never compiled out
 */

#define PARAM_LIST(pl)                  pl

/*
 * Error reporting.  These versions add callers module and line#.  Since
 * _THIS_MODULE gets compiled out when ACPI_DEBUG isn't defined, only
 * use it in debug mode.
 */

#ifdef ACPI_DEBUG

#define REPORT_INFO(fp)                 {_report_info(_THIS_MODULE,__LINE__,_COMPONENT); \
									  debug_print_raw PARAM_LIST(fp);}
#define REPORT_ERROR(fp)                {_report_error(_THIS_MODULE,__LINE__,_COMPONENT); \
											debug_print_raw PARAM_LIST(fp);}
#define REPORT_WARNING(fp)              {_report_warning(_THIS_MODULE,__LINE__,_COMPONENT); \
											debug_print_raw PARAM_LIST(fp);}

#else

#define REPORT_INFO(fp)                 {_report_info("ACPI",__LINE__,_COMPONENT); \
											debug_print_raw PARAM_LIST(fp);}
#define REPORT_ERROR(fp)                {_report_error("ACPI",__LINE__,_COMPONENT); \
											debug_print_raw PARAM_LIST(fp);}
#define REPORT_WARNING(fp)              {_report_warning("ACPI",__LINE__,_COMPONENT); \
											debug_print_raw PARAM_LIST(fp);}

#endif

/* Error reporting.  These versions pass thru the module and line# */

#define _REPORT_INFO(a,b,c,fp)          {_report_info(a,b,c); \
											debug_print_raw PARAM_LIST(fp);}
#define _REPORT_ERROR(a,b,c,fp)         {_report_error(a,b,c); \
											debug_print_raw PARAM_LIST(fp);}
#define _REPORT_WARNING(a,b,c,fp)       {_report_warning(a,b,c); \
											debug_print_raw PARAM_LIST(fp);}

/* Buffer dump macros */

#define DUMP_BUFFER(a,b)                acpi_cm_dump_buffer((u8 *)a,b,DB_BYTE_DISPLAY,_COMPONENT)

/*
 * Debug macros that are conditionally compiled
 */

#ifdef ACPI_DEBUG

#define MODULE_NAME(name)               static char *_THIS_MODULE = name;

/*
 * Function entry tracing.
 * The first parameter should be the procedure name as a quoted string.  This is declared
 * as a local string ("_Proc_name) so that it can be also used by the function exit macros below.
 */

#define FUNCTION_TRACE(a)               char * _proc_name = a;\
										function_trace(_THIS_MODULE,__LINE__,_COMPONENT,a)
#define FUNCTION_TRACE_PTR(a,b)         char * _proc_name = a;\
										function_trace_ptr(_THIS_MODULE,__LINE__,_COMPONENT,a,(void *)b)
#define FUNCTION_TRACE_U32(a,b)         char * _proc_name = a;\
										function_trace_u32(_THIS_MODULE,__LINE__,_COMPONENT,a,(u32)b)
#define FUNCTION_TRACE_STR(a,b)         char * _proc_name = a;\
										function_trace_str(_THIS_MODULE,__LINE__,_COMPONENT,a,(NATIVE_CHAR *)b)
/*
 * Function exit tracing.
 * WARNING: These macros include a return statement.  This is usually considered
 * bad form, but having a separate exit macro is very ugly and difficult to maintain.
 * One of the FUNCTION_TRACE macros above must be used in conjunction with these macros
 * so that "_Proc_name" is defined.
 */
#define return_VOID                     {function_exit(_THIS_MODULE,__LINE__,_COMPONENT,_proc_name);return;}
#define return_ACPI_STATUS(s)           {function_status_exit(_THIS_MODULE,__LINE__,_COMPONENT,_proc_name,s);return(s);}
#define return_VALUE(s)                 {function_value_exit(_THIS_MODULE,__LINE__,_COMPONENT,_proc_name,(ACPI_INTEGER)s);return(s);}
#define return_PTR(s)                   {function_ptr_exit(_THIS_MODULE,__LINE__,_COMPONENT,_proc_name,(u8 *)s);return(s);}


/* Conditional execution */

#define DEBUG_EXEC(a)                   a
#define NORMAL_EXEC(a)

#define DEBUG_DEFINE(a)                 a;
#define DEBUG_ONLY_MEMBERS(a)           a;
#define _OPCODE_NAMES
#define _VERBOSE_STRUCTURES


/* Stack and buffer dumping */

#define DUMP_STACK_ENTRY(a)             acpi_aml_dump_operand(a)
#define DUMP_OPERANDS(a,b,c,d,e)        acpi_aml_dump_operands(a,b,c,d,e,_THIS_MODULE,__LINE__)


#define DUMP_ENTRY(a,b)                 acpi_ns_dump_entry (a,b)
#define DUMP_TABLES(a,b)                acpi_ns_dump_tables(a,b)
#define DUMP_PATHNAME(a,b,c,d)          acpi_ns_dump_pathname(a,b,c,d)
#define DUMP_RESOURCE_LIST(a)           acpi_rs_dump_resource_list(a)
#define BREAK_MSG(a)                    acpi_os_breakpoint (a)

/*
 * Generate INT3 on ACPI_ERROR (Debug only!)
 */

#define ERROR_BREAK
#ifdef  ERROR_BREAK
#define BREAK_ON_ERROR(lvl)             if ((lvl)&ACPI_ERROR) acpi_os_breakpoint("Fatal error encountered\n")
#else
#define BREAK_ON_ERROR(lvl)
#endif

/*
 * Master debug print macros
 * Print iff:
 *    1) Debug print for the current component is enabled
 *    2) Debug error level or trace level for the print statement is enabled
 *
 */

#define TEST_DEBUG_SWITCH(lvl)          if (((lvl) & acpi_dbg_level) && (_COMPONENT & acpi_dbg_layer))

#define DEBUG_PRINT(lvl,fp)             TEST_DEBUG_SWITCH(lvl) {\
											debug_print_prefix (_THIS_MODULE,__LINE__);\
											debug_print_raw PARAM_LIST(fp);\
											BREAK_ON_ERROR(lvl);}

#define DEBUG_PRINT_RAW(lvl,fp)         TEST_DEBUG_SWITCH(lvl) {\
											debug_print_raw PARAM_LIST(fp);}


/* Assert macros */

#define ACPI_ASSERT(exp)                if(!(exp)) \
											acpi_os_dbg_assert(#exp, __FILE__, __LINE__, "Failed Assertion")

#define DEBUG_ASSERT(msg, exp)          if(!(exp)) \
											acpi_os_dbg_assert(#exp, __FILE__, __LINE__, msg)


#else
/*
 * This is the non-debug case -- make everything go away,
 * leaving no executable debug code!
 */

#define MODULE_NAME(name)
#define _THIS_MODULE ""

#define DEBUG_EXEC(a)
#define NORMAL_EXEC(a)                  a;

#define DEBUG_DEFINE(a)
#define DEBUG_ONLY_MEMBERS(a)
#define FUNCTION_TRACE(a)
#define FUNCTION_TRACE_PTR(a,b)
#define FUNCTION_TRACE_U32(a,b)
#define FUNCTION_TRACE_STR(a,b)
#define FUNCTION_EXIT
#define FUNCTION_STATUS_EXIT(s)
#define FUNCTION_VALUE_EXIT(s)
#define DUMP_STACK_ENTRY(a)
#define DUMP_OPERANDS(a,b,c,d,e)
#define DUMP_ENTRY(a,b)
#define DUMP_TABLES(a,b)
#define DUMP_PATHNAME(a,b,c,d)
#define DUMP_RESOURCE_LIST(a)
#define DEBUG_PRINT(l,f)
#define DEBUG_PRINT_RAW(l,f)
#define BREAK_MSG(a)

#define return_VOID                     return
#define return_ACPI_STATUS(s)           return(s)
#define return_VALUE(s)                 return(s)
#define return_PTR(s)                   return(s)

#define ACPI_ASSERT(exp)
#define DEBUG_ASSERT(msg, exp)

#endif

/*
 * Some code only gets executed when the debugger is built in.
 * Note that this is entirely independent of whether the
 * DEBUG_PRINT stuff (set by ACPI_DEBUG) is on, or not.
 */
#ifdef ENABLE_DEBUGGER
#define DEBUGGER_EXEC(a)                a
#else
#define DEBUGGER_EXEC(a)
#endif


/*
 * For 16-bit code, we want to shrink some things even though
 * we are using ACPI_DEBUG to get the debug output
 */
#ifdef _IA16
#undef DEBUG_ONLY_MEMBERS
#undef _VERBOSE_STRUCTURES
#define DEBUG_ONLY_MEMBERS(a)
#endif


#ifdef ACPI_DEBUG

/*
 * 1) Set name to blanks
 * 2) Copy the object name
 */

#define ADD_OBJECT_NAME(a,b)            MEMSET (a->common.name, ' ', sizeof (a->common.name));\
										STRNCPY (a->common.name, acpi_gbl_ns_type_names[b], sizeof (a->common.name))

#else

#define ADD_OBJECT_NAME(a,b)

#endif


#endif /* ACMACROS_H */
