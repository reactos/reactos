#include "../gdi32api.h"

INT
Test_GetStockObject(PTESTINFO pti)
{
	/* Test limits and error */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetStockObject(0) != NULL);
	TEST(GetStockObject(21) != NULL);
	RTEST(GetStockObject(-1) == NULL);
	RTEST(GetStockObject(9) == NULL);
	RTEST(GetStockObject(22) == NULL);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test for the stock bit */
	RTEST((UINT)GetStockObject(WHITE_BRUSH) && GDI_HANDLE_STOCK_MASK);

	/* Test for correct types */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(WHITE_BRUSH)) == GDI_OBJECT_TYPE_BRUSH);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(DC_BRUSH)) == GDI_OBJECT_TYPE_BRUSH);
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(WHITE_PEN)) == GDI_OBJECT_TYPE_PEN);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(DC_PEN)) == GDI_OBJECT_TYPE_PEN);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(ANSI_VAR_FONT)) == GDI_OBJECT_TYPE_FONT);
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(DEFAULT_PALETTE)) == GDI_OBJECT_TYPE_PALETTE);

	return APISTATUS_NORMAL;
}
