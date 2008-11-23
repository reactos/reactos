<?php

/**
 * class Language
 * 
 */
class Language
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   * 
   *
   * @param string _lang 

   * @return 
   * @access public
   */
  public static function checkStatic( $lang_code )
  {
    global $roscms_standard_language;

    switch ($lang_code) {
      case 'ar':
      case 'bg':
      case 'ca':
      case 'cz':
      case 'da':
      case 'de':
      case 'el':
      case 'en':
      case 'es':
      case 'fr':
      case 'he':
      case 'hu':
      case 'id':
      case 'it':
      case 'ja':
      case 'ko':
      case 'lt':
      case 'nl':
      case 'no':
      case 'pl':
      case 'pt':
      case 'ro':
      case 'ru':
      case 'sk':
      case 'sv':
      case 'tw':
      case 'uk':
      case 'vi':
      case 'zh':
        return $lang_code;
        break;
      case '*':
      default:
        return $roscms_standard_language;
        break;
    }
  } // end of member function check_lang
} // end of Language
?>
