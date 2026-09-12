#!/usr/bin/env python3
"""
This script enables processing of symcryptasm files so that they can be assembled in a variety of
environments without requiring forking or duplication of source files - symcryptasm files phrase
assembly in an assembler and environment agnostic way.

The current target assemblers are:
    MASM, GAS, and armasm64 (Arm64 assembler which ships with MSVC)
The current target environments are:
    amd64 Windows (using the Microsoft x64 calling convention),
    amd64 Linux (using the SystemV amd64 calling convention),
    arm64 Windows (using the aapcs64 calling convention),
    arm64 Windows (using the arm64ec calling convention), and
    arm64 Linux (using the aapcs64 calling convention)


The plan is to rephrase all remaining .asm in SymCrypt as symcryptasm, extending support as
appropriate to enable this effort.

Normally the processing of symcryptasm files takes place in 2 passes. The first pass is performed by
this symcryptasm_processor.py script, which does the more stateful processing, outputting a .cppasm
file. If the processed symcryptasm file includes other files via the INCLUDE directive, the contents
of the included files are merged at their point of inclusion to generate a single expanded symcryptasm
file which is saved with a .symcryptasmexp extension to the output folder. For symcryptasm files which
do not include other files, there's no corresponding .symcryptasmexp file as it would be identical to
the source file.

The .cppasm files are further processed by the C preprocessor to do more simple stateless text
substitutions, outputting a .asm file which can be assembled by the target assembler for the target
environment.
The exception is when using the armasm64 assembler, which uses the C preprocessor before assembling
its inputs already; so the output of this script is directly assembled by armasm64.

We have set up the intermediate generated files to be created in the output directories in both
razzle and CMake builds.

### symcryptasm syntax ###

Different calling conventions pass arguments to functions in different registers, have differing
numbers of volatile and non-volatile registers, and use the stack in different ways.

We define our own register naming scheme which abstracts away the differences between calling
conventions. The generalities of the naming scheme will be similar across target architectures, but
refer to the Architecture specifics below for details. For the following general information we use
the notation R<n> to denote registers in the symcryptasm register naming scheme.


A leaf function (a function which does not call another function) begins with an invocation of the
FUNCTION_START macro which currently takes 3 mandatory and 2 optional arguments:
1) The function name
    This must be the name that matches the corresponding declaration of the function
2) The number of arguments (arg_count) that the function takes
    These arguments will be accessible in some contiguous region of the symcrypt registers at the
    start of the function
        On amd64 this contiguous region is R1..R<arg_count>
        On arm64 this contiguous region is R0..R<arg_count-1>
    Note: arg_count need not correspond to the exact number of argument in the function declaration
    if the assembly does not use some tail of the arguments
3) The number of registers (reg_count) that the function uses
    These registers will be accessible as R0..R<reg_count-1>
4) Stack allocation size (stack_alloc_size) for local variables (amd64 only)
    This parameter is optional, default = 0 if not provided.
    Allows reserving space for local variables on the stack. This value will be rounded to nearest
    multiple of 8 and the stack pointer will be aligned on 16B boundary. The allocated space does
    not include the space needed to save non-volatile registers or shadow space and for function use only.
    If the function is not nested, rsp will point to the beginning of the buffer after the prologue.
    Otherwise, rsp will point to shadow space, which is right below the allocated buffer on the stack.
    In this case, the function has to access the local buffer by using an offset equal to the size of the
    shadow space (32B).
5) The number of Xmm registers (xmm_reg_count) that the function uses (amd64 only)
    This parameter is optional and can take on values [0,16], default = 0 if not provided.
    If a non-zero value is specified, used registers are assumed to be Xmm0..Xmm<xmm_reg_count-1>
    When xmm_reg_count > 6, used Xmm registers starting from Xmm6 will be saved on the
    stack on amd64 Windows environment.

A leaf function ends with the FUNCTION_END macro, which also takes the function name
    (a FUNCTION_END macro's function name must match the preceding FUNCTION_START's name)

At the function start a prologue is generated which arranges the arguments appropriately in
registers, and saves non-volatile registers that have been requested to be used.
At the function end an epilogue is generated which restores the non-volatile registers and returns.


A nested function (a function which does call another function) is specified similarly, only using
NESTED_FUNCTION_START and NESTED_FUNCTION_END macros. A nested function currently updates and aligns
the stack pointer in the function prologue, and avoids use of the redzone in the SystemV ABI.
Nested functions are not currently supported for Arm64.


A macro begins with an invocation of the MACRO_START macro which takes the Macro name, and variable
number of macros argument names. It ends with MACRO_END.

### Architecture specifics ###

### amd64 ###
We allow up to 15 registers to be addressed, with the names:
Q0-Q14 (64-bit registers), D0-D14 (32-bit registers), W0-W14 (16-bit registers), and B0-B14 (8-bit
registers)
Xmm0-Xmm5 registers may be used directly in assembly too, as in both amd64 calling conventions we
currently support, these registers are volatile so do not need any special handling. If the number
of used Xmm registers specified in FUNCTION_START macro is greater than 6, those registers among
Xmm6..Xmm15 are saved/restored on amd64 Windows. There is no save/restore for Xmm16-Xmm31 as of now.

On function entry we insert a prologue which ensures:
Q0 is the result register (the return value of the function, and the low half of a multiplication)
Q1-Q6 are the first 6 arguments passed to the function

Additionally, there is a special case for functions using mul or mulx instructions, as these
instructions make rdx a special register. Functions using these instructions may address Q0-Q13,
and QH. As rdx is used to pass arguments, its value is moved to another register in the function
prologue. The MUL_FUNCTION_START and MUL_FUNCTION_END macros are used in this case.
    We currently do not support nested mul functions, as we have none of them.

Stack layout for amd64 is as follows. Xmm registers are volatile and not saved on Linux.

            Memory          Exists if
    |-------------------|
    |                   |
    |   Shadow space    |
    |                   |
    |-------------------|
    |   Return address  |
    |-------------------|
    |   Non-volatile    |
    |   general purpose |   reg_count > volatile_registers
    |   registers       |
    |-------------------|
    |   Non-volatile    |
    |   Xmm registers   |   xmm_reg_count > 6 and Windows
    |  (Windows only)   |
    |-------------------|---> 16B aligned
    |                   |
    |   Local variables |   stack_alloc_size > 0
    |                   |
    |-------------------|---> 16B aligned
    |                   |
    |   Shadow space    |   nested
    | (nested function) |
    |-------------------|---> 16B aligned



### arm64 ###
We allow up to 23 registers to be addressed, with the names:
X_0-X_22 (64-bit registers) and W_0-W_22 (32-bit registers)
v0-v7 ASIMD registers may by used directly in assembly too, as in both arm64 calling conventions we
currently support, these registers are volatile so do not need any special handling

X_0 is always the result register and the first argument passed to the function.
X_1-X_7 are the arguments 2-8 passed to the function


### arm (32) ###
We allow the registers r0-r12 to be addressed as r13-r15 are special registers that we cannot use as general purpose registers.
As r12 is volatile in a leaf function, it should be used in preference to r4, to avoid spilling/restoring a register.

"""

import re
import types
import logging
import os

class Register:
    """A class to represent registers"""

    def __init__(self, name64, name32, name16=None, name8=None):
        self.name64 = name64
        self.name32 = name32
        self.name16 = name16
        self.name8  = name8

# amd64 registers
AMD64_RAX = Register("rax",  "eax",   "ax",   "al")
AMD64_RBX = Register("rbx",  "ebx",   "bx",   "bl")
AMD64_RCX = Register("rcx",  "ecx",   "cx",   "cl")
AMD64_RDX = Register("rdx",  "edx",   "dx",   "dl")
AMD64_RSI = Register("rsi",  "esi",   "si",  "sil")
AMD64_RDI = Register("rdi",  "edi",   "di",  "dil")
AMD64_RSP = Register("rsp",  "esp",   "sp",  "spl")
AMD64_RBP = Register("rbp",  "ebp",   "bp",  "bpl")
AMD64_R8  = Register( "r8",  "r8d",  "r8w",  "r8b")
AMD64_R9  = Register( "r9",  "r9d",  "r9w",  "r9b")
AMD64_R10 = Register("r10", "r10d", "r10w", "r10b")
AMD64_R11 = Register("r11", "r11d", "r11w", "r11b")
AMD64_R12 = Register("r12", "r12d", "r12w", "r12b")
AMD64_R13 = Register("r13", "r13d", "r13w", "r13b")
AMD64_R14 = Register("r14", "r14d", "r14w", "r14b")
AMD64_R15 = Register("r15", "r15d", "r15w", "r15b")

# arm64 registers
ARM64_R0  = Register( "x0",  "w0")
ARM64_R1  = Register( "x1",  "w1")
ARM64_R2  = Register( "x2",  "w2")
ARM64_R3  = Register( "x3",  "w3")
ARM64_R4  = Register( "x4",  "w4")
ARM64_R5  = Register( "x5",  "w5")
ARM64_R6  = Register( "x6",  "w6")
ARM64_R7  = Register( "x7",  "w7")
ARM64_R8  = Register( "x8",  "w8")
ARM64_R9  = Register( "x9",  "w9")
ARM64_R10 = Register("x10", "w10")
ARM64_R11 = Register("x11", "w11")
ARM64_R12 = Register("x12", "w12")
ARM64_R13 = Register("x13", "w13")
ARM64_R14 = Register("x14", "w14")
ARM64_R15 = Register("x15", "w15")
ARM64_R16 = Register("x16", "w16")
ARM64_R17 = Register("x17", "w17")
ARM64_R18 = Register("x18", "w18")
ARM64_R19 = Register("x19", "w19")
ARM64_R20 = Register("x20", "w20")
ARM64_R21 = Register("x21", "w21")
ARM64_R22 = Register("x22", "w22")
ARM64_R23 = Register("x23", "w23")
ARM64_R24 = Register("x24", "w24")
ARM64_R25 = Register("x25", "w25")
ARM64_R26 = Register("x26", "w26")
ARM64_R27 = Register("x27", "w27")
ARM64_R28 = Register("x28", "w28")
ARM64_R29 = Register("x29", "w29") # Frame Pointer
ARM64_R30 = Register("x30", "w30") # Link Register

# arm32 registers
ARM32_R0  = Register(None,  "r0")
ARM32_R1  = Register(None,  "r1")
ARM32_R2  = Register(None,  "r2")
ARM32_R3  = Register(None,  "r3")
ARM32_R4  = Register(None,  "r4")
ARM32_R5  = Register(None,  "r5")
ARM32_R6  = Register(None,  "r6")
ARM32_R7  = Register(None,  "r7")
ARM32_R8  = Register(None,  "r8")
ARM32_R9  = Register(None,  "r9")
ARM32_R10 = Register(None, "r10")
ARM32_R11 = Register(None, "r11")
ARM32_R12 = Register(None, "r12")
ARM32_R13 = Register(None, "r13")
ARM32_R14 = Register(None, "r14")
ARM32_R15 = Register(None, "r15")

# Assembler constants
ASSEMBLER_MASM      = "masm"
ASSEMBLER_ARMASM64  = "armasm64"
ASSEMBLER_GAS       = "gas"
ASSEMBLER_GAS_MACHO = "gas-macho"
ASSEMBLER_GAS_PE    = "gas-pe"

class CallingConvention:
    """A class to represent calling conventions"""

    def __init__(self, name, architecture, mapping, max_arguments, argument_registers, volatile_registers, gen_prologue_fn, gen_epilogue_fn, gen_get_memslot_offset_fn):
        self.name = name
        self.architecture = architecture
        self.mapping = mapping
        self.max_arguments = max_arguments
        self.argument_registers = argument_registers
        self.volatile_registers = volatile_registers
        self.gen_prologue_fn = types.MethodType(gen_prologue_fn, self)
        self.gen_epilogue_fn = types.MethodType(gen_epilogue_fn, self)
        self.gen_get_memslot_offset_fn = types.MethodType(gen_get_memslot_offset_fn, self)


def get_mul_mapping_from_normal_mapping(mapping, argument_registers):
    """Gets the register mapping used in functions requiring special rdx handling.

    In amd64, when using mul and mulx, rdx is a special register.
    rdx is also used for passing arguments in both Msft and System V calling conventions.
    In asm functions that use mul or mulx, we will explicitly move the argument passed in
    rdx to a different volatile register in the function prologue, and in the function body
    we refer to rdx using (Q|D|W|B)H.
    """
    rdx_index = None
    return_mapping = { 'H': AMD64_RDX }
    for (index, register) in mapping.items():
        if register == AMD64_RDX:
            rdx_index = index
            break
    for (index, register) in mapping.items():
        # preserve argument registers
        if (index <= argument_registers) and (index != rdx_index):
            return_mapping[index] = register
        # replace rdx with the first non-argument register
        if index == argument_registers+1:
            return_mapping[rdx_index] = register
        # shuffle all later registers down to fill the gap
        if index > argument_registers+1:
            return_mapping[index-1] = register
    return return_mapping

# Microsoft x64 calling convention
MAPPING_AMD64_MSFT = {
    0: AMD64_RAX, # Result register / volatile
    1: AMD64_RCX, # Argument 1 / volatile
    2: AMD64_RDX, # Argument 2 / volatile
    3: AMD64_R8,  # Argument 3 / volatile
    4: AMD64_R9,  # Argument 4 / volatile
    5: AMD64_R10, # volatile
    6: AMD64_R11, # volatile
    7: AMD64_RSI, # All registers from rsi are non-volatile and need to be saved/restored in epi/prologue
    8: AMD64_RDI,
    9: AMD64_RBP,
    10:AMD64_RBX,
    11:AMD64_R12,
    12:AMD64_R13,
    13:AMD64_R14,
    14:AMD64_R15,
    # currently not mapping rsp
}


def calc_amd64_stack_allocation_sizes(self, reg_count, stack_alloc_size, xmm_reg_count, nested):
    """ Calculate the sizes of different regions on the stack for adjusting the stack pointer
        in the function prologue/epilogue accordingly.

        Given the number of general purpose and Xmm registers used by a function and the amount of
        local buffer it requires, this function calculates and returns as a tuple:
            1 - reg_save_size : space required to save general purpose registers
            2 - xmm_save_size : space required to save Xmm registers if any, including 16B alignment
                                padding if necessary
            3 - stack_alloc_aligned_size : space required to store local variables based on the requested
                                buffer size in stack_alloc_size, rounded to multiple of 8 and 16B aligned
            4 - shadow_space_allocation_size : space required for shadow store/home location if the function
                                is nested, including 16B alignment padding if necessary
    """

    # Keep track of stack alignment during each step
    # We assume rsp % 16 is either 0 or 8 all the time
    # Initially we have rsp % 16 = 8 because return address is pushed on 16B aligned stack
    aligned_on_16B = False

    # Calculate the space needed to save general purpose registers on the stack
    saved_reg_gp = 0 if reg_count <= self.volatile_registers else (reg_count - self.volatile_registers)
    reg_save_size = 8 * saved_reg_gp
    if saved_reg_gp % 2:
        aligned_on_16B = True

    # Calculate the space needed to save Xmm registers on the stack
    saved_reg_xmm = 0 if xmm_reg_count <= 6 else (xmm_reg_count - 6)
    xmm_save_size = 16 * saved_reg_xmm
    if xmm_save_size > 0 and not aligned_on_16B:
        xmm_save_size += 8
        aligned_on_16B = True

    # Calculate space needed for local buffer
    # Round the requested buffer size to multiple of 8 and align it on 16B boundary
    stack_alloc_aligned_size = 0
    if stack_alloc_size > 0:
        stack_alloc_qwords = (stack_alloc_size + 7) // 8
        stack_alloc_adjusted_size = 8 * stack_alloc_qwords
        stack_alloc_aligned_size = stack_alloc_adjusted_size
        if aligned_on_16B ^ ((stack_alloc_aligned_size % 16) == 0):
            stack_alloc_aligned_size += 8
            aligned_on_16B = True

    # If we are a nested function, we need to align the stack to 16B, and allocate space for up to 4
    # memory slots not in the redzone. We can use the same logic as on the MSFT x64 side to allocate
    # our own space for 32B of local variables (whereas on the MSFT side, we use this for allocating
    # space for a function we are about to call)
    shadow_space_allocation_size = 32 + 8 * (not aligned_on_16B) if nested else 0

    return reg_save_size, xmm_save_size, stack_alloc_aligned_size, shadow_space_allocation_size



def gen_prologue_amd64_msft(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, mul_fixup="", nested=False):

    prologue = "\n"

    # Calculate the sizes of the buffers needed for saving registers, local variable buffer and shadow space.
    # Each of the sections other than general purpose registers are aligned on 16B boundary and some of them
    # may include an 8B padding in their size.
    reg_save_size, xmm_save_size, stack_alloc_aligned_size, shadow_space_allocation_size = calc_amd64_stack_allocation_sizes(
        self, reg_count, stack_alloc_size, xmm_reg_count, nested)

    # Save general purpose registers
    if reg_count > self.volatile_registers:
        prologue += ".byte 0x40; push Q%s\n.seh_pushreg Q%s\n" % (self.volatile_registers, self.volatile_registers)
        for i in range(self.volatile_registers+1, reg_count):
            prologue += "push Q%s\n.seh_pushreg Q%s\n" % (i, i)

    # Allocate space on the stack
    stack_total_size = xmm_save_size + stack_alloc_aligned_size + shadow_space_allocation_size
    if stack_total_size > 0:
        prologue += "sub rsp, %d\n.seh_stackalloc %d\n" % (stack_total_size, stack_total_size)

    # Save Xmm registers
    if xmm_save_size > 0:
        for i in range(6, xmm_reg_count):
            prologue += "movdqa xmmword ptr [rsp + %d], xmm%d\n.seh_savexmm xmm%d, %d\n" % (shadow_space_allocation_size + stack_alloc_aligned_size + 16 * (i - 6), i, i, shadow_space_allocation_size + stack_alloc_aligned_size + 16 * (i - 6))

    prologue += "\n.seh_endprologue\n\n"

    prologue += mul_fixup

    # put additional arguments into Q5-Q6 (we do not support more than 6 arguments for now)
    # stack_offset to get the 5th argument is:
    # 32B of shadow space + 8B return address + (8*#pushed registers in prologue) + (16*#saved xmm registers in prologue) + local buffer size + shadow_space_allocation_size
    stack_offset = 32 + 8 + reg_save_size + xmm_save_size + stack_alloc_aligned_size + shadow_space_allocation_size
    for i in range(self.argument_registers+1, arg_count+1):
        prologue += "mov Q%s, [rsp + %d]\n" % (i, stack_offset)
        stack_offset += 8
    return prologue

def gen_prologue_amd64_msft_mul(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_prologue_amd64_msft(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, mul_fixup = "mov Q2, QH\n", nested = False)

def gen_prologue_amd64_msft_nested(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_prologue_amd64_msft(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, mul_fixup = "", nested = True)

def gen_epilogue_amd64_msft(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested = False):

    epilogue = "\n"

    reg_save_size, xmm_save_size, stack_alloc_aligned_size, shadow_space_allocation_size = calc_amd64_stack_allocation_sizes(
        self, reg_count, stack_alloc_size, xmm_reg_count, nested)

    # Restore non-volatile Xmm registers
    if xmm_save_size > 0:
        for i in range(6, xmm_reg_count):
            epilogue += "movdqa xmm%d, xmmword ptr [rsp + %d]\n" % (i, shadow_space_allocation_size + stack_alloc_aligned_size + 16 * (i - 6))

    # Restore stack pointer
    stack_total_size = xmm_save_size + stack_alloc_aligned_size + shadow_space_allocation_size
    if stack_total_size > 0:
        epilogue += "add rsp, %d\n" % stack_total_size

    #epilogue += "BEGIN_EPILOGUE\n"

    if reg_count > self.volatile_registers:
        for i in reversed(range(self.volatile_registers, reg_count)):
            epilogue += "pop Q%s\n" % i

    epilogue += "ret\n"
    return epilogue

def gen_epilogue_amd64_msft_nested(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_epilogue_amd64_msft(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested = True)

def gen_get_memslot_offset_amd64_msft(self, slot, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested = False):
    # only support 4 memory slots for now (in shadow space)
    if(slot >= 4):
        logging.error("symcryptasm currently only support 4 memory slots! (requested slot%d)" % slot)
        exit(1)

    # 8B for return address + (8*#pushed registers in prologue) + (16*#saved XMM registers) + local buffer size + shadow space
    reg_save_size, xmm_save_size, stack_alloc_aligned_size, shadow_space_allocation_size = calc_amd64_stack_allocation_sizes(
        self, reg_count, stack_alloc_size, xmm_reg_count, nested)
    stack_offset = 8 + reg_save_size + xmm_save_size + stack_alloc_aligned_size + shadow_space_allocation_size
    return "%d /*MEMSLOT%d*/" % (stack_offset+(8*slot), slot)

def gen_get_memslot_offset_amd64_msft_nested(self, slot, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_get_memslot_offset_amd64_msft(self, slot, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested = True)

CALLING_CONVENTION_AMD64_MSFT = CallingConvention(
    "msft_x64", "amd64", MAPPING_AMD64_MSFT, 6, 4, 7,
    gen_prologue_amd64_msft, gen_epilogue_amd64_msft, gen_get_memslot_offset_amd64_msft)
CALLING_CONVENTION_AMD64_MSFT_MUL = CallingConvention(
    "msft_x64", "amd64", get_mul_mapping_from_normal_mapping(MAPPING_AMD64_MSFT, 4), 6, 4, 6,
    gen_prologue_amd64_msft_mul, gen_epilogue_amd64_msft, gen_get_memslot_offset_amd64_msft)
CALLING_CONVENTION_AMD64_MSFT_NESTED = CallingConvention(
    "msft_x64", "amd64", MAPPING_AMD64_MSFT, 6, 4, 7,
    gen_prologue_amd64_msft_nested, gen_epilogue_amd64_msft_nested, gen_get_memslot_offset_amd64_msft_nested)

# AMD64 System V calling convention
MAPPING_AMD64_SYSTEMV = {
    0: AMD64_RAX, # Result register / volatile
    1: AMD64_RDI, # Argument 1 / volatile
    2: AMD64_RSI, # Argument 2 / volatile
    3: AMD64_RDX, # Argument 3 / volatile
    4: AMD64_RCX, # Argument 4 / volatile
    5: AMD64_R8,  # Argument 5 / volatile
    6: AMD64_R9,  # Argument 6 / volatile
    7: AMD64_R10, # volatile
    8: AMD64_R11, # volatile
    9: AMD64_RBX, # All registers from rbx are non-volatile and need to be saved/restored in epi/prologue
    10:AMD64_RBP,
    11:AMD64_R12,
    12:AMD64_R13,
    13:AMD64_R14,
    14:AMD64_R15
    # currently not mapping rsp
}

def gen_prologue_amd64_systemv(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, mul_fixup = "", nested = False):

    # Calculate the sizes required for each section
    # We need to call with xmm_reg_count=0 to avoid allocation/alignment for saving Xmm registers since they're
    # volatile for this calling convention.
    reg_save_size, xmm_save_size, stack_alloc_aligned_size, shadow_space_allocation_size = calc_amd64_stack_allocation_sizes(
        self, reg_count, stack_alloc_size, 0, nested)

    # push volatile registers onto the stack
    prologue = "\n"
    if reg_count > self.volatile_registers:
        for i in range(self.volatile_registers, reg_count):
            prologue += "push Q%s\n" % i

    # update stack pointer if local buffer size is nonzero or shadow space exists
    if stack_alloc_aligned_size + shadow_space_allocation_size > 0:
        prologue += "sub rsp, %s // allocate local buffer, memslot space and align stack\n\n" % str(stack_alloc_aligned_size + shadow_space_allocation_size)

    prologue += mul_fixup

    # do not support more than 6 arguments for now
    # # put additional arguments into Q7-Qn
    # # stack_offset to get the 7th argument is:
    # # 8B for return address
    # stack_offset = 8
    # for i in range(self.argument_registers+1, arg_count+1):
    #     prologue += "mov Q%s, [rsp + %d]\n" % (i, stack_offset)
    #     stack_offset += 8

    return prologue

def gen_prologue_amd64_systemv_mul(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_prologue_amd64_systemv(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, mul_fixup = "mov Q3, QH\n", nested = False)

def gen_prologue_amd64_systemv_nested(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_prologue_amd64_systemv(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, mul_fixup = "", nested = True)

def gen_epilogue_amd64_systemv(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested = False):

    epilogue = ""

    # Calculate the sizes required for each section
    # We need to call with xmm_reg_count=0 to avoid allocation/alignment for saving Xmm registers since they're
    # volatile for this calling convention.
    reg_save_size, xmm_save_size, stack_alloc_aligned_size, shadow_space_allocation_size = calc_amd64_stack_allocation_sizes(
        self, reg_count, stack_alloc_size, 0, nested)

    # update stack pointer if local buffer size is nonzero or shadow space exists
    if stack_alloc_aligned_size + shadow_space_allocation_size > 0:
        epilogue += "add rsp, %s // deallocate local buffer, memslot space and align stack\n\n" % str(stack_alloc_aligned_size + shadow_space_allocation_size)

    if reg_count > self.volatile_registers:
        for i in reversed(range(self.volatile_registers, reg_count)):
            epilogue += "pop Q%s\n" % i
    epilogue += "ret\n"
    return epilogue

def gen_epilogue_amd64_systemv_nested(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_epilogue_amd64_systemv(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested=True)

def gen_get_memslot_offset_amd64_systemv(self, slot, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested = False):
    # only support 4 memory slots for now
    if(slot >= 4):
        logging.error("symcryptasm currently only support 4 memory slots! (requested slot%d)" % slot)
        exit(1)
    # For leaf functions, use the top of the redzone below the stack pointer
    offset = -8 * (slot+1)
    if nested:
        # For nested functions, use the 32B of memslot space above the stack pointer created in the prologue
        offset = 8*slot
    return "%d /*MEMSLOT%d*/" % (offset, slot)

def gen_get_memslot_offset_amd64_systemv_nested(self, slot, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    return gen_get_memslot_offset_amd64_systemv(self, slot, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested=True)

CALLING_CONVENTION_AMD64_SYSTEMV = CallingConvention(
    "amd64_systemv", "amd64", MAPPING_AMD64_SYSTEMV, 6, 6, 9,
    gen_prologue_amd64_systemv, gen_epilogue_amd64_systemv, gen_get_memslot_offset_amd64_systemv)
CALLING_CONVENTION_AMD64_SYSTEMV_MUL = CallingConvention(
    "amd64_systemv", "amd64", get_mul_mapping_from_normal_mapping(MAPPING_AMD64_SYSTEMV, 6), 6, 6, 8,
    gen_prologue_amd64_systemv_mul, gen_epilogue_amd64_systemv, gen_get_memslot_offset_amd64_systemv)
CALLING_CONVENTION_AMD64_SYSTEMV_NESTED = CallingConvention(
    "amd64_systemv", "amd64", MAPPING_AMD64_SYSTEMV, 6, 6, 9,
    gen_prologue_amd64_systemv_nested, gen_epilogue_amd64_systemv_nested, gen_get_memslot_offset_amd64_systemv_nested)


# ARM64 calling conventions
MAPPING_ARM64_AAPCS64 = {
    0: ARM64_R0,  # Argument 1 / Result register / volatile
    1: ARM64_R1,  # Argument 2 / volatile
    2: ARM64_R2,  # Argument 3 / volatile
    3: ARM64_R3,  # Argument 4 / volatile
    4: ARM64_R4,  # Argument 5 / volatile
    5: ARM64_R5,  # Argument 6 / volatile
    6: ARM64_R6,  # Argument 7 / volatile
    7: ARM64_R7,  # Argument 8 / volatile
    8: ARM64_R8,  # Indirect result location / volatile
    9: ARM64_R9,  # volatile
    10:ARM64_R10, # volatile
    11:ARM64_R11, # volatile
    12:ARM64_R12, # volatile
    13:ARM64_R13, # volatile
    14:ARM64_R14, # volatile
    15:ARM64_R15, # volatile
    # R16 and R17 are intra-procedure-call temporary registers which may be used by the linker
    # We cannot use these registers for local scratch if we call out to arbitrary procedures, but
    # currently we only have leaf functions in Arm64 symcryptasm.
    16:ARM64_R16, # IP0 / volatile
    17:ARM64_R17, # IP1 / volatile
    # R18 is a platform register which has a special meaning in kernel mode - we do not use it
    18:ARM64_R19, # non-volatile
    19:ARM64_R20, # non-volatile
    20:ARM64_R21, # non-volatile
    21:ARM64_R22, # non-volatile
    22:ARM64_R23, # non-volatile
    # We could map more registers (R24-R28) but we can only support 23 registers for ARM64EC, and we
    # don't use this many registers in any symcryptasm yet
}

MAPPING_ARM64_ARM64ECMSFT = {
    0: ARM64_R0,  # Argument 1 / Result register / volatile
    1: ARM64_R1,  # Argument 2 / volatile
    2: ARM64_R2,  # Argument 3 / volatile
    3: ARM64_R3,  # Argument 4 / volatile
    4: ARM64_R4,  # Argument 5 / volatile
    5: ARM64_R5,  # Argument 6 / volatile
    6: ARM64_R6,  # Argument 7 / volatile
    7: ARM64_R7,  # Argument 8 / volatile
    8: ARM64_R8,  # Indirect result location / volatile
    9: ARM64_R9,  # volatile
    10:ARM64_R10, # volatile
    11:ARM64_R11, # volatile
    12:ARM64_R12, # volatile
    # R13 and R14 are reserved in ARM64EC
    13:ARM64_R15, # volatile
    14:ARM64_R16, # volatile
    15:ARM64_R17, # volatile
    16:ARM64_R19, # non-volatile
    17:ARM64_R20, # non-volatile
    18:ARM64_R21, # non-volatile
    19:ARM64_R22, # non-volatile
    # R23 and R24 are reserved in ARM64EC
    20:ARM64_R25, # non-volatile
    21:ARM64_R26, # non-volatile
    22:ARM64_R27, # non-volatile
    # R28 is reserved in ARM64EC
}

# ARM32 calling convention
# A subroutine must preserve the contents of the registers r4-r8, r10, r11 and SP (and r9 in PCS variants that designate r9 as v6).
MAPPING_ARM32_AAPCS32 = {
    0: ARM32_R0,  # Argument 1 / Result register / volatile
    1: ARM32_R1,  # Argument 2 / Result register / volatile
    2: ARM32_R2,  # Argument 3 / volatile
    3: ARM32_R3,  # Argument 4 / volatile
    4: ARM32_R4,  # non-volatile
    5: ARM32_R5,  # non-volatile
    6: ARM32_R6,  # non-volatile
    7: ARM32_R7,  # non-volatile
    8: ARM32_R8,  # non-volatile
    9: ARM32_R9,  # reserved for something
    10:ARM32_R10, # non-volatile
    11:ARM32_R11, # FP non-volatile
    12:ARM32_R12, # volatile for leaf functions
    13:ARM32_R13, # SP
    14:ARM32_R14, # LR
    15:ARM32_R15, # PC
}

def gen_prologue_aapcs32(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    assert(not stack_alloc_size and not xmm_reg_count)
    prologue = ""
    # Always spill at least 1 register (LR).
    # LR needs to be saved for nested functions but for now we'll store it always
    # since we don't differentiate between nested and leaf functions for arm yet.
    registers_to_spill = []
    logging.info(f"prologue {reg_count} > {self.volatile_registers}")
    if reg_count > self.volatile_registers:
        for i in range(self.volatile_registers, reg_count):
            registers_to_spill.append('r%s' % i)
        # Stack pointer is word 4B aligned
        # required_stack_space = 4 * len(registers_to_spill)
    registers_to_spill.append('lr')
    prologue += "push {" + ",".join(registers_to_spill) + "}\n"
    if reg_count > 7:
        prologue += ".seh_save_regs_w {" + ",".join(registers_to_spill) + "}\n"
    else:
        prologue += ".seh_save_regs {" + ",".join(registers_to_spill) + "}\n"
    prologue += ".seh_endprologue\n"
    return prologue

def gen_epilogue_aapcs32(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    assert(not stack_alloc_size and not xmm_reg_count)
    epilogue = ""

    registers_to_spill = []
    logging.info(f"epilogue {reg_count} > {self.volatile_registers}")
    if reg_count > self.volatile_registers:
        for i in range(self.volatile_registers, reg_count):
            registers_to_spill.append('r%s' % i)
        # Stack pointer is word 4B aligned
        # required_stack_space = 4 * len(registers_to_spill)
    registers_to_spill.append('pc')
    epilogue += "pop {" + ",".join(registers_to_spill) + "}\n"
    return epilogue

def gen_prologue_arm64_armasm64(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    prologue = ""

    if reg_count > self.volatile_registers:
        # Calculate required stack space
        # If we allocate stack space we must spill fp and lr, so we always spill at least 2 registers
        registers_to_spill = 2 + reg_count - self.volatile_registers
        # Stack pointer remain 16B aligned, so round up to the nearest multiple of 16B
        required_stack_space = 16 * ((registers_to_spill + 1) // 2)
        prologue += "    PROLOG_SAVE_REG_PAIR fp, lr, #-%d! // allocate %d bytes of stack; store FP/LR\n" % (required_stack_space, required_stack_space)

        stack_offset = 16
        for i in range(self.volatile_registers, reg_count-1, 2):
            prologue += "    PROLOG_SAVE_REG_PAIR X_%d, X_%d, #%d\n" % (i, i+1, stack_offset)
            stack_offset += 16
        if registers_to_spill % 2 == 1:
            prologue += "    PROLOG_SAVE_REG      X_%d, #%d\n" % (reg_count-1, stack_offset)

    return prologue

def gen_epilogue_arm64_armasm64(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    epilogue = ""

    if reg_count > self.volatile_registers:
        # Calculate required stack space
        # If we allocate stack space we must spill fp and lr, so we always spill at least 2 registers
        registers_to_spill = 2 + reg_count - self.volatile_registers
        # Stack pointer remain 16B aligned, so round up to the nearest multiple of 16B
        required_stack_space = 16 * ((registers_to_spill + 1) // 2)

        stack_offset = required_stack_space-16
        if registers_to_spill % 2 == 1:
            epilogue += "    EPILOG_RESTORE_REG      X_%d, #%d\n" % (reg_count-1, stack_offset)
            stack_offset -= 16
        for i in reversed(range(self.volatile_registers, reg_count-1, 2)):
            epilogue += "    EPILOG_RESTORE_REG_PAIR X_%d, X_%d, #%d\n" % (i, i+1, stack_offset)
            stack_offset -= 16
        epilogue += "    EPILOG_RESTORE_REG_PAIR fp, lr, #%d! // deallocate %d bytes of stack; restore FP/LR\n" % (required_stack_space, required_stack_space)
        epilogue += "    EPILOG_RETURN\n"
    else:
        epilogue += "    ret\n"

    return epilogue

def gen_prologue_arm64_gas(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    prologue = ""

    if reg_count > self.volatile_registers:
        # Calculate required stack space
        # If we allocate stack space we must spill fp and lr, so we always spill at least 2 registers
        registers_to_spill = 2 + reg_count - self.volatile_registers
        # Stack pointer remain 16B aligned, so round up to the nearest multiple of 16B
        required_stack_space = 16 * ((registers_to_spill + 1) // 2)
        prologue += "    stp fp, lr, [sp, #-%d]! // allocate %d bytes of stack; store FP/LR\n" % (required_stack_space, required_stack_space)

        stack_offset = 16
        for i in range(self.volatile_registers, reg_count-1, 2):
            prologue += "    stp X_%d, X_%d, [sp, #%d]\n" % (i, i+1, stack_offset)
            stack_offset += 16
        if registers_to_spill % 2 == 1:
            prologue += "    str      X_%d, [sp, #%d]\n" % (reg_count-1, stack_offset)

    return prologue

def gen_prologue_arm64_gas_pe(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    prologue = ""

    if reg_count > self.volatile_registers:
        # Calculate required stack space
        # If we allocate stack space we must spill fp and lr, so we always spill at least 2 registers
        registers_to_spill = 2 + reg_count - self.volatile_registers
        # Stack pointer remain 16B aligned, so round up to the nearest multiple of 16B
        required_stack_space = 16 * ((registers_to_spill + 1) // 2)
        prologue += "    stp fp, lr, [sp, #-%d]! // allocate %d bytes of stack; store FP/LR\n" % (required_stack_space, required_stack_space)
        prologue += "    .seh_save_fplr_x %d\n" % required_stack_space

        stack_offset = 16
        for i in range(self.volatile_registers, reg_count-1, 2):
            prologue += "    stp X_%d, X_%d, [sp, #%d]\n" % (i, i+1, stack_offset)
            prologue += "    .seh_save_regp X_%d, %d\n" % (i, stack_offset)
            stack_offset += 16
        if registers_to_spill % 2 == 1:
            prologue += "    str      X_%d, [sp, #%d]\n" % (reg_count-1, stack_offset)
            prologue += "    .seh_save_reg X_%d, %d\n" % (reg_count-1, stack_offset)

    prologue += "    .seh_endprologue\n"
    return prologue

def gen_epilogue_arm64_gas(self, arg_count, reg_count, stack_alloc_size, xmm_reg_count):
    epilogue = ""

    if reg_count > self.volatile_registers:
        # Calculate required stack space
        # If we allocate stack space we must spill fp and lr, so we always spill at least 2 registers
        registers_to_spill = 2 + reg_count - self.volatile_registers
        # Stack pointer remain 16B aligned, so round up to the nearest multiple of 16B
        required_stack_space = 16 * ((registers_to_spill + 1) // 2)

        stack_offset = required_stack_space-16
        if registers_to_spill % 2 == 1:
            epilogue += "    ldr      X_%d, [sp, #%d]\n" % (reg_count-1, stack_offset)
            stack_offset -= 16
        for i in reversed(range(self.volatile_registers, reg_count-1, 2)):
            epilogue += "    ldp X_%d, X_%d, [sp, #%d]\n" % (i, i+1, stack_offset)
            stack_offset -= 16
        epilogue += "    ldp fp, lr, [sp], #%d // deallocate %d bytes of stack; restore FP/LR\n" % (required_stack_space, required_stack_space)
    epilogue += "    ret\n"

    return epilogue

def gen_get_memslot_offset_arm64(self, slot, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested=False):
    logging.error("symcryptasm currently does not support memory slots for arm64!")
    exit(1)

CALLING_CONVENTION_ARM64_AAPCS64_ARMASM64 = CallingConvention(
    "arm64_aapcs64", "arm64", MAPPING_ARM64_AAPCS64, 8, 8, 18,
    gen_prologue_arm64_armasm64, gen_epilogue_arm64_armasm64, gen_get_memslot_offset_arm64)

CALLING_CONVENTION_ARM64_AAPCS64_GAS = CallingConvention(
    "arm64_aapcs64", "arm64", MAPPING_ARM64_AAPCS64, 8, 8, 18,
    gen_prologue_arm64_gas, gen_epilogue_arm64_gas, gen_get_memslot_offset_arm64)

CALLING_CONVENTION_ARM64_AAPCS64_GAS_PE = CallingConvention(
    "arm64_aapcs64", "arm64", MAPPING_ARM64_AAPCS64, 8, 8, 18,
    gen_prologue_arm64_gas_pe, gen_epilogue_arm64_gas, gen_get_memslot_offset_arm64)

CALLING_CONVENTION_ARM64EC_GAS_PE = CallingConvention(
    "arm64ec_msft", "arm64", MAPPING_ARM64_ARM64ECMSFT, 8, 8, 16,
    gen_prologue_arm64_gas_pe, gen_epilogue_arm64_gas, gen_get_memslot_offset_arm64)

CALLING_CONVENTION_ARM64EC_MSFT = CallingConvention(
    "arm64ec_msft", "arm64", MAPPING_ARM64_ARM64ECMSFT, 8, 8, 16,
    gen_prologue_arm64_armasm64, gen_epilogue_arm64_armasm64, gen_get_memslot_offset_arm64)

CALLING_CONVENTION_ARM32_AAPCS32 = CallingConvention(
    "arm32_aapcs32", "arm32", MAPPING_ARM32_AAPCS32, 4, 4, 4,
    gen_prologue_aapcs32, gen_epilogue_aapcs32, gen_get_memslot_offset_arm64)

def gen_function_defines(architecture, mapping, arg_count, reg_count, start=True):
    defines = ""
    if architecture == "amd64":
        prefix64 = "Q"
        prefix32 = "D"
        prefix16 = "W"
        prefix8  = "B"
    elif architecture == "arm64":
        prefix64 = "X_"
        prefix32 = "W_"
    elif architecture == "arm32":
        return defines
    else:
        logging.error("Unhandled architecture (%s) in gen_function_defines" % architecture)
        exit(1)

    for (index, reg) in mapping.items():
        if (index != 'H') and (index >= max(arg_count+1, reg_count)):
            continue
        if start:
            if (reg.name64 is not None):
                defines += "#define %s%s %s\n" % (prefix64, index, reg.name64)
            if (reg.name32 is not None):
                defines += "#define %s%s %s\n" % (prefix32, index, reg.name32)
            if (reg.name16 is not None):
                defines += "#define %s%s %s\n" % (prefix16, index, reg.name16)
            if (reg.name8 is not None):
                defines += "#define %s%s %s\n" % (prefix8,  index, reg.name8)
        else:
            if (reg.name64 is not None):
                defines += "#undef %s%s\n" % (prefix64, index)
            if (reg.name32 is not None):
                defines += "#undef %s%s\n" % (prefix32, index)
            if (reg.name16 is not None):
                defines += "#undef %s%s\n" % (prefix16, index)
            if (reg.name8 is not None):
                defines += "#undef %s%s\n" % (prefix8,  index)
    return defines

def gen_function_start_defines(architecture, mapping, arg_count, reg_count):
    return gen_function_defines(architecture, mapping, arg_count, reg_count, start=True)

def gen_function_end_defines(architecture, mapping, arg_count, reg_count):
    return gen_function_defines(architecture, mapping, arg_count, reg_count, start=False)

def replace_identifier(arg, newarg, line):
    """Replaces all instances of identifier arg with newarg in string line."""
    argmatch = r"(^|[^a-zA-Z0-9_])" + arg + r"(?![a-zA-Z0-9_])"
    line = re.sub(argmatch, r"\1" + newarg, line)
    return line

MASM_FRAMELESS_FUNCTION_ENTRY   = "LEAF_ENTRY %s"
MASM_FRAMELESS_FUNCTION_END     = "LEAF_END %s"
MASM_FRAME_FUNCTION_ENTRY       = "NESTED_ENTRY %s"
MASM_FRAME_FUNCTION_END         = "NESTED_END %s"

# MASM function macros takes the text area as an argument
MASM_FUNCTION_TEMPLATE      = "%s, _TEXT\n"
# ARMASM64 function macros must be correctly indented
ARMASM64_FUNCTION_TEMPLATE  = "    %s\n"

GAS_FUNCTION_ENTRY_ELF    = "%s: .global %s\n.type %s, %%function\n// .func %s\n"
GAS_FUNCTION_ENTRY_MACHO  = "%s: .global %s\n// .func %s\n"
GAS_FUNCTION_END      = "// .endfunc // %s"
GAS_FUNCTION_ENTRY_PE  = "%s:\n.globl %s\n.def %s; .scl 2; .type 32; .endef\n.seh_proc %s\n"
GAS_FUNCTION_END_PE    = ".seh_endproc\n"

def generate_prologue(assembler, calling_convention, function_name, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested):
    function_entry = None
    if assembler in [ASSEMBLER_MASM, ASSEMBLER_ARMASM64]:
        # need to identify and mark up frame functions in masm and armasm64
        # for masm we also consider Xmm register and local buffer use (stack_alloc_size)
        if assembler == ASSEMBLER_MASM and (nested or (reg_count > calling_convention.volatile_registers) or (xmm_reg_count > 6) or (stack_alloc_size > 0)):
            function_entry = MASM_FRAME_FUNCTION_ENTRY % (function_name)
        elif nested or (reg_count > calling_convention.volatile_registers):
            function_entry = MASM_FRAME_FUNCTION_ENTRY % (function_name)
        else:
            function_entry = MASM_FRAMELESS_FUNCTION_ENTRY % (function_name)

        if assembler == ASSEMBLER_MASM:
            function_entry = MASM_FUNCTION_TEMPLATE % function_entry
        elif assembler == ASSEMBLER_ARMASM64:
            function_entry = ARMASM64_FUNCTION_TEMPLATE % function_entry
    elif assembler == ASSEMBLER_GAS_PE:
        function_entry = GAS_FUNCTION_ENTRY_PE % (function_name, function_name, function_name, function_name)
    elif assembler == ASSEMBLER_GAS_MACHO:
        # macOS uses Mach-O format which doesn't support .type directive
        function_entry = GAS_FUNCTION_ENTRY_MACHO % (function_name, function_name, function_name)
    elif assembler == ASSEMBLER_GAS:
        function_entry = GAS_FUNCTION_ENTRY_ELF % (function_name, function_name, function_name, function_name)
    else:
        logging.error("Unhandled assembler (%s) in generate_prologue" % assembler)
        exit(1)

    prologue = gen_function_start_defines(calling_convention.architecture, calling_convention.mapping, arg_count, reg_count)
    prologue += "%s" % (function_entry)
    prologue += calling_convention.gen_prologue_fn(arg_count, reg_count, stack_alloc_size, xmm_reg_count)

    return prologue

def generate_epilogue(assembler, calling_convention, function_name, arg_count, reg_count, stack_alloc_size, xmm_reg_count, nested):
    function_end = None
    if assembler in [ASSEMBLER_MASM, ASSEMBLER_ARMASM64]:
        # need to identify and mark up frame functions in masm
        # for masm we also consider Xmm register and local buffer use (stack_alloc_size)
        if assembler == ASSEMBLER_MASM and (nested or (reg_count > calling_convention.volatile_registers) or (xmm_reg_count > 6) or (stack_alloc_size > 0)):
            function_end = MASM_FRAME_FUNCTION_END % (function_name)
        elif nested or (reg_count > calling_convention.volatile_registers):
            function_end = MASM_FRAME_FUNCTION_END % (function_name)
        else:
            function_end = MASM_FRAMELESS_FUNCTION_END % (function_name)

        if assembler == ASSEMBLER_MASM:
            function_end = MASM_FUNCTION_TEMPLATE % function_end
        elif assembler == ASSEMBLER_ARMASM64:
            function_end = ARMASM64_FUNCTION_TEMPLATE % function_end
    elif assembler == ASSEMBLER_GAS_PE:
            function_end = GAS_FUNCTION_END_PE
    elif assembler in [ASSEMBLER_GAS, ASSEMBLER_GAS_MACHO]:
            function_end = GAS_FUNCTION_END % function_name
    else:
        logging.error("Unhandled assembler (%s) in generate_epilogue" % assembler)
        exit(1)

    epilogue = calling_convention.gen_epilogue_fn(arg_count, reg_count, stack_alloc_size, xmm_reg_count)
    epilogue += "%s" % (function_end)
    epilogue += gen_function_end_defines(calling_convention.architecture, calling_convention.mapping, arg_count, reg_count)

    return epilogue

MASM_MACRO_START    = "%s MACRO %s\n"
MASM_MACRO_END      = "ENDM\n"
ARMASM64_MACRO_START= "    MACRO\n    %s %s"
ARMASM64_MACRO_END  = "    MEND\n"
GAS_MACRO_START     = ".macro %s %s\n"
GAS_MACRO_END       = ".endm\n"
MASM_ALTERNATE_ENTRY= "ALTERNATE_ENTRY %s\n"
GAS_ALTERNATE_ENTRY = "%s: .global %s\n"
ARMASM64_ALTERNATE_ENTRY= "    ALTERNATE_ENTRY %s\n"

FUNCTION_START_PATTERN  = re.compile(r"\s*(NESTED_)?(MUL_)?FUNCTION_START\s*\(\s*([a-zA-Z0-9_\(\)]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*(,\s*[0-9\*\+\-]+)?\s*(,\s*[0-9]+)?\s*\)")
FUNCTION_END_PATTERN    = re.compile(r"\s*(NESTED_)?(MUL_)?FUNCTION_END\s*\(\s*([a-zA-Z0-9_\(\)]+)\s*\)")
GET_MEMSLOT_PATTERN     = re.compile(r"GET_MEMSLOT_OFFSET\s*\(\s*slot([0-9]+)\s*\)")
ALTERNATE_ENTRY_PATTERN = re.compile(r"\s*ALTERNATE_ENTRY\s*\(\s*([a-zA-Z0-9_\(\)]+)\s*\)")
MACRO_START_PATTERN     = re.compile(r"\s*MACRO_START\s*\(\s*([A-Z_0-9]+)\s*,([^\)]+)\)")
MACRO_END_PATTERN       = re.compile(r"\s*MACRO_END\s*\(\s*\)")
INCLUDE_PATTERN         = re.compile(r"\s*INCLUDE\s*\(\s*([^\s]+)\s*\)")

class ProcessingStateMachine:
    """A class to hold the state when processing a file and handle files line by line"""

    def __init__(self, assembler, normal_calling_convention, mul_calling_convention, nested_calling_convention):
        self.assembler = assembler
        self.normal_calling_convention = normal_calling_convention
        self.mul_calling_convention = mul_calling_convention
        self.nested_calling_convention = nested_calling_convention

        self.function_start_match = None
        self.function_start_line = 0
        self.is_nested_function = None
        self.is_mul_function = None
        self.calling_convention = None
        self.function_name = None
        self.arg_count = None
        self.reg_count = None
        self.stack_alloc_size = None
        self.xmm_reg_count = None

        self.macro_start_match = None
        self.macro_name = None
        self.macro_args = None

    def process_line(self, line, line_num):
        if self.function_start_match == None and self.macro_start_match == None:
            return self.process_normal_line(line, line_num)
        elif self.function_start_match != None:
            return self.process_function_line(line, line_num)
        elif self.macro_start_match != None:
            return self.process_macro_line(line, line_num)
        else:
            logging.error("Whoops, something is broken with the state machine (failed at line %d)" % line_num)
            exit(1)

    def process_normal_line(self, line, line_num):
        # Not currently in a function or macro
        match = FUNCTION_START_PATTERN.match(line)
        if (match):
            return self.process_start_function(match, line, line_num)

        match = MACRO_START_PATTERN.match(line)
        if (match):
            return self.process_start_macro(match, line, line_num)

        # Not starting a function or a macro
        return line

    def process_start_function(self, match, line, line_num):
        # Entering a new function
        self.function_start_match = match
        self.function_start_line = line_num
        self.is_nested_function = (match.group(1) == "NESTED_")
        self.is_mul_function = (match.group(2) == "MUL_")
        self.function_name = match.group(3)
        self.arg_count = int(match.group(4))
        self.reg_count = int(match.group(5))
        # last two parameters are optional and their corresponding capturing groups will be None if not supplied
        self.stack_alloc_size = 0 if not match.group(6) else eval(match.group(6).strip(", \'\""))
        self.xmm_reg_count = 0 if not match.group(7) else int(match.group(7).strip(", "))

        if self.is_nested_function and self.nested_calling_convention is None:
            logging.error(
                "symcryptasm nested functions are not currently supported with assembler (%s) and architecture (%s)!\n\t"
                "%s (line %d)"
                % (self.assembler, self.normal_calling_convention.architecture, line, line_num))
            exit(1)
        if self.is_mul_function and self.mul_calling_convention is None:
            logging.error(
                "symcryptasm mul functions are not supported with assembler (%s) and architecture (%s)!\n\t"
                "%s (line %d)"
                % (self.assembler, self.normal_calling_convention.architecture, line, line_num))
            exit(1)
        if self.is_nested_function and self.is_mul_function:
            logging.error(
                "Too many prefixes for symcryptasm function - currently only 1 of prefix, MUL_ or NESTED_, is supported!\n\t"
                "%s (line %d)"
                % (line, line_num))
            exit(1)
        if self.arg_count > self.normal_calling_convention.max_arguments:
            logging.error(
                "Too many (%d) arguments for symcryptasm function - only %d arguments are supported by calling convention (%s)\n\t"
                "%s (line %d)"
                % (self.arg_count, self.normal_calling_convention.max_arguments, self.normal_calling_convention.name, match.group(0), line_num))
            exit(1)
        if self.reg_count > len(self.normal_calling_convention.mapping):
            logging.error(
                "Too many (%d) registers required for symcryptasm function - only %d registers are mapped by calling convention (%s)\n\t"
                "%s (line %d)"
                % (self.reg_count, len(self.normal_calling_convention.mapping), self.normal_calling_convention.name, match.group(0), line_num))
            exit(1)
        if self.is_mul_function and self.reg_count > len(self.mul_calling_convention.mapping)-1:
            logging.error(
                "Too many (%d) registers required for symcryptasm mul function - only %d registers are mapped by calling convention (%s)\n\t"
                "%s (line %d)"
                % (self.reg_count, len(self.mul_calling_convention.mapping)-1, self.mul_calling_convention.name, match.group(0), line_num))
            exit(1)
        if not 0 <= self.xmm_reg_count <= 16:
            logging.error(
                "Invalid number of used XMM registers (%d) specified for calling convention (%s) - must be in range [0,16]\n\t"
                "%s (line %d)"
                % (self.xmm_reg_count, self.mul_calling_convention.name, match.group(6), line_num))
            exit(1)


        logging.info("%d: function start %s, %d, %d" % (line_num, self.function_name, self.arg_count, self.reg_count))

        if self.is_nested_function:
            self.calling_convention = self.nested_calling_convention
        elif self.is_mul_function:
            self.calling_convention = self.mul_calling_convention
        else:
            self.calling_convention = self.normal_calling_convention

        return generate_prologue(self.assembler,
                                 self.calling_convention,
                                 self.function_name,
                                 self.arg_count,
                                 self.reg_count,
                                 self.stack_alloc_size,
                                 self.xmm_reg_count,
                                 self.is_nested_function
                                 )

    def process_start_macro(self, match, line, line_num):
        self.macro_start_match = match
        self.macro_name = match.group(1)
        self.macro_args = [ x.strip() for x in match.group(2).split(",") ]

        logging.info("%d: macro start %s, %s" % (line_num, self.macro_name, self.macro_args))

        if self.assembler == ASSEMBLER_MASM:
            return MASM_MACRO_START % (self.macro_name, match.group(2))
        elif self.assembler in [ASSEMBLER_GAS, ASSEMBLER_GAS_MACHO, ASSEMBLER_GAS_PE]:
            return GAS_MACRO_START % (self.macro_name, match.group(2))
        elif self.assembler == ASSEMBLER_ARMASM64:
            # In armasm64 we need to escape all macro arguments with $
            prefixed_args = ", $".join(self.macro_args)
            if prefixed_args:
                prefixed_args = "$" + prefixed_args
            return ARMASM64_MACRO_START % (self.macro_name, prefixed_args)
        else:
            logging.error("Unhandled assembler (%s) in process_start_macro" % self.assembler)
            exit(1)

    def process_function_line(self, line, line_num):
        # Currently in a function
        match = ALTERNATE_ENTRY_PATTERN.match(line)
        if (match):
            if self.assembler == ASSEMBLER_MASM:
                return MASM_ALTERNATE_ENTRY % match.group(1)
            elif self.assembler in [ASSEMBLER_GAS, ASSEMBLER_GAS_MACHO, ASSEMBLER_GAS_PE]:
                    return GAS_ALTERNATE_ENTRY % (match.group(1), match.group(1))
            elif self.assembler == ASSEMBLER_ARMASM64:
                return ARMASM64_ALTERNATE_ENTRY % match.group(1)
            else:
                logging.error("Unhandled assembler (%s) in process_function_line" % self.assembler)
                exit(1)

        match = FUNCTION_END_PATTERN.match(line)
        if (match):
            # Check the end function has same prefix as previous start function
            if  (self.is_nested_function ^ (match.group(1) == "NESTED_")) or \
                (self.is_mul_function ^ (match.group(2) == "MUL_")):
                logging.error("Function start and end do not have same MUL_ or NESTED_ prefix!\n\tStart: %s (line %d)\n\tEnd:   %s (line %d)" \
                    % (self.function_start_match.group(0), self.function_start_line, match.group(0), line_num))
                exit(1)
            # Check the end function pattern has the same label as the previous start function pattern
            if self.function_name != match.groups()[-1]:
                logging.error("Function start label does not match Function end label!\n\tStart: %s (line %d)\n\tEnd:   %s (line %d)" \
                    % (self.function_name, self.function_start_line, match.groups()[-1], line_num))
                exit(1)

            epilogue = generate_epilogue(self.assembler, self.calling_convention, self.function_name, self.arg_count, self.reg_count, self.stack_alloc_size, self.xmm_reg_count, self.is_nested_function)

            logging.info("%d: function end %s" % (line_num, self.function_name))

            self.function_start_match = None
            self.function_start_line = 0
            self.is_nested_function = None
            self.is_mul_function = None
            self.calling_convention = None
            self.function_name = None
            self.arg_count = None
            self.reg_count = None
            self.stack_alloc_size = None
            self.xmm_reg_count = None

            return epilogue

        # replace any GET_MEMSLOT_OFFSET macros in line
        match = GET_MEMSLOT_PATTERN.search(line)
        while(match):
            slot = int(match.group(1))
            replacement = self.calling_convention.gen_get_memslot_offset_fn(slot, self.arg_count, self.reg_count, self.stack_alloc_size, self.xmm_reg_count)
            line = GET_MEMSLOT_PATTERN.sub(replacement, line)
            match = GET_MEMSLOT_PATTERN.search(line)

            logging.info("%d: memslot macro %d" % (line_num, slot))

        # Not modifying the line any further
        return line

    def process_macro_line(self, line, line_num):
        # Currently in a macro
        match = MACRO_END_PATTERN.match(line)
        if (match):
            logging.info("%d: macro end %s" % (line_num, self.macro_name))

            self.macro_start_match = None
            self.macro_name = None
            self.macro_args = None

            if self.assembler == ASSEMBLER_MASM:
                return MASM_MACRO_END
            elif self.assembler in [ASSEMBLER_GAS, ASSEMBLER_GAS_MACHO, ASSEMBLER_GAS_PE]:
                return GAS_MACRO_END
            elif self.assembler == ASSEMBLER_ARMASM64:
                return ARMASM64_MACRO_END
            else:
                logging.error("Unhandled assembler (%s) in process_macro_line" % self.assembler)
                exit(1)

        arg_prefix = ""
        if self.assembler == ASSEMBLER_ARMASM64:
            # In armasm64 macros we need to escape all of the macro arguments with a $ in the macro body
            arg_prefix = "$"
        elif self.assembler in [ASSEMBLER_GAS, ASSEMBLER_GAS_MACHO, ASSEMBLER_GAS_PE]:
            # In GAS macros we need to escape all of the macro arguments with a backslash in the macro body
            arg_prefix = r"\\"

        if arg_prefix:
            for arg in self.macro_args:
                line = replace_identifier(arg, arg_prefix + arg, line)

        # Not modifying the line any further
        return line


def expand_files(filename, line_num_parent, line_parent):
    ''' Read the contents of filename and insert include files where there's an INCLUDE directive'''
    if line_parent:
        logging.info(
        "expand file %s\n\t"
        "%s (line %d)"
        % (filename, line_parent, line_num_parent))
    base_dir = os.path.dirname(filename)
    expanded_lines = []
    has_includes = False
    try:
        infile = open(filename)
        for line_num, line in enumerate(infile.readlines()):
            match = INCLUDE_PATTERN.match(line)
            if (match):
                include_file = match.group(1)
                full_path = base_dir + "/" + include_file
                logging.info("%d: including file %s" % (line_num + 1, include_file))
                logging.info("%d: full path %s" % (line_num + 1, full_path))
                include_lines, _ = expand_files(full_path, line_num + 1, line)
                expanded_lines.extend(include_lines)
                has_includes = True
            else:
                expanded_lines.append(line)
        infile.close()
    except IOError:
        logging.error(
            "cannot open include file %s\n\t"
            "%s (line %d)"
            % (filename, line_parent, line_num_parent))
        exit(1)

    return expanded_lines, has_includes

def gen_file_header(assembler, architecture, calling_convention):
    """ Generate header to be inserted at the beginning of each symcryptasm file"""
    header = ""

    if assembler == ASSEMBLER_MASM:
        header += "// begin masm header\n"
        header += "option casemap:none\n"
        header += "// end masm header\n\n"

    return header

def process_file(assembler, architecture, calling_convention, infilename, outfilename):
    normal_calling_convention = None

    if assembler in [ASSEMBLER_MASM, ASSEMBLER_GAS_PE]:
        if architecture == "amd64" and calling_convention == "msft":
            normal_calling_convention = CALLING_CONVENTION_AMD64_MSFT
            mul_calling_convention = CALLING_CONVENTION_AMD64_MSFT_MUL
            nested_calling_convention = CALLING_CONVENTION_AMD64_MSFT_NESTED
        elif architecture == "arm64" and calling_convention == "aapcs64":
            normal_calling_convention = CALLING_CONVENTION_ARM64_AAPCS64_GAS_PE
            mul_calling_convention = None
            nested_calling_convention = None
        elif architecture == "arm64" and calling_convention == "arm64ec":
            normal_calling_convention = CALLING_CONVENTION_ARM64EC_GAS_PE
            mul_calling_convention = None
            nested_calling_convention = None
        elif architecture == "arm" and calling_convention == "aapcs32":
            normal_calling_convention = CALLING_CONVENTION_ARM32_AAPCS32
            mul_calling_convention = None
            nested_calling_convention = None
    elif assembler in [ASSEMBLER_GAS, ASSEMBLER_GAS_MACHO]:
        if architecture == "amd64" and calling_convention == "systemv":
            normal_calling_convention = CALLING_CONVENTION_AMD64_SYSTEMV
            mul_calling_convention = CALLING_CONVENTION_AMD64_SYSTEMV_MUL
            nested_calling_convention = CALLING_CONVENTION_AMD64_SYSTEMV_NESTED
        elif architecture == "arm64" and calling_convention == "aapcs64":
            normal_calling_convention = CALLING_CONVENTION_ARM64_AAPCS64_GAS
            mul_calling_convention = None
            nested_calling_convention = None
        elif architecture == "arm" and calling_convention == "aapcs32":
            normal_calling_convention = CALLING_CONVENTION_ARM32_AAPCS32
            mul_calling_convention = None
            nested_calling_convention = None
    elif assembler == ASSEMBLER_ARMASM64:
        if architecture == "arm64" and calling_convention == "aapcs64":
            normal_calling_convention = CALLING_CONVENTION_ARM64_AAPCS64_ARMASM64
            mul_calling_convention = None
            nested_calling_convention = None
        elif architecture == "arm64" and calling_convention == "arm64ec":
            normal_calling_convention = CALLING_CONVENTION_ARM64EC_MSFT
            mul_calling_convention = None
            nested_calling_convention = None
    else:
        logging.error("Unhandled assembler (%s) in process_file" % assembler)
        exit(1)

    if normal_calling_convention is None:
        logging.error("Unhandled combination (%s + %s + %s) in process_file"
            % (assembler, architecture, calling_convention))
        exit(1)

    file_processing_state = ProcessingStateMachine(
        assembler, normal_calling_convention, mul_calling_convention, nested_calling_convention)

    #
    # expand included files
    #
    expanded_lines = []

    # suppress header insertion
    header = ""
    # insert assembler specific header per symcryptasm file
    #header = gen_file_header(assembler, architecture, calling_convention)
    #expanded_lines.extend(header)

    # expand_files() is called recursively when a .symcryptasm file contains an INCLUDE directive,
    # except for the first call here where we're starting to process the input source file
    # as if it was included by some other file.
    expanded_file, infile_has_includes = expand_files(infilename, 0, "")
    expanded_lines.extend(expanded_file)

    # if header was nonempty or there were any INCLUDE directives, output the expanded source file for debugging
    if header or infile_has_includes:
        expanded_filename = os.path.dirname(outfilename) + "/" + os.path.basename(infilename) + "exp"
        with open(expanded_filename, "w") as outfile:
            outfile.writelines(expanded_lines)

    # iterate through file line by line in one pass
    with open(outfilename, "w") as outfile:
        for line_num, line in enumerate(expanded_lines):
            processed_line = file_processing_state.process_line(line, line_num)
            # logging.info("processed line: %s" % processed_line)
            outfile.write(processed_line)

if __name__ == "__main__":
    import argparse

    # logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser(description="Preprocess symcryptasm into files that will be further processed with C preprocessor to generate MASM or GAS")
    parser.add_argument('assembler', type=str, help='Assembler that we want to preprocess for', choices=[ASSEMBLER_MASM, ASSEMBLER_ARMASM64, ASSEMBLER_GAS, ASSEMBLER_GAS_MACHO, ASSEMBLER_GAS_PE])
    parser.add_argument('architecture', type=str, help='Architecture that we want to preprocess for', choices=['amd64', 'arm64', 'arm'])
    parser.add_argument('calling_convention', type=str, help='Calling convention that we want to preprocess for', choices=['msft', 'systemv', 'aapcs64', 'arm64ec', 'aapcs32'])
    parser.add_argument('inputfile', type=str, help='Path to input file')
    parser.add_argument('outputfile', type=str, help='Path to output file')

    args = parser.parse_args()
    process_file(args.assembler, args.architecture, args.calling_convention, args.inputfile, args.outputfile)
    logging.info("Done")
