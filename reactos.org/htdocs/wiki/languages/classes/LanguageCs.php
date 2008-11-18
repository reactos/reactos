<?php

/** Czech (česky)
 *
 * @ingroup Language
 */
class LanguageCs extends Language {
	# Grammatical transformations, needed for inflected languages
	# Invoked by putting {{grammar:case|word}} in a message
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['cs'][$case][$word]) ) {
			return $wgGrammarForms['cs'][$case][$word];
		}
		# allowed values for $case:
		#	1sg, 2sg, ..., 7sg -- nominative, genitive, ... (in singular)
		switch ( $word ) {
			case 'Wikibooks':
			case 'Wikiknihy':
				switch ( $case ) {
					case '2sg':
						return 'Wikiknih';
					case '3sg':
						return 'Wikiknihám';
					case '6sg':
						return 'Wikiknihách';
					case '7sg':
						return 'Wikiknihami';
					default:
						return 'Wikiknihy';
				}
			case 'Wikipedia':
			case 'Wikipedie':
				switch ( $case ) {
					case '3sg':
					case '4sg':
					case '6sg':
						return 'Wikipedii';
					case '7sg':
						return 'Wikipedií';
					default:
						return 'Wikipedie';
				}
			case 'Wiktionary':
			case 'Wikcionář':
			case 'Wikislovník':
				switch ( $case ) {
					case '2sg':
					case '3sg':
					case '5sg':
					case '6sg':
						return 'Wikislovníku';
					case '7sg':
						return 'Wikislovníkem';
					default:
						return 'Wikislovník';
				}
			case 'Wikiquote':
			case 'Wikicitáty':
				switch ( $case ) {
					case '2sg':
						return 'Wikicitátů';
					case '3sg':
						return 'Wikicitátům';
					case '6sg':
						return 'Wikicitátech';
					default:
						return 'Wikicitáty';
				}
			case 'Wikinews':
			case 'Wikizprávy':
				switch ( $case ) {
					case '2sg':
						return 'Wikizpráv';
					case '3sg':
						return 'Wikizprávám';
					case '6sg':
						return 'Wikizprávách';
					case '7sg':
						return 'Wikizprávami';
					default:
						return 'Wikizprávy';
				}
			case 'Wikiversity':
			case 'Wikiverzita':
				switch ( $case ) {
					case '2sg':
						return 'Wikiverzity';
					case '3sg':
					case '6sg':
						return 'Wikiverzitě';
					case '4sg':
						return 'Wikiverzitu';
					case '5sg':
						return 'Wikiverzito';
					case '7sg':
						return 'Wikiverzitou';
					default:
						return 'Wikiverzita';
				}
			case 'Wikispecies':
			case 'Wikidruhy':
				switch ( $case ) {
					case '2sg':
						return 'Wikidruhů';
					case '3sg':
						return 'Wikidruhům';
					case '6sg':
						return 'Wikidruzích';
					default:
						return 'Wikidruhy';
				}
		}
		# unknown
		return $word;
	}

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 3 );

		switch ( $count ) {
			case 1:
				return $forms[0];
			case 2:
			case 3:
			case 4:
				return $forms[1];
			default:
				return $forms[2];
		}
	}
}
