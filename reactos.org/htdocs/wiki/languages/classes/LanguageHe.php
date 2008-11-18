<?php

/**
 * Hebrew (עברית)
 *
 * @ingroup Language
 *
 * @author Rotem Liss
 */
class LanguageHe extends Language {
	/**
	 * Convert grammar forms of words.
	 *
	 * Available cases:
	 * "prefixed" (or "תחילית") - when the word has a prefix
	 *
	 * @param string the word to convert
	 * @param string the case
	 */
	public function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['he'][$case][$word]) ) {
			return $wgGrammarForms['he'][$case][$word];
		}
		
		switch ( $case ) {
			case 'prefixed':
			case 'תחילית':
				# Duplicate the "Waw" if prefixed
				if ( substr( $word, 0, 2 ) == "ו" && substr( $word, 0, 4 ) != "וו" ) {
					$word = "ו".$word;
				}
				
				# Remove the "He" if prefixed
				if ( substr( $word, 0, 2 ) == "ה" ) {
					$word = substr( $word, 2 );
				}
				
				# Add a hyphen if non-Hebrew letters
				if ( substr( $word, 0, 2 ) < "א" || substr( $word, 0, 2 ) > "ת" ) {
					$word = "־".$word;
				}
		}
		
		return $word;
	}
	
	/**
	 * Gets a number and uses the suited form of the word.
	 *
	 * @param integer the number of items
	 * @param string the first form (singular)
	 * @param string the second form (plural)
	 * @param string the third form (2 items, plural is used if not applicable and not specified
	 * @param not used (for compatibility with ancestor)
	 * @param not used (for compatibility with ancestor)
	 *
	 * @return string of the suited form of word
	 */
	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 3 );

		if ( $count == '1' ) {
			return $forms[0];
		} elseif ( $count == '2' && isset($forms[2]) ) {
			return $forms[2];
		} else {
			return $forms[1];
		}
	}
}
