<?php

/**
 * @ingroup Language
 */
class LanguageRmy extends Language {
	/**
	 * Convert from the nominative form of a noun to some other case
	 * Invoked with {{GRAMMAR:case|word}}
	 *
	 * Cases: nominative, genitive-m-sg, genitive-f-sg, dative, locative, ablative, instrumental
	 */
	public function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['rmy'][$case][$word]) ) {
			return $wgGrammarForms['rmy'][$case][$word];
		}

		switch ( $case ) {
			case 'genitive-m-sg': # genitive (m.sg.)
				if ( $word == 'Vikipidiya' ) {
					$word = 'Vikipidiyako';
				} elseif ( $word == 'Vikcyonaro' ) {
					$word = 'Vikcyonaresko';
				}
			break;
			case 'genitive-f-sg': # genitive (f.sg.)
				if ( $word == 'Vikipidiya' ) {
					$word = 'Vikipidiyaki';
				} elseif ( $word == 'Vikcyonaro' ) {
					$word = 'Vikcyonareski';
				}
			break;
			case 'genitive-pl': # genitive (pl.)
				if ( $word == 'Vikipidiya' ) {
					$word = 'Vikipidiyake';
				} elseif ( $word == 'Vikcyonaro' ) {
					$word = 'Vikcyonareske';
				}
			break;
			case 'dativ':
				if ( $word == 'Vikipidiyake' ) {
					$word = 'Wikipediji';
				} elseif ( $word == 'Vikcyonaro' ) {
					$word = 'Vikcyonareske';
				}
			break;
			case 'locative':
				if ( $word == 'Vikipidiyate' ) {
					$word = 'Wikipedijo';
				} elseif ( $word == 'Vikcyonaro' ) {
					$word = 'Vikcyonareste';
				}
			break;
			case 'ablative':
				if ( $word == 'Vikipidiyatar' ) {
					$word = 'o Wikipediji';
				} elseif ( $word == 'Vikcyonaro' ) {
					$word = 'Vikcyonarestar';
				}
			break;
			case 'instrumental':
				if ( $word == 'Vikipidiyasa' ) {
					$word = 'z Wikipedijo';
				} elseif ( $word == 'Vikcyonaro' ) {
					$word = 'Vikcyonaresa';
				}
			break;
		}

		return $word; # this will return the original value for 'nominative' and all undefined case values
	}
}
