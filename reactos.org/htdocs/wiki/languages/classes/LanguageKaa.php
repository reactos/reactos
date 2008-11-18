<?php

/** Karakalpak (Qaraqalpaqsha)
 *
 * @ingroup Language
 */
class LanguageKaa extends Language {

	# Convert from the nominative form of a noun to some other case
	# Invoked with {{GRAMMAR:case|word}}
	/**
	 * Cases: genitive, dative, accusative, locative, ablative, comitative + possessive forms
	 */
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset( $wgGrammarForms['kaa'][$case][$word] ) ) {
		     return $wgGrammarForms['kaa'][$case][$word];
		}
		/* Full code of function convertGrammar() is in development. Updates coming soon. */
		return $word;
	}
	/*
	 * It fixes issue with ucfirst for transforming 'i' to 'İ'
	 *
	 */
	function ucfirst ( $string ) {
		if ( $string[0] == 'i' ) {
			$string = 'İ' . substr( $string, 1 );
		} else {
			$string = parent::ucfirst( $string );
		}
		return $string;

	}

	/*
	 * It fixes issue with  lcfirst for transforming 'I' to 'ı'
	 *
	 */
	function lcfirst ( $string ) {
		if ( $string[0] == 'I' ) {
			$string = 'ı' . substr( $string, 1 );
		} else {
			$string = parent::lcfirst( $string );
		}
		return $string;
	}

	/**
	 * Avoid grouping whole numbers between 0 to 9999
	 */
	function commafy( $_ ) {
		if ( !preg_match( '/^\d{1,4}$/', $_ ) ) {
			return strrev( (string)preg_replace( '/(\d{3})(?=\d)(?!\d*\.)/', '$1,', strrev($_) ) );
		} else {
			return $_;
		}
	}

}
