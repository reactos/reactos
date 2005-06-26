/*
 * Definitions for the offsets of members in the KV86M_REGISTERS
 */
#define	KV86M_REGISTERS_EBP	(0x0)
#define	KV86M_REGISTERS_EDI	(0x4)
#define	KV86M_REGISTERS_ESI	(0x8)
#define KV86M_REGISTERS_EDX	(0xC)
#define	KV86M_REGISTERS_ECX	(0x10)
#define KV86M_REGISTERS_EBX	(0x14)
#define KV86M_REGISTERS_EAX	(0x18)
#define	KV86M_REGISTERS_DS	(0x1C)
#define KV86M_REGISTERS_ES	(0x20)
#define KV86M_REGISTERS_FS	(0x24)
#define KV86M_REGISTERS_GS	(0x28)
#define KV86M_REGISTERS_EIP     (0x2C)
#define KV86M_REGISTERS_CS      (0x30)
#define KV86M_REGISTERS_EFLAGS  (0x34)
#define	KV86M_REGISTERS_ESP     (0x38)
#define KV86M_REGISTERS_SS	(0x3C)

#define TF_SAVED_EXCEPTION_STACK (0x8C)
#define TF_REGS                  (0x90)
#define TF_ORIG_EBP              (0x94)


/* TSS Offsets */
#define KTSS_ESP0      (0x4)
#define KTSS_CR3       (0x1C)
#define KTSS_EFLAGS    (0x24)
#define KTSS_IOMAPBASE (0x66)

/*
 * Defines for accessing KPCR and KTHREAD structure members
 */
#define KTHREAD_INITIAL_STACK     0x18
#define KTHREAD_STACK_LIMIT       0x1C
#define KTHREAD_TEB               0x20
#define KTHREAD_KERNEL_STACK      0x28
#define KTHREAD_NPX_STATE         0x31
#define KTHREAD_STATE             0x2D
#define KTHREAD_APCSTATE_PROCESS  0x34 + 0x10
#define KTHREAD_PENDING_USER_APC  0x34 + 0x16
#define KTHREAD_PENDING_KERNEL_APC 0x34 + 0x15
#define KTHREAD_CONTEXT_SWITCHES  0x4C
#define KTHREAD_WAIT_IRQL         0x54
#define KTHREAD_SERVICE_TABLE     0xDC
#define KTHREAD_PREVIOUS_MODE     0x137
#define KTHREAD_TRAP_FRAME        0x128
#define KTHREAD_CALLBACK_STACK    0x120

#define KPROCESS_DIRECTORY_TABLE_BASE 0x18
#define KPROCESS_LDT_DESCRIPTOR0      0x20
#define KPROCESS_LDT_DESCRIPTOR1      0x24
#define KPROCESS_IOPM_OFFSET          0x30

#define KPCR_EXCEPTION_LIST       0x0
#define KPCR_INITIAL_STACK        0x4
#define KPCR_STACK_LIMIT          0x8
#define KPCR_SELF                 0x1C
#define KPCR_GDT                  0x3C
#define KPCR_TSS                  0x40
#define KPCR_CURRENT_THREAD       0x124
#define KPCR_NPX_THREAD           0x2A4

/* FPU Save Area Offsets */
#define FN_CONTROL_WORD        0x0
#define FN_STATUS_WORD         0x4
#define FN_TAG_WORD            0x8
#define FN_DATA_SELECTOR       0x18
#define FN_CR0_NPX_STATE       0x20C
#define SIZEOF_FX_SAVE_AREA    528

/* Trap Frame Offsets */
#define KTRAP_FRAME_DEBUGEBP     (0x0)
#define KTRAP_FRAME_DEBUGEIP     (0x4)
#define KTRAP_FRAME_DEBUGARGMARK (0x8)
#define KTRAP_FRAME_DEBUGPOINTER (0xC)
#define KTRAP_FRAME_TEMPSS       (0x10)
#define KTRAP_FRAME_TEMPESP      (0x14)
#define KTRAP_FRAME_DR0          (0x18)
#define KTRAP_FRAME_DR1          (0x1C)
#define KTRAP_FRAME_DR2          (0x20)
#define KTRAP_FRAME_DR3          (0x24)
#define KTRAP_FRAME_DR6          (0x28)
#define KTRAP_FRAME_DR7          (0x2C)
#define KTRAP_FRAME_GS           (0x30)
#define KTRAP_FRAME_RESERVED1    (0x32)
#define KTRAP_FRAME_ES           (0x34)
#define KTRAP_FRAME_RESERVED2    (0x36)
#define KTRAP_FRAME_DS           (0x38)
#define KTRAP_FRAME_RESERVED3    (0x3A)
#define KTRAP_FRAME_EDX          (0x3C)
#define KTRAP_FRAME_ECX          (0x40)
#define KTRAP_FRAME_EAX          (0x44)
#define KTRAP_FRAME_PREVIOUS_MODE (0x48)
#define KTRAP_FRAME_EXCEPTION_LIST (0x4C)
#define KTRAP_FRAME_FS             (0x50)
#define KTRAP_FRAME_RESERVED4      (0x52)
#define KTRAP_FRAME_EDI            (0x54)
#define KTRAP_FRAME_ESI            (0x58)
#define KTRAP_FRAME_EBX            (0x5C)
#define KTRAP_FRAME_EBP            (0x60)
#define KTRAP_FRAME_ERROR_CODE     (0x64)
#define KTRAP_FRAME_EIP            (0x68)
#define KTRAP_FRAME_CS             (0x6C)
#define KTRAP_FRAME_EFLAGS         (0x70)
#define KTRAP_FRAME_ESP            (0x74)
#define KTRAP_FRAME_SS             (0x78)
#define KTRAP_FRAME_RESERVED5      (0x7A)
#define KTRAP_FRAME_V86_ES         (0x7C)
#define KTRAP_FRAME_RESERVED6      (0x7E)
#define KTRAP_FRAME_V86_DS         (0x80)
#define KTRAP_FRAME_RESERVED7      (0x82)
#define KTRAP_FRAME_V86_FS         (0x84)
#define KTRAP_FRAME_RESERVED8      (0x86)
#define KTRAP_FRAME_V86_GS         (0x88)
#define KTRAP_FRAME_RESERVED9      (0x8A)
#define KTRAP_FRAME_SIZE           (0x8C)

/* User Shared Data */
#define KUSER_SHARED_SYSCALL       0x7FFE0300
#define KUSER_SHARED_SYSCALL_RET   0x7FFE0304
