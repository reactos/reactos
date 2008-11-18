<?php
/** Belarusian in Taraškievica orthography (Беларуская тарашкевіца)
  *
  * @ingroup Language
  *
  * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
  * @bug 1638, 2135
  * @link http://be.wikipedia.org/wiki/Talk:LanguageBe.php
  * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License
  * @license http://www.gnu.org/copyleft/fdl.html GNU Free Documentation License
  */

class LanguageBe_tarask extends Language {
	/**
	* Plural form transformations
	*
	* $wordform1 - singular form (for 1, 21, 31, 41...)
	* $wordform2 - plural form (for 2, 3, 4, 22, 23, 24, 32, 33, 34...)
	* $wordform3 - plural form (for 0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 25, 26...)
	*/

	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }

		//if no number with word, then use $form[0] for singular and $form[1] for plural or zero
		if( count($forms) === 2 ) return $count == 1 ? $forms[0] : $forms[1];

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
   * Cases: родны, вінавальны, месны
   */
	function convertGrammar( $word, $case ) {
		switch ( $case ) {
			case 'родны': # genitive
				if ( $word == 'Вікіпэдыя' ) {
					$word = 'Вікіпэдыі';
				} elseif ( $word == 'ВікіСлоўнік' ) {
					$word = 'ВікіСлоўніка';
				} elseif ( $word == 'ВікіКнігі' ) {
					$word = 'ВікіКніг';
				} elseif ( $word == 'ВікіКрыніца' ) {
					$word = 'ВікіКрыніцы';
				} elseif ( $word == 'ВікіНавіны' ) {
					$word = 'ВікіНавін';
				} elseif ( $word == 'ВікіВіды' ) {
					$word = 'ВікіВідаў';
				}
			break;
			case 'вінавальны': # akusative
				if ( $word == 'Вікіпэдыя' ) {
					$word = 'Вікіпэдыю';
				} elseif ( $word == 'ВікіСлоўнік' ) {
					$word = 'ВікіСлоўнік';
				} elseif ( $word == 'ВікіКнігі' ) {
					$word = 'ВікіКнігі';
				} elseif ( $word == 'ВікіКрыніца' ) {
					$word = 'ВікіКрыніцу';
				} elseif ( $word == 'ВікіНавіны' ) {
					$word = 'ВікіНавіны';
				} elseif ( $word == 'ВікіВіды' ) {
					$word = 'ВікіВіды';
				}
			break;
			case 'месны': # prepositional
				if ( $word == 'Вікіпэдыя' ) {
					$word = 'Вікіпэдыі';
				} elseif ( $word == 'ВікіСлоўнік' ) {
					$word = 'ВікіСлоўніку';
				} elseif ( $word == 'ВікіКнігі' ) {
					$word = 'ВікіКнігах';
				} elseif ( $word == 'ВікіКрыніца' ) {
					$word = 'ВікіКрыніцы';
				} elseif ( $word == 'ВікіНавіны' ) {
					$word = 'ВікіНавінах';
				} elseif ( $word == 'ВікіВіды' ) {
					$word = 'ВікіВідах';
				}
			break;
		}

		return $word; # this will return the original value for 'назоўны' (nominative) and all undefined case values
	}
}
