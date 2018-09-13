// ******************************
// Intel Confidential
// ******************************


#include <stdarg.h>
#include "ki.h"
#include "fedefs.h"

// MACH
#ifndef IN_KERNEL
extern void RaiseException ();
#endif

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#ifndef unix
#include <string.h>
#endif

// MACH #ifdef WIN32_OR_WIN64
// MACH #include <process.h>
// MACH #endif

#include "fepublic.h"
#include "fehelper.h"
#include "fesupprt.h"

void fp82_default_fp_exception_fault(void *ps, EM_uint_t isrcode);
void fp82_default_fp_exception_trap(void *ps, EM_uint_t isr_code);

#define RESTORE_CONSTANTS {                   \
   FR[0]       = FP_ZERO;                     \
   FR[1]       = FP_ONE;                      \
   PR[0]       = 1;                           \
   GR[0].value = 0;                           \
   GR[0].nat   = 0;                           \
}


// ***************************************************************
// GET/PUT F1
// ***************************************************************
void
GETSTATE_F1(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1, 
   EM_fp_reg_specifier   Fr3, 
   EM_fp_reg_specifier   Fr4, 
   EM_fp_reg_specifier   Fr2)  {

}

void
PUTSTATE_F1(
   EM_fp_reg_specifier Fr1) {

   RESTORE_CONSTANTS;                   
}

// ***************************************************************
// GET/PUT F4
// ***************************************************************
void
GETSTATE_F4(
   EM_pred_reg_specifier Pr0,
   EM_pred_reg_specifier Pr1,
   EM_pred_reg_specifier Pr2,
   EM_fp_reg_specifier   Fr2,
   EM_fp_reg_specifier   Fr3)  {
}

void
PUTSTATE_F4(
   EM_pred_reg_specifier Pr1,
   EM_pred_reg_specifier Pr2) {

    RESTORE_CONSTANTS;                   
}

// ***************************************************************
// GET/PUT F6
// ***************************************************************
void
GETSTATE_F6(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_pred_reg_specifier Pr2,
   EM_fp_reg_specifier   Fr2,
   EM_fp_reg_specifier   Fr3)  {
}

void
PUTSTATE_F6(
   EM_fp_reg_specifier   Fr1,
   EM_pred_reg_specifier Pr2) {

    RESTORE_CONSTANTS;                   
}


// ***************************************************************
// GET/PUT F7
// ***************************************************************
void
GETSTATE_F7(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_pred_reg_specifier Pr2,
   EM_fp_reg_specifier   Fr3)  {
}


void
PUTSTATE_F7(
   EM_fp_reg_specifier   Fr1,
   EM_pred_reg_specifier Pr2) {

    RESTORE_CONSTANTS;                   
}

// ***************************************************************
// GET/PUT F8
// ***************************************************************
void
GETSTATE_F8(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_fp_reg_specifier   Fr2, 
   EM_fp_reg_specifier   Fr3)  {
}

void
PUTSTATE_F8(
   EM_fp_reg_specifier   Fr1) {

   RESTORE_CONSTANTS;                   
}

// ***************************************************************
// GET/PUT F10
// ***************************************************************
void
GETSTATE_F10(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_fp_reg_specifier   Fr2)  {

    SIGNED_FORM   = 0;
    TRUNC_FORM    = 0;
    UNSIGNED_FORM = 0;
}

void
PUTSTATE_F10(
   EM_fp_reg_specifier   Fr1)  {

   RESTORE_CONSTANTS;      
}


void fp82_default_fp_exception_fault(void *vps, EM_uint_t isr_code) {
EM_state_type *ps;
   ps = (EM_state_type *)vps;
   ps->state_MERCED_RTL &= ~0xffff0000;
   ps->state_MERCED_RTL |= (isr_code << 16);
   ps->trap_type = 1; // fault
}


void fp82_default_fp_exception_trap(void *vps, EM_uint_t isr_code) {
EM_state_type *ps;
   ps = (EM_state_type *)vps;
   ps->state_MERCED_RTL &= ~0xffff0000;
   ps->state_MERCED_RTL |= (isr_code << 16);
   ps->trap_type = 0; // trap
}


/*************************************************************
fp82_EM_initialize_state()
*************************************************************/
void
fp82_EM_initialize_state(EM_state_type *ps) {
    EM_int_t i;

    PSR.be                = 0;
    PSR.dfl               = 0;
    PSR.dfh               = 0;
    PSR.mfl               = 0;
    PSR.mfh               = 0;

    FPSR.traps_vd         = 1;
    FPSR.traps_dd         = 1;
    FPSR.traps_zd         = 1;
    FPSR.traps_od         = 1;
    FPSR.traps_ud         = 1;
    FPSR.traps_id         = 1;

    FPSR.sf0_controls_ftz = 0;
    FPSR.sf0_controls_wre = 1;
    FPSR.sf0_controls_pc  = sf_double_extended;
    FPSR.sf0_controls_rc  = rc_rn;
    FPSR.sf0_controls_td  = 0;
    FPSR.sf0_flags_v      = 0;
    FPSR.sf0_flags_d      = 0;
    FPSR.sf0_flags_z      = 0;
    FPSR.sf0_flags_o      = 0;
    FPSR.sf0_flags_u      = 0;
    FPSR.sf0_flags_i      = 0;
 
    FPSR.sf1_controls_ftz = 0;
    FPSR.sf1_controls_wre = 1;
    FPSR.sf1_controls_pc  = sf_double_extended;
    FPSR.sf1_controls_rc  = rc_rn;
    FPSR.sf1_controls_td  = 0;
    FPSR.sf1_flags_v      = 0;
    FPSR.sf1_flags_d      = 0;
    FPSR.sf1_flags_z      = 0;
    FPSR.sf1_flags_o      = 0;
    FPSR.sf1_flags_u      = 0;
    FPSR.sf1_flags_i      = 0;

    FPSR.sf2_controls_ftz = 0;
    FPSR.sf2_controls_wre = 1;
    FPSR.sf2_controls_pc  = sf_double_extended;
    FPSR.sf2_controls_rc  = rc_rn;
    FPSR.sf2_controls_td  = 0;
    FPSR.sf2_flags_v      = 0;
    FPSR.sf2_flags_d      = 0;
    FPSR.sf2_flags_z      = 0;
    FPSR.sf2_flags_o      = 0;
    FPSR.sf2_flags_u      = 0;
    FPSR.sf2_flags_i      = 0;

    FPSR.sf3_controls_ftz = 0;
    FPSR.sf3_controls_wre = 1;
    FPSR.sf3_controls_pc  = sf_double_extended;
    FPSR.sf3_controls_rc  = rc_rn;
    FPSR.sf3_controls_td  = 0;
    FPSR.sf3_flags_v      = 0;
    FPSR.sf3_flags_d      = 0;
    FPSR.sf3_flags_z      = 0;
    FPSR.sf3_flags_o      = 0;
    FPSR.sf3_flags_u      = 0;
    FPSR.sf3_flags_i      = 0;
    FPSR.reserved         = 0;

    PR[0] = 1;
    for (i=1;i<EM_NUM_PR;i++)
        PR[i] = 0;

    ps->state_MERCED_RTL = 0;

    FR[0] = FP_ZERO;
    FR[1] = FP_ONE;

    for (i=2;i<MAX_REAL_FR_INDEX;i++)
        FR[i] = FP_ZERO;

    for (i=0;i<MAX_REAL_GR_INDEX;i++) {
        GR[i].value = 0;
        GR[i].nat   = 0;
    }

    ps->state_fp82_fp_exception_fault = fp82_default_fp_exception_fault;
    ps->state_fp82_fp_exception_trap = fp82_default_fp_exception_trap;
}


/* Other EM ISA helper functions */
 
EM_boolean_t
fp82_fp_software_assistance_required(EM_state_type *ps, 
    EM_opcode_type calling_instruction, ...)
{
    return(0);
}



INLINE void
disabled_fp_register_fault(EM_uint_t isr_code, EM_uint_t itype)
{
#ifdef IN_KERNEL
    FP_EMULATION_ERROR0 ("disabled_fp_register_fault () Internal Error\n");
#else
    printf ("disabled_fp_register_fault () Internal Error\n");
    exit (1);
#endif
}


INLINE void
fp_exception_fault(EM_uint_t isr_code)
{

    PR[0]       = 1;
    GR[0].value = 0;
    GR[0].nat   = 0;
    FR[0]       = FP_ZERO;
    FR[1]       = FP_ONE;

    ps->state_fp82_fp_exception_fault((EM_state_type *)ps, isr_code);

}

INLINE void
fp_exception_trap(EM_uint_t isr_code)
{

    isr_code |= 0x00000001;

    ps->state_fp82_fp_exception_trap((EM_state_type *)ps, isr_code);

}


INLINE void
illegal_operation_fault(EM_uint_t non_rs)
{
#ifdef IN_KERNEL
    FP_EMULATION_ERROR0 ("illegal_operation_fault () Internal Error\n");
#else
    printf ("illegal_operation_fault () Internal Error\n");
    exit (1);
#endif
}

INLINE void
check_target_register(EM_uint_t reg_specifier, EM_uint_t itype)
{
  if(reg_specifier == 0) {
#ifdef IN_KERNEL
    FP_EMULATION_ERROR0 ("fp_check_target_register () Internal Error\n");
#else
    printf ("fp_check_target_register () Internal Error\n");
    exit (1);
#endif
  }
}

void
fp_check_target_register(EM_uint_t reg_specifier)
{
  if( (reg_specifier == 0) || (reg_specifier == 1) ){
#ifdef IN_KERNEL
    FP_EMULATION_ERROR0 ("fp_check_target_register () Internal Error\n")
#else
    printf ("fp_check_target_register () Internal Error\n");
    exit (1);
#endif
  }
}


INLINE void
reserved_register_field_fault(EM_uint_t val)
{
#ifdef IN_KERNEL
    FP_EMULATION_ERROR0 ("reserved_register_field_fault () Internal Error\n")
#else
    printf ("reserved_register_field_fault () Internal Error\n");
    exit (1);
#endif
}


