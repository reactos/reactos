<?php
/** Samogitian (Žemaitėška)
 *
 * @ingroup Language
 *
 * @author Niklas Laxström
 */
class LanguageBat_smg extends Language {

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 4 );

		$count = abs( $count );
		if ( $count === 0 || ($count%100 === 0 || ($count%100 >= 10 && $count%100 < 20)) ) {
			return $forms[2];
		} elseif ( $count%10 === 1 ) {
			return $forms[0];
		} elseif ( $count%10 === 2 ) {
			return $forms[1];
		} else {
			return $forms[3];
		}
	}
}
