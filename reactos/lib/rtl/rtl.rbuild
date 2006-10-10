<module name="rtl" type="staticlibrary">
	<define name="__USE_W32API" />
	<define name="_NTOSKRNL_" />
	<define name="__NO_CTYPE_INLINES" />
	<define name="NO_RTL_INLINES" />
	<define name="_NTSYSTEM_" />
	<define name="_NTDLLBUILD_" />
	<define name="_SEH_NO_NATIVE_NLG" />
	<include base="rtl">.</include>
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
			<file>chkstk_asm.s</file>
			<file>cos_asm.s</file>
			<file>debug_asm.S</file>
			<file>except_asm.s</file>
			<file>exception.c</file>
			<file>fabs_asm.s</file>
			<file>floor_asm.s</file>
			<file>ftol_asm.s</file>
			<file>log_asm.s</file>
			<file>random_asm.S</file>
			<file>rtlswap.S</file>
			<file>rtlmem.s</file>
			<file>pow_asm.s</file>
			<file>res_asm.s</file>
			<file>sin_asm.s</file>
			<file>sqrt_asm.s</file>
			<file>tan_asm.s</file>
		</directory>
	</if>
	<directory name="austin">
		<file>avl.c</file>
		<file>tree.c</file>
	</directory>

      <ifnot property="ARCH" value="i386">
             <file>memgen.c</file>
             <file>mem.c</file> 
      </ifnot>

	<file>access.c</file>
	<file>acl.c</file>
	<file>atom.c</file>
	<file>bitmap.c</file>
	<file>bootdata.c</file>
	<file>compress.c</file>
	<file>condvar.c</file>
	<file>crc32.c</file>
	<file>critical.c</file>
	<file>dbgbuffer.c</file>
	<file>debug.c</file>
	<file>dos8dot3.c</file>
	<file>encode.c</file>
	<file>env.c</file>
	<file>error.c</file>
	<file>exception.c</file>
	<file>generictable.c</file>
	<file>handle.c</file>
	<file>heap.c</file>
	<file>image.c</file>
	<file>message.c</file>
	<file>largeint.c</file>
	<file>luid.c</file>
	<file>network.c</file>
	<file>nls.c</file>
	<file>path.c</file>
	<file>ppb.c</file>
	<file>process.c</file>
	<file>propvar.c</file>
	<file>qsort.c</file>
	<file>random.c</file>
	<file>rangelist.c</file>
	<file>registry.c</file>
	<file>res.c</file>
	<file>resource.c</file>
	<file>sd.c</file>
	<file>security.c</file>
	<file>sid.c</file>
	<file>sprintf.c</file>
	<file>srw.c</file>
	<file>swprintf.c</file>
	<file>splaytree.c</file>
	<file>thread.c</file>
	<file>time.c</file>
	<file>timezone.c</file>
	<file>timerqueue.c</file>
	<file>unicode.c</file>
	<file>unicodeprefix.c</file>
	<file>vectoreh.c</file>
	<file>version.c</file>
	<file>workitem.c</file>
	<pch>rtl.h</pch>
</module>
