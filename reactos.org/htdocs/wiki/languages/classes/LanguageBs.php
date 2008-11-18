<?php

/** Bosnian (bosanski)
 *
 * @ingroup Language
 */
class LanguageBs extends Language {

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 3 );

		if ($count > 10 && floor(($count % 100) / 10) == 1) {
			return $forms[2];
		} else {
			switch ($count % 10) {
				case 1:  return $forms[0];
				case 2:
				case 3:
				case 4:  return $forms[1];
				default: return $forms[2];
			}
		}
	}

	# Convert from the nominative form of a noun to some other case
	# Invoked with {{GRAMMAR:case|word}}
	/**
	 * Cases: genitiv, dativ, akuzativ, vokativ, instrumental, lokativ
	 */
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['bs'][$case][$word]) ) {
			return $wgGrammarForms['bs'][$case][$word];
		}
		switch ( $case ) {
			case 'genitiv': # genitive
				if ( $word == 'Wikipedia' ) {
					$word = 'Wikipedije';
				} elseif ( $word == 'Wikiknjige' ) {
					$word = 'Wikiknjiga';
				} elseif ( $word == 'Wikivijesti' ) {
					$word = 'Wikivijesti';
				} elseif ( $word == 'Wikicitati' ) {
					$word = 'Wikicitata';
				} elseif ( $word == 'Wikiizvor' ) {
					$word = 'Wikiizvora';
				} elseif ( $word == 'Vikirječnik' ) {
					$word = 'Vikirječnika';
				}
			break;
			case 'dativ': # dative
				if ( $word == 'Wikipedia' ) {
					$word = 'Wikipediji';
				} elseif ( $word == 'Wikiknjige' ) {
					$word = 'Wikiknjigama';
				} elseif ( $word == 'Wikicitati' ) {
					$word = 'Wikicitatima';
				} elseif ( $word == 'Wikivijesti' ) {
					$word = 'Wikivijestima';
				} elseif ( $word == 'Wikiizvor' ) {
					$word = 'Wikiizvoru';
				} elseif ( $word == 'Vikirječnik' ) {
					$word = 'Vikirječniku';
				}
			break;
			case 'akuzativ': # akusative
				if ( $word == 'Wikipedia' ) {
					$word = 'Wikipediju';
				} elseif ( $word == 'Wikiknjige' ) {
					$word = 'Wikiknjige';
				} elseif ( $word == 'Wikicitati' ) {
					$word = 'Wikicitate';
				} elseif ( $word == 'Wikivijesti' ) {
					$word = 'Wikivijesti';
				} elseif ( $word == 'Wikiizvor' ) {
					$word = 'Wikiizvora';
				} elseif ( $word == 'Vikirječnik' ) {
					$word = 'Vikirječnika';
				}
			break;
			case 'vokativ': # vocative
				if ( $word == 'Wikipedia' ) {
					$word = 'Wikipedijo';
				} elseif ( $word == 'Wikiknjige' ) {
					$word = 'Wikiknjige';
				} elseif ( $word == 'Wikicitati' ) {
					$word = 'Wikicitati';
				} elseif ( $word == 'Wikivijesti' ) {
					$word = 'Wikivijesti';
				} elseif ( $word == 'Wikiizvor' ) {
					$word = 'Wikizivoru';
				} elseif ( $word == 'Vikirječnik' ) {
					$word = 'Vikirječniče';
				}
			break;
			case 'instrumental': # instrumental
				if ( $word == 'Wikipedia' ) {
					$word = 's Wikipediom';
				} elseif ( $word == 'Wikiknjige' ) {
					$word = 's Wikiknjigama';
				} elseif ( $word == 'Wikicitati' ) {
					$word = 's Wikicitatima';
				} elseif ( $word == 'Wikivijesti' ) {
					$word = 's Wikivijestima';
				} elseif ( $word == 'Wikiizvor' ) {
					$word = 's Wikiizvorom';
				} elseif ( $word == 'Vikirječnik' ) {
					$word = 's Vikirječnikom';
				} else {
					$word = 's ' . $word;
				}
			break;
			case 'lokativ': # locative
				if ( $word == 'Wikipedia' ) {
					$word = 'o Wikipediji';
				} elseif ( $word == 'Wikiknjige' ) {
					$word = 'o Wikiknjigama';
				} elseif ( $word == 'Wikicitati' ) {
					$word = 'o Wikicitatima';
				} elseif ( $word == 'Wikivijesti' ) {
					$word = 'o Wikivijestima';
				} elseif ( $word == 'Wikiizvor' ) {
					$word = 'o Wikiizvoru';
				} elseif ( $word == 'Vikirječnik' ) {
					$word = 'o Vikirječniku';
				} else {
					$word = 'o ' . $word;
				}
			break;
		}

		return $word; # this will return the original value for 'nominativ' (nominative) and all undefined case values
	}
}
