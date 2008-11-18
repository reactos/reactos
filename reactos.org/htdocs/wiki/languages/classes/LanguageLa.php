<?php

/** Latin (lingua Latina)
 *
 * @ingroup Language
 */
class LanguageLa extends Language {
	/**
	 * Convert from the nominative form of a noun to some other case
	 *
	 * Just used in a couple places for sitenames; special-case as necessary.
	 * Rules are far from complete.
	 *
	 * Cases: genitive, accusative, ablative
	 */
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['la'][$case][$word]) ) {
			return $wgGrammarForms['la'][$case][$word];
		}

		switch ( $case ) {
		case 'genitive':
			// only a few declensions, and even for those mostly the singular only
			$in  = array(	'/u[ms]$/',                	# 2nd declension singular
					'/ommunia$/',              	# 3rd declension neuter plural (partly)
					'/a$/',                    	# 1st declension singular
					'/libri$/', '/nuntii$/',   	# 2nd declension plural (partly)
					'/tio$/', '/ns$/', '/as$/',	# 3rd declension singular (partly)
					'/es$/'                    	# 5th declension singular
					);
			$out = array(	'i',
					'ommunium',
					'ae',
					'librorum', 'nuntiorum',
					'tionis', 'ntis', 'atis',
					'ei'
					);
			return preg_replace( $in, $out, $word );
		case 'accusative':
			// only a few declensions, and even for those mostly the singular only
			$in  = array(	'/u[ms]$/',                	# 2nd declension singular
					'/a$/',                    	# 1st declension singular
					'/ommuniam$/',              	# 3rd declension neuter plural (partly)
					'/libri$/', '/nuntii$/',   	# 2nd declension plural (partly)
					'/tio$/', '/ns$/', '/as$/',	# 3rd declension singular (partly)
					'/es$/'                    	# 5th declension singular
					);
			$out = array(	'um',
					'am',
					'ommunia',
					'libros', 'nuntios',
					'tionem', 'ntem', 'atem',
					'em'
					);
			return preg_replace( $in, $out, $word );
		case 'ablative':
			// only a few declensions, and even for those mostly the singular only
			$in  = array(	'/u[ms]$/',                	# 2nd declension singular
					'/ommunia$/',              	# 3rd declension neuter plural (partly)
					'/a$/',                    	# 1st declension singular
					'/libri$/', '/nuntii$/',   	# 2nd declension plural (partly)
					'/tio$/', '/ns$/', '/as$/',	# 3rd declension singular (partly)
					'/es$/'                    	# 5th declension singular
					);
			$out = array(	'o',
					'ommunibus',
					'a',
					'libris', 'nuntiis',
					'tione', 'nte', 'ate',
					'e'
					);
			return preg_replace( $in, $out, $word );
		default:
			return $word;
		}
	}
}
