<module name="fitz" type="staticlibrary" stdlib="host" allowwarnings="true">
 	<library>ntdll</library>
 	<library>kernel32</library>
 	<library>libjpeg</library>
 	<library>zlib</library>
 	<library>freetype</library>
 	<define name="HAVE_CONFIG_H" />
 	<define name="__USE_W32API" />
 	<define name="WIN32" />
 	<define name="_WIN32" />
 	<define name="_WINDOWS" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_DEBUG" />
	<define name="DEBUG" />
	<define name="NEED_MATH">1</define>
	<define name="NEED_STRLCPY">1</define>
	<define name="NEED_STRSEP">1</define>
	<define name="inline"></define>
	<define name="__REACTOS__" />
	<define name="USE_GCC_PRAGMAS" />
 	<include base="libjpeg">.</include>
 	<include base="zlib">.</include>
 	<include base="freetype">include</include>
 	<include base="fitz">.</include>
 	<include>baseutils</include>
 	<include>fitz</include>
 	<include>fitz/fonts</include>
 	<include>fitz/include</include>
 	<include>fitz/include/fitz</include>
 	<include>fitz/include/mupdf</include>
 	<include>fitz/include/samus</include>
 	<include>fitz/base</include>
 	<include>fitz/stream</include>
 	<include>fitz/raster</include>
 	<include>fitz/world</include>
 	<include>fitz/mupdf</include>
	<directory name="fitz">
		<directory name="fonts">
 			<file>Dingbats.cff.c</file>
 			<file>NimbusMonL-Bold.cff.c</file>
 			<file>NimbusMonL-BoldObli.cff.c</file>
 			<file>NimbusMonL-Regu.cff.c</file>
 			<file>NimbusMonL-ReguObli.cff.c</file>
 			<file>NimbusRomNo9L-Medi.cff.c</file>
 			<file>NimbusRomNo9L-MediItal.cff.c</file>
 			<file>NimbusRomNo9L-Regu.cff.c</file>
 			<file>NimbusRomNo9L-ReguItal.cff.c</file>
 			<file>NimbusSanL-Bold.cff.c</file>
 			<file>NimbusSanL-BoldItal.cff.c</file>
			<file>NimbusSanL-Regu.cff.c</file>
			<file>NimbusSanL-ReguItal.cff.c</file>
			<file>StandardSymL.cff.c</file>
			<file>URWChanceryL-MediItal.cff.c</file>
		</directory>
		<directory name="base">
			<file>base_cpudep.c</file>
			<file>base_error.c</file>
			<file>base_hash.c</file>
			<file>base_matrix.c</file>
			<file>base_memory.c</file>
			<file>base_rect.c</file>
			<file>base_rune.c</file>
			<file>util_getopt.c</file>
			<file>util_strlcat.c</file>
			<file>util_strlcpy.c</file>
			<file>util_strsep.c</file>
		</directory>
		<directory name="stream">
			<file>crypt_arc4.c</file>
			<file>crypt_crc32.c</file>
			<file>crypt_md5.c</file>
			<file>filt_a85d.c</file>
			<file>filt_a85e.c</file>
			<file>filt_ahxd.c</file>
			<file>filt_ahxe.c</file>
			<file>filt_arc4.c</file>
			<file>filt_dctd.c</file>
			<file>filt_dcte.c</file>
			<file>filt_faxd.c</file>
			<file>filt_faxdtab.c</file>
			<file>filt_faxe.c</file>
			<file>filt_faxetab.c</file>
			<file>filt_flate.c</file>
			<file>filt_lzwd.c</file>
			<file>filt_lzwe.c</file>
			<file>filt_null.c</file>
			<file>filt_pipeline.c</file>
			<file>filt_predict.c</file>
			<file>filt_rld.c</file>
			<file>filt_rle.c</file>
			<file>obj_array.c</file>
			<file>obj_dict.c</file>
			<file>obj_parse.c</file>
			<file>obj_print.c</file>
			<file>obj_simple.c</file>
			<file>stm_buffer.c</file>
			<file>stm_filter.c</file>
			<file>stm_misc.c</file>
			<file>stm_open.c</file>
			<file>stm_read.c</file>
			<file>stm_write.c</file>
		</directory>
		<directory name="raster">
			<file>glyphcache.c</file>
			<file>imagedraw.c</file>
			<file>imagescale.c</file>
			<file>imageunpack.c</file>
			<file>meshdraw.c</file>
			<file>pathfill.c</file>
			<file>pathscan.c</file>
			<file>pathstroke.c</file>
			<file>pixmap.c</file>
			<file>porterduff.c</file>
			<file>render.c</file>
		</directory>
		<directory name="world">
			<file>node_misc1.c</file>
			<file>node_misc2.c</file>
			<file>node_optimize.c</file>
			<file>node_path.c</file>
			<file>node_text.c</file>
			<file>node_tree.c</file>
			<file>res_colorspace.c</file>
			<file>res_font.c</file>
			<file>res_image.c</file>
			<file>res_shade.c</file>
		</directory>
		<directory name="mupdf">
			<file>pdf_annot.c</file>
			<file>pdf_build.c</file>
			<file>pdf_cmap.c</file>
			<file>pdf_colorspace1.c</file>
			<file>pdf_colorspace2.c</file>
			<file>pdf_crypt.c</file>
			<file>pdf_debug.c</file>
			<file>pdf_doctor.c</file>
			<file>pdf_font.c</file>
			<file>pdf_fontagl.c</file>
			<file>pdf_fontenc.c</file>
			<file>pdf_fontfile.c</file>
			<file>pdf_function.c</file>
			<file>pdf_image.c</file>
			<file>pdf_interpret.c</file>
			<file>pdf_lex.c</file>
			<file>pdf_nametree.c</file>
			<file>pdf_open.c</file>
			<file>pdf_outline.c</file>
			<file>pdf_page.c</file>
			<file>pdf_pagetree.c</file>
			<file>pdf_parse.c</file>
			<file>pdf_pattern.c</file>
			<file>pdf_repair.c</file>
			<file>pdf_resources.c</file>
			<file>pdf_save.c</file>
			<file>pdf_shade.c</file>
			<file>pdf_shade1.c</file>
			<file>pdf_shade4.c</file>
			<file>pdf_store.c</file>
			<file>pdf_stream.c</file>
			<file>pdf_type3.c</file>
			<file>pdf_unicode.c</file>
			<file>pdf_xobject.c</file>
			<file>pdf_xref.c</file>
		</directory>
	</directory>
</module>
