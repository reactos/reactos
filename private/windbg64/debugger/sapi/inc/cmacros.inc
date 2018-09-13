comment $
cmacros - assembly macros for interfacing to hhls
(C)Copyright Microsoft Corp. 1984-1988
$
.xcref
.xcref ??_out
??_out macro t
ifndef ?QUIET
%out t
endif
endm
outif macro name,defval,onmsg,offmsg
ifndef name
ifb <defval>
name=0
else
name=defval
endif
endif
if name
name=1
ifnb <onmsg>
??_out <! onmsg>
endif
else
ifnb <offmsg>
??_out <! offmsg>
endif
endif
endm
.xcref ??error
??error macro msg
e r r o r ----- msg
.err
endm
.xcref ASMpass
.xcref memS,memM,memL,memC,memH,memMOD,sizec,sized
if1
ASMpass=1
ifdef ?SMALL
memS=1
endif
ifdef ?MEDIUM
memM=1
endif
ifdef ?COMPACT
memC=1
endif
ifdef ?LARGE
memL=1
endif
ifdef ?HUGE
memH=1
endif
??_out <cMacros Version 5.20 - Copyright (c) Microsoft Corp. 1984-1988>
outif memS,0,<Small model>
outif memM,0,<Medium model>
outif memL,0,<Large model>
outif memC,0,<Compact model>
outif memH,0,<Huge model>
memMOD= memS + memM + memL + memC + memH
if memMOD ne 1
if memMOD eq 0
memS = 1
else
??error <more than 1 memory model selected>
endif
endif
sizec= memM + memL + memH
sized= memL + memC + (memH*2)
outif ?DF,0,<No segments or groups will be defined>
outif ?TF,0,<Epilog sequences assume valid SP>
outif ?WIN,1,<Windows support>
if ?WIN eq 1
outif ?PLM,1,<>
else
outif ?PLM,1,<PL/M calling convention>
endif
ifndef ?NODATA
?nodata1=0
else
?nodata1=1
??_out <! NODATA module>
endif
ifndef ?CHKSTK
?chkstk1=0
else
?chkstk1=1
ifdef ?CHKSTKPROC
??_out <! Private stack checking enabled>
else
??_out <! Stack checking enabled>
endif
endif
ifndef DOS5
?DOS5=0
else
?DOS5=1
??_out <! DOS5 module>
endif
ifdef ?PROFILE
??_out <! Native profiling enabled>
endif
else
ASMpass=2
endif
.xcref ?n,?ax,?ah,?al,?bx,?bh
.xcref ?bl,?cx,?ch,?cl,?dx,?dh
.xcref ?dl,?si,?di,?es,?ds,?bp
.xcref ?sp,?ss,?cs
.xcref ?rsl,?cpd,?argl,?argc,?ba
.xcref ?acb,???,?po
.xcref ?pas,?pc
.xcref uconcat,mpush,mpop
.xcref ?ri,?pp,?pp1,?al1
.xcref ?ad,?ap,?atal,?dd,?dd1,?dd2
.xcref ?pg,?pg1,?aloc,?cs1,?cs2
.xcref ?DF,?TF,?ff,?PLM,?WIN,?ia,?pu,?adj
.xcref ?uf,?rp,?nx,?nd,?nodata1,?chkstk1,?DOS5
.xcref ?wfp,arg,cCall,cProc,assumes,?cs3,?cs2,?cs1
.xcref defgrp,addseg,createSeg
.xcref save,outif,errnz,errn$,errnz1
.xcref ?PLMPrevParm,?gcc
.xcref ?cCall1,?pcc
?rsl = 0
?cpd = 0
?argl = 0
?argc = 0
?ba = 0
?acb = 0
??? = 0
?po = 0
?pas = 0
?pc = 0
?ia = 0
?pu = 0
?adj = 0
?rp = 0
?uf = 0
?nd = 0
?nx = 0
?wfp = 0
?ff = 0
?dd2 = 0
?cCall1 = 0
?pcc = 0
?PLMPrevParm = 0
.xcref ?casen
if1
?casen = 0
endif
?n = 0000000000000000b
?ax = 0000000000000011b
?ah = 0000000000000001b
?al = 0000000000000010b
?bx = 0000000000001100b
?bh = 0000000000000100b
?bl = 0000000000001000b
?cx = 0000000000110000b
?ch = 0000000000010000b
?cl = 0000000000100000b
?dx = 0000000011000000b
?dh = 0000000001000000b
?dl = 0000000010000000b
?si = 0000000100000000b
?di = 0000001000000000b
?es = 0000010000000000b
?ds = 0000100000000000b
?bp = 0001000000000000b
?sp = 0010000000000000b
?ss = 0100000000000000b
?cs = 1000000000000000b
.cref
uconcat macro a,b,c,d,e,f,g
a&b c&d e&f&g
endm
mpush macro r
irp x,<ax,bx,cx,dx,si,di,es,ds,bp,sp,ss,cs>
if (r and ?&&x)
	push	x
endif
endm
endm
mpop macro r
irp x,<cs,ss,sp,bp,ds,es,di,si,dx,cx,bx,ax>
if (r and ?&&x)
	pop	x
endif
endm
endm
save macro r
?rsl=0
?ri ?rsl,<r>
endm
?ri macro n,r
irp x,<r>
ifdef ?&&x
n=n or ?&&x
endif
endm
endm
.xcref
.xcref parmB,parmW,parmD,parmQ,parmT,parmCP,parmDP
.cref
parmB macro n
?pp <n>,<byte>,2,1
endm
parmW macro n
?pp <n>,<word>,2,2
endm
parmD macro n
ife ?PLM
irp x,<n>
?pp <&&x>,<dword>,0,4
?pp <off_&&x>,<word>,2,2
?pp <seg_&&x>,<word>,2,2
endm
else
irp x,<n>
?pp <seg_&&x>,<word>,2,2
?pp <off_&&x>,<word>,2,2
?pp <&&x>,<dword>,0,4
endm
endif
endm
parmQ macro n
?pp <n>,<qword>,8,8
endm
parmT macro n
?pp <n>,<tbyte>,10,10
endm
if sizec
parmCP macro n
parmD <n>
endm
else
parmCP macro n
parmW <n>
endm
endif
if sized
parmDP macro n
parmD <n>
endm
else
parmDP macro n
parmW <n>
endm
endif
?pp macro n,t,l,s
if ?cpd
.xcref
irp x,<n>
.xcref ?t&&x
?t&&x=s
ife ?PLM
?pp1 x,<t>,,,%(?po+?adj)
?po=?po+l
else
?PLMPrevParm=?PLMPrevParm+1
?po=?po+l
?pp1 x,<t>,%?po,%?adj,,%?PLMPrevParm,%(?PLMPrevParm-1)
endif
endm
.cref
else
??error <parm(s) "&n" declared outside proc def>
endif
endm
?pp1 macro n,t,o,a,b,cpc,ppc
ife ?PLM
n equ (t ptr [bp+b])
else
.xcref
.xcref ?PLMParm&cpc
.cref
?PLMParm&cpc &macro po
uconcat <n>,,<equ>,,<(t ptr [bp+>,%(a+po-o),<])>
?PLMParm&ppc po
purge ?PLMParm&cpc
&endm
endif
endm
ifndef ?NOPARMR
.xcref
.xcref ?pr,parmR
.cref
parmR macro n,r,r2
?pr n,r,r2,%?rp,%(?ia+2)
endm
?pr macro n,r,r2,i,o
.xcref
ifnb <r2>
parmR seg_&n,r
parmR off_&n,r2
n equ (dword ptr [bp-o-2])
.xcref ?t&n
?t&n=4
else
.xcref ?rp&i
?rp&i=0
ifdef ?&r
?rp&i=?&r
endif
if ??? or (?cpd eq 0) or (?rp&i eq 0)
??error <invalid parmR encountered: &n,&r>
exitm
endif
n equ (word ptr [bp-o])
?t&n=2
irp x,<bh,ch,dh,bl,cl,dl,ah,al>
if ?&&x eq ?&r
n equ (byte ptr [bp-o])
?t&n=1
exitm
endif
endm
?ia=?ia+2
?rp=?rp+1
endif
.cref
endm
endif
.xcref
.xcref localB,localW,localD,localQ,localT,localCP,localDP,localV
.cref
localB macro n
?aloc <n>,<byte ptr>,1,1,0
endm
localW macro n
?aloc <n>,<word ptr>,2,2,1
endm
localD macro n
irp x,<n>
?aloc <seg_&&x>,<word ptr>,2,2,1
?aloc <off_&&x>,<word ptr>,2,2,1
?aloc <&&x>,<dword ptr>,0,4,1
endm
endm
localQ macro n
?aloc <n>,<qword ptr>,8,8,1
endm
localT macro n
?aloc <n>,<tbyte ptr>,10,10,1
endm
if sizec
localCP macro n
localD <n>
endm
else
localCP macro n
localW <n>
endm
endif
if sized
localDP macro n
localD <n>
endm
else
localDP macro n
localW <n>
endm
endif
localV macro n,a
?aloc <n>,,%(a),0,1
endm
?aloc macro n,t,l,s,a
if ?cpd
.xcref
irp x,<n>
???=???+l
if a
???=((??? + 1) and 0fffeh)
endif
?al1 x,<t>,%(???+?ia)
.xcref ?t&&x
?t&&x=s
endm
.cref
else
??error <locals "&n" declared outside procedure def>
endif
endm
?al1 macro n,t,o
n equ (t [bp-o])
endm
?gcc macro s,i,cc
s = i
ifnb <cc>
ifidn <cc>,<C>
s=0
endif
ifidn <cc>,<PLM>
s=1
endif
ifidn <cc>,<PASCAL>
s=1
endif
endif
endm
ifndef ?NOGLOBAL
.xcref
.xcref globalB,globalW,globalD,globalQ,globalT,globalCP,globalDP
.cref
globalB macro n,i,s,c
?ad <n>,1
?dd n,1,<byte>,<db>,<i>,<s>,<c>
endm
globalW macro n,i,s,c
?ad <n>,2
?dd n,1,<word>,<dw>,<i>,<s>,<c>
endm
globalD macro n,i,s,c
?ad <n>,4
?dd n,1,<dword>,<dd>,<i>,<s>,<c>
off_&n equ n
seg_&n equ n[2]
endm
globalQ macro n,i,s,c
?ad <n>,8
?dd n,1,<qword>,<dq>,<i>,<s>,<c>
endm
globalT macro n,i,s,c
?ad <n>,10
?dd n,1,<tbyte>,<dt>,<i>,<s>,<c>
endm
if sizec
globalCP macro n,i,s,c
globalD n,<i>,<s>,<c>
endm
else
globalCP macro n,i,s,c
globalW n,<i>,<s>,<c>
endm
endif
if sized
globalDP macro n,i,s,c
globalD n,<i>,<s>,<c>
endm
else
globalDP macro n,i,s,c
globalW n,<i>,<s>,<c>
endm
endif
endif
ifndef ?NOSTATIC
.xcref
.xcref staticB,staticW,staticD,staticQ,staticT,staticCP,staticDP
.cref
staticB macro n,i,s
?ad <n>,1
?dd n,0,<byte>,<db>,<i>,<s>,<PLM>
endm
staticW macro n,i,s
?ad <n>,2
?dd n,0,<word>,<dw>,<i>,<s>,<PLM>
endm
staticD macro n,i,s
?ad <n>,4
?dd n,0,<dword>,<dd>,<i>,<s>,<PLM>
endm
staticQ macro n,i,s
?ad <n>,8
?dd n,0,<qword>,<dq>,<i>,<s>,<PLM>
endm
staticT macro n,i,s
?ad <n>,10
?dd n,0,<tbyte>,<dt>,<i>,<s>,<PLM>
endm
if sizec
staticCP macro n,i,s
staticD n,<i>,<s>
endm
else
staticCP macro n,i,s
staticW n,<i>,<s>
endm
endif
if sized
staticDP macro n,i,s
staticD n,<i>,<s>
endm
else
staticDP macro n,i,s
staticW n,<i>,<s>
endm
endif
endif
?dd macro n,p,t,d,i,s,c
?gcc ?dd2,%?PLM,<c>
ife ?dd2
n label t
?dd1 _&n,p,<d>,<i>,<s>
else
?dd1 n,p,<d>,<i>,<s>
endif
endm
?dd1 macro n,p,d,i,s
if p
public n
endif
ifb <s>
n d i
else
ifb <i>
n d s dup (?)
else
n d s dup (i)
endif
endif
endm
ifndef ?NOEXTERN
.xcref
.xcref ?ex1,?ex2,externB,externW,externD,externQ,externT
.xcref externNP,externFP,externP,externCP,externDP,externA
.cref
?ex2 = 0
externA macro n,c
?ex1 <n>,40h,<abs>,<c>,<>
endm
externB macro n,c
?ex1 <n>,1,<byte>,<c>,<>
endm
externW macro n,c
?ex1 <n>,2,<word>,<c>,<>
endm
externD macro n,c
?ex1 <n>,4,<dword>,<c>,<>
endm
externQ macro n,c
?ex1 <n>,8,<qword>,<c>,<>
endm
externT macro n,c
?ex1 <n>,10,<tbyte>,<c>,<>
endm
externNP macro n,c
?ex1 <n>,2,<near>,<c>,<cc>
endm
externFP macro n,c
?ex1 <n>,4,<far>,<c>,<cc>
endm
if sizec
externP macro n,c
?ex1 <n>,4,<far>,<c>,<cc>
endm
else
externP macro n,c
?ex1 <n>,2,<near>,<c>,<cc>
endm
endif
if sizec
externCP macro n,c
?ex1 <n>,4,<dword>,<c>,<>
endm
else
externCP macro n,c
?ex1 <n>,2,<word>,<c>,<>
endm
endif
if sized
externDP macro n,c
?ex1 <n>,4,<dword>,<c>,<>
endm
else
externDP macro n,c
?ex1 <n>,2,<word>,<c>,<>
endm
endif
?ex1 macro n,s,d,c,scv
?gcc ?ex2,%?PLM,<c>
irp x,<n>
.xcref
.xcref ?t&&x
.cref
?t&&x=s
ife ?ex2
extrn _&&x:&d
x equ _&&x
else
extrn x:&d
endif
ifidn <scv>,<cc>
.xcref
.xcref ?CC&&x
.cref
?CC&&x=?ex2
endif
endm
endm
endif
ifndef ?NOLABEL
.xcref
.xcref ?lb1,?lblpu,?lb2
.xcref labelB,labelW,labelD,labelQ,labelT
.xcref labelNP,labelFP,labelP,labelCP,labelDP
.cref
?lblpu = 0
?lb2 = 0
labelB macro n,c
?lb1 <n>,1,<byte>,<c>
endm
labelW macro n,c
?lb1 <n>,2,<word>,<c>
endm
labelD macro n,c
?lb1 <n>,4,<dword>,<c>
endm
labelQ macro n,c
?lb1 <n>,8,<qword>,<c>
endm
labelT macro n,c
?lb1 <n>,10,<tbyte>,<c>
endm
labelNP macro n,c
?lb1 <n>,2,<near>,<c>
endm
labelFP macro n,c
?lb1 <n>,4,<far>,<c>
endm
if sizec
labelP macro n,c
?lb1 <n>,4,<far>,<c>
endm
else
labelP macro n,c
?lb1 <n>,2,<near>,<c>
endm
endif
if sizec
labelCP macro n,c
?lb1 <n>,4,<dword>,<c>
endm
else
labelCP macro n,c
?lb1 <n>,2,<word>,<c>
endm
endif
if sized
labelDP macro n,c
?lb1 <n>,4,<dword>,<c>
endm
else
labelDP macro n,c
?lb1 <n>,2,<word>,<c>
endm
endif
?lb1 macro n,s,d,c
?gcc ?lb2,%?PLM,<c>
?lblpu=0
irp x,<n>
ifidn <x>,<PUBLIC>
?lblpu=1
else
.xcref
.xcref ?t&&x
.cref
?t&&x=s
ife ?lb2
if ?lblpu
public _&&x
endif
_&&x label &d
x equ _&&x
else
if ?lblpu
public x
endif
x label &d
endif
endif
endm
endm
endif
ifndef ?NODEF
.xcref
.xcref defB,defW,defD,defQ,defT,defCP,defDP
.cref
defB macro n
?ad <n>,1
endm
defW macro n
?ad <n>,2
endm
defD macro n
?ad <n>,4
endm
defQ macro n
?ad <n>,8
endm
defT macro n
?ad <n>,10
endm
if sizec
defCP macro n
defD <n>
endm
else
defCP macro n
defW <n>
endm
endif
if sized
defDP macro n
defD <n>
endm
else
defDP macro n
defW <n>
endm
endif
endif
?ad macro n,s
irp x,<n>
.xcref
.xcref ?t&&x
.cref
?t&&x=s
endm
endm
ifndef ?NOPTR
.xcref
.xcref regPtr,farPtr
.cref
regPtr macro n,s,o
farPtr n,s,o
endm
farPtr macro n,s,o
.xcref
.xcref ?t&n
.cref
n &macro
	push	s
	push	o
&endm
?t&n=80h
endm
endif
arg macro a
irp x,<a>
?argc=?argc+1
?atal <x>,%?argc
endm
endm
?atal macro n,i
.xcref
.xcref ?ali&i
.cref
?ali&i &macro
?ap n
&endm
endm
?ap macro n
?argl=?argl+2
ifdef ?t&n
ife ?t&n-1
	push	word ptr (n)
exitm
endif
ife ?t&n-2
	push	n
exitm
endif
ife ?t&n-4
	push	word ptr (n)[2]
	push	word ptr (n)
?argl=?argl+2
exitm
endif
ife ?t&n-8
	push	word ptr (n)[6]
	push	word ptr (n)[4]
	push	word ptr (n)[2]
	push	word ptr (n)
?argl=?argl+6
exitm
endif
if ?t&n and 80h
n
?argl=?argl+2
exitm
endif
ife ?t&n
	push	word ptr (n)
exitm
endif
endif
	push	n
endm
cCall macro n,a,c
ifnb <a>
arg <a>
endif
mpush %?rsl
ifdef ?CC&n
?cCall1=?CC&n
else
?cCall1=?PLM
endif
ifnb <c>
?gcc ?cCall1,%?cCall1,<c>
endif
?argl=0
ife ?cCall1
?acb=?argc
else
?acb=1
endif
rept ?argc
uconcat <?ali>,%?acb
uconcat <purge>,,<?ali>,%?acb
ife ?cCall1
?acb=?acb-1
else
?acb=?acb+1
endif
endm
	call	n
if ((?cCall1 eq 0) and (?argl ne 0))
	add	sp,?argl
endif
mpop %?rsl
?rsl=0
?argc= 0
?argl= 0
endm
cProc macro n,cf,a
if ?cpd
?utpe
endif
?cpd=1
???=0
?argc=0
?ba=0
?po=0
?pu=0
?ia=0
?adj=4
?rp=0
?uf=0
?wfp=?WIN
?ff=0
?pas=0
?pcc=?PLM
ifnb <a>
?ri ?pas,<a>
endif
?pc=sizec
?nd=?nodata1
?nx=0
irp x,<cf>
ifidn <x>,<FAR>
?pc=1
endif
ifidn <x>,<NEAR>
?pc=0
endif
ifidn <x>,<PUBLIC>
?pu=1
endif
ifidn <x>,<SMALL>
?uf=1
endif
ifidn <x>,<DATA>
?nd=0
endif
ifidn <x>,<NODATA>
?nd=1
endif
ifidn <x>,<ATOMIC>
?nx=1
endif
ifidn <x>,<C>
?pcc=0
endif
ifidn <x>,<PLM>
?pcc=1
endif
ifidn <x>,<PASCAL>
?pcc=1
endif
ifidn <x>,<WIN>
?wfp=1
endif
ifidn <x>,<NONWIN>
?wfp=0
endif
endm
if ?pcc
?PLMPrevParm=0
.xcref
.xcref ?PLMParm0
.cref
?PLMParm0 &macro
purge ?PLMParm0
&endm
endif
.xcref
.xcref ?CC&n
.cref
?CC&n=?pcc
if (?nx eq 1) and (?nd eq 0)
?nx = 0
??error <ATOMIC specified without NODATA - ATOMIC ignored>
endif
if ?pc
if ?wfp
ife ?nx
?ia=2
?pas = ?pas and (not ?ds)
endif
endif
?adj=?adj+2
else
?wfp=0
endif
?pas = ?pas and (not (?sp+?cs+?ss))
if ?uf
?pas = ?pas and (not (?bp+?si+?di))
endif
ife ?pcc
?pg <_&n>,%?pu,%?pc,%?pas,%?wfp,<n>,%?pcc
else
?pg <n>,%?pu,%?pc,%?pas,%?wfp,<n>,%?pcc
endif
endm
?pg macro n,p,c,a,w,nnu,cc
.xcref
if ?uf
if ?nd
??error <NODATA encountered in &n - user frame ignored>
?uf=0
endif
endif
.xcref cBegin
cBegin &macro g
.xcref
if cc
uconcat <?PLMParm>,%?PLMPrevParm,%?po
endif
if ?uf
if ?rp
??error <parmR encountered in &n - user frame ignored>
?uf=0
endif
endif
?pg1 <n>,c,a,%?po,w,%?uf,%?nd,%?rp,cc
?cpd=0
?argc=0
?ba=1
???=(???+1) and 0fffeh
if p
public n
endif
ife c
n proc near
else
n proc far
endif
ife cc
nnu equ n
endif
ifidn <g>,<nogen>
if ???+?po+a+?rp
??_out <cBegin - possible invalid use of nogen>
endif
else
if ?uf
?mf c,%???,%?po
mpush a
else
if w
ife ?nd
	mov	ax,ds
	nop
endif
ife ?nx
ife ?DOS5
	inc	bp
endif
	push	bp
	mov	bp,sp
	push	ds
else
if ?ff+???+?po+?rp
	push	bp
	mov	bp,sp
endif
endif
ife ?nd
	mov	ds,ax
endif
else
if ?ff+???+?po+?rp
	push	bp
	mov	bp,sp
endif
endif
if ?rp
?uf=0
rept ?rp
uconcat mpush,,?rp,%?uf
?uf=?uf+1
endm
endif
if ???
if ?chkstk1
ifdef ?CHKSTKPROC
?CHKSTKPROC %???
else
	mov	ax,???
ife cc
	call	_chkstk
else
	call	chkstk
endif
endif
else
	sub	sp,???
endif
endif
mpush a
endif
ifdef ?PROFILE
if c
	call	StartNMeas
endif
endif
endif
.cref
purge cBegin
&endm
.xcref ?utpe
?utpe &macro
??error <unterminated procedure definition: "&n">
&endm
.cref
endm
?pg1 macro n,c,a,o,w,f,d,r,cc
.xcref
.xcref cEnd
cEnd &macro g
.xcref
?ba=0
ifidn <g>,<nogen>
if o+a+r
??_out <cEnd - possible invalid use of nogen>
endif
else
ifdef ?PROFILE
if c
call StopNMeas
endif
endif
mpop a
if f
	db	0c3h
else
if w
ife ?nx
if (?TF eq 0) or (???+?rp)
	lea	sp,-2[bp]
endif
	pop	ds
	pop	bp
ife ?DOS5
	dec	bp
endif
else
if (?TF eq 0) or (???+?rp)
	mov	sp,bp
endif
if ???+?po+?rp
	pop	bp
endif
endif
else
if ?ff+???+?po+?rp
if (?TF eq 0) or (???+?rp)
	mov	sp,bp
endif
	pop	bp
endif
endif
ife cc
	ret
else
	ret	o
endif
endif
endif
n endp
.cref
purge cEnd
&endm
.cref
endm
assumes macro s,ln
ifndef ln&_assumes
assume s:ln
else
ln&_assumes s
endif
endm
createSeg macro n,ln,a,co,cl,grp
ifnb <grp>
addseg grp,n
else
ln&OFFSET equ offset n:
ln&BASE equ n
?cs3 <ln>,<n>
endif
ifnb <cl>
n segment a co '&cl'
else
n segment a co
endif
n ends
?cs1 <ln>,<n>
endm
addseg macro grp,seg
.xcref
.xcref grp&_def
.cref
ifndef grp&_def
grp&_def=0
endif
if grp&_def ne ASMpass
.xcref
.xcref grp&_add
.cref
grp&_add &macro s
grp&_in <seg>,s
&endm
.xcref
.xcref grp&_in
.cref
grp&_in &macro sl,s
ifb <s>
grp group sl
else
grp&_add &macro ns
grp&_in <sl,s>,ns
&endm
endif
&endm
grp&_def=ASMpass
else
grp&_add seg
endif
endm
defgrp macro grp,ln
addseg grp
ifnb <ln>
irp x,<ln>
?cs3 <&x>,<grp>
x&&OFFSET equ offset grp:
x&&BASE equ grp
endm
endif
endm
?cs1 macro ln,n
.xcref
.xcref ln&_sbegin
.cref
ln&_sbegin &macro
.xcref
.xcref ?mf
.cref
?mf &&macro c,l,p
if c
	extrn	n&_FARFRAME:near
	call	n&_FARFRAME
else
	extrn	n&_NEARFRAME:near
	call	n&_NEARFRAME
endif
	db	l shr 1
	db	p shr 1
&&endm
?cs2 <ln>,<n>
n segment
&endm
endm
?cs2 macro ln,n
.xcref
.xcref sEnd
.cref
sEnd &macro
n ends
purge ?mf
purge sEnd
&endm
endm
?cs3 macro ln,n
.xcref
.xcref ln&_assumes
.cref
ln&_assumes &macro s
assume s:&n
&endm
endm
.xcref
.xcref sBegin
.cref
sBegin macro ln
ln&_sbegin
endm
ife ?DF
createSeg _TEXT,Code,word,public,CODE
ife ?nodata1
createSeg _DATA,Data,word,public,DATA,DGROUP
defgrp DGROUP,Data
endif
if ?chkstk1
ifndef ?CHKSTKPROC
externp <chkstk>
endif
endif
endif
errnz macro x
if2
if x
errnz1 <x>,%(x)
endif
endif
endm
errnz1 macro x1,x2
= *errnz* x1 = x2
.err
endm
errn$ macro l,x
errnz <offset $ - offset l x>
endm
ifdef ?PROFILE
externFP <StartNMeas,StopNMeas>
endif
