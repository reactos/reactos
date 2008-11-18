<?php

/** Lower Sorbian (Dolnoserbski)
 *
 * @ingroup Language
 */
class LanguageDsb extends Language {

	# Convert from the nominative form of a noun to some other case
	# Invoked with {{GRAMMAR:case|word}}

	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset( $wgGrammarForms['hsb'][$case][$word] ) ) {
			return $wgGrammarForms['hsb'][$case][$word];
		}

		switch ( $case ) {
			case 'genitiw': # genitive
				if ( $word == 'Wikipedija' ) {
					$word = 'Wikipedije';
				} elseif ( $word == 'Wikiknihi' ) {
					$word = 'Wikiknih';
				} elseif ( $word == 'Wikinowiny' ) {
					$word = 'Wikinowin';
				} elseif ( $word == 'Wikižórło' ) {
					$word = 'Wikižórła';
				} elseif ( $word == 'Wikicitaty' ) {
					$word = 'Wikicitatow';
				} elseif ( $word == 'Wikisłownik' ) {
					$word = 'Wikisłownika';
				}
				break;
			case 'datiw': # dativ
				if ( $word == 'Wikipedija' ) {
					$word = 'Wikipediji';
				} elseif ( $word == 'Wikiknihi' ) {
					$word = 'Wikikniham';
				} elseif ( $word == 'Wikinowiny' ) {
					$word = 'Wikinowinam';
				} elseif ( $word == 'Wikižórło' ) {
					$word = 'Wikižórłu';
				} elseif ( $word == 'Wikicitaty' ) {
					$word = 'Wikicitatam';
				} elseif ( $word == 'Wikisłownik' ) {
					$word = 'Wikisłownikej';
				}
				break;
			case 'akuzativ': # akuzativ
				if ( $word == 'Wikipedija' ) {
					$word = 'Wikipediju';
				} elseif ( $word == 'Wikiknihi' ) {
					$word = 'Wikiknknihi';
				} elseif ( $word == 'Wikinowiny' ) {
					$word = 'Wikinowiny';
				} elseif ( $word == 'Wikižórło' ) {
					$word = 'Wikižórło';
				} elseif ( $word == 'Wikicitaty' ) {
					$word = 'Wikicitaty';
				} elseif ( $word == 'Wikisłownik' ) {
					$word = 'Wikisłownik';
				}
				break;
			case 'instrumental': # instrumental
				if ( $word == 'Wikipedija' ) {
					$word = 'Wikipediju';
				} elseif ( $word == 'Wikiknihi' ) {
					$word = 'Wikiknihami';
				} elseif ( $word == 'Wikinowiny' ) {
					$word = 'Wikinowinami';
				} elseif ( $word == 'Wikižórło' ) {
					$word = 'Wikižórłom';
				} elseif ( $word == 'Wikicitaty' ) {
					$word = 'Wikicitatami';
				} elseif ( $word == 'Wikisłownik' ) {
					$word = 'Wikisłownikom';
				} else {
					$word = 'z ' . $word;
				}
				break;
			case 'lokatiw': # lokatiw
				if ( $word == 'Wikipedija' ) {
					$word = 'Wikipediji';
				} elseif ( $word == 'Wikiknihi' ) {
					$word = 'Wikiknihach';
				} elseif ( $word == 'Wikinowiny' ) {
					$word = 'Wikinowinach';
				} elseif ( $word == 'Wikižórło' ) {
					$word = 'Wikižórłu';
				} elseif ( $word == 'Wikicitaty' ) {
					$word = 'Wikicitatach';
				} elseif ( $word == 'Wikisłownik' ) {
					$word = 'Wikisłowniku';
				} else {
					$word = 'wo ' . $word;
				}
				break;
			}

		return $word; # this will return the original value for 'nominatiw' (nominativ) and all undefined case values
	}

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 4 );

		switch ( abs( $count ) % 100 ) {
			case 1:  return $forms[0]; // singular
			case 2:  return $forms[1]; // dual
			case 3:
			case 4:  return $forms[2]; // plural
			default: return $forms[3]; // pluralgen
		}
	}
}
