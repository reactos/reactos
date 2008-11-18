<?php

/** Irish (Gaeilge)
 *
 * @ingroup Language
 */
class LanguageGa extends Language {
	# Convert day names
	# Invoked with {{GRAMMAR:transformation|word}}
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['ga'][$case][$word]) ) {
			return $wgGrammarForms['ga'][$case][$word];
		}

		switch ( $case ) {
		case 'genitive':
			switch ($word) {
			case 'Vicipéid':     $word = 'Vicipéide'; break;
			case 'Vicífhoclóir': $word = 'Vicífhoclóra'; break;
			case 'Vicíleabhair': $word = 'Vicíleabhar'; break;
			case 'Vicíshliocht': $word = 'Vicíshleachta'; break;
			case 'Vicífhoinse':  $word = 'Vicífhoinse'; break;
			case 'Vicíghnéithe': $word = 'Vicíghnéithe'; break;
			case 'Vicínuacht':   $word = 'Vicínuachta'; break;
			}

		case 'ainmlae':
			switch ($word) {
			case 'an Domhnach':
				$word = 'Dé Domhnaigh'; break;
			case 'an Luan':
				$word = 'Dé Luain'; break;
			case 'an Mháirt':
				$word = 'Dé Mháirt'; break;
			case 'an Chéadaoin':
				$word = 'Dé Chéadaoin'; break;
			case 'an Déardaoin':
				$word = 'Déardaoin'; break;
			case 'an Aoine':
				$word = 'Dé hAoine'; break;
			case 'an Satharn':
				$word = 'Dé Sathairn'; break;
			}
		}
		return $word;
	}
}
