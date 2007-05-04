#line 2 "GetStockObject.c"

#include "../gditest.h"

BOOL Test_GetStockObject(INT* passed, INT* failed)
{
	/* Test limits and error */
	SetLastError(ERROR_SUCCESS);
	TEST(GetStockObject(0) != NULL);
	TEST(GetStockObject(21) != NULL);
	TEST(GetStockObject(-1) == NULL);
	TEST(GetStockObject(9) == NULL);
	TEST(GetStockObject(22) == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test for the stock bit */
	TEST((UINT)GetStockObject(WHITE_BRUSH) && GDI_HANDLE_STOCK_MASK);

	/* Test for correct types */
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(WHITE_BRUSH)) == GDI_OBJECT_TYPE_BRUSH);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(DC_BRUSH)) == GDI_OBJECT_TYPE_BRUSH);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(WHITE_PEN)) == GDI_OBJECT_TYPE_PEN);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(DC_PEN)) == GDI_OBJECT_TYPE_PEN);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(ANSI_VAR_FONT)) == GDI_OBJECT_TYPE_FONT);
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(DEFAULT_PALETTE)) == GDI_OBJECT_TYPE_PALETTE);

	return TRUE;
}
