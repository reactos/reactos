<?php
/** Welsh (Cymraeg)
 *
 * @ingroup Language
 *
 * @author Niklas Laxström
 */
class LanguageCy extends Language {
	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 6 );
		$count = abs( $count );
		if ( $count >= 0 && $count <= 3 ) {
			return $forms[$count];
		} elseif ( $count == 6 ) {
			return $forms[4];
		} else {
			return $forms[5];
		}
	}
}
