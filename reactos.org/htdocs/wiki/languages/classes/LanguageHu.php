<?php

/** Hungarian localisation for MediaWiki
 *
 * @ingroup Language
 */
class LanguageHu extends Language {
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms[$this->getCode()][$case][$word]) ) {
			return $wgGrammarForms[$this->getCode()][$case][$word];
		}

		static $localForms = array(
			'rol' => array(
				'Wikipédia'   => 'Wikipédiáról',
				'Wikidézet'   => 'Wikidézetről',
				'Wikiszótár'  => 'Wikiszótárról',
				'Wikikönyvek' => 'Wikikönyvekről',
			),
			'ba' => array(
				'Wikipédia'   => 'Wikipédiába',
				'Wikidézet'   => 'Wikidézetbe',
				'Wikiszótár'  => 'Wikiszótárba',
				'Wikikönyvek' => 'Wikikönyvekbe',
			),
			'k' => array(
				'Wikipédia'   => 'Wikipédiák',
				'Wikidézet'   => 'Wikidézetek',
				'Wikiszótár'  => 'Wikiszótárak',
				'Wikikönyvek' => 'Wikikönyvek',
			)
		);

		if ( isset( $localForms[$case][$word] ) ) {
			return $localForms[$case][$word];
		}

		switch ( $case ) {
			case 'rol':
				return $word . 'ról';
			case 'ba':
				return $word . 'ba';
			case 'k':
				return $word . 'k';
		}
	}
}


