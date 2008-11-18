<?php

/**
 * Slovak (Slovenčina)
 *
 * @ingroup Language
 */
class LanguageSk extends Language {
	# Convert from the nominative form of a noun to some other case
	# Invoked with {{GRAMMAR:case|word}}
	/**
	 * Cases: genitív, datív, akuzatív, lokál, inštrumentál
	 */
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['sk'][$case][$word]) ) {
			return $wgGrammarForms['sk'][$case][$word];
		}

		switch ( $case ) {
			case 'genitív':
				if ( $word == 'Wikipédia' ) {
					$word = 'Wikipédie';
				} elseif ( $word == 'Wikislovník' ) {
					$word = 'Wikislovníku';
				} elseif ( $word == 'Wikicitáty' ) {
					$word = 'Wikicitátov';
				} elseif ( $word == 'Wikiknihy' ) {
					$word = 'Wikikníh';
				}
			break;
			case 'datív':
				if ( $word == 'Wikipédia' ) {
					$word = 'Wikipédii';
				} elseif ( $word == 'Wikislovník' ) {
					$word = 'Wikislovníku';
				} elseif ( $word == 'Wikicitáty' ) {
					$word = 'Wikicitátom';
				} elseif ( $word == 'Wikiknihy' ) {
					$word = 'Wikiknihám';
				}
			break;
			case 'akuzatív':
				if ( $word == 'Wikipédia' ) {
					$word = 'Wikipédiu';
				} elseif ( $word == 'Wikislovník' ) {
					$word = 'Wikislovník';
				} elseif ( $word == 'Wikicitáty' ) {
					$word = 'Wikicitáty';
				} elseif ( $word == 'Wikiknihy' ) {
					$word = 'Wikiknihy';
				}
			break;
			case 'lokál':
				if ( $word == 'Wikipédia' ) {
					$word = 'Wikipédii';
				} elseif ( $word == 'Wikislovník' ) {
					$word = 'Wikislovníku';
				} elseif ( $word == 'Wikicitáty' ) {
					$word = 'Wikicitátoch';
				} elseif ( $word == 'Wikiknihy' ) {
					$word = 'Wikiknihách';
				}
			break;
			case 'inštrumentál':
				if ( $word == 'Wikipédia' ) {
					$word = 'Wikipédiou';
				} elseif ( $word == 'Wikislovník' ) {
					$word = 'Wikislovníkom';
				} elseif ( $word == 'Wikicitáty' ) {
					$word = 'Wikicitátmi';
				} elseif ( $word == 'Wikiknihy' ) {
					$word = 'Wikiknihami';
				}
			break;
		}
		return $word;
	}

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 3 );

		if ( $count == 1 ) {
			$index = 0;
		} elseif ( $count == 2 || $count == 3 || $count == 4 ) {
			$index = 1;
		} else {
			$index = 2;
		}
		return $forms[$index];
	}
}
