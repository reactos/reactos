<?php

/** Armenian (Հայերեն)
 *
 * @ingroup Language
 * @author Ruben Vardanyan (Me@RubenVardanyan.com)
 */
class LanguageHy extends Language {
	# Convert from the nominative form of a noun to some other case
	# Invoked with {{grammar:case|word}}
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms['hy'][$case][$word]) ) {
			return $wgGrammarForms['hy'][$case][$word];
		}

		# These rules are not perfect, but they are currently only used for site names so it doesn't
		# matter if they are wrong sometimes. Just add a special case for your site name if necessary.

		#join and array_slice instead mb_substr
		$ar = array();
		preg_match_all( '/./us', $word, $ar );
		if (!preg_match("/[a-zA-Z_]/us", $word))
			switch ( $case ) {
				case 'genitive': #սեռական հոլով
					if (join('',array_slice($ar[0],-1))=='ա')
						$word = join('',array_slice($ar[0],0,-1)).'այի';
					elseif (join('',array_slice($ar[0],-1))=='ո')
						$word=join('',array_slice($ar[0],0,-1)).'ոյի';
					elseif (join('',array_slice($ar[0],-4))=='գիրք')
						$word=join('',array_slice($ar[0],0,-4)).'գրքի';
					else
						$word.='ի';
					break;
				case 'dative':  #Տրական հոլով
					#stub
					break;
				case 'accusative': #Հայցական հոլով
					#stub
					break;
				case 'instrumental':  #
					#stub
					break;
				case 'prepositional': #
					#stub
					break;
			}
		return $word;
	}

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 2 );

		return (abs($count) <= 1) ? $forms[0] : $forms[1];
	}

	/*
	 * Armenian numeric format is "12 345,67" but "1234,56"
	 */

	function commafy($_) {
		if (!preg_match('/^\d{1,4}$/',$_)) {
			return strrev((string)preg_replace('/(\d{3})(?=\d)(?!\d*\.)/','$1,',strrev($_)));
		} else {
			return $_;
		}
	}
}
