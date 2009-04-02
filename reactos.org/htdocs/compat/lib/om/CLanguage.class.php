<?php

/**
 * class CLanguage
 * 
 */
class CLanguage
{


  /**
   * returns a valid language code, if possible return the param code itself
   *
   * @param string lang_code
   * @return string
   * @access public
   */
  public static function validate( $lang )
  {

		if ($lang) {
			if (@preg_match('/^([a-zA-Z]+)(-[a-zA-Z]+)?$/', $lang, $matches)) {
				$checked_lang = @strtolower($matches[1]);
				switch($checked_lang) {
				case 'de':
				case 'en':
				case 'fr':
				case 'ru':
					break;
				default:
					$checked_lang = null;
				}
			}
		}
    if ($checked_lang === null) {
      $checked_lang = 'en';
    }
	
		return $checked_lang;
  } // end of member function validate



} // end of CLanguage
?>
