/*****************************************************************************
    emocii.h

    Owner: DaleG
    Copyright (c) 1996-1997 Microsoft Corporation

    OpCode Interpreter Instruction definition file

*****************************************************************************/

#ifndef EMOCII_H
#define EMOCII_H

MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

// Define "Push Immediate" operations
#define ociiImmLong         (-1)                        // Push literal long
#define ociiImmULong        (-2)                        // Push literal ulong
#define ociiImmShort        (-3)                        // Push literal short
#define ociiImmUShort       (-4)                        // Push literal ushort
#define ociiImmChar         (-5)                        // Push literal char
#define ociiImmUChar        (-6)                        // Push literal uchar
#define ociiImmFloat        (-7)                        // Push literal float
#define ociiStackValue      (-8)                        // Push val from stack
#define ociiStackAddr       (-9)                        // Push addr from stack
#define ociiGlobalValue     (-10)                       // Push global value
#define ociiGlobalAddr      (-11)                       // Push addr of global
#define ociiEventValue      (-12)                       // Push event value
#define ociiEventAddr       (-13)                       // Push event address
#define ociiImmSz           (-14)                       // Push ptr to string
#define ociiImmRg           (-15)                       // Push ptr to array

#define ociiRulFirst        55                          // 1st emruloci.h fn

// Define builtin functions
#define ociiDelayGoToDirul  55                          // DelayGoToDirul()
#define ociiSignal          56                          // Signal a node
#define ociiSignalFrom      57                          // Signal node from 2nd
#define ociiRulParams       58                          // Set rulebase params
#define ociiDefEvent        59                          // Define an event
#define ociiDefRule         60                          // Define a rule
#define ociiMapEvalLevels   61                          // Map levels for oci


#define ociiUserFirst       62                          // 1st User-defined fn


// Define function argument "counts" that are not fixed
#define ocadNonEval         (-3)                        // Non-evaluating fn
#define ocadVarArgs         (-4)                        // Var-args function


/* M  S  O  O  C  I  I */
/*----------------------------------------------------------------------------
    %%Type: MSOOCII
    %%Contact: daleg

    Interpreter instruction definition.
----------------------------------------------------------------------------*/

typedef short MSOOCII;                                  // Interp instr

#define MSOOCV long                                     // Interp ret value
//typedef long MSOOCV;                                  // Interp ret value

typedef signed char MSOOCAD;                            // Arg descriptor


// Return whether the instruction pointer refers to a variable
#define FVarLpocii(pocii) \
            (*pocii == ociiEventValue)


// Define data types: these must have same order as access functions below
typedef enum
    {
    ocdtChar    =   0,  //  0
    ocdtUChar,          //  1
    ocdtShort,          //  2
    ocdtUShort,         //  3
    ocdtInt,            //  4
    ocdtUInt,           //  5
    ocdtLong,           //  6
    ocdtULong,          //  7
    ocdtFloat,          //  8
    ocdtDouble,         //  9
    ocdtLDouble,        //  10
    ocdtPointer,        //  11
    ocdtVoid,           //  12
    ocdtVarArg          //  13              (Take anything)
    } OCDT;


/*----------------------------------------------------------------------------
    Interpreter op-code value for functions.
    This must be maintained in the order that the functions will appear
    in the op-code v-table.
----------------------------------------------------------------------------*/

typedef enum
    {
    ipfnOcv_log_and = 0,                    //  0
    ipfnOcv_log_or,                         //  1
    ipfnOcv_log_not,                        //  2
    ipfnOcv_less_than,                      //  3
    ipfnOcv_less_eql,                       //  4
    ipfnOcv_eql,                            //  5
    ipfnOcv_gtr_eql,                        //  6
    ipfnOcv_gtr_than,                       //  7
    ipfnOcv_not_eql,                        //  8
    ipfnOcv_assign,                         //  9
    ipfnOcv_plus,                           //  10
    ipfnOcv_minus,                          //  11
    ipfnOcv_mult,                           //  12
    ipfnOcv_divide,                         //  13
    ipfnOcv_mod,                            //  14
    ipfnOcv_increment,                      //  15
    ipfnOcv_decrement,                      //  16
    ipfnOcv_unary_plus,                     //  17
    ipfnOcv_unary_minus,                    //  18
    ipfnOcv_bitwise_not,                    //  19
    ipfnOcv_bitwise_and,                    //  20
    ipfnOcv_bitwise_or,                     //  21
    ipfnOcv_bitwise_xor,                    //  22
    ipfnOcv_shift_l,                        //  23
    ipfnOcv_shift_r,                        //  24
    ipfnOcv_dereference,                    //  25
    ipfnOcv_addr_of,                        //  26
    ipfnOcv_cast_as,                        //  27
    ipfnOcv_if,                             //  28
    ipfnOcv_inline_if,                      //  29
    ipfnOcv_let,                            //  30
    ipfnOcv_progn,                          //  31
    ipfnOcv_compound_stmt,                  //  32
    ipfnOcv_get_char,                       //  33
    ipfnOcv_get_uchar,                      //  34
    ipfnOcv_get_short,                      //  35
    ipfnOcv_get_ushort,                     //  36
    ipfnOcv_get_int,                        //  37
    ipfnOcv_get_uint,                       //  38
    ipfnOcv_get_long,                       //  39
    ipfnOcv_get_ulong,                      //  40
    ipfnOcv_get_float,                      //  41
    ipfnOcv_get_double,                     //  42
    ipfnOcv_get_ldouble,                    //  43
    ipfnOcv_set_char,                       //  44
    ipfnOcv_set_uchar,                      //  45
    ipfnOcv_set_short,                      //  46
    ipfnOcv_set_ushort,                     //  47
    ipfnOcv_set_int,                        //  48
    ipfnOcv_set_uint,                       //  49
    ipfnOcv_set_long,                       //  50
    ipfnOcv_set_ulong,                      //  51
    ipfnOcv_set_float,                      //  52
    ipfnOcv_set_double,                     //  53
    ipfnOcv_set_ldouble,                    //  54
    } OCIT;

#define ipfnOcvGetTypeFirst         ipfnOcv_get_char
#define ipfnOcvSetTypeFirst         ipfnOcv_set_char

MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif /* !EMOCII_H */

