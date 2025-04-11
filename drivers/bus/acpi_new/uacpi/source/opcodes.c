#include <uacpi/internal/opcodes.h>

#ifndef UACPI_BAREBONES_MODE

#define UACPI_OP(opname, opcode, ...) \
    { #opname, .decode_ops = __VA_ARGS__, .code = opcode },

#define UACPI_OUT_OF_LINE_OP(opname, opcode, out_of_line_buf, props) \
    {                                                                \
      .name = #opname,                                               \
      .indirect_decode_ops = out_of_line_buf,                        \
      .properties = props,                                           \
      .code = opcode,                                                \
    },

static const struct uacpi_op_spec opcode_table[0x100] = {
    UACPI_ENUMERATE_OPCODES
};

static const struct uacpi_op_spec ext_opcode_table[] = {
    UACPI_ENUMERATE_EXT_OPCODES
};

#define _(op) (op & 0x00FF)

static const uacpi_u8 ext_op_to_idx[0x100] = {
    [_(UACPI_AML_OP_MutexOp)]       = 1,  [_(UACPI_AML_OP_EventOp)]       = 2,
    [_(UACPI_AML_OP_CondRefOfOp)]   = 3,  [_(UACPI_AML_OP_CreateFieldOp)] = 4,
    [_(UACPI_AML_OP_LoadTableOp)]   = 5,  [_(UACPI_AML_OP_LoadOp)]        = 6,
    [_(UACPI_AML_OP_StallOp)]       = 7,  [_(UACPI_AML_OP_SleepOp)]       = 8,
    [_(UACPI_AML_OP_AcquireOp)]     = 9,  [_(UACPI_AML_OP_SignalOp)]      = 10,
    [_(UACPI_AML_OP_WaitOp)]        = 11, [_(UACPI_AML_OP_ResetOp)]       = 12,
    [_(UACPI_AML_OP_ReleaseOp)]     = 13, [_(UACPI_AML_OP_FromBCDOp)]     = 14,
    [_(UACPI_AML_OP_ToBCDOp)]       = 15, [_(UACPI_AML_OP_UnloadOp)]      = 16,
    [_(UACPI_AML_OP_RevisionOp)]    = 17, [_(UACPI_AML_OP_DebugOp)]       = 18,
    [_(UACPI_AML_OP_FatalOp)]       = 19, [_(UACPI_AML_OP_TimerOp)]       = 20,
    [_(UACPI_AML_OP_OpRegionOp)]    = 21, [_(UACPI_AML_OP_FieldOp)]       = 22,
    [_(UACPI_AML_OP_DeviceOp)]      = 23, [_(UACPI_AML_OP_ProcessorOp)]   = 24,
    [_(UACPI_AML_OP_PowerResOp)]    = 25, [_(UACPI_AML_OP_ThermalZoneOp)] = 26,
    [_(UACPI_AML_OP_IndexFieldOp)]  = 27, [_(UACPI_AML_OP_BankFieldOp)]   = 28,
    [_(UACPI_AML_OP_DataRegionOp)]  = 29,
};

const struct uacpi_op_spec *uacpi_get_op_spec(uacpi_aml_op op)
{
    if (op > 0xFF)
        return &ext_opcode_table[ext_op_to_idx[_(op)]];

    return &opcode_table[op];
}

#define PARSE_FIELD_ELEMENTS(parse_loop_pc)                            \
    /* Parse every field element found inside */                       \
    UACPI_PARSE_OP_IF_HAS_DATA, 44,                                    \
        /* Look at the first byte */                                   \
        UACPI_PARSE_OP_LOAD_IMM, 1,                                    \
                                                                       \
        /* ReservedField := 0x00 PkgLength */                          \
        UACPI_PARSE_OP_IF_LAST_EQUALS, 0x00, 3,                        \
            UACPI_PARSE_OP_PKGLEN,                                     \
            UACPI_PARSE_OP_JMP, parse_loop_pc,                         \
                                                                       \
        /* AccessField := 0x01 AccessType AccessAttrib */              \
        UACPI_PARSE_OP_IF_LAST_EQUALS, 0x01, 6,                        \
            UACPI_PARSE_OP_LOAD_IMM, 1,                                \
            UACPI_PARSE_OP_LOAD_IMM, 1,                                \
            UACPI_PARSE_OP_JMP, parse_loop_pc,                         \
                                                                       \
        /* ConnectField := <0x02 NameString> | <0x02 BufferData> */    \
        UACPI_PARSE_OP_IF_LAST_EQUALS, 0x02, 5,                        \
            UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,                   \
            UACPI_PARSE_OP_TYPECHECK, UACPI_OBJECT_BUFFER,             \
            UACPI_PARSE_OP_JMP, parse_loop_pc,                         \
                                                                       \
        /* ExtendedAccessField := 0x03 AccessType ExtendedAccessAttrib \
         *                                        AccessLength */      \
        UACPI_PARSE_OP_IF_LAST_EQUALS, 0x03, 8,                        \
            UACPI_PARSE_OP_LOAD_IMM, 1,                                \
            UACPI_PARSE_OP_LOAD_IMM, 1,                                \
            UACPI_PARSE_OP_LOAD_IMM, 1,                                \
            UACPI_PARSE_OP_JMP, parse_loop_pc,                         \
                                                                       \
        /* NamedField := NameSeg PkgLength */                          \
                                                                       \
        /*                                                             \
         * Discard the immediate, as it's the first byte of the        \
         * nameseg. We don't need it.                                  \
         */                                                            \
        UACPI_PARSE_OP_ITEM_POP,                                       \
        UACPI_PARSE_OP_AML_PC_DECREMENT,                               \
        UACPI_PARSE_OP_CREATE_NAMESTRING,                              \
        UACPI_PARSE_OP_PKGLEN,                                         \
        UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_FIELD_UNIT,    \
        UACPI_PARSE_OP_JMP, parse_loop_pc,                             \
                                                                       \
    UACPI_PARSE_OP_INVOKE_HANDLER,                                     \
    UACPI_PARSE_OP_END

uacpi_u8 uacpi_field_op_decode_ops[] = {
    UACPI_PARSE_OP_TRACKED_PKGLEN,
    UACPI_PARSE_OP_EXISTING_NAMESTRING,
    UACPI_PARSE_OP_LOAD_IMM, 1,
    PARSE_FIELD_ELEMENTS(4),
};

uacpi_u8 uacpi_bank_field_op_decode_ops[] = {
    UACPI_PARSE_OP_TRACKED_PKGLEN,
    UACPI_PARSE_OP_EXISTING_NAMESTRING,
    UACPI_PARSE_OP_EXISTING_NAMESTRING,
    UACPI_PARSE_OP_OPERAND,
    UACPI_PARSE_OP_LOAD_IMM, 1,
    PARSE_FIELD_ELEMENTS(6),
};

uacpi_u8 uacpi_index_field_op_decode_ops[] = {
    UACPI_PARSE_OP_TRACKED_PKGLEN,
    UACPI_PARSE_OP_EXISTING_NAMESTRING,
    UACPI_PARSE_OP_EXISTING_NAMESTRING,
    UACPI_PARSE_OP_LOAD_IMM, 1,
    PARSE_FIELD_ELEMENTS(5),
};

uacpi_u8 uacpi_load_op_decode_ops[] = {
    // Storage for the scope pointer, this is left as 0 in case of errors
    UACPI_PARSE_OP_LOAD_ZERO_IMM,
    UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_METHOD,
    UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,
    UACPI_PARSE_OP_TARGET,

    /*
     * Invoke the handler here to initialize the table. If this fails, it's
     * expected to keep the item 0 as NULL, which is checked below to return
     * false to the caller of Load.
     */
    UACPI_PARSE_OP_INVOKE_HANDLER,
    UACPI_PARSE_OP_IF_NULL, 0, 3,
        UACPI_PARSE_OP_LOAD_FALSE_OBJECT,
        UACPI_PARSE_OP_JMP, 15,

    UACPI_PARSE_OP_LOAD_TRUE_OBJECT,
    UACPI_PARSE_OP_DISPATCH_TABLE_LOAD,

    /*
     * Invoke the handler a second time to initialize any AML GPE handlers that
     * might've been loaded from this table.
     */
    UACPI_PARSE_OP_INVOKE_HANDLER,
    UACPI_PARSE_OP_STORE_TO_TARGET, 3,
    UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,
    UACPI_PARSE_OP_END,
};

uacpi_u8 uacpi_load_table_op_decode_ops[] = {
    // Storage for the scope pointer, this is left as 0 in case of errors
    UACPI_PARSE_OP_LOAD_ZERO_IMM,
    UACPI_PARSE_OP_OBJECT_ALLOC_TYPED, UACPI_OBJECT_METHOD,
    // Index of the table we are going to be loaded to unref it later
    UACPI_PARSE_OP_LOAD_ZERO_IMM,
    // Storage for the target pointer, this is left as 0 if none was requested
    UACPI_PARSE_OP_LOAD_ZERO_IMM,

    UACPI_PARSE_OP_LOAD_INLINE_IMM, 1, 5,
    UACPI_PARSE_OP_IF_NOT_NULL, 4, 5,
        UACPI_PARSE_OP_STRING,
        UACPI_PARSE_OP_IMM_DECREMENT, 4,
        UACPI_PARSE_OP_JMP, 8,
    UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL,

    /*
     * Invoke the handler here to initialize the table. If this fails, it's
     * expected to keep the item 0 as NULL, which is checked below to return
     * false to the caller of Load.
     */
    UACPI_PARSE_OP_INVOKE_HANDLER,
    UACPI_PARSE_OP_IF_NULL, 0, 3,
        UACPI_PARSE_OP_LOAD_FALSE_OBJECT,
        UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,
        UACPI_PARSE_OP_END,

    UACPI_PARSE_OP_LOAD_TRUE_OBJECT,
    UACPI_PARSE_OP_DISPATCH_TABLE_LOAD,

    /*
     * Invoke the handler a second time to block the store to target in case
     * the load above failed, as well as do any AML GPE handler initialization.
     */
    UACPI_PARSE_OP_INVOKE_HANDLER,

    // If we were given a target to store to, do the store
    UACPI_PARSE_OP_IF_NOT_NULL, 3, 3,
        UACPI_PARSE_OP_STORE_TO_TARGET_INDIRECT, 3, 10,

    UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV,
    UACPI_PARSE_OP_END,
};

#define POP(x) UACPI_PARSE_OP_##x

static
const uacpi_char *const pop_names[UACPI_PARSE_OP_MAX + 1] = {
    [POP(END)] = "<END-OF-OP>",
    [POP(SKIP_WITH_WARN_IF_NULL)] = "SKIP_WITH_WARN_IF_NULL",
    [POP(EMIT_SKIP_WARN)] = "EMIT_SKIP_WARN",
    [POP(SIMPLE_NAME)] = "SIMPLE_NAME",
    [POP(SUPERNAME)] = "SUPERNAME",
    [POP(SUPERNAME_OR_UNRESOLVED)] = "SUPERNAME_OR_UNRESOLVED",
    [POP(TERM_ARG)] = "TERM_ARG",
    [POP(TERM_ARG_UNWRAP_INTERNAL)] = "TERM_ARG_UNWRAP_INTERNAL",
    [POP(TERM_ARG_OR_NAMED_OBJECT)] = "TERM_ARG_OR_NAMED_OBJECT",
    [POP(TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED)] = "TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED",
    [POP(OPERAND)] = "OPERAND",
    [POP(STRING)] = "STRING",
    [POP(COMPUTATIONAL_DATA)] = "COMPUTATIONAL_DATA",
    [POP(TARGET)] = "TARGET",
    [POP(PKGLEN)] = "PKGLEN",
    [POP(TRACKED_PKGLEN)] = "TRACKED_PKGLEN",
    [POP(CREATE_NAMESTRING)] = "CREATE_NAMESTRING",
    [POP(CREATE_NAMESTRING_OR_NULL_IF_LOAD)] = "CREATE_NAMESTRING_OR_NULL_IF_LOAD",
    [POP(EXISTING_NAMESTRING)] = "EXISTING_NAMESTRING",
    [POP(EXISTING_NAMESTRING_OR_NULL)] = "EXISTING_NAMESTRING_OR_NULL",
    [POP(EXISTING_NAMESTRING_OR_NULL_IF_LOAD)] = "EXISTING_NAMESTRING_OR_NULL_IF_LOAD",
    [POP(INVOKE_HANDLER)] = "INVOKE_HANDLER",
    [POP(OBJECT_ALLOC)] = "OBJECT_ALLOC",
    [POP(EMPTY_OBJECT_ALLOC)] = "EMPTY_OBJECT_ALLOC",
    [POP(OBJECT_CONVERT_TO_SHALLOW_COPY)] = "OBJECT_CONVERT_TO_SHALLOW_COPY",
    [POP(OBJECT_CONVERT_TO_DEEP_COPY)] = "OBJECT_CONVERT_TO_DEEP_COPY",
    [POP(OBJECT_ALLOC_TYPED)] = "OBJECT_ALLOC_TYPED",
    [POP(RECORD_AML_PC)] = "RECORD_AML_PC",
    [POP(LOAD_INLINE_IMM_AS_OBJECT)] = "LOAD_INLINE_IMM_AS_OBJECT",
    [POP(LOAD_INLINE_IMM)] = "LOAD_INLINE_IMM",
    [POP(LOAD_ZERO_IMM)] = "LOAD_ZERO_IMM",
    [POP(LOAD_IMM)] = "LOAD_IMM",
    [POP(LOAD_IMM_AS_OBJECT)] = "LOAD_IMM_AS_OBJECT",
    [POP(LOAD_FALSE_OBJECT)] = "LOAD_FALSE_OBJECT",
    [POP(LOAD_TRUE_OBJECT)] = "LOAD_TRUE_OBJECT",
    [POP(TRUNCATE_NUMBER)] = "TRUNCATE_NUMBER",
    [POP(TYPECHECK)] = "TYPECHECK",
    [POP(INSTALL_NAMESPACE_NODE)] = "INSTALL_NAMESPACE_NODE",
    [POP(OBJECT_TRANSFER_TO_PREV)] = "OBJECT_TRANSFER_TO_PREV",
    [POP(OBJECT_COPY_TO_PREV)] = "OBJECT_COPY_TO_PREV",
    [POP(STORE_TO_TARGET)] = "STORE_TO_TARGET",
    [POP(STORE_TO_TARGET_INDIRECT)] = "STORE_TO_TARGET_INDIRECT",
    [POP(UNREACHABLE)] = "UNREACHABLE",
    [POP(BAD_OPCODE)] = "BAD_OPCODE",
    [POP(AML_PC_DECREMENT)] = "AML_PC_DECREMENT",
    [POP(IMM_DECREMENT)] = "IMM_DECREMENT",
    [POP(ITEM_POP)] = "ITEM_POP",
    [POP(DISPATCH_METHOD_CALL)] = "DISPATCH_METHOD_CALL",
    [POP(DISPATCH_TABLE_LOAD)] = "DISPATCH_TABLE_LOAD",
    [POP(CONVERT_NAMESTRING)] = "CONVERT_NAMESTRING",
    [POP(IF_HAS_DATA)] = "IF_HAS_DATA",
    [POP(IF_NULL)] = "IF_NULL",
    [POP(IF_LAST_NULL)] = "IF_LAST_NULL",
    [POP(IF_NOT_NULL)] = "IF_NOT_NULL",
    [POP(IF_LAST_NOT_NULL)] = "IF_NOT_NULL",
    [POP(IF_LAST_EQUALS)] = "IF_LAST_EQUALS",
    [POP(IF_LAST_FALSE)] = "IF_LAST_FALSE",
    [POP(IF_LAST_TRUE)] = "IF_LAST_TRUE",
    [POP(SWITCH_TO_NEXT_IF_EQUALS)] = "SWITCH_TO_NEXT_IF_EQUALS",
    [POP(IF_SWITCHED_FROM)] = "IF_SWITCHED_FROM",
    [POP(JMP)] = "JMP",
};

const uacpi_char *uacpi_parse_op_to_string(enum uacpi_parse_op op)
{
    if (uacpi_unlikely(op > UACPI_PARSE_OP_MAX))
        return "<INVALID-OP>";

    return pop_names[op];
}

#endif // !UACPI_BAREBONES_MODE
