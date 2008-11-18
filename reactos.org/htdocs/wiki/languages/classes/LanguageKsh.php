<?php

/** Ripuarian (Ripoarėsh)
 *
 * @ingroup Language
 *
 * @author Purodha Blissenbach
 */
class LanguageKsh extends Language {
	/**
	 * Avoid grouping whole numbers between 0 to 9999
	 */
	public function commafy( $_ ) {
		if ( !preg_match( '/^\d{1,4}$/', $_ ) ) {
			return strrev( (string)preg_replace( '/(\d{3})(?=\d)(?!\d*\.)/', '$1,', strrev( $_ ) ) );
		} else {
			return $_;
		}
	}

	/**
	 * Handle cases of (1, other, 0) or (1, other)
	 */
	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 3 );

		if ( $count == 1 ) {
			return $forms[0];
		} elseif ( $count == 0 ) {
			return $forms[2];
		} else {
			return $forms[1];
		}
	}
}
