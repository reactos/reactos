/*
 * Basic support for controlling the 8259 Programmable Interrupt Controllers.
 *
 * Initially written by Michael Brown (mcb30).
 */

#include <etherboot.h>
#include <pic8259.h>

#ifdef DEBUG_IRQ
#define DBG(...) printf ( __VA_ARGS__ )
#else
#define DBG(...)
#endif

/* Current locations of trivial IRQ handler.  These will change at
 * runtime when relocation is used; the handler needs to be copied to
 * base memory before being installed.
 */
void (*trivial_irq_handler)P((void)) = _trivial_irq_handler;
uint16_t volatile *trivial_irq_trigger_count = &_trivial_irq_trigger_count;
segoff_t *trivial_irq_chain_to = &_trivial_irq_chain_to;
uint8_t *trivial_irq_chain = &_trivial_irq_chain;
irq_t trivial_irq_installed_on = IRQ_NONE;

/* Previous trigger count for trivial IRQ handler */
static uint16_t trivial_irq_previous_trigger_count = 0;

/* Install a handler for the specified IRQ.  Address of previous
 * handler will be stored in previous_handler.  Enabled/disabled state
 * of IRQ will be preserved across call, therefore if the handler does
 * chaining, ensure that either (a) IRQ is disabled before call, or
 * (b) previous_handler points directly to the place that the handler
 * picks up its chain-to address.
 */

int install_irq_handler ( irq_t irq, segoff_t *handler,
			  uint8_t *previously_enabled,
			  segoff_t *previous_handler ) {
	segoff_t *irq_vector = IRQ_VECTOR ( irq );
	*previously_enabled = irq_enabled ( irq );

	if ( irq > IRQ_MAX ) {
		DBG ( "Invalid IRQ number %d\n" );
		return 0;
	}

	previous_handler->segment = irq_vector->segment;
	previous_handler->offset = irq_vector->offset;
	if ( *previously_enabled ) disable_irq ( irq );
	DBG ( "Installing handler at %hx:%hx for IRQ %d, leaving %s\n",
		  handler->segment, handler->offset, irq,
		  ( *previously_enabled ? "enabled" : "disabled" ) );
	DBG ( "...(previous handler at %hx:%hx)\n",
		  previous_handler->segment, previous_handler->offset );
	irq_vector->segment = handler->segment;
	irq_vector->offset = handler->offset;
	if ( *previously_enabled ) enable_irq ( irq );
	return 1;
}

/* Remove handler for the specified IRQ.  Routine checks that another
 * handler has not been installed that chains to handler before
 * uninstalling handler.  Enabled/disabled state of the IRQ will be
 * restored to that specified by previously_enabled.
 */

int remove_irq_handler ( irq_t irq, segoff_t *handler,
			 uint8_t *previously_enabled,
			 segoff_t *previous_handler ) {
	segoff_t *irq_vector = IRQ_VECTOR ( irq );

	if ( irq > IRQ_MAX ) {
		DBG ( "Invalid IRQ number %d\n" );
		return 0;
	}
	if ( ( irq_vector->segment != handler->segment ) ||
	     ( irq_vector->offset != handler->offset ) ) {
		DBG ( "Cannot remove handler for IRQ %d\n" );
		return 0;
	}

	DBG ( "Removing handler for IRQ %d\n", irq );
	disable_irq ( irq );
	irq_vector->segment = previous_handler->segment;
	irq_vector->offset = previous_handler->offset;
	if ( *previously_enabled ) enable_irq ( irq );
	return 1;
}

/* Install the trivial IRQ handler.  This routine installs the
 * handler, tests it and enables the IRQ.
 */

int install_trivial_irq_handler ( irq_t irq ) {
	segoff_t trivial_irq_handler_segoff = SEGOFF(trivial_irq_handler);
	
	if ( trivial_irq_installed_on != IRQ_NONE ) {
		DBG ( "Can install trivial IRQ handler only once\n" );
		return 0;
	}
	if ( SEGMENT(trivial_irq_handler) > 0xffff ) {
		DBG ( "Trivial IRQ handler not in base memory\n" );
		return 0;
	}

	DBG ( "Installing trivial IRQ handler on IRQ %d\n", irq );
	if ( ! install_irq_handler ( irq, &trivial_irq_handler_segoff,
				     trivial_irq_chain,
				     trivial_irq_chain_to ) )
		return 0;
	trivial_irq_installed_on = irq;

	DBG ( "Testing trivial IRQ handler\n" );
	disable_irq ( irq );
	*trivial_irq_trigger_count = 0;
	trivial_irq_previous_trigger_count = 0;
	fake_irq ( irq );
	if ( ! trivial_irq_triggered ( irq ) ) {
		DBG ( "Installation of trivial IRQ handler failed\n" );
		remove_trivial_irq_handler ( irq );
		return 0;
	}
	DBG ( "Trivial IRQ handler installed successfully\n" );
	enable_irq ( irq );
	return 1;
}

/* Remove the trivial IRQ handler.
 */

int remove_trivial_irq_handler ( irq_t irq ) {
	segoff_t trivial_irq_handler_segoff = SEGOFF(trivial_irq_handler);

	if ( trivial_irq_installed_on == IRQ_NONE ) return 1;
	if ( irq != trivial_irq_installed_on ) {
		DBG ( "Cannot uninstall trivial IRQ handler from IRQ %d; "
		      "is installed on IRQ %d\n", irq,
		      trivial_irq_installed_on );
		return 0;
	}

	if ( ! remove_irq_handler ( irq, &trivial_irq_handler_segoff,
				    trivial_irq_chain,
				    trivial_irq_chain_to ) )
		return 0;

	if ( trivial_irq_triggered ( trivial_irq_installed_on ) ) {
		DBG ( "Sending EOI for unwanted trivial IRQ\n" );
		send_specific_eoi ( trivial_irq_installed_on );
	}

	trivial_irq_installed_on = IRQ_NONE;
	return 1;
}

/* Safe method to detect whether or not trivial IRQ has been
 * triggered.  Using this call avoids potential race conditions.  This
 * call will return success only once per trigger.
 */

int trivial_irq_triggered ( irq_t irq ) {
	uint16_t trivial_irq_this_trigger_count = *trivial_irq_trigger_count;
	int triggered = ( trivial_irq_this_trigger_count -
			  trivial_irq_previous_trigger_count );
	
	/* irq is not used at present, but we have it in the API for
	 * future-proofing; in case we want the facility to have
	 * multiple trivial IRQ handlers installed simultaneously.
	 *
	 * Avoid compiler warning about unused variable.
	 */
	if ( irq == IRQ_NONE ) {};
	
	trivial_irq_previous_trigger_count = trivial_irq_this_trigger_count;
	return triggered ? 1 : 0;
}

/* Copy trivial IRQ handler to a new location.  Typically used to copy
 * the handler into base memory; when relocation is being used we need
 * to do this before installing the handler.
 *
 * Call with target=NULL in order to restore the handler to its
 * original location.
 */

int copy_trivial_irq_handler ( void *target, size_t target_size ) {
	irq_t currently_installed_on = trivial_irq_installed_on;
	uint32_t offset = ( target == NULL ? 0 :
			    target - &_trivial_irq_handler_start );

	if (( target != NULL ) && ( target_size < TRIVIAL_IRQ_HANDLER_SIZE )) {
		DBG ( "Insufficient space to copy trivial IRQ handler\n" );
		return 0;
	}

	if ( currently_installed_on != IRQ_NONE ) {
		DBG ("WARNING: relocating trivial IRQ handler while in use\n");
		if ( ! remove_trivial_irq_handler ( currently_installed_on ) )
			return 0;
	}

	/* Do the actual copy */
	if ( target != NULL ) {
		DBG ( "Copying trivial IRQ handler to %hx:%hx\n",
		      SEGMENT(target), OFFSET(target) );
		memcpy ( target, &_trivial_irq_handler_start,
			 TRIVIAL_IRQ_HANDLER_SIZE );
	} else {
		DBG ( "Restoring trivial IRQ handler to original location\n" );
	}
	/* Update all the pointers to structures within the handler */
	trivial_irq_handler = ( void (*)P((void)) )
		( (void*)_trivial_irq_handler + offset );
	trivial_irq_trigger_count = (uint16_t*)
		( (void*)&_trivial_irq_trigger_count + offset );
	trivial_irq_chain_to = (segoff_t*)
		( (void*)&_trivial_irq_chain_to + offset );
	trivial_irq_chain = (uint8_t*)
		( (void*)&_trivial_irq_chain + offset );

	if ( currently_installed_on != IRQ_NONE ) {
		if ( ! install_trivial_irq_handler ( currently_installed_on ) )
			return 0;
	}
	return 1;
}

/* Send non-specific EOI(s).  This seems to be inherently unsafe.
 */

void send_nonspecific_eoi ( irq_t irq ) {
	DBG ( "Sending non-specific EOI for IRQ %d\n", irq );
	if ( irq >= IRQ_PIC_CUTOFF ) {
		outb ( ICR_EOI_NON_SPECIFIC, PIC2_ICR );
	}		
	outb ( ICR_EOI_NON_SPECIFIC, PIC1_ICR );
}

/* Send specific EOI(s).
 */

void send_specific_eoi ( irq_t irq ) {
	DBG ( "Sending specific EOI for IRQ %d\n", irq );
	outb ( ICR_EOI_SPECIFIC | ICR_VALUE(irq), ICR_REG(irq) );
	if ( irq >= IRQ_PIC_CUTOFF ) {
		outb ( ICR_EOI_SPECIFIC | ICR_VALUE(CHAINED_IRQ),
		       ICR_REG(CHAINED_IRQ) );
	}
}

/* Dump current 8259 status: enabled IRQs and handler addresses.
 */

#ifdef DEBUG_IRQ
void dump_irq_status ( void ) {
	int irq = 0;
	
	for ( irq = 0; irq < 16; irq++ ) {
		if ( irq_enabled ( irq ) ) {
			printf ( "IRQ%d enabled, ISR at %hx:%hx\n", irq,
				 IRQ_VECTOR(irq)->segment,
				 IRQ_VECTOR(irq)->offset );
		}
	}
}
#endif
