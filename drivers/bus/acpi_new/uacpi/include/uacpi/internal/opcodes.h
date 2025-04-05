#pragma once

#include <uacpi/types.h>

typedef uacpi_u16 uacpi_aml_op;

#define UACPI_EXT_PREFIX 0x5B
#define UACPI_EXT_OP(op) ((UACPI_EXT_PREFIX << 8) | (op))

#define UACPI_DUAL_NAME_PREFIX 0x2E
#define UACPI_MULTI_NAME_PREFIX 0x2F
#define UACPI_NULL_NAME 0x00

/*
 * Opcodes that tell the parser VM how to take apart every AML instruction.
 * Every AML opcode has a list of these that is executed by the parser.
 */
enum uacpi_parse_op {
    UACPI_PARSE_OP_END = 0,

    /*
     * End the execution of the current instruction with a warning if the item
     * at decode_ops[pc + 1] is NULL.
     */
    UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL,

    // Emit a warning as if the current opcode is being skipped
    UACPI_PARSE_OP_EMIT_SKIP_WARN,

    // SimpleName := NameString | ArgObj | LocalObj
    UACPI_PARSE_OP_SIMPLE_NAME,

    // SuperName := SimpleName | DebugObj | ReferenceTypeOpcode
    UACPI_PARSE_OP_SUPERNAME,
    // The resulting item will be set to null if name couldn't be resolved
    UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED,

    // TermArg := ExpressionOpcode | DataObject | ArgObj | LocalObj
    UACPI_PARSE_OP_TERM_ARG,
    UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,

    /*
     * Same as TERM_ARG, but named references are passed as-is.
     * This means methods are not invoked, fields are not read, etc.
     */
    UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT,

    /*
     * Same as UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT but allows unresolved
     * name strings.
     */
    UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED,

    // Operand := TermArg => Integer
    UACPI_PARSE_OP_OPERAND,

    // TermArg => String
    UACPI_PARSE_OP_STRING,

    /*
     * ComputationalData := ByteConst | WordConst | DWordConst | QWordConst |
     *                      String | ConstObj | RevisionOp | DefBuffer
     */
    UACPI_PARSE_OP_COMPUTATIONAL_DATA,

    // Target := SuperName | NullName
    UACPI_PARSE_OP_TARGET,

    // Parses a pkglen
    UACPI_PARSE_OP_PKGLEN,

    /*
     * Parses a pkglen and records it, the end of this pkglen is considered
     * the end of the instruction. The PC is always set to the end of this
     * package once parser reaches UACPI_PARSE_OP_END.
     */
    UACPI_PARSE_OP_TRACKED_PKGLEN,

    /*
     * Parse a NameString and create the last nameseg.
     * Note that this errors out if last nameseg already exists.
     */
    UACPI_PARSE_OP_CREATE_NAMESTRING,

    /*
     * same as UACPI_PARSE_OP_CREATE_NAMESTRING, but attempting to create an
     * already existing object is not fatal if currently loading a table.
     */
    UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,

    /*
     * Parse a NameString and put the node into the ready parts array.
     * Note that this errors out if the referenced node doesn't exist.
     */
    UACPI_PARSE_OP_EXISTING_NAMESTRING,

    /*
     * Same as UACPI_PARSE_OP_EXISTING_NAMESTRING except the op doesn't error
     * out if namestring couldn't be resolved.
     */
    UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL,

    /*
     * Same as UACPI_PARSE_OP_EXISTING_NAMESTRING, but undefined references
     * are not fatal if currently loading a table.
     */
    UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL_IF_LOAD,

    // Invoke a handler at op_handlers[spec->code]
    UACPI_PARSE_OP_INVOKE_HANDLER,

    // Allocate an object an put it at the front of the item list
    UACPI_PARSE_OP_OBJECT_ALLOC,

    UACPI_PARSE_OP_EMPTY_OBJECT_ALLOC,

    // Convert last item into a shallow/deep copy of itself
    UACPI_PARSE_OP_OBJECT_CONVERT_TO_SHALLOW_COPY,
    UACPI_PARSE_OP_OBJECT_CONVERT_TO_DEEP_COPY,

    /*
     * Same as UACPI_PARSE_OP_OBJECT_ALLOC except the type of the allocated
     * object is specified at decode_ops[pc + 1]
     */
    UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,

    // Record current AML program counter as a QWORD immediate
    UACPI_PARSE_OP_RECORD_AML_PC,

    // Load a QWORD immediate located at decode_ops[pc + 1]
    UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT,

    // Load a decode_ops[pc + 1] byte imm at decode_ops[pc + 2]
    UACPI_PARSE_OP_LOAD_INLINE_IMM,

    // Load a QWORD zero immediate
    UACPI_PARSE_OP_LOAD_ZERO_IMM,

    // Load a decode_ops[pc + 1] byte imm from the instructions stream
    UACPI_PARSE_OP_LOAD_IMM,

    // Same as UACPI_PARSE_OP_LOAD_IMM, expect the resulting value is an object
    UACPI_PARSE_OP_LOAD_IMM_AS_OBJECT,

    // Create & Load an integer constant representing either true or false
    UACPI_PARSE_OP_LOAD_FALSE_OBJECT,
    UACPI_PARSE_OP_LOAD_TRUE_OBJECT,

    // Truncate the last item in the list if needed
    UACPI_PARSE_OP_TRUNCATE_NUMBER,

    // Ensure the type of item is decode_ops[pc + 1]
    UACPI_PARSE_OP_TYPECHECK,

    // Install the namespace node specified in items[decode_ops[pc + 1]]
    UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE,

    // Move item to the previous (preempted) op
    UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,

    /*
     * Same as UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV, but the object
     * is copied instead. (Useful when dealing with multiple targets)
     * TODO: optimize this so that we can optionally move the object
     *       if target was a null target.
     */
    UACPI_PARSE_OP_OBJECT_COPY_TO_PREV,

    // Store the last item to the target at items[decode_ops[pc + 1]]
    UACPI_PARSE_OP_STORE_TO_TARGET,

    /*
     * Store the item at items[decode_ops[pc + 2]] to target
     * at items[decode_ops[pc + 1]]
     */
    UACPI_PARSE_OP_STORE_TO_TARGET_INDIRECT,

    /*
     * Error if reached. Should be used for opcodes that are supposed to be
     * converted at op parse time, e.g. invoking a method or referring to
     * a named object.
     */
    UACPI_PARSE_OP_UNREACHABLE,

    // Invalid opcode, should never be encountered in the stream
    UACPI_PARSE_OP_BAD_OPCODE,

    // Decrement the current AML instruction pointer
    UACPI_PARSE_OP_AML_PC_DECREMENT,

    // Decrement the immediate at decode_ops[pc + 1]
    UACPI_PARSE_OP_IMM_DECREMENT,

    // Remove the last item off the item stack
    UACPI_PARSE_OP_ITEM_POP,

    // Dispatch the method call from items[0] and return from current op_exec
    UACPI_PARSE_OP_DISPATCH_METHOD_CALL,

    /*
     * Dispatch a table load with scope node at items[0] and method at items[1].
     * The last item is expected to be an integer object that is set to 0 in
     * case load fails.
     */
    UACPI_PARSE_OP_DISPATCH_TABLE_LOAD,

    /*
     * Convert the current resolved namestring to either a method call
     * or a named object reference.
     */
    UACPI_PARSE_OP_CONVERT_NAMESTRING,

    /*
     * Execute the next instruction only if currently tracked package still
     * has data left, otherwise skip decode_ops[pc + 1] bytes.
     */
    UACPI_PARSE_OP_IF_HAS_DATA,

    /*
     * Execute the next instruction only if the handle at
     * items[decode_ops[pc + 1]] is null. Otherwise skip
     * decode_ops[pc + 2] bytes.
     */
    UACPI_PARSE_OP_IF_NULL,

   /*
    * Execute the next instruction only if the handle at
    * items[-1] is null. Otherwise skip decode_ops[pc + 1] bytes.
    */
    UACPI_PARSE_OP_IF_LAST_NULL,

    // The inverse of UACPI_PARSE_OP_IF_NULL
    UACPI_PARSE_OP_IF_NOT_NULL,

    // The inverse of UACPI_PARSE_OP_IF_LAST_NULL
    UACPI_PARSE_OP_IF_LAST_NOT_NULL,

    /*
     * Execute the next instruction only if the last immediate is equal to
     * decode_ops[pc + 1], otherwise skip decode_ops[pc + 2] bytes.
     */
    UACPI_PARSE_OP_IF_LAST_EQUALS,

   /*
    * Execute the next instruction only if the last object is a false value
    * (has a value of 0), otherwise skip decode_ops[pc + 1] bytes.
    */
    UACPI_PARSE_OP_IF_LAST_FALSE,

    // The inverse of UACPI_PARSE_OP_IF_LAST_FALSE
    UACPI_PARSE_OP_IF_LAST_TRUE,

    /*
     * Switch to opcode at decode_ops[pc + 1] only if the next AML instruction
     * in the stream is equal to it. Note that this looks ahead of the tracked
     * package if one is active. Switching to the next op also applies the
     * currently tracked package.
     */
    UACPI_PARSE_OP_SWITCH_TO_NEXT_IF_EQUALS,

   /*
    * Execute the next instruction only if this op was switched to from op at
    * (decode_ops[pc + 1] | decode_ops[pc + 2] << 8), otherwise skip
    * decode_ops[pc + 3] bytes.
    */
    UACPI_PARSE_OP_IF_SWITCHED_FROM,

    /*
     * pc = decode_ops[pc + 1]
     */
    UACPI_PARSE_OP_JMP,
    UACPI_PARSE_OP_MAX = UACPI_PARSE_OP_JMP,
};
const uacpi_char *uacpi_parse_op_to_string(enum uacpi_parse_op op);

/*
 * A few notes about op properties:
 * Technically the spec says that RefOfOp is considered a SuperName, but NT
 * disagrees about this. For example Store(..., RefOf) fails with
 * "Invalid SuperName". MethodInvocation could also technically be considered
 * a SuperName, but NT doesn't allow that either: Store(..., MethodInvocation)
 * fails with "Invalid Target Method, expected a DataObject" error.
 */

enum uacpi_op_property {
    UACPI_OP_PROPERTY_TERM_ARG = 1,
    UACPI_OP_PROPERTY_SUPERNAME = 2,
    UACPI_OP_PROPERTY_SIMPLE_NAME = 4,
    UACPI_OP_PROPERTY_TARGET = 8,

    // The ops to execute are pointed to by indirect_decode_ops
    UACPI_OP_PROPERTY_OUT_OF_LINE = 16,

    // Error if encountered in the AML byte strem
    UACPI_OP_PROPERTY_RESERVED = 128,
};

struct uacpi_op_spec {
    uacpi_char *name;
    union {
        uacpi_u8 decode_ops[16];
        uacpi_u8 *indirect_decode_ops;
    };
    uacpi_u8 properties;
    uacpi_aml_op code;
};

const struct uacpi_op_spec *uacpi_get_op_spec(uacpi_aml_op);

#define UACPI_INTERNAL_OP(code) \
    UACPI_OP(Internal_##code, code, { UACPI_PARSE_OP_UNREACHABLE })

#define UACPI_BAD_OPCODE(code) \
    UACPI_OP(Reserved_##code, code, { UACPI_PARSE_OP_BAD_OPCODE })

#define UACPI_METHOD_CALL_OPCODE(nargs)                        \
    UACPI_OP(                                                  \
        InternalOpMethodCall##nargs##Args, 0xF7 + nargs,       \
        {                                                      \
            UACPI_PARSE_OP_LOAD_INLINE_IMM, 1, nargs,          \
            UACPI_PARSE_OP_IF_NOT_NULL, 1, 6,                  \
                UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,       \
                UACPI_PARSE_OP_OBJECT_CONVERT_TO_SHALLOW_COPY, \
                UACPI_PARSE_OP_IMM_DECREMENT, 1,               \
                UACPI_PARSE_OP_JMP, 3,                         \
            UACPI_PARSE_OP_OBJECT_ALLOC,                       \
            UACPI_PARSE_OP_DISPATCH_METHOD_CALL,               \
            UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,            \
        },                                                     \
        UACPI_OP_PROPERTY_TERM_ARG |                           \
        UACPI_OP_PROPERTY_RESERVED                             \
    )

/*
 * -------------------------------------------------------------
 * RootChar := ‘\’
 * ParentPrefixChar := ‘^’
 * ‘\’ := 0x5C
 * ‘^’ := 0x5E
 * MultiNamePrefix := 0x2F
 * DualNamePrefix := 0x2E
 * ------------------------------------------------------------
 * ‘A’-‘Z’ := 0x41 - 0x5A
 * ‘_’ := 0x5F
 * LeadNameChar := ‘A’-‘Z’ | ‘_’
 * NameSeg := <leadnamechar namechar namechar namechar>
 * NameString := <rootchar namepath> | <prefixpath namepath>
 * PrefixPath := Nothing | <’^’ prefixpath>
 * DualNamePath := DualNamePrefix NameSeg NameSeg
 * MultiNamePath := MultiNamePrefix SegCount NameSeg(SegCount)
 */
#define UACPI_UNRESOLVED_NAME_STRING_OP(character, code)        \
    UACPI_OP(                                                   \
        UACPI_InternalOpUnresolvedNameString_##character, code, \
        {                                                       \
            UACPI_PARSE_OP_AML_PC_DECREMENT,                    \
            UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL,         \
            UACPI_PARSE_OP_CONVERT_NAMESTRING,                  \
        },                                                      \
        UACPI_OP_PROPERTY_SIMPLE_NAME |                         \
        UACPI_OP_PROPERTY_SUPERNAME |                           \
        UACPI_OP_PROPERTY_TERM_ARG                              \
    )

#define UACPI_BUILD_LOCAL_OR_ARG_OP(prefix, base, offset) \
UACPI_OP(                                                 \
    prefix##offset##Op, base + offset,                    \
    {                                                     \
        UACPI_PARSE_OP_EMPTY_OBJECT_ALLOC,                \
        UACPI_PARSE_OP_INVOKE_HANDLER,                    \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,           \
    },                                                    \
    UACPI_OP_PROPERTY_SUPERNAME |                         \
    UACPI_OP_PROPERTY_TERM_ARG |                          \
    UACPI_OP_PROPERTY_SIMPLE_NAME                         \
)                                                         \

#define UACPI_LOCALX_OP(idx) UACPI_BUILD_LOCAL_OR_ARG_OP(Local, 0x60, idx)
#define UACPI_ARGX_OP(idx) UACPI_BUILD_LOCAL_OR_ARG_OP(Arg, 0x68, idx)

#define UACPI_BUILD_PACKAGE_OP(name, code, jmp_off, ...)           \
UACPI_OP(                                                          \
    name##Op, code,                                                \
    {                                                              \
        UACPI_PARSE_OP_TRACKED_PKGLEN,                             \
        ##__VA_ARGS__,                                             \
        UACPI_PARSE_OP_IF_HAS_DATA, 4,                             \
            UACPI_PARSE_OP_RECORD_AML_PC,                          \
            UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED, \
            UACPI_PARSE_OP_JMP, jmp_off,                           \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_PACKAGE,   \
        UACPI_PARSE_OP_INVOKE_HANDLER,                             \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                    \
    },                                                             \
    UACPI_OP_PROPERTY_TERM_ARG                                     \
)

#define UACPI_BUILD_BINARY_MATH_OP(prefix, code)                 \
UACPI_OP(                                                        \
    prefix##Op, code,                                            \
    {                                                            \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_TRUNCATE_NUMBER,                          \
        UACPI_PARSE_OP_STORE_TO_TARGET, 2,                       \
        UACPI_PARSE_OP_OBJECT_COPY_TO_PREV,                      \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)

#define UACPI_BUILD_UNARY_MATH_OP(type, code)                    \
UACPI_OP(                                                        \
    type##Op, code,                                              \
    {                                                            \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_STORE_TO_TARGET, 1,                       \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)

#define UACPI_DO_BUILD_BUFFER_FIELD_OP(type, code, node_idx, ...)     \
UACPI_OP(                                                             \
    type##FieldOp, code,                                              \
    {                                                                 \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                      \
        UACPI_PARSE_OP_TYPECHECK, UACPI_OBJECT_BUFFER,                \
        UACPI_PARSE_OP_OPERAND,                                       \
        ##__VA_ARGS__,                                                \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,             \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, node_idx,              \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_BUFFER_FIELD, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                                \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, node_idx,              \
    }                                                                 \
)

#define UACPI_BUILD_BUFFER_FIELD_OP(type, code) \
    UACPI_DO_BUILD_BUFFER_FIELD_OP(Create##type, code, 2)

#define UACPI_INTEGER_LITERAL_OP(type, code, bytes)              \
UACPI_OP(                                                        \
    type##Prefix, code,                                          \
    {                                                            \
        UACPI_PARSE_OP_LOAD_IMM_AS_OBJECT, bytes,                \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \

#define UACPI_BUILD_BINARY_LOGIC_OP(type, code)                  \
UACPI_OP(                                                        \
    type##Op, code,                                              \
    {                                                            \
        UACPI_PARSE_OP_COMPUTATIONAL_DATA,                       \
        UACPI_PARSE_OP_COMPUTATIONAL_DATA,                       \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)

#define UACPI_BUILD_TO_OP(kind, code, dst_type)      \
UACPI_OP(                                            \
    To##kind##Op, code,                              \
    {                                                \
        UACPI_PARSE_OP_COMPUTATIONAL_DATA,           \
        UACPI_PARSE_OP_TARGET,                       \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, dst_type, \
        UACPI_PARSE_OP_INVOKE_HANDLER,               \
        UACPI_PARSE_OP_STORE_TO_TARGET, 1,           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,      \
    },                                               \
    UACPI_OP_PROPERTY_TERM_ARG                       \
)

#define UACPI_BUILD_INC_DEC_OP(prefix, code)                     \
UACPI_OP(                                                        \
    prefix##Op, code,                                            \
    {                                                            \
        UACPI_PARSE_OP_SUPERNAME,                                \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_TRUNCATE_NUMBER,                          \
        UACPI_PARSE_OP_STORE_TO_TARGET, 0,                       \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \

#define UACPI_ENUMERATE_OPCODES                                  \
UACPI_OP(                                                        \
    ZeroOp, 0x00,                                                \
    {                                                            \
        UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT,                \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TARGET |                                   \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_OP(                                                        \
    OneOp, 0x01,                                                 \
    {                                                            \
        UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT,                \
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BAD_OPCODE(0x02)                                           \
UACPI_BAD_OPCODE(0x03)                                           \
UACPI_BAD_OPCODE(0x04)                                           \
UACPI_BAD_OPCODE(0x05)                                           \
UACPI_OP(                                                        \
    AliasOp, 0x06,                                               \
    {                                                            \
        UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL_IF_LOAD,      \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,        \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 0,                \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 1,                \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 1,                \
    }                                                            \
)                                                                \
UACPI_BAD_OPCODE(0x07)                                           \
UACPI_OP(                                                        \
    NameOp, 0x08,                                                \
    {                                                            \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,        \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 0,                \
        UACPI_PARSE_OP_OBJECT_CONVERT_TO_DEEP_COPY,              \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 0,                \
    }                                                            \
)                                                                \
UACPI_BAD_OPCODE(0x09)                                           \
UACPI_INTEGER_LITERAL_OP(Byte, 0x0A, 1)                          \
UACPI_INTEGER_LITERAL_OP(Word, 0x0B, 2)                          \
UACPI_INTEGER_LITERAL_OP(DWord, 0x0C, 4)                         \
UACPI_OP(                                                        \
    StringPrefix, 0x0D,                                          \
    {                                                            \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_STRING,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_INTEGER_LITERAL_OP(QWord, 0x0E, 8)                         \
UACPI_BAD_OPCODE(0x0F)                                           \
UACPI_OP(                                                        \
    ScopeOp, 0x10,                                               \
    {                                                            \
        UACPI_PARSE_OP_TRACKED_PKGLEN,                           \
        UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL_IF_LOAD,      \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 1,                \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
    BufferOp, 0x11,                                              \
    {                                                            \
        UACPI_PARSE_OP_TRACKED_PKGLEN,                           \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_RECORD_AML_PC,                            \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_BUFFER,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_PACKAGE_OP(                                          \
    Package, 0x12, 3,                                            \
    UACPI_PARSE_OP_LOAD_IMM, 1                                   \
)                                                                \
UACPI_BUILD_PACKAGE_OP(                                          \
    VarPackage, 0x13, 2,                                         \
    UACPI_PARSE_OP_OPERAND                                       \
)                                                                \
UACPI_OP(                                                        \
    MethodOp, 0x14,                                              \
    {                                                            \
        UACPI_PARSE_OP_TRACKED_PKGLEN,                           \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,        \
        UACPI_PARSE_OP_LOAD_IMM, 1,                              \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 1,                \
        UACPI_PARSE_OP_RECORD_AML_PC,                            \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_METHOD,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 1,                \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
    ExternalOp, 0x15,                                            \
    {                                                            \
        UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL,              \
        UACPI_PARSE_OP_LOAD_IMM, 1,                              \
        UACPI_PARSE_OP_LOAD_IMM, 1,                              \
    }                                                            \
)                                                                \
UACPI_BAD_OPCODE(0x16)                                           \
UACPI_BAD_OPCODE(0x17)                                           \
UACPI_BAD_OPCODE(0x18)                                           \
UACPI_BAD_OPCODE(0x19)                                           \
UACPI_BAD_OPCODE(0x1A)                                           \
UACPI_BAD_OPCODE(0x1B)                                           \
UACPI_BAD_OPCODE(0x1C)                                           \
UACPI_BAD_OPCODE(0x1D)                                           \
UACPI_BAD_OPCODE(0x1E)                                           \
UACPI_BAD_OPCODE(0x1F)                                           \
UACPI_BAD_OPCODE(0x20)                                           \
UACPI_BAD_OPCODE(0x21)                                           \
UACPI_BAD_OPCODE(0x22)                                           \
UACPI_BAD_OPCODE(0x23)                                           \
UACPI_BAD_OPCODE(0x24)                                           \
UACPI_BAD_OPCODE(0x25)                                           \
UACPI_BAD_OPCODE(0x26)                                           \
UACPI_BAD_OPCODE(0x27)                                           \
UACPI_BAD_OPCODE(0x28)                                           \
UACPI_BAD_OPCODE(0x29)                                           \
UACPI_BAD_OPCODE(0x2A)                                           \
UACPI_BAD_OPCODE(0x2B)                                           \
UACPI_BAD_OPCODE(0x2C)                                           \
UACPI_BAD_OPCODE(0x2D)                                           \
UACPI_UNRESOLVED_NAME_STRING_OP(DualNamePrefix, 0x2E)            \
UACPI_UNRESOLVED_NAME_STRING_OP(MultiNamePrefix, 0x2F)           \
UACPI_INTERNAL_OP(0x30)                                          \
UACPI_INTERNAL_OP(0x31)                                          \
UACPI_INTERNAL_OP(0x32)                                          \
UACPI_INTERNAL_OP(0x33)                                          \
UACPI_INTERNAL_OP(0x34)                                          \
UACPI_INTERNAL_OP(0x35)                                          \
UACPI_INTERNAL_OP(0x36)                                          \
UACPI_INTERNAL_OP(0x37)                                          \
UACPI_INTERNAL_OP(0x38)                                          \
UACPI_INTERNAL_OP(0x39)                                          \
UACPI_BAD_OPCODE(0x3A)                                           \
UACPI_BAD_OPCODE(0x3B)                                           \
UACPI_BAD_OPCODE(0x3C)                                           \
UACPI_BAD_OPCODE(0x3D)                                           \
UACPI_BAD_OPCODE(0x3E)                                           \
UACPI_BAD_OPCODE(0x3F)                                           \
UACPI_BAD_OPCODE(0x40)                                           \
UACPI_UNRESOLVED_NAME_STRING_OP(A, 0x41)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(B, 0x42)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(C, 0x43)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(D, 0x44)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(E, 0x45)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(F, 0x46)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(G, 0x47)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(H, 0x48)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(I, 0x49)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(J, 0x4A)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(K, 0x4B)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(L, 0x4C)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(M, 0x4D)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(N, 0x4E)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(O, 0x4F)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(P, 0x50)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(Q, 0x51)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(R, 0x52)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(S, 0x53)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(T, 0x54)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(U, 0x55)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(V, 0x56)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(W, 0x57)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(X, 0x58)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(Y, 0x59)                         \
UACPI_UNRESOLVED_NAME_STRING_OP(Z, 0x5A)                         \
UACPI_INTERNAL_OP(0x5B)                                          \
UACPI_UNRESOLVED_NAME_STRING_OP(RootChar, 0x5C)                  \
UACPI_BAD_OPCODE(0x5D)                                           \
UACPI_UNRESOLVED_NAME_STRING_OP(ParentPrefixChar, 0x5E)          \
UACPI_UNRESOLVED_NAME_STRING_OP(Underscore, 0x5F)                \
UACPI_LOCALX_OP(0)                                               \
UACPI_LOCALX_OP(1)                                               \
UACPI_LOCALX_OP(2)                                               \
UACPI_LOCALX_OP(3)                                               \
UACPI_LOCALX_OP(4)                                               \
UACPI_LOCALX_OP(5)                                               \
UACPI_LOCALX_OP(6)                                               \
UACPI_LOCALX_OP(7)                                               \
UACPI_ARGX_OP(0)                                                 \
UACPI_ARGX_OP(1)                                                 \
UACPI_ARGX_OP(2)                                                 \
UACPI_ARGX_OP(3)                                                 \
UACPI_ARGX_OP(4)                                                 \
UACPI_ARGX_OP(5)                                                 \
UACPI_ARGX_OP(6)                                                 \
UACPI_BAD_OPCODE(0x6F)                                           \
UACPI_OP(                                                        \
    StoreOp, 0x70,                                               \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG,                                 \
        UACPI_PARSE_OP_SUPERNAME,                                \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_ITEM_POP,                                 \
        UACPI_PARSE_OP_OBJECT_COPY_TO_PREV,                      \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_OP(                                                        \
    RefOfOp, 0x71,                                               \
    {                                                            \
        UACPI_PARSE_OP_SUPERNAME,                                \
        UACPI_PARSE_OP_OBJECT_ALLOC,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_BINARY_MATH_OP(Add, 0x72)                            \
UACPI_OP(                                                        \
    ConcatOp, 0x73,                                              \
    {                                                            \
        UACPI_PARSE_OP_COMPUTATIONAL_DATA,                       \
        UACPI_PARSE_OP_COMPUTATIONAL_DATA,                       \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_BUFFER,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_STORE_TO_TARGET, 2,                       \
        UACPI_PARSE_OP_OBJECT_COPY_TO_PREV,                      \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_BINARY_MATH_OP(Subtract, 0x74)                       \
UACPI_BUILD_INC_DEC_OP(Increment, 0x75)                          \
UACPI_BUILD_INC_DEC_OP(Decrement, 0x76)                          \
UACPI_BUILD_BINARY_MATH_OP(Multiply, 0x77)                       \
UACPI_OP(                                                        \
    DivideOp, 0x78,                                              \
    {                                                            \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_STORE_TO_TARGET, 3,                       \
        UACPI_PARSE_OP_OBJECT_COPY_TO_PREV,                      \
        UACPI_PARSE_OP_STORE_TO_TARGET_INDIRECT, 2, 4,           \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_BINARY_MATH_OP(ShiftLeft, 0x79)                      \
UACPI_BUILD_BINARY_MATH_OP(ShiftRight, 0x7A)                     \
UACPI_BUILD_BINARY_MATH_OP(And, 0x7B)                            \
UACPI_BUILD_BINARY_MATH_OP(Nand, 0x7C)                           \
UACPI_BUILD_BINARY_MATH_OP(Or, 0x7D)                             \
UACPI_BUILD_BINARY_MATH_OP(Nor, 0x7E)                            \
UACPI_BUILD_BINARY_MATH_OP(Xor, 0x7F)                            \
UACPI_BUILD_UNARY_MATH_OP(Not, 0x80)                             \
UACPI_BUILD_UNARY_MATH_OP(FindSetLeftBit, 0x81)                  \
UACPI_BUILD_UNARY_MATH_OP(FindSetRightBit, 0x82)                 \
UACPI_OP(                                                        \
    DerefOfOp, 0x83,                                             \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_OBJECT_ALLOC,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_OP(                                                        \
    ConcatResOp, 0x84,                                           \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_TYPECHECK, UACPI_OBJECT_BUFFER,           \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_TYPECHECK, UACPI_OBJECT_BUFFER,           \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_BUFFER,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_STORE_TO_TARGET, 2,                       \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_BINARY_MATH_OP(Mod, 0x85)                            \
UACPI_OP(                                                        \
    NotifyOp, 0x86,                                              \
    {                                                            \
    /* This is technically wrong according to spec but I was */  \
    /* unable to find any examples of anything else after    */  \
    /* inspecting about 500 AML dumps. Spec says this is a   */  \
    /* SuperName that must evaluate to Device/ThermalZone or */  \
    /* Processor, just ignore for now.                       */  \
        UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL_IF_LOAD,      \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 0,                \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
    SizeOfOp, 0x87,                                              \
    {                                                            \
        UACPI_PARSE_OP_SUPERNAME,                                \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_OP(                                                        \
    IndexOp, 0x88,                                               \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_EMPTY_OBJECT_ALLOC,                       \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_STORE_TO_TARGET, 2,                       \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG |                                 \
    UACPI_OP_PROPERTY_SUPERNAME |                                \
    UACPI_OP_PROPERTY_SIMPLE_NAME                                \
)                                                                \
UACPI_OP(                                                        \
    MatchOp, 0x89,                                               \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_TYPECHECK, UACPI_OBJECT_PACKAGE,          \
        UACPI_PARSE_OP_LOAD_IMM, 1,                              \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_LOAD_IMM, 1,                              \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_BUFFER_FIELD_OP(DWord, 0x8A)                         \
UACPI_BUILD_BUFFER_FIELD_OP(Word, 0x8B)                          \
UACPI_BUILD_BUFFER_FIELD_OP(Byte, 0x8C)                          \
UACPI_BUILD_BUFFER_FIELD_OP(Bit, 0x8D)                           \
UACPI_OP(                                                        \
    ObjectTypeOp, 0x8E,                                          \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT,                 \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_BUFFER_FIELD_OP(QWord, 0x8F)                         \
UACPI_BUILD_BINARY_LOGIC_OP(Land, 0x90)                          \
UACPI_BUILD_BINARY_LOGIC_OP(Lor, 0x91)                           \
UACPI_OP(                                                        \
    LnotOp, 0x92,                                                \
    {                                                            \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_OBJECT_ALLOC,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_BUILD_BINARY_LOGIC_OP(LEqual, 0x93)                        \
UACPI_BUILD_BINARY_LOGIC_OP(LGreater, 0x94)                      \
UACPI_BUILD_BINARY_LOGIC_OP(LLess, 0x95)                         \
UACPI_BUILD_TO_OP(Buffer, 0x96, UACPI_OBJECT_BUFFER)             \
UACPI_BUILD_TO_OP(DecimalString, 0x97, UACPI_OBJECT_STRING)      \
UACPI_BUILD_TO_OP(HexString, 0x98, UACPI_OBJECT_STRING)          \
UACPI_BUILD_TO_OP(Integer, 0x99, UACPI_OBJECT_INTEGER)           \
UACPI_BAD_OPCODE(0x9A)                                           \
UACPI_BAD_OPCODE(0x9B)                                           \
UACPI_OP(                                                        \
    ToStringOp, 0x9C,                                            \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_TYPECHECK, UACPI_OBJECT_BUFFER,           \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_STRING,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_STORE_TO_TARGET, 2,                       \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_OP(                                                        \
    CopyObjectOp, 0x9D,                                          \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG,                                 \
        UACPI_PARSE_OP_OBJECT_COPY_TO_PREV,                      \
        UACPI_PARSE_OP_SIMPLE_NAME,                              \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_OP(                                                        \
    MidOp, 0x9E,                                                 \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_TARGET,                                   \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_BUFFER,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_STORE_TO_TARGET, 3,                       \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)                                                                \
UACPI_OP(                                                        \
    ContinueOp, 0x9F,                                            \
    {                                                            \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
    IfOp, 0xA0,                                                  \
    {                                                            \
        UACPI_PARSE_OP_TRACKED_PKGLEN,                           \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_IF_LAST_NULL, 3,                          \
            UACPI_PARSE_OP_EMIT_SKIP_WARN,                       \
            UACPI_PARSE_OP_JMP, 9,                               \
        UACPI_PARSE_OP_IF_LAST_FALSE, 4,                         \
            UACPI_PARSE_OP_SWITCH_TO_NEXT_IF_EQUALS, 0xA1, 0x00, \
            UACPI_PARSE_OP_END,                                  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
   ElseOp, 0xA1,                                                 \
   {                                                             \
       UACPI_PARSE_OP_IF_SWITCHED_FROM, 0xA0, 0x00, 10,          \
           UACPI_PARSE_OP_IF_LAST_NULL, 3,                       \
               UACPI_PARSE_OP_TRACKED_PKGLEN,                    \
               UACPI_PARSE_OP_EMIT_SKIP_WARN,                    \
               UACPI_PARSE_OP_END,                               \
           UACPI_PARSE_OP_ITEM_POP,                              \
           UACPI_PARSE_OP_ITEM_POP,                              \
           UACPI_PARSE_OP_PKGLEN,                                \
           UACPI_PARSE_OP_INVOKE_HANDLER,                        \
           UACPI_PARSE_OP_END,                                   \
       UACPI_PARSE_OP_TRACKED_PKGLEN,                            \
   }                                                             \
)                                                                \
UACPI_OP(                                                        \
    WhileOp, 0xA2,                                               \
    {                                                            \
        UACPI_PARSE_OP_TRACKED_PKGLEN,                           \
        UACPI_PARSE_OP_OPERAND,                                  \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 1,                \
        UACPI_PARSE_OP_IF_LAST_TRUE, 1,                          \
            UACPI_PARSE_OP_INVOKE_HANDLER,                       \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
    NoopOp, 0xA3,                                                \
    {                                                            \
        UACPI_PARSE_OP_END,                                      \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
    ReturnOp, 0xA4,                                              \
    {                                                            \
        UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                 \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    }                                                            \
)                                                                \
UACPI_OP(                                                        \
    BreakOp, 0xA5,                                               \
    {                                                            \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    }                                                            \
)                                                                \
UACPI_BAD_OPCODE(0xA6)                                           \
UACPI_BAD_OPCODE(0xA7)                                           \
UACPI_BAD_OPCODE(0xA8)                                           \
UACPI_BAD_OPCODE(0xA9)                                           \
UACPI_BAD_OPCODE(0xAA)                                           \
UACPI_BAD_OPCODE(0xAB)                                           \
UACPI_BAD_OPCODE(0xAC)                                           \
UACPI_BAD_OPCODE(0xAD)                                           \
UACPI_BAD_OPCODE(0xAE)                                           \
UACPI_BAD_OPCODE(0xAF)                                           \
UACPI_BAD_OPCODE(0xB0)                                           \
UACPI_BAD_OPCODE(0xB1)                                           \
UACPI_BAD_OPCODE(0xB2)                                           \
UACPI_BAD_OPCODE(0xB3)                                           \
UACPI_BAD_OPCODE(0xB4)                                           \
UACPI_BAD_OPCODE(0xB5)                                           \
UACPI_BAD_OPCODE(0xB6)                                           \
UACPI_BAD_OPCODE(0xB7)                                           \
UACPI_BAD_OPCODE(0xB8)                                           \
UACPI_BAD_OPCODE(0xB9)                                           \
UACPI_BAD_OPCODE(0xBA)                                           \
UACPI_BAD_OPCODE(0xBB)                                           \
UACPI_BAD_OPCODE(0xBC)                                           \
UACPI_BAD_OPCODE(0xBD)                                           \
UACPI_BAD_OPCODE(0xBE)                                           \
UACPI_BAD_OPCODE(0xBF)                                           \
UACPI_BAD_OPCODE(0xC0)                                           \
UACPI_BAD_OPCODE(0xC1)                                           \
UACPI_BAD_OPCODE(0xC2)                                           \
UACPI_BAD_OPCODE(0xC3)                                           \
UACPI_BAD_OPCODE(0xC4)                                           \
UACPI_BAD_OPCODE(0xC5)                                           \
UACPI_BAD_OPCODE(0xC6)                                           \
UACPI_BAD_OPCODE(0xC7)                                           \
UACPI_BAD_OPCODE(0xC8)                                           \
UACPI_BAD_OPCODE(0xC9)                                           \
UACPI_BAD_OPCODE(0xCA)                                           \
UACPI_BAD_OPCODE(0xCB)                                           \
UACPI_OP(                                                        \
    BreakPointOp, 0xCC,                                          \
    {                                                            \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
    }                                                            \
)                                                                \
UACPI_BAD_OPCODE(0xCD)                                           \
UACPI_BAD_OPCODE(0xCE)                                           \
UACPI_BAD_OPCODE(0xCF)                                           \
UACPI_BAD_OPCODE(0xD0)                                           \
UACPI_BAD_OPCODE(0xD1)                                           \
UACPI_BAD_OPCODE(0xD2)                                           \
UACPI_BAD_OPCODE(0xD3)                                           \
UACPI_BAD_OPCODE(0xD4)                                           \
UACPI_BAD_OPCODE(0xD5)                                           \
UACPI_BAD_OPCODE(0xD6)                                           \
UACPI_BAD_OPCODE(0xD7)                                           \
UACPI_BAD_OPCODE(0xD8)                                           \
UACPI_BAD_OPCODE(0xD9)                                           \
UACPI_BAD_OPCODE(0xDA)                                           \
UACPI_BAD_OPCODE(0xDB)                                           \
UACPI_BAD_OPCODE(0xDC)                                           \
UACPI_BAD_OPCODE(0xDD)                                           \
UACPI_BAD_OPCODE(0xDE)                                           \
UACPI_BAD_OPCODE(0xDF)                                           \
UACPI_BAD_OPCODE(0xE0)                                           \
UACPI_BAD_OPCODE(0xE1)                                           \
UACPI_BAD_OPCODE(0xE2)                                           \
UACPI_BAD_OPCODE(0xE3)                                           \
UACPI_BAD_OPCODE(0xE4)                                           \
UACPI_BAD_OPCODE(0xE5)                                           \
UACPI_BAD_OPCODE(0xE6)                                           \
UACPI_BAD_OPCODE(0xE7)                                           \
UACPI_BAD_OPCODE(0xE8)                                           \
UACPI_BAD_OPCODE(0xE9)                                           \
UACPI_BAD_OPCODE(0xEA)                                           \
UACPI_BAD_OPCODE(0xEB)                                           \
UACPI_BAD_OPCODE(0xEC)                                           \
UACPI_BAD_OPCODE(0xED)                                           \
UACPI_BAD_OPCODE(0xEE)                                           \
UACPI_BAD_OPCODE(0xEF)                                           \
UACPI_BAD_OPCODE(0xF0)                                           \
UACPI_BAD_OPCODE(0xF1)                                           \
UACPI_BAD_OPCODE(0xF2)                                           \
UACPI_BAD_OPCODE(0xF3)                                           \
UACPI_OP(                                                        \
    InternalOpReadFieldAsBuffer, 0xF4,                           \
    {                                                            \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_BUFFER,  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG |                                 \
    UACPI_OP_PROPERTY_RESERVED                                   \
)                                                                \
UACPI_OP(                                                        \
    InternalOpReadFieldAsInteger, 0xF5,                          \
    {                                                            \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_INTEGER, \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG |                                 \
    UACPI_OP_PROPERTY_RESERVED                                   \
)                                                                \
UACPI_OP(                                                        \
    InternalOpNamedObject, 0xF6,                                 \
    {                                                            \
        UACPI_PARSE_OP_EMPTY_OBJECT_ALLOC,                       \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_SIMPLE_NAME |                              \
    UACPI_OP_PROPERTY_SUPERNAME |                                \
    UACPI_OP_PROPERTY_TERM_ARG |                                 \
    UACPI_OP_PROPERTY_RESERVED                                   \
)                                                                \
UACPI_METHOD_CALL_OPCODE(0)                                      \
UACPI_METHOD_CALL_OPCODE(1)                                      \
UACPI_METHOD_CALL_OPCODE(2)                                      \
UACPI_METHOD_CALL_OPCODE(3)                                      \
UACPI_METHOD_CALL_OPCODE(4)                                      \
UACPI_METHOD_CALL_OPCODE(5)                                      \
UACPI_METHOD_CALL_OPCODE(6)                                      \
UACPI_METHOD_CALL_OPCODE(7)                                      \
UACPI_OP(                                                        \
    OnesOp, 0xFF,                                                \
    {                                                            \
        UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT,                \
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,          \
        UACPI_PARSE_OP_TRUNCATE_NUMBER,                          \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,                  \
    },                                                           \
    UACPI_OP_PROPERTY_TERM_ARG                                   \
)

extern uacpi_u8 uacpi_field_op_decode_ops[];
extern uacpi_u8 uacpi_index_field_op_decode_ops[];
extern uacpi_u8 uacpi_bank_field_op_decode_ops[];
extern uacpi_u8 uacpi_load_op_decode_ops[];
extern uacpi_u8 uacpi_load_table_op_decode_ops[];

#define UACPI_BUILD_NAMED_SCOPE_OBJECT_OP(name, code, type, ...) \
UACPI_OP(                                                        \
    name##Op, UACPI_EXT_OP(code),                                \
    {                                                            \
        UACPI_PARSE_OP_TRACKED_PKGLEN,                           \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,        \
        ##__VA_ARGS__,                                           \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 1,                \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, type,                 \
        UACPI_PARSE_OP_INVOKE_HANDLER,                           \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 1,                \
    }                                                            \
)

#define UACPI_BUILD_TO_FROM_BCD(type, code)     \
UACPI_OP(                                       \
    type##BCDOp, UACPI_EXT_OP(code),            \
    {                                           \
        UACPI_PARSE_OP_OPERAND,                 \
        UACPI_PARSE_OP_TARGET,                  \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,      \
            UACPI_OBJECT_INTEGER,               \
        UACPI_PARSE_OP_INVOKE_HANDLER,          \
        UACPI_PARSE_OP_STORE_TO_TARGET, 1,      \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV, \
    },                                          \
    UACPI_OP_PROPERTY_TERM_ARG                  \
)

#define UACPI_ENUMERATE_EXT_OPCODES                         \
UACPI_OP(                                                   \
    ReservedExtOp, UACPI_EXT_OP(0x00),                      \
    {                                                       \
        UACPI_PARSE_OP_BAD_OPCODE,                          \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    MutexOp, UACPI_EXT_OP(0x01),                            \
    {                                                       \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,   \
        UACPI_PARSE_OP_LOAD_IMM, 1,                         \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 0,           \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,                  \
            UACPI_OBJECT_MUTEX,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 0,           \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    EventOp, UACPI_EXT_OP(0x02),                            \
    {                                                       \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,   \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 0,           \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,                  \
            UACPI_OBJECT_EVENT,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 0,           \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    CondRefOfOp, UACPI_EXT_OP(0x12),                        \
    {                                                       \
        UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED,             \
        UACPI_PARSE_OP_TARGET,                              \
        UACPI_PARSE_OP_IF_NULL, 0, 3,                       \
            UACPI_PARSE_OP_LOAD_FALSE_OBJECT,               \
            UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,         \
            UACPI_PARSE_OP_END,                             \
        UACPI_PARSE_OP_OBJECT_ALLOC,                        \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_STORE_TO_TARGET, 1,                  \
        UACPI_PARSE_OP_LOAD_TRUE_OBJECT,                    \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,             \
    },                                                      \
    UACPI_OP_PROPERTY_TERM_ARG                              \
)                                                           \
UACPI_DO_BUILD_BUFFER_FIELD_OP(                             \
    Create, UACPI_EXT_OP(0x13), 3,                          \
    UACPI_PARSE_OP_OPERAND                                  \
)                                                           \
UACPI_OUT_OF_LINE_OP(                                       \
    LoadTableOp, UACPI_EXT_OP(0x1F),                        \
    uacpi_load_table_op_decode_ops,                         \
    UACPI_OP_PROPERTY_TERM_ARG |                            \
    UACPI_OP_PROPERTY_OUT_OF_LINE                           \
)                                                           \
UACPI_OUT_OF_LINE_OP(                                       \
    LoadOp, UACPI_EXT_OP(0x20),                             \
    uacpi_load_op_decode_ops,                               \
    UACPI_OP_PROPERTY_TERM_ARG |                            \
    UACPI_OP_PROPERTY_OUT_OF_LINE                           \
)                                                           \
UACPI_OP(                                                   \
    StallOp, UACPI_EXT_OP(0x21),                            \
    {                                                       \
        UACPI_PARSE_OP_OPERAND,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    SleepOp, UACPI_EXT_OP(0x22),                            \
    {                                                       \
        UACPI_PARSE_OP_OPERAND,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    AcquireOp, UACPI_EXT_OP(0x23),                          \
    {                                                       \
        UACPI_PARSE_OP_SUPERNAME,                           \
        UACPI_PARSE_OP_LOAD_IMM, 2,                         \
        UACPI_PARSE_OP_LOAD_TRUE_OBJECT,                    \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,             \
    },                                                      \
    UACPI_OP_PROPERTY_TERM_ARG                              \
)                                                           \
UACPI_OP(                                                   \
    SignalOp, UACPI_EXT_OP(0x24),                           \
    {                                                       \
        UACPI_PARSE_OP_SUPERNAME,                           \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    WaitOp, UACPI_EXT_OP(0x25),                             \
    {                                                       \
        UACPI_PARSE_OP_SUPERNAME,                           \
        UACPI_PARSE_OP_OPERAND,                             \
        UACPI_PARSE_OP_LOAD_TRUE_OBJECT,                    \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,             \
    },                                                      \
    UACPI_OP_PROPERTY_TERM_ARG                              \
)                                                           \
UACPI_OP(                                                   \
    ResetOp, UACPI_EXT_OP(0x26),                            \
    {                                                       \
        UACPI_PARSE_OP_SUPERNAME,                           \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    ReleaseOp, UACPI_EXT_OP(0x27),                          \
    {                                                       \
        UACPI_PARSE_OP_SUPERNAME,                           \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
    }                                                       \
)                                                           \
UACPI_BUILD_TO_FROM_BCD(From, 0x28)                         \
UACPI_BUILD_TO_FROM_BCD(To, 0x29)                           \
UACPI_OP(                                                   \
    UnloadOp, UACPI_EXT_OP(0x2A),                           \
    {                                                       \
        UACPI_PARSE_OP_SUPERNAME,                           \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    RevisionOp, UACPI_EXT_OP(0x30),                         \
    {                                                       \
        UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT,           \
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,             \
    },                                                      \
    UACPI_OP_PROPERTY_TERM_ARG                              \
)                                                           \
UACPI_OP(                                                   \
    DebugOp, UACPI_EXT_OP(0x31),                            \
    {                                                       \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,                  \
        UACPI_OBJECT_DEBUG,                                 \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,             \
    },                                                      \
    UACPI_OP_PROPERTY_TERM_ARG |                            \
    UACPI_OP_PROPERTY_SUPERNAME |                           \
    UACPI_OP_PROPERTY_TARGET                                \
)                                                           \
UACPI_OP(                                                   \
    FatalOp, UACPI_EXT_OP(0x32),                            \
    {                                                       \
        UACPI_PARSE_OP_LOAD_IMM, 1,                         \
        UACPI_PARSE_OP_LOAD_IMM, 4,                         \
        UACPI_PARSE_OP_OPERAND,                             \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
    }                                                       \
)                                                           \
UACPI_OP(                                                   \
    TimerOp, UACPI_EXT_OP(0x33),                            \
    {                                                       \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,                  \
            UACPI_OBJECT_INTEGER,                           \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,             \
    },                                                      \
    UACPI_OP_PROPERTY_TERM_ARG                              \
)                                                           \
UACPI_OP(                                                   \
    OpRegionOp, UACPI_EXT_OP(0x80),                         \
    {                                                       \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,   \
        UACPI_PARSE_OP_LOAD_IMM, 1,                         \
        UACPI_PARSE_OP_OPERAND,                             \
        UACPI_PARSE_OP_OPERAND,                             \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 0,           \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,                  \
            UACPI_OBJECT_OPERATION_REGION,                  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 0,           \
    }                                                       \
)                                                           \
UACPI_OUT_OF_LINE_OP(                                       \
    FieldOp, UACPI_EXT_OP(0x81),                            \
    uacpi_field_op_decode_ops,                              \
    UACPI_OP_PROPERTY_OUT_OF_LINE                           \
)                                                           \
UACPI_BUILD_NAMED_SCOPE_OBJECT_OP(                          \
    Device, 0x82, UACPI_OBJECT_DEVICE                       \
)                                                           \
UACPI_BUILD_NAMED_SCOPE_OBJECT_OP(                          \
    Processor, 0x83, UACPI_OBJECT_PROCESSOR,                \
    UACPI_PARSE_OP_LOAD_IMM, 1,                             \
    UACPI_PARSE_OP_LOAD_IMM, 4,                             \
    UACPI_PARSE_OP_LOAD_IMM, 1                              \
)                                                           \
UACPI_BUILD_NAMED_SCOPE_OBJECT_OP(                          \
    PowerRes, 0x84, UACPI_OBJECT_POWER_RESOURCE,            \
    UACPI_PARSE_OP_LOAD_IMM, 1,                             \
    UACPI_PARSE_OP_LOAD_IMM, 2                              \
)                                                           \
UACPI_BUILD_NAMED_SCOPE_OBJECT_OP(                          \
    ThermalZone, 0x85, UACPI_OBJECT_THERMAL_ZONE            \
)                                                           \
UACPI_OUT_OF_LINE_OP(                                       \
    IndexFieldOp, UACPI_EXT_OP(0x86),                       \
    uacpi_index_field_op_decode_ops,                        \
    UACPI_OP_PROPERTY_OUT_OF_LINE                           \
)                                                           \
UACPI_OUT_OF_LINE_OP(                                       \
    BankFieldOp, UACPI_EXT_OP(0x87),                        \
    uacpi_bank_field_op_decode_ops,                         \
    UACPI_OP_PROPERTY_OUT_OF_LINE                           \
)                                                           \
UACPI_OP(                                                   \
    DataRegionOp, UACPI_EXT_OP(0x88),                       \
    {                                                       \
        UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD,   \
        UACPI_PARSE_OP_STRING,                              \
        UACPI_PARSE_OP_STRING,                              \
        UACPI_PARSE_OP_STRING,                              \
        UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL, 0,           \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED,                  \
            UACPI_OBJECT_OPERATION_REGION,                  \
        UACPI_PARSE_OP_INVOKE_HANDLER,                      \
        UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE, 0,           \
    }                                                       \
)

enum uacpi_aml_op {
#define UACPI_OP(name, code, ...) UACPI_AML_OP_##name = code,
#define UACPI_OUT_OF_LINE_OP(name, code, ...) UACPI_AML_OP_##name = code,
    UACPI_ENUMERATE_OPCODES
    UACPI_ENUMERATE_EXT_OPCODES
#undef UACPI_OP
#undef UACPI_OUT_OF_LINE_OP
};
