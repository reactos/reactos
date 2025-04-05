#include <uacpi/types.h>

/*
 * BEFORE YOU USE THIS API:
 * uACPI manages FADT registers on its own entirely, you should only use this
 * API directly if there's absolutely no other way for your use case, e.g.
 * implementing a CPU idle state driver that does C state switching or similar.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

typedef enum uacpi_register {
    UACPI_REGISTER_PM1_STS = 0,
    UACPI_REGISTER_PM1_EN,
    UACPI_REGISTER_PM1_CNT,
    UACPI_REGISTER_PM_TMR,
    UACPI_REGISTER_PM2_CNT,
    UACPI_REGISTER_SLP_CNT,
    UACPI_REGISTER_SLP_STS,
    UACPI_REGISTER_RESET,
    UACPI_REGISTER_SMI_CMD,
    UACPI_REGISTER_MAX = UACPI_REGISTER_SMI_CMD,
} uacpi_register;

/*
 * Read a register from FADT
 *
 * NOTE: write-only bits (if any) are cleared automatically
 */
uacpi_status uacpi_read_register(uacpi_register, uacpi_u64*);

/*
 * Write a register from FADT
 *
 * NOTE:
 * - Preserved bits (if any) are preserved automatically
 * - If a register is made up of two (e.g. PM1a and PM1b) parts, the input
 *   is written to both at the same time
 */
uacpi_status uacpi_write_register(uacpi_register, uacpi_u64);

/*
 * Write a register from FADT
 *
 * NOTE:
 * - Preserved bits (if any) are preserved automatically
 * - For registers that are made up of two (e.g. PM1a and PM1b) parts, the
 *   provided values are written to their respective physical register
 */
uacpi_status uacpi_write_registers(uacpi_register, uacpi_u64, uacpi_u64);

typedef enum uacpi_register_field {
    UACPI_REGISTER_FIELD_TMR_STS = 0,
    UACPI_REGISTER_FIELD_BM_STS,
    UACPI_REGISTER_FIELD_GBL_STS,
    UACPI_REGISTER_FIELD_PWRBTN_STS,
    UACPI_REGISTER_FIELD_SLPBTN_STS,
    UACPI_REGISTER_FIELD_RTC_STS,
    UACPI_REGISTER_FIELD_PCIEX_WAKE_STS,
    UACPI_REGISTER_FIELD_HWR_WAK_STS,
    UACPI_REGISTER_FIELD_WAK_STS,
    UACPI_REGISTER_FIELD_TMR_EN,
    UACPI_REGISTER_FIELD_GBL_EN,
    UACPI_REGISTER_FIELD_PWRBTN_EN,
    UACPI_REGISTER_FIELD_SLPBTN_EN,
    UACPI_REGISTER_FIELD_RTC_EN,
    UACPI_REGISTER_FIELD_PCIEXP_WAKE_DIS,
    UACPI_REGISTER_FIELD_SCI_EN,
    UACPI_REGISTER_FIELD_BM_RLD,
    UACPI_REGISTER_FIELD_GBL_RLS,
    UACPI_REGISTER_FIELD_SLP_TYP,
    UACPI_REGISTER_FIELD_HWR_SLP_TYP,
    UACPI_REGISTER_FIELD_SLP_EN,
    UACPI_REGISTER_FIELD_HWR_SLP_EN,
    UACPI_REGISTER_FIELD_ARB_DIS,
    UACPI_REGISTER_FIELD_MAX = UACPI_REGISTER_FIELD_ARB_DIS,
} uacpi_register_field;

/*
 * Read a field from a FADT register
 *
 * NOTE: The value is automatically masked and shifted down as appropriate,
 *       the client code doesn't have to do any bit manipulation. E.g. for
 *       a field at 0b???XX??? the returned value will contain just the 0bXX
 */
uacpi_status uacpi_read_register_field(uacpi_register_field, uacpi_u64*);

/*
 * Write to a field of a FADT register
 *
 * NOTE: The value is automatically masked and shifted up as appropriate,
 *       the client code doesn't have to do any bit manipulation. E.g. for
 *       a field at 0b???XX??? the passed value should be just 0bXX
 */
uacpi_status uacpi_write_register_field(uacpi_register_field, uacpi_u64);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
