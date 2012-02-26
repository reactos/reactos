#ifndef _UHCIDBG_H_
#define _UHCIDBG_H_

#define DBGLVL_OFF				0		// if gDebugLevel set to this, there is NO debug output
#define DBGLVL_MINIMUM			1		// minimum verbosity
#define DBGLVL_DEFAULT			2		// default verbosity level if no registry override
#define DBGLVL_MEDIUM			3		// medium verbosity
#define DBGLVL_HIGH				4		// highest 'safe' level (without severely affecting timing )
#define DBGLVL_MAXIMUM			5		// maximum level, may be dangerous
#define DBGLVL_ULTRA			6		// ultra, prints hell lots of stuff from ISR/allocs/etc

#ifndef DBGSTR_PREFIX
#define DBGSTR_PREFIX "wood_uhci: "
#endif

#define DEBUG_UHCI				TRUE
#define DEBUG_HUB				TRUE
#define DEBUG_DEV_MGR			TRUE

#define DPRINT DbgPrint


#define UHCI_DBGOUTSIZE              512

#define hcd_dbg_print_cond( ilev, cond, _x_) \
	if( debug_level && ( ilev <= debug_level ) && ( cond )) { \
			DPRINT( DBGSTR_PREFIX ); \
			DPRINT _x_ ; \
	}

#define hcd_dbg_print( ilev, _x_)  		hcd_dbg_print_cond( ilev, TRUE, _x_ )

extern ULONG debug_level;

#if DBG

#define uhci_dbg_print_cond( ilev, cond, _x_ )				hcd_dbg_print_cond( ilev, cond, _x_ )
#define uhci_dbg_print( ilev, _x_)  	hcd_dbg_print_cond( ilev, TRUE, _x_ )

#define uhci_trap_cond( ilev, cond ) 	if ( debug_level && ( ilev <= debug_level ) && (cond) ) TRAP()
#define uhci_trap( ilev )      			uhci_trap_cond( ilev, TRUE )


#define uhci_assert( cond ) 			ASSERT( cond )
#define dbg_count_list( _x_ ) 			usb_count_list( _x_ )

#define TRAP() DbgBreakPoint()

#else // if not DBG

// dummy definitions that go away in the retail build

#define uhci_dbg_print_cond( ilev, cond, _x_ )
#define uhci_dbg_print( ilev, _x_)
#define uhci_trap_cont( ilev, cond )
#define uhci_trap( ilev )
#define uhci_assert( cond )
#define TRAP()
#define dbg_count_list( _x_ )   0

#endif //DBG

#define usb_dbg_print( ilev, _x_ )    				uhci_dbg_print( ilev, _x_ )
#define ehci_dbg_print( ilev, _x_ )				 	uhci_dbg_print( ilev, _x_ )
#define ohci_dbg_print( ilev, _x_ )					uhci_dbg_print( ilev, _x_ )
#define ehci_dbg_print_cond( ilev, cond, _x_ )		uhci_dbg_print_cond( ilev, cond, _x_ )

#define DO_NOTHING

LONG usb_count_list( struct _LIST_ENTRY* list_head );
#endif // included
