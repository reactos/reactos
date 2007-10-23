<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="libcntpr" type="staticlibrary">
	<define name="__NO_CTYPE_INLINES" />
	<define name="NO_RTL_INLINES" />
	<define name="_NTSYSTEM_" />
	<define name="_NTDLLBUILD_" />
	<define name="_SEH_NO_NATIVE_NLG" />
	<if property="ARCH" value="i386">
		<define name="__MINGW_IMPORT">"extern __attribute__ ((dllexport))"</define>
	</if>
	<include base="libcntpr">.</include>

	<directory name="except">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>chkstk_asm.s</file>
				<file>seh.s</file>
			</directory>
		</if>
		<if property="ARCH" value="powerpc">
			<directory name="powerpc">
				<file>chkstk_asm.s</file>
				<file>seh.s</file>
			</directory>
		</if>
	</directory>
	<directory name="math">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>alldiv_asm.s</file>
				<file>alldvrm_asm.s</file>
				<file>allmul_asm.s</file>
				<file>allrem_asm.s</file>
				<file>allshl_asm.s</file>
				<file>allshr_asm.s</file>
				<file>atan_asm.s</file>
				<file>aulldiv_asm.s</file>
				<file>aulldvrm_asm.s</file>
				<file>aullrem_asm.s</file>
				<file>aullshr_asm.s</file>
				<file>ceil_asm.s</file>
				<file>cos_asm.s</file>
				<file>fabs_asm.s</file>
				<file>floor_asm.s</file>
				<file>ftol_asm.s</file>
				<file>log_asm.s</file>
				<file>pow_asm.s</file>
				<file>sin_asm.s</file>
				<file>sqrt_asm.s</file>
				<file>tan_asm.s</file>
			</directory>
		</if>
		<file>abs.c</file>
		<file>bsearch.c</file>
		<file>labs.c</file>
		<file>rand.c</file>
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
		<file>memccpy.c</file>
		<file>memcmp.c</file>
		<file>memicmp.c</file>
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
		<file>ctype.c</file>
		<file>strcspn.c</file>
		<file>stricmp.c</file>
		<file>strnicmp.c</file>
		<file>strlwr.c</file>
		<file>strrev.c</file>
		<file>strset.c</file>
		<file>strstr.c</file>
		<file>strupr.c</file>
		<file>strpbrk.c</file>
		<file>strspn.c</file>
		<file>wstring.c</file>
		<file>wcsrev.c</file>
		<file>wcsnset.c</file>
		<file>atoi64.c</file>
		<file>atoi.c</file>
		<file>atol.c</file>
		<file>itoa.c</file>
		<file>itow.c</file>
		<file>lfind.c</file>
		<file>mbstowcs.c</file>
		<file>splitp.c</file>
		<file>strtol.c</file>
		<file>strtoul.c</file>
		<file>strtoull.c</file>
		<file>wcstol.c</file>
		<file>wcstombs.c</file>
		<file>wcstoul.c</file>
		<file>wtoi64.c</file>
		<file>wtoi.c</file>
		<file>wtol.c</file>
		<file>sscanf.c</file>
	</directory>

	<pch>libcntpr.h</pch>
</module>
