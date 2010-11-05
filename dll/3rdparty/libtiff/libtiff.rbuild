<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="libtiff" type="win32dll" entrypoint="0" installbase="system32" installname="libtiff.dll" allowwarnings="true" crt="msvcrt">
	<define name="WIN32" />
	<define name="NDEBUG" />
	<define name="BUILD_LIBTIFF_DLL" />
	<define name="DLL_EXPORT" />
	<define name="USE_WIN32_FILEIO" />
	<include base="libtiff">.</include>
	<include base="ReactOS">include/reactos/libs/zlib</include>
	<include base="ReactOS">include/reactos/libs/libtiff</include>
	<library>getopt</library>
	<library>user32</library>
	<library>zlib</library>
	<file>mkg3states.c</file>
	<file>tif_aux.c</file>
	<file>tif_close.c</file>
	<file>tif_codec.c</file>
	<file>tif_color.c</file>
	<file>tif_compress.c</file>
	<file>tif_dir.c</file>
	<file>tif_dirinfo.c</file>
	<file>tif_dirread.c</file>
	<file>tif_dirwrite.c</file>
	<file>tif_dumpmode.c</file>
	<file>tif_error.c</file>
	<file>tif_extension.c</file>
	<file>tif_fax3.c</file>
	<file>tif_fax3sm.c</file>
	<file>tif_flush.c</file>
	<file>tif_getimage.c</file>
	<file>tif_jbig.c</file>
	<file>tif_jpeg.c</file>
	<file>tif_luv.c</file>
	<file>tif_lzw.c</file>
	<file>tif_next.c</file>
	<file>tif_ojpeg.c</file>
	<file>tif_open.c</file>
	<file>tif_packbits.c</file>
	<file>tif_pixarlog.c</file>
	<file>tif_predict.c</file>
	<file>tif_print.c</file>
	<file>tif_read.c</file>
	<file>tif_strip.c</file>
	<file>tif_swab.c</file>
	<file>tif_thunder.c</file>
	<file>tif_tile.c</file>
	<file>tif_version.c</file>
	<file>tif_warning.c</file>
	<file>tif_win32.c</file>
	<file>tif_write.c</file>
	<file>tif_zip.c</file>
</module>
