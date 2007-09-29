<module name="poppler" type="staticlibrary" stdlib="host" allowwarnings="true">
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
	<define name="USE_OWN_GET_AUTH_DATA" />
	<define name="POPPLER_DATADIR">\"\"</define>
	<define name="ENABLE_ZLIB">1</define>
	<define name="ENABLE_LIBJPEG">1</define>
	<define name="__REACTOS__" />
	<define name="USE_GCC_PRAGMAS" />
 	<include base="libjpeg">.</include>
 	<include base="zlib">.</include>
 	<include base="freetype">include</include>
 	<include base="poppler">.</include>
 	<include>src</include>
 	<include>baseutils</include>
 	<include>poppler</include>
 	<include>poppler/goo</include>
 	<include>poppler/fofi</include>
 	<include>poppler/splash</include>
 	<include>poppler/poppler</include>
	<directory name="poppler">
		<directory name="poppler">
 			<file>Annot.cc</file>
 			<file>Array.cc</file>
 			<file>BuiltinFont.cc</file>
 			<file>BuiltinFontTables.cc</file>
 			<file>Catalog.cc</file>
 			<file>CharCodeToUnicode.cc</file>
 			<file>CMap.cc</file>
 			<file>DCTStream.cc</file>
 			<file>Decrypt.cc</file>
 			<file>Dict.cc</file>
 			<file>FlateStream.cc</file>
			<file>FontEncodingTables.cc</file>
			<file>Function.cc</file>
			<file>Gfx.cc</file>
			<file>GfxFont.cc</file>
			<file>GfxState.cc</file>
			<file>GlobalParams.cc</file>
			<file>GlobalParamsWin.cc</file>
			<file>JArithmeticDecoder.cc</file>
			<file>JBIG2Stream.cc</file>
			<file>JPXStream.cc</file>
			<file>Lexer.cc</file>
			<file>Link.cc</file>
			<file>NameToCharCode.cc</file>
			<file>Object.cc</file>
			<file>Outline.cc</file>
			<file>OutputDev.cc</file>
			<file>Page.cc</file>
			<file>PageLabelInfo.cc</file>
			<file>Parser.cc</file>
			<file>PDFDoc.cc</file>
			<file>PDFDocEncoding.cc</file>
			<file>PSTokenizer.cc</file>
			<file>SecurityHandler.cc</file>
			<file>Sound.cc</file>
			<file>SplashOutputDev.cc</file>
			<file>Stream.cc</file>
			<file>TextOutputDev.cc</file>
			<file>UGooString.cc</file>
			<file>UnicodeMap.cc</file>
			<file>UnicodeTypeTable.cc</file>
			<file>XpdfPluginAPI.cc</file>
			<file>XRef.cc</file>
		</directory>
		<directory name="goo">
		 	<file>FastAlloc.cc</file>
 			<file>FastFixedAllocator.cc</file>
 			<file>FixedPoint.cc</file>
			<file>gfile.cc</file>
			<file>gmem.c</file>
			<file>gmempp.cc</file>
			<file>GooHash.cc</file>
			<file>GooList.cc</file>
			<file>GooString.cc</file>
			<file>GooTimer.cc</file>
		</directory>
		<directory name="fofi">
 			<file>FoFiBase.cc</file>
 			<file>FoFiEncodings.cc</file>
 			<file>FoFiTrueType.cc</file>
 			<file>FoFiType1.cc</file>
			<file>FoFiType1C.cc</file>
		</directory>
		<directory name="splash">
			<file>Splash.cc</file>
			<file>SplashBitmap.cc</file>
			<file>SplashClip.cc</file>
			<file>SplashFont.cc</file>
			<file>SplashFontEngine.cc</file>
			<file>SplashFontFile.cc</file>
			<file>SplashFontFileID.cc</file>
			<file>SplashFTFont.cc</file>
			<file>SplashFTFontEngine.cc</file>
			<file>SplashFTFontFile.cc</file>
			<file>SplashPath.cc</file>
			<file>SplashPattern.cc</file>
			<file>SplashScreen.cc</file>
			<file>SplashState.cc</file>
			<file>SplashXPath.cc</file>
			<file>SplashXPathScanner.cc</file>
		</directory>
	</directory>
</module>
