<?php

/** Slovenian (Slovenščina)
 *
 * @ingroup Language
 */
class LanguageSl extends Language {
	# Convert from the nominative form of a noun to some other case
	# Invoked with {{GRAMMAR:case|word}}
	/**
	 * Cases: rodilnik, dajalnik, tožilnik, mestnik, orodnik
	 */
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['sl'][$case][$word]) ) {
			return $wgGrammarForms['sl'][$case][$word];
		}

		switch ( $case ) {
			case 'rodilnik': # genitive
				switch ( $word ) {
					case 'Wikipedija': $word = 'Wikipedije'; break 2;
					case 'Wikiknjige': $word = 'Wikiknjig'; break 2;
					case 'Wikinovice': $word = 'Wikinovic'; break 2;
					case 'Wikinavedek': $word = 'Wikinavedka'; break 2;
					case 'Wikivir': $word = 'Wikivira'; break 2;
					case 'Wikislovar': $word = 'Wikislovarja'; break 2;
				}
			case 'dajalnik': # dativ
				switch ( $word ) {
					case 'Wikipedija': $word = 'Wikipediji'; break 2;
					case 'Wikiknjige': $word = 'Wikiknjigam'; break 2;
					case 'Wikinovice': $word = 'Wikinovicam'; break 2;
					case 'Wikinavedek': $word = 'Wikinavedku'; break 2;
					case 'Wikivir': $word = 'Wikiviru'; break 2;
					case 'Wikislovar': $word = 'Wikislovarju'; break 2;
				}
			case 'tožilnik': # akuzatív
				switch ( $word ) {
					case 'Wikipedija': $word = 'Wikipedijo'; break 2;
					case 'Wikiknjige':
					case 'Wikinovice':
					case 'Wikinavedek':
					case 'Wikivir':
					case 'Wikislovar':
						// Don't change, just fall through
						break 2;
				}
			case 'mestnik': # locative
				switch ( $word ) {
					case 'Wikipedija': $word = 'o Wikipediji'; break 2;
					case 'Wikiknjige': $word = 'o Wikiknjigah'; break 2;
					case 'Wikinovice': $word = 'o Wikinovicah'; break 2;
					case 'Wikinavedek': $word = 'o Wikinavedku'; break 2;
					case 'Wikivir': $word = 'o Wikiviru'; break 2;
					case 'Wikislovar': $word = 'o Wikislovarju'; break 2;
					default: $word = 'o ' . $word; break 2;
				}
			case 'orodnik': # instrumental
				switch ( $word ) {
					case 'Wikipedija': $word = 'z Wikipedijo'; break 2;
					case 'Wikiknjige': $word = 'z Wikiknjigami'; break 2;
					case 'Wikinovice': $word = 'z Wikinovicami'; break 2;
					case 'Wikinavedek': $word = 'z Wikinavedkom'; break 2;
					case 'Wikivir': $word = 'z Wikivirom'; break 2;
					case 'Wikislovar': $word = 'z Wikislovarjem'; break 2;
					default: $word = 'z ' . $word;
				}
		}

		return $word; # this will return the original value for 'imenovalnik' (nominativ) and all undefined case values
	}

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 5 );

		if ( $count % 100 == 1 ) {
			$index = 0;
		} elseif ( $count % 100 == 2 ) {
			$index = 1;
		} elseif ( $count % 100 == 3 || $count % 100 == 4 ) {
			$index = 2;
		} elseif ( $count != 0 ) {
			$index = 3;
		} else {
			$index = 4;
		}
		return $forms[$index];
	}
}
