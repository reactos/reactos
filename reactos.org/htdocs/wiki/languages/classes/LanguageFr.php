<?php

/** French (Français)
 *
 * @ingroup Language
 */
class LanguageFr extends Language {
	/**
	 * Use singular form for zero (see bug 7309)
	 */
	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 2 );

		return ($count <= 1) ? $forms[0] : $forms[1];
	}
}
