/*
 * Written by José Fonseca <j_r_fonseca@yahoo.co.uk>
 */


/*
 * void _mesa_mmx_blend( GLcontext *ctx,
 *                       GLuint n, 
 *                       const GLubyte mask[],
 *                       GLchan rgba[][4], 
 *                       CONST GLchan dest[][4] )
 * 
 */
ALIGNTEXT16
GLOBL GLNAME( TAG(_mesa_mmx_blend) )
HIDDEN( TAG(_mesa_mmx_blend) )
GLNAME( TAG(_mesa_mmx_blend) ):

    PUSH_L     ( EBP )
    MOV_L      ( ESP, EBP )
    PUSH_L     ( ESI )
    PUSH_L     ( EDI )
    PUSH_L     ( EBX )

    MOV_L      ( REGOFF(12, EBP), ECX )		/* n */
    CMP_L      ( CONST(0), ECX)
    JE         ( LLTAG(GMB_return) )

    MOV_L      ( REGOFF(16, EBP), EBX )		/* mask */
    MOV_L      ( REGOFF(20, EBP), EDI )         /* rgba */
    MOV_L      ( REGOFF(24, EBP), ESI )         /* dest */

    INIT
    
    TEST_L     ( CONST(4), EDI )		/* align rgba on an 8-byte boundary */
    JZ         ( LLTAG(GMB_align_end) )

    CMP_B      ( CONST(0), REGIND(EBX) )	/* *mask == 0 */
    JE         ( LLTAG(GMB_align_continue) )

    /* runin */
#define ONE(x)	x
#define TWO(x)  
    MAIN       ( EDI, ESI )
#undef ONE
#undef TWO

LLTAG(GMB_align_continue):

    DEC_L      ( ECX )				/* n -= 1 */
    INC_L      ( EBX )		                /* mask += 1 */
    ADD_L      ( CONST(4), EDI )		/* rgba += 1 */
    ADD_L      ( CONST(4), ESI )		/* dest += 1 */ 

LLTAG(GMB_align_end):

    CMP_L      ( CONST(2), ECX)
    JB         ( LLTAG(GMB_loop_end) )

ALIGNTEXT16
LLTAG(GMB_loop_begin):

    CMP_W      ( CONST(0), REGIND(EBX) )	/* *mask == 0 && *(mask + 1) == 0 */
    JE         ( LLTAG(GMB_loop_continue) )

    /* main loop */
#define ONE(x)
#define TWO(x)	x
    MAIN       ( EDI, ESI )
#undef ONE
#undef TWO

LLTAG(GMB_loop_continue):

    DEC_L      ( ECX )
    DEC_L      ( ECX )				/* n -= 2 */
    ADD_L      ( CONST(2), EBX )		/* mask += 2 */
    ADD_L      ( CONST(8), EDI )		/* rgba += 2 */
    ADD_L      ( CONST(8), ESI )		/* dest += 2 */ 
    CMP_L      ( CONST(2), ECX )
    JAE        ( LLTAG(GMB_loop_begin) )

LLTAG(GMB_loop_end):

    CMP_L      ( CONST(1), ECX )
    JB         ( LLTAG(GMB_done) )

    CMP_B      ( CONST(0), REGIND(EBX) )	/* *mask == 0 */
    JE         ( LLTAG(GMB_done) )

    /* runout */
#define ONE(x)	x
#define TWO(x)
    MAIN       ( EDI, ESI )
#undef ONE
#undef TWO

LLTAG(GMB_done):

    EMMS

LLTAG(GMB_return):

    POP_L      ( EBX )
    POP_L      ( EDI )
    POP_L      ( ESI )
    MOV_L      ( EBP, ESP )
    POP_L      ( EBP )
    RET

#undef TAG
#undef LLTAG
#undef INIT
#undef MAIN
