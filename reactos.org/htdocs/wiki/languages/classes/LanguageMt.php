<?php

/** Maltese (Malti)
 *
 * @ingroup Language
 *
 * @author Niklas Laxström
 */

class LanguageMt extends Language {
	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }

		$forms = $this->preConvertPlural( $forms, 4 );

		if ( $count === 1 ) $index = 0;
		elseif ( $count === 0 || ( $count%100>1 && $count%100<11) ) $index = 1;
		elseif ( $count%100>10 && $count%100<20 ) $index = 2;
		else $index = 3;
		return $forms[$index];
	}
}