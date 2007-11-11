<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="crt" type="staticlibrary">
	<include base="crt">.</include>
	<include base="crt">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__MINGW_IMPORT">extern</define>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="USE_MSVCRT_PREFIX" />
	<define name="_MSVCRT_LIB_" />
	<define name="_MSVCRT_" />
	<define name="_MT" />
	<define name="__NO_CTYPE_INLINES" />
	<pch>precomp.h</pch>
	<directory name="conio">
		<file>cgets.c</file>
		<file>cprintf.c</file>
		<file>cputs.c</file>
		<file>getch.c</file>
		<file>getche.c</file>
		<file>kbhit.c</file>
		<file>putch.c</file>
		<file>ungetch.c</file>
	</directory>
	<directory name="direct">
		<file>chdir.c</file>
		<file>chdrive.c</file>
		<file>getcwd.c</file>
		<file>getdcwd.c</file>
		<file>getdfree.c</file>
		<file>getdrive.c</file>
		<file>mkdir.c</file>
		<file>rmdir.c</file>
		<file>wchdir.c</file>
		<file>wgetcwd.c</file>
		<file>wgetdcwd.c</file>
		<file>wmkdir.c</file>
		<file>wrmdir.c</file>
	</directory>
	<directory name="except">
		<file>abnorter.c</file>
		<file>cpp.c</file>
		<file>cppexcept.c</file>
		<file>matherr.c</file>
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>prolog.s</file>
				<file>seh.s</file>
				<file>unwind.c</file>
			</directory>
		</if>
		<if property="ARCH" value="powerpc">
			<directory name="powerpc">
				<file>seh.s</file>
				<file>chkstk_asm.s</file>
			</directory>
		</if>
		<file>xcptfil.c</file>
	</directory>
	<directory name="float">
		<file>chgsign.c</file>
		<file>copysign.c</file>
		<file>fpclass.c</file>
		<file>fpecode.c</file>
		<file>fpreset.c</file>
		<file>isnan.c</file>
		<file>nafter.c</file>
		<file>scalb.c</file>
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>clearfp.c</file>
				<file>cntrlfp.c</file>
				<file>logb.c</file>
				<file>statfp.c</file>
			</directory>
		</if>
	</directory>
	<directory name="io">
		<file>access.c</file>
		<file>chmod.c</file>
		<file>chsize.c</file>
		<file>close.c</file>
		<file>commit.c</file>
		<file>create.c</file>
		<file>dup.c</file>
		<file>dup2.c</file>
		<file>eof.c</file>
		<file>filelen.c</file>
		<file>fileleni.c</file>
		<file>find.c</file>
		<file>fmode.c</file>
		<file>isatty.c</file>
		<file>locking.c</file>
		<file>lseek.c</file>
		<file>lseeki64.c</file>
		<file>mktemp.c</file>
		<file>open.c</file>
		<file>pipe.c</file>
		<file>read.c</file>
		<file>setmode.c</file>
		<file>sopen.c</file>
		<file>stubs.c</file>
		<file>tell.c</file>
		<file>telli64.c</file>
		<file>umask.c</file>
		<file>unlink.c</file>
		<file>utime.c</file>
		<file>waccess.c</file>
		<file>wchmod.c</file>
		<file>wcreate.c</file>
		<file>wfind.c</file>
		<file>wmktemp.c</file>
		<file>wopen.c</file>
		<file>write.c</file>
		<file>wunlink.c</file>
		<file>wutime.c</file>
	</directory>
	<directory name="locale">
		<file>locale.c</file>
	</directory>
	<directory name="math">
		<file>acos.c</file>
		<file>adjust.c</file>
		<file>asin.c</file>
		<file>cabs.c</file>
		<file>cosh.c</file>
		<file>div.c</file>
		<file>frexp.c</file>
		<file>huge_val.c</file>
		<file>hypot.c</file>
		<file>j0_y0.c</file>
		<file>j1_y1.c</file>
		<file>jn_yn.c</file>
		<file>ldiv.c</file>
		<file>modf.c</file>
		<file>rand.c</file>
		<file>s_modf.c</file>
		<file>sinh.c</file>
		<file>stubs.c</file>
		<file>tanh.c</file>
		<file>pow_asm.c</file>

		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>atan2.c</file>
				<file>exp.c</file>
				<file>fmod.c</file>
				<file>ldexp.c</file>
				<file>atan_asm.s</file>
				<file>pow_asm.s</file>
				<file>log10_asm.s</file>
			</directory>
		</if>
		<ifnot property="ARCH" value="i386">
		</ifnot>
	</directory>

	<directory name="mbstring">
		<file>hanzen.c</file>
		<file>ischira.c</file>
		<file>iskana.c</file>
		<file>iskmoji.c</file>
		<file>iskpun.c</file>
		<file>islead.c</file>
		<file>islwr.c</file>
		<file>ismbal.c</file>
		<file>ismbaln.c</file>
		<file>ismbc.c</file>
		<file>ismbgra.c</file>
		<file>ismbkaln.c</file>
		<file>ismblead.c</file>
		<file>ismbpri.c</file>
		<file>ismbpun.c</file>
		<file>ismbtrl.c</file>
		<file>isuppr.c</file>
		<file>jistojms.c</file>
		<file>jmstojis.c</file>
		<file>mbbtype.c</file>
		<file>mbccpy.c</file>
		<file>mbclen.c</file>
		<file>mbscat.c</file>
		<file>mbschr.c</file>
		<file>mbscmp.c</file>
		<file>mbscoll.c</file>
		<file>mbscpy.c</file>
		<file>mbscspn.c</file>
		<file>mbsdec.c</file>
		<file>mbsdup.c</file>
		<file>mbsicmp.c</file>
		<file>mbsicoll.c</file>
		<file>mbsinc.c</file>
		<file>mbslen.c</file>
		<file>mbslwr.c</file>
		<file>mbsncat.c</file>
		<file>mbsnccnt.c</file>
		<file>mbsncmp.c</file>
		<file>mbsncoll.c</file>
		<file>mbsncpy.c</file>
		<file>mbsnextc.c</file>
		<file>mbsnicmp.c</file>
		<file>mbsnicoll.c</file>
		<file>mbsninc.c</file>
		<file>mbsnset.c</file>
		<file>mbspbrk.c</file>
		<file>mbsrchr.c</file>
		<file>mbsrev.c</file>
		<file>mbsset.c</file>
		<file>mbsspn.c</file>
		<file>mbsspnp.c</file>
		<file>mbsstr.c</file>
		<file>mbstok.c</file>
		<file>mbstrlen.c</file>
		<file>mbsupr.c</file>
	</directory>

	<directory name="mem">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>memchr_asm.s</file>
				<file>memcpy_asm.s</file>
				<file>memmove_asm.s</file>
				<file>memset_asm.s</file>
			</directory>
		</if>
		<ifnot property="ARCH" value="i386">
			<file>memchr.c</file>
			<file>memcpy.c</file>
			<file>memmove.c</file>
			<file>memset.c</file>
		</ifnot>
		<file>memcmp.c</file>
		<file>memicmp.c</file>
	</directory>

	<directory name="misc">
		<file>amsg.c</file>
		<file>assert.c</file>
		<file>crtmain.c</file>
		<file>environ.c</file>
		<file>getargs.c</file>
		<file>initterm.c</file>
		<file>lock.c</file>
		<file>purecall.c</file>
		<file>stubs.c</file>
		<file>tls.c</file>
	</directory>
	<directory name="process">
		<file>_cwait.c</file>
		<file>_system.c</file>
		<file>dll.c</file>
		<file>process.c</file>
		<file>procid.c</file>
		<file>thread.c</file>
		<file>threadid.c</file>
		<file>threadx.c</file>
		<file>wprocess.c</file>
	</directory>
	<directory name="search">
		<file>lfind.c</file>
		<file>lsearch.c</file>
	</directory>
	<directory name="setjmp">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>setjmp.s</file>
			</directory>
		</if>
	</directory>
	<directory name="signal">
		<file>signal.c</file>
		<file>xcptinfo.c</file>
	</directory>
	<directory name="stdio">
		<file>allocfil.c</file>
		<file>clearerr.c</file>
		<file>fclose.c</file>
		<file>fdopen.c</file>
		<file>feof.c</file>
		<file>ferror.c</file>
		<file>fflush.c</file>
		<file>fgetc.c</file>
		<file>fgetchar.c</file>
		<file>fgetpos.c</file>
		<file>fgets.c</file>
		<file>fgetws.c</file>
		<file>filbuf.c</file>
		<file>fileno.c</file>
		<file>flsbuf.c</file>
		<file>fopen.c</file>
		<file>fprintf.c</file>
		<file>fputc.c</file>
		<file>fputchar.c</file>
		<file>fputs.c</file>
		<file>fputws.c</file>
		<file>fread.c</file>
		<file>freopen.c</file>
		<file>fseek.c</file>
		<file>fsetpos.c</file>
		<file>fsopen.c</file>
		<file>ftell.c</file>
		<file>fwalk.c</file>
		<file>fwprintf.c</file>
		<file>fwrite.c</file>
		<file>getc.c</file>
		<file>getchar.c</file>
		<file>gets.c</file>
		<file>getw.c</file>
		<file>perror.c</file>
		<file>popen.c</file>
		<file>printf.c</file>
		<file>putc.c</file>
		<file>putchar.c</file>
		<file>puts.c</file>
		<file>putw.c</file>
		<file>putwchar.c</file>
		<file>remove.c</file>
		<file>rename.c</file>
		<file>rewind.c</file>
		<file>rmtmp.c</file>
		<file>setbuf.c</file>
		<file>setvbuf.c</file>
		<file>sprintf.c</file>
		<file>swprintf.c</file>
		<file>stdhnd.c</file>
		<file>tempnam.c</file>
		<file>tmpfile.c</file>
		<file>tmpnam.c</file>
		<file>ungetc.c</file>
		<file>ungetwc.c</file>
		<file>vfprintf.c</file>
		<file>vfwprint.c</file>
		<file>vprintf.c</file>
		<file>vsprintf.c</file>
		<file>vswprintf.c</file>
		<file>vwprintf.c</file>
		<file>wfdopen.c</file>
		<file>wfopen.c</file>
		<file>wfreopen.c</file>
		<file>wfsopen.c</file>
		<file>wpopen.c</file>
		<file>wprintf.c</file>
		<file>wremove.c</file>
		<file>wrename.c</file>
		<file>wtempnam.c</file>
		<file>wtmpnam.c</file>
	</directory>
	<directory name="stdlib">
		<file>_exit.c</file>
		<file>abort.c</file>
		<file>atexit.c</file>
		<file>ecvt.c</file>
		<file>ecvtbuf.c</file>
		<file>errno.c</file>
		<file>fcvt.c</file>
		<file>fcvtbuf.c</file>
		<file>fullpath.c</file>
		<file>gcvt.c</file>
		<file>getenv.c</file>
		<file>makepath.c</file>
		<file>malloc.c</file>
		<file>mbtowc.c</file>
		<file>obsol.c</file>
		<file>putenv.c</file>
		<file>rot.c</file>
		<file>senv.c</file>
		<file>swab.c</file>
		<file>wfulpath.c</file>
		<file>wputenv.c</file>
		<file>wsenv.c</file>
		<file>wmakpath.c</file>
	</directory>
	<directory name="string">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>strcat_asm.s</file>
				<file>strchr_asm.s</file>
				<file>strcmp_asm.s</file>
				<file>strcpy_asm.s</file>
				<file>strlen_asm.s</file>
				<file>strncat_asm.s</file>
				<file>strncmp_asm.s</file>
				<file>strncpy_asm.s</file>
				<file>strnlen_asm.s</file>
				<file>strrchr_asm.s</file>
				<file>wcscat_asm.s</file>
				<file>wcschr_asm.s</file>
				<file>wcscmp_asm.s</file>
				<file>wcscpy_asm.s</file>
				<file>wcslen_asm.s</file>
				<file>wcsncat_asm.s</file>
				<file>wcsncmp_asm.s</file>
				<file>wcsncpy_asm.s</file>
				<file>wcsnlen_asm.s</file>
				<file>wcsrchr_asm.s</file>
			</directory>
		</if>
		<ifnot property="ARCH" value="i386">
			<file>strcat.c</file>
			<file>strchr.c</file>
			<file>strcmp.c</file>
			<file>strcpy.c</file>
			<file>strlen.c</file>
			<file>strncat.c</file>
			<file>strncmp.c</file>
			<file>strncpy.c</file>
			<file>strnlen.c</file>
			<file>strrchr.c</file>
			<file>wcscat.c</file>
			<file>wcschr.c</file>
			<file>wcscmp.c</file>
			<file>wcscpy.c</file>
			<file>wcslen.c</file>
			<file>wcsncat.c</file>
			<file>wcsncmp.c</file>
			<file>wcsncpy.c</file>
			<file>wcsnlen.c</file>
			<file>wcsrchr.c</file>
		</ifnot>
		<file>atof.c</file>
		<file>ctype.c</file>
		<file>lasttok.c</file>
		<file>strcoll.c</file>
		<file>strdup.c</file>
		<file>strerror.c</file>
		<file>stricmp.c</file>
		<file>strlwr.c</file>
		<file>strncoll.c</file>
		<file>strnicmp.c</file>
		<file>strpbrk.c</file>
		<file>strrev.c</file>
		<file>strset.c</file>
		<file>strstr.c</file>
		<file>strtod.c</file>
		<file>strtok.c</file>
		<file>strtoul.c</file>
		<file>strtoull.c</file>
		<file>strupr.c</file>
		<file>strxfrm.c</file>
		<file>wcstod.c</file>
		<file>wcstombs.c</file>
		<file>wctomb.c</file>
		<file>wsplitp.c</file>
	</directory>
	<directory name="sys_stat">
		<file>fstat.c</file>
		<file>fstati64.c</file>
		<file>futime.c</file>
		<file>stat.c</file>
		<file>wstat.c</file>
		<file>systime.c</file>
	</directory>
	<directory name="time">
		<file>clock.c</file>
		<file>ctime.c</file>
		<file>difftime.c</file>
		<file>ftime.c</file>
		<file>strdate.c</file>
		<file>strftime.c</file>
		<file>strtime.c</file>
		<file>time.c</file>
		<file>tz_vars.c</file>
		<file>wctime.c</file>
		<file>wstrdate.c</file>
		<file>wstrtime.c</file>
	</directory>
	<directory name="wstring">
		<file>wcscoll.c</file>
		<file>wcscspn.c</file>
		<file>wcsdup.c</file>
		<file>wcsicmp.c</file>
		<file>wcslwr.c</file>
		<file>wcsnicmp.c</file>
		<file>wcspbrk.c</file>
		<file>wcsrev.c</file>
		<file>wcsset.c</file>
		<file>wcsspn.c</file>
		<file>wcsstr.c</file>
		<file>wcstok.c</file>
		<file>wcsupr.c</file>
		<file>wcsxfrm.c</file>
		<file>wlasttok.c</file>
	</directory>
	<directory name="wine">
		<file>heap.c</file>
		<file>scanf.c</file>
		<file>thread.c</file>
		<file>undname.c</file>
	</directory>
</module>
