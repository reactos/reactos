TBBUTTON StdButtons[] = {
    /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
    {TBICON_NEW,      ID_NEW,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* new */
    {TBICON_OPEN,     ID_OPEN,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* open */
    {TBICON_SAVE,     ID_SAVE,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* save */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_PRINT,    ID_PRINTPRE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* print */
    {TBICON_PRINTPRE, ID_PRINT,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* print preview */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_CUT,      ID_CUT,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* cut */
    {TBICON_COPY,     ID_COPY,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* copy */
    {TBICON_PASTE,    ID_PASTE,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* paste */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_UNDO,     ID_UNDO,    TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* undo */
    {TBICON_REDO,     ID_REDO,    TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* redo */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
};
