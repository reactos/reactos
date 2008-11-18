<?php
/** Arabic (العربية)
 *
 * @ingroup Language
 *
 * @author Niklas Laxström
 */
class LanguageAr extends Language {

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 5 );

		if ( $count == 1 ) {
			$index = 0;
		} elseif( $count == 2 ) {
			$index = 1;
		} elseif( $count < 11 && $count > 2 ) {
			$index = 2;
		} elseif( $count % 100 == 0) {
			$index = 3;
		} else {
			$index = 4;
		}
		return $forms[$index];
	}
}
