function write(x) { System.print(x); }
function regs(n) { return System.regs(n); }
function debugebp() { return regs(0); }
function debugeip() { return regs(1); }
function tf_argmark() { return regs(2); }
function tf_pointer() { return regs(3); }
function tf_tempcs() { return regs(4); } 
function tf_tempeip() { return regs(5); } 
function dr0() { return regs(6); } 
function dr1() { return regs(7); } 
function dr2() { return regs(8); } 
function dr3() { return regs(9); } 
function dr6() { return regs(10); } 
function dr7() { return regs(11); } 
function gs()  { return regs(12) & 0xffff; }
function es()  { return regs(13) & 0xffff; }
function ds()  { return regs(14) & 0xffff; }
function edx() { return regs(15); }
function ecx() { return regs(16); }
function eax() { return regs(17); }
function tf_pmode() { return regs(18); }
function tf_exl() { return regs(19); }
function fs()  { return regs(20) & 0xffff; }
function edi() { return regs(21); }
function esi() { return regs(22); }
function ebx() { return regs(23); }
function ebp() { return regs(24); }
function error_code() { return regs(25); }
function eip() { return regs(26); }
function cs()  { return regs(27) & 0xffff; }
function eflags() { return regs(28); }
function esp() { return regs(29); }
function ss()  { return regs(30) & 0xffff; }
function v86_es() { return regs(31) & 0xffff; }
function v86_ds() { return regs(32) & 0xffff; }
function v86_fs() { return regs(33) & 0xffff; }
function v86_gs() { return regs(34) & 0xffff; }
function peekl(a) { return System.mread(4,a); }
function pokel(a,b) { return System.mwrite(4,a,b); }
function peekw(a) { return System.mread(2,a); }
function pokew(a,b) { return System.mwrite(2,a,b); }
function peek(a) { return System.mread(1,a); }
function poke(a,b) { return System.mwrite(1,a,b); }
function regread(x,y) { return System.regread(x,y); }
function findmodule(str) { return System.findmodule(str); }

MOD_NAME = 0;
MOD_BASE = 1;
MOD_LEN  = 2;
MOD_SYM  = 3;
MOD_SYM_STABS_BASE   = 0;
MOD_SYM_STABS_LEN    = 1;
MOD_SYM_STABSTR_BASE = 2;
MOD_SYM_STABSTR_LEN  = 3;

function getlocals_ofmodule(mod,addr) {
    /* Ok, we have the module...  let's scan symbols if they exist */
    if( mod[MOD_SYM].length > 0 ) {
	stabs_base   = mod[MOD_SYM][MOD_SYM_STABS_BASE];
	stabs_len    = mod[MOD_SYM][MOD_SYM_STABS_LEN];
	stabstr_base = mod[MOD_SYM][MOD_SYM_STABSTR_BASE];
	stabstr_len  = mod[MOD_SYM][MOD_SYM_STABSTR_LEN];

	for( i = stabs_base; i < stabs_base + stabs_len; i++ ) {
	    if( )
	}
    }
}

function getlocals(addr) {
    /* Find the address range of addr */
    i = 0;
    do {
	mod = System.getmodule(i);
	if( mod && 
	    mod[MOD_BASE] <= addr && 
	    mod[MOD_BASE] + mod[MOD_LEN] > addr )
	    return getlocals_ofmodule(mod,addr);
	}
    } while( mod );

    return Array(0);
}
write('JS Registry Init Complete.  Welcome to ReactOS kernel scripting');
