<?php

/** Old Church Slavonic (Ѩзыкъ словѣньскъ)
 *
 * @ingroup Language
 */
class LanguageCu extends Language {
	# Convert from the nominative form of a noun to some other case
	# Invoked with {{grammar:case|word}}
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['сu'][$case][$word]) ) {
			return $wgGrammarForms['сu'][$case][$word];
		}

		# These rules are not perfect, but they are currently only used for site names so it doesn't
		# matter if they are wrong sometimes. Just add a special case for your site name if necessary.

		#join and array_slice instead mb_substr
		$ar = array();
		preg_match_all( '/./us', $word, $ar );
		if (!preg_match("/[a-zA-Z_]/us", $word))
			switch ( $case ) {
				case 'genitive': #родительный падеж
					if ((join('',array_slice($ar[0],-4))=='вики') || (join('',array_slice($ar[0],-4))=='Вики'))
						{}
					elseif (join('',array_slice($ar[0],-2))=='ї')
						$word = join('',array_slice($ar[0],0,-2)).'їѩ';
					break;
				case 'accusative': #винительный падеж
					#stub
					break;
			}
		return $word;
	}

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 4 );

		switch ($count % 10) {
			case 1:  return $forms[0];
			case 2:  return $forms[1];
			case 3:
			case 4:  return $forms[2];
			default: return $forms[3];
		}
	}
}
