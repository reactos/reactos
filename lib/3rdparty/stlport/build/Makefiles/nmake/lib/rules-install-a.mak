# Time-stamp: <03/10/17 19:29:26 ptr>
# $Id$

install-static: install-release-static install-dbg-static install-stldbg-static

install-release-static: release-static $(INSTALL_STATIC_LIB_DIR)
	$(INSTALL_A) $(A_NAME_OUT) $(INSTALL_STATIC_LIB_DIR)
	@if exist $(A_PDB_NAME_OUT) $(INSTALL_A) $(A_PDB_NAME_OUT) $(INSTALL_STATIC_LIB_DIR)

install-dbg-static: dbg-static $(INSTALL_STATIC_LIB_DIR_DBG)
	$(INSTALL_A) $(A_NAME_OUT_DBG) $(INSTALL_STATIC_LIB_DIR_DBG)
	@if exist $(A_PDB_NAME_OUT_DBG) $(INSTALL_A) $(A_PDB_NAME_OUT_DBG) $(INSTALL_STATIC_LIB_DIR_DBG)

install-stldbg-static: stldbg-static $(INSTALL_STATIC_LIB_DIR_STLDBG)
	$(INSTALL_A) $(A_NAME_OUT_STLDBG) $(INSTALL_STATIC_LIB_DIR_STLDBG)
	@if exist $(A_PDB_NAME_OUT_STLDBG) $(INSTALL_A) $(A_PDB_NAME_OUT_STLDBG) $(INSTALL_STATIC_LIB_DIR_STLDBG)

