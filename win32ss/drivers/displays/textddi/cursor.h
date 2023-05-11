typedef struct tagCURSOR
{
    BOOLEAN Visible;
    DWORD x, y;
} CURSOR;

FORCEINLINE
VOID
CURSOR_vInit(
    OUT CURSOR *pcur)
{
}

FORCEINLINE
VOID
CURSOR_Paint(
    IN CURSOR *pcur)
{
    return;
    if (pcur->Visible)
        DPRINT("CURSOR_Paint: x=%u y=%u\n", pcur->x, pcur->y);
    else
        DPRINT("CURSOR_Paint: hide cursor\n");
}

FORCEINLINE
VOID
CURSOR_SetVisible(
    IN CURSOR *pcur,
    IN BOOLEAN bVisible)
{
    if (pcur->Visible ^ bVisible)
    {
        pcur->Visible = bVisible;
        CURSOR_Paint(pcur);
    }
}

FORCEINLINE
VOID
CURSOR_SetPosition(
    IN CURSOR *pcur,
    IN DWORD x,
    IN DWORD y)
{
    if (pcur->Visible && (x != pcur->x || y != pcur->y))
    {
        pcur->x = x;
        pcur->y = y;
        CURSOR_Paint(pcur);
    }
}
