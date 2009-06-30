<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<bootstrap installbase="$(CDOUTPUT)" />
	<importlibrary definition="ntoskrnl.pspec" />
	<define name="__NTOSKRNL__" />
	<define name="_NTOSKRNL_" />
	<define name="_NTSYSTEM_" />
	<define name="__NO_CTYPE_INLINES" />
	<define name="WIN9X_COMPAT_SPINLOCK" />
	<define name="_IN_KERNEL_" />
	<if property="_WINKD_" value="1">
		<define name="_WINKD_" />
	</if>
	<if property="_ELF_" value="1">
		<define name="_ELF_" />
	</if>
	<include base="cmlib">.</include>
	<include base="ntoskrnl">include</include>
	<include base="ntoskrnl" root="intermediate"></include>
	<include base="ntoskrnl" root="intermediate">include</include>
	<include base="ntoskrnl" root="intermediate">include/internal</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<library>csq</library>
	<library>hal</library>
	<library>pseh</library>
	<library>cmlib</library>
	<library>rtl</library>
	<library>rossym</library>
	<library>libcntpr</library>
	<library>kdcom</library>
	<library>bootvid</library>
	<library>wdmguid</library>
	<dependency>bugcodes</dependency>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38269
	<directory name="include">
		<pch>precomp.h</pch>
	</directory>
	-->
	<directory name="ke">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file first="true">boot.S</file>
				<file>abios.c</file>
				<file>cpu.c</file>
				<file>ctxswitch.S</file>
				<file>exp.c</file>
				<file>irqobj.c</file>
				<file>kiinit.c</file>
				<file>ldt.c</file>
				<file>mtrr.c</file>
				<file>patpge.c</file>
				<file>systimer.S</file>
				<file>thrdini.c</file>
				<file>trap.s</file>
				<file>usercall_asm.S</file>
				<file>usercall.c</file>
				<file>v86vdm.c</file>
				<file>v86m_sup.S</file>
			</directory>
		</if>
		<if property="ARCH" value="arm">
			<directory name="arm">
				<file first="true">boot.s</file>
				<file>cpu.c</file>
				<file>ctxswtch.s</file>
				<file>exp.c</file>
				<file>kiinit.c</file>
				<file>stubs_asm.s</file>
				<file>thrdini.c</file>
				<file>time.c</file>
				<file>trap.s</file>
				<file>trapc.c</file>
				<file>usercall.c</file>
			</directory>
		</if>
		<if property="ARCH" value="powerpc">
			<directory name="powerpc">
				<file first="true">main_asm.S</file>
				<file>cpu.c</file>
				<file>exp.c</file>
				<file>kiinit.c</file>
				<file>ppc_irq.c</file>
				<file>stubs.c</file>
				<file>systimer.c</file>
				<file>thrdini.c</file>
				<file>ctxswitch.c</file>
				<file>ctxhelp.S</file>
			</directory>
		</if>
		<file>apc.c</file>
		<file>balmgr.c</file>
		<file>bug.c</file>
		<file>clock.c</file>
		<file>config.c</file>
		<file>devqueue.c</file>
		<file>dpc.c</file>
		<file>eventobj.c</file>
		<file>except.c</file>
		<file>freeldr.c</file>
		<file>gate.c</file>
		<file>gmutex.c</file>
		<file>ipi.c</file>
		<file>krnlinit.c</file>
		<file>mutex.c</file>
		<file>procobj.c</file>
		<file>profobj.c</file>
		<file>queue.c</file>
		<file>semphobj.c</file>
		<file>spinlock.c</file>
		<file>thrdschd.c</file>
		<file>thrdobj.c</file>
		<file>timerobj.c</file>
		<file>wait.c</file>
	</directory>
	<directory name="cc">
		<file>cacheman.c</file>
		<file>copy.c</file>
		<file>fs.c</file>
		<file>mdl.c</file>
		<file>pin.c</file>
		<file>view.c</file>
	</directory>
	<directory name="config">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>cmhardwr.c</file>
			</directory>
		</if>
		<if property="ARCH" value="arm">
			<directory name="arm">
				<file>cmhardwr.c</file>
			</directory>
		</if>
		<if property="ARCH" value="powerpc">
			<directory name="powerpc">
				<file>cmhardwr.c</file>
			</directory>
		</if>
		<file>cmalloc.c</file>
		<file>cmapi.c</file>
		<file>cmboot.c</file>
		<file>cmcheck.c</file>
		<file>cmcontrl.c</file>
		<file>cmconfig.c</file>
		<file>cmdata.c</file>
		<file>cmdelay.c</file>
		<file>cmindex.c</file>
		<file>cminit.c</file>
		<file>cmhook.c</file>
		<file>cmkcbncb.c</file>
		<file>cmkeydel.c</file>
		<file>cmlazy.c</file>
		<file>cmmapvw.c</file>
		<file>cmname.c</file>
		<file>cmparse.c</file>
		<file>cmse.c</file>
		<file>cmsecach.c</file>
		<file>cmsysini.c</file>
		<file>cmvalue.c</file>
		<file>cmvalche.c</file>
		<file>cmwraprs.c</file>
		<file>ntapi.c</file>
	</directory>
	<directory name="dbgk">
		<file>dbgkutil.c</file>
		<file>dbgkobj.c</file>
	</directory>
	<directory name="ex" root="intermediate">
		<file>zw.S</file>
	</directory>
	<directory name="ex">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>interlck_asm.S</file>
				<file>fastinterlck_asm.S</file>
				<file>ioport.S</file>
			</directory>
		</if>
		<file>atom.c</file>
		<file>callback.c</file>
		<file>dbgctrl.c</file>
		<file>efi.c</file>
		<file>event.c</file>
		<file>evtpair.c</file>
		<file>exintrin.c</file>
		<file>fastinterlck.c</file>
		<file>fmutex.c</file>
		<file>handle.c</file>
		<file>harderr.c</file>
		<file>hdlsterm.c</file>
		<file>init.c</file>
		<file>keyedevt.c</file>
		<file>locale.c</file>
		<file>lookas.c</file>
		<file>mutant.c</file>
		<file>pushlock.c</file>
		<file>profile.c</file>
		<file>resource.c</file>
		<file>rundown.c</file>
		<file>sem.c</file>
		<file>shutdown.c</file>
		<file>sysinfo.c</file>
		<file>time.c</file>
		<file>timer.c</file>
		<file>uuid.c</file>
		<file>win32k.c</file>
		<file>work.c</file>
		<file>xipdisp.c</file>
		<file>zone.c</file>
	</directory>
	<directory name="fsrtl">
		<file>dbcsname.c</file>
		<file>fastio.c</file>
		<file>faulttol.c</file>
		<file>filelock.c</file>
		<file>filter.c</file>
		<file>filtrctx.c</file>
		<file>fsfilter.c</file>
		<file>fsrtlpc.c</file>
		<file>largemcb.c</file>
		<file>name.c</file>
		<file>notify.c</file>
		<file>oplock.c</file>
		<file>pnp.c</file>
		<file>stackovf.c</file>
		<file>tunnel.c</file>
		<file>unc.c</file>
	</directory>
	<directory name="fstub">
		<file>disksup.c</file>
		<file>fstubex.c</file>
		<file>halstub.c</file>
	</directory>
	<directory name="inbv">
		<file>inbv.c</file>
	</directory>
	<directory name="io">
		<directory name="iomgr">
			<file>adapter.c</file>
			<file>arcname.c</file>
			<file>bootlog.c</file>
			<file>controller.c</file>
			<file>device.c</file>
			<file>deviface.c</file>
			<file>driver.c</file>
			<file>drvrlist.c</file>
			<file>error.c</file>
			<file>file.c</file>
			<file>iocomp.c</file>
			<file>ioevent.c</file>
			<file>iofunc.c</file>
			<file>iomdl.c</file>
			<file>iomgr.c</file>
			<file>iorsrce.c</file>
			<file>iotimer.c</file>
			<file>iowork.c</file>
			<file>irp.c</file>
			<file>irq.c</file>
			<file>ramdisk.c</file>
			<file>rawfs.c</file>
			<file>remlock.c</file>
			<file>util.c</file>
			<file>symlink.c</file>
			<file>volume.c</file>
		</directory>
		<directory name="pnpmgr">
			<file>plugplay.c</file>
			<file>pnpdma.c</file>
			<file>pnpmgr.c</file>
			<file>pnpnotify.c</file>
			<file>pnpreport.c</file>
			<file>pnproot.c</file>
		</directory>
	</directory>
	<directory name="kd">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>kdmemsup.c</file>
			</directory>
		</if>
	</directory>
	<if property="_WINKD_" value="0">
		<directory name="kdbg">
			<if property="ARCH" value="i386">
				<directory name="i386">
					<if property="KDBG" value="1">
						<group>
							<file>i386-dis.c</file>
							<file>kdb_help.S</file>
							<file>longjmp.S</file>
							<file>setjmp.S</file>
						</group>
					</if>
				</directory>
			</if>
			<if property="KDBG" value="1">
				<file>kdb.c</file>
				<file>kdb_cli.c</file>
				<file>kdb_expr.c</file>
				<file>kdb_keyboard.c</file>
				<file>kdb_serial.c</file>
			</if>
			<if property="DBG_OR_KDBG" value="true">
				<file>kdb_symbols.c</file>
			</if>
		</directory>
		<directory name="kd">
			<directory name="wrappers">
				<file>bochs.c</file>
				<if property="ARCH" value="i386">
					<file>gdbstub.c</file>
				</if>
				<if property="ARCH" value="powerpc">
					<file>gdbstub_powerpc.c</file>
				</if>
				<file>kdbg.c</file>
			</directory>
			<file>kdinit.c</file>
			<file>kdio.c</file>
			<file>kdmain.c</file>
		</directory>
	</if>
	<if property="_WINKD_" value ="1">
		<directory name="kd64">
			<file>kdapi.c</file>
			<file>kdbreak.c</file>
			<file>kddata.c</file>
			<file>kdinit.c</file>
			<file>kdlock.c</file>
			<file>kdprint.c</file>
			<file>kdtrap.c</file>
		</directory>
	</if>
	<directory name="lpc">
		<file>close.c</file>
		<file>complete.c</file>
		<file>connect.c</file>
		<file>create.c</file>
		<file>listen.c</file>
		<file>port.c</file>
		<file>reply.c</file>
		<file>send.c</file>
	</directory>
	<directory name="mm">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>page.c</file>
			</directory>
		</if>
		<if property="ARCH" value="arm">
			<directory name="arm">
				<file>stubs.c</file>
			</directory>
		</if>
		<if property="ARCH" value="powerpc">
			<directory name="powerpc">
				<file>pfault.c</file>
				<file>page.c</file>
			</directory>
		</if>
		<directory name="ARM3">
			<file>contmem.c</file>
			<file>drvmgmt.c</file>
			<file>dynamic.c</file>
			<file>hypermap.c</file>
			<file>init.c</file>
			<file>iosup.c</file>
			<file>mdlsup.c</file>
			<file>pool.c</file>
			<file>procsup.c</file>
			<file>syspte.c</file>
		</directory>
		<file>anonmem.c</file>
		<file>balance.c</file>
		<file>dbgpool.c</file>
		<file>freelist.c</file>
		<file>marea.c</file>
		<file>mmfault.c</file>
		<file>mmsup.c</file>
		<file>mminit.c</file>
		<file>mpw.c</file>
		<file>ncache.c</file>
		<file>npool.c</file>
		<file>pagefile.c</file>
		<file>pageop.c</file>
		<file>pe.c</file>
		<file>pool.c</file>
		<file>ppool.c</file>
		<file>procsup.c</file>
		<file>region.c</file>
		<file>rmap.c</file>
		<file>section.c</file>
		<file>sysldr.c</file>
		<file>virtual.c</file>
		<if property="_ELF_" value="1">
			<file>elf32.c</file>
			<file>elf64.c</file>
		</if>
	</directory>
	<directory name="ob">
		<file>obdir.c</file>
		<file>obinit.c</file>
		<file>obhandle.c</file>
		<file>obname.c</file>
		<file>oblife.c</file>
		<file>obref.c</file>
		<file>obsdcach.c</file>
		<file>obsecure.c</file>
		<file>oblink.c</file>
		<file>obwait.c</file>
	</directory>
	<directory name="po">
		<file>power.c</file>
		<file>events.c</file>
	</directory>
	<directory name="ps">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>psctx.c</file>
			</directory>
		</if>
		<if property="ARCH" value="arm">
			<directory name="arm">
				<file>psctx.c</file>
			</directory>
		</if>
		<file>debug.c</file>
		<file>job.c</file>
		<file>kill.c</file>
		<file>psnotify.c</file>
		<file>process.c</file>
		<file>psmgr.c</file>
		<file>query.c</file>
		<file>quota.c</file>
		<file>security.c</file>
		<file>state.c</file>
		<file>thread.c</file>
		<file>win32.c</file>
	</directory>
	<directory name="rtl">
		<if property="ARCH" value="arm">
			<directory name="arm">
				<file>rtlexcpt.c</file>
			</directory>
		</if>
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>stack.S</file>
			</directory>
		</if>
		<file>libsupp.c</file>
		<file>misc.c</file>
	</directory>
	<directory name="se">
		<file>access.c</file>
		<file>acl.c</file>
		<file>audit.c</file>
		<file>lsa.c</file>
		<file>priv.c</file>
		<file>sd.c</file>
		<file>semgr.c</file>
		<file>sid.c</file>
		<file>token.c</file>
	</directory>
	<directory name="vdm">
		<if property="ARCH" value="i386">
			<file>vdmmain.c</file>
			<file>vdmexec.c</file>
		</if>
	</directory>
	<directory name="wmi">
		<file>wmi.c</file>
	</directory>
	<file>ntoskrnl.rc</file>
	<linkerscript>ntoskrnl_$(ARCH).lnk</linkerscript>

	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</group>
