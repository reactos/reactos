/**
 * Set up the protection chaining interface (i.e. "unlock move permissions" checkbox)
 * on the protection form
 *
 * @param String tableId Identifier of the table containing UI bits
 * @param String labelText Text to use for the checkbox label
 */
function protectInitialize( tableId, labelText, types ) {
	if( !( document.createTextNode && document.getElementById && document.getElementsByTagName ) )
		return false;
		
	var box = document.getElementById( tableId );
	if( !box )
		return false;
		
	var tbody = box.getElementsByTagName( 'tbody' )[0];
	var row = document.createElement( 'tr' );
	tbody.appendChild( row );
	
	row.appendChild( document.createElement( 'td' ) );
	var col = document.createElement( 'td' );
	row.appendChild( col );
	// If there is only one protection type, there is nothing to chain
	if( types > 1 ) {
		var check = document.createElement( 'input' );
		check.id = 'mwProtectUnchained';
		check.type = 'checkbox';
		col.appendChild( check );
		addClickHandler( check, protectChainUpdate );

		col.appendChild( document.createTextNode( ' ' ) );
		var label = document.createElement( 'label' );
		label.htmlFor = 'mwProtectUnchained';
		label.appendChild( document.createTextNode( labelText ) );
		col.appendChild( label );

		check.checked = !protectAllMatch();
		protectEnable( check.checked );
	}
	
	setCascadeCheckbox();
	
	return true;
}

/**
* Determine if, given the cascadeable protection levels
* and what is currently selected, if the cascade box 
* can be checked
*
* @return boolean
*
*/
function setCascadeCheckbox() {
	// For non-existent titles, there is no cascade option
	if( !document.getElementById( 'mwProtect-cascade' ) ) {
		return false;
	}
	var lists = protectSelectors();
	for( var i = 0; i < lists.length; i++ ) {
		if( lists[i].selectedIndex > -1 ) {
			var items = lists[i].getElementsByTagName( 'option' );
			var selected = items[ lists[i].selectedIndex ].value;
			if( !isCascadeableLevel(selected) ) {
				document.getElementById( 'mwProtect-cascade' ).checked = false;
				document.getElementById( 'mwProtect-cascade' ).disabled = true;
				return false;
			}
		}
	}
	document.getElementById( 'mwProtect-cascade' ).disabled = false;
	return true;
}

/**
* Is this protection level cascadeable?
* @param String level
*
* @return boolean
*
*/
function isCascadeableLevel( level ) { 	 
	for (var k = 0; k < wgCascadeableLevels.length; k++) { 	 
		if ( wgCascadeableLevels[k] == level ) { 	 
			return true; 	 
		} 
	} 	 
	return false; 	 
}

/**
 * When protection levels are locked together, update the rest
 * when one action's level changes
 *
 * @param Element source Level selector that changed
 */
function protectLevelsUpdate(source) {
	if( !protectUnchained() )
		protectUpdateAll( source.selectedIndex );
	setCascadeCheckbox();
}

/**
 * Update chain status and enable/disable various bits of the UI
 * when the user changes the "unlock move permissions" checkbox
 */
function protectChainUpdate() {
	if( protectUnchained() ) {
		protectEnable( true );
	} else {
		protectChain();
		protectEnable( false );
	}
	setCascadeCheckbox();
}

/**
 * Are all actions protected at the same level?
 *
 * @return boolean
 */
function protectAllMatch() {
	var values = new Array();
	protectForSelectors(function(set) {
		values[values.length] = set.selectedIndex;
	});
	for (var i = 1; i < values.length; i++) {
		if (values[i] != values[0]) {
			return false;
		}
	}
	return true;
}

/**
 * Is protection chaining on or off?
 *
 * @return bool
 */
function protectUnchained() {
	var unchain = document.getElementById( 'mwProtectUnchained' );
	return unchain
		? unchain.checked
		: true; // No control, so we need to let the user set both levels
}

/**
 * Find the highest-protected action and set all others to that level
 */
function protectChain() {
	var maxIndex = -1;
	protectForSelectors(function(set) {
		if (set.selectedIndex > maxIndex) {
			maxIndex = set.selectedIndex;
		}
	});
	protectUpdateAll(maxIndex);
}

/**
 * Protect all actions at the specified level
 *
 * @param int index Protection level
 */
function protectUpdateAll(index) {
	protectForSelectors(function(set) {
		if (set.selectedIndex != index) {
			set.selectedIndex = index;
		}
	});
}

/**
 * Apply a callback to each protection selector
 *
 * @param callable func Callback function
 */
function protectForSelectors(func) {
	var selectors = protectSelectors();
	for (var i = 0; i < selectors.length; i++) {
		func(selectors[i]);
	}
}

/**
 * Get a list of all protection selectors on the page
 *
 * @return Array
 */
function protectSelectors() {
	var all = document.getElementsByTagName("select");
	var ours = new Array();
	for (var i = 0; i < all.length; i++) {
		var set = all[i];
		if (set.id.match(/^mwProtect-level-/)) {
			ours[ours.length] = set;
		}
	}
	return ours;
}

/**
 * Enable/disable protection selectors
 *
 * @param boolean val Enable?
 */
function protectEnable(val) {
	// fixme
	var first = true;
	protectForSelectors(function(set) {
		if (first) {
			first = false;
		} else {
			set.disabled = !val;
			set.style.visible = val ? "visible" : "hidden";
		}
	});
}
