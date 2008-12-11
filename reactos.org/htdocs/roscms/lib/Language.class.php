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
        return RosCMS::getInstance()->siteLanguage();
        break;
    }
  } // end of member function check_lang


  public static function getStandardId( )
  {
    static $id;

    if (empty($id)){
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_LANGUAGES." WHERE name_short = :lang_code LIMIT 1");
      $stmt->bindParam('lang_code',RosCMS::getInstance()->siteLanguage(),PDO::PARAM_STR);
      $stmt->execute();
      $id = $stmt->fetchColumn();
    }
    return $id;


  }
} // end of Language
?>
