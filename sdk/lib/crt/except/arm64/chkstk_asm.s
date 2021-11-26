
/* INCLUDES ******************************************************************/

/* We need one of these first! */
/* #include <kxarm64.h> */

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY __chkstk
    /* TODO: add an assert fail call, as this is unimplemented */
    LEAF_END __chkstk

    LEAF_ENTRY __alloca_probe
    /* TODO: add an assert fail call, as this is unimplemented */
    LEAF_END __alloca_probe

    END
/* EOF */
