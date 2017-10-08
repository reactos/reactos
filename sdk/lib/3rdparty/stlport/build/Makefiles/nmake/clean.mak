# -*- makefile -*- Time-stamp: <03/10/29 22:20:01 ptr>
# $Id$

clean:
	@if exist $(OUTPUT_DIR)\*.o del /F /Q $(OUTPUT_DIR)\*.o
	@if exist $(OUTPUT_DIR_DBG)\*.o del /F /Q $(OUTPUT_DIR_DBG)\*.o
	@if exist $(OUTPUT_DIR_STLDBG)\*.o del /F /Q $(OUTPUT_DIR_STLDBG)\*.o
	@if exist $(OUTPUT_DIR_A)\*.o del /F /Q $(OUTPUT_DIR_A)\*.o
	@if exist $(OUTPUT_DIR_A_DBG)\*.o del /F /Q $(OUTPUT_DIR_A_DBG)\*.o
	@if exist $(OUTPUT_DIR_A_STLDBG)\*.o del /F /Q $(OUTPUT_DIR_A_STLDBG)\*.o
	@if exist $(OUTPUT_DIR)\*.obj del /F /Q $(OUTPUT_DIR)\*.obj
	@if exist $(OUTPUT_DIR_DBG)\*.obj del /F /Q $(OUTPUT_DIR_DBG)\*.obj
	@if exist $(OUTPUT_DIR_STLDBG)\*.obj del /F /Q $(OUTPUT_DIR_STLDBG)\*.obj
	@if exist $(OUTPUT_DIR_A)\*.obj del /F /Q $(OUTPUT_DIR_A)\*.obj
	@if exist $(OUTPUT_DIR_A_DBG)\*.obj del /F /Q $(OUTPUT_DIR_A_DBG)\*.obj
	@if exist $(OUTPUT_DIR_A_STLDBG)\*.obj del /F /Q $(OUTPUT_DIR_A_STLDBG)\*.obj
