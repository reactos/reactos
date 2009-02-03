<?php

/**
 * class Language
 * 
 */
class Language
{


  /**
   * returns a valid language code, if possible return the param code itself
   *
   * @param string lang_code
   * @return string
   * @access public
   */
  public static function validate( $lang_code )
  {
    // check for valid code
    $stmt=&DBConnection::getInstance()->prepare("SELECT name_short FROM ".ROSCMST_LANGUAGES." WHERE name_short=:code");
    $stmt->bindParam('code',$lang_code,PDO::PARAM_STR);
    $stmt->execute();
    $valid=$stmt->fetchColumn();
    
    // if we have a result, thats a valid code
    if ($valid !== false) {
      return $valid;
    }
    return RosCMS::getInstance()->siteLanguage();
  } // end of member function validate



  /**
   * returns the id of our standard language
   *
   * @return int
   * @access public
   */
  public static function getStandardId( )
  {
    static $id;

    // check if we already have our id
    if (empty($id)){
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_LANGUAGES." WHERE name_short = :lang_code LIMIT 1");
      $stmt->bindParam('lang_code',RosCMS::getInstance()->siteLanguage(),PDO::PARAM_STR);
      $stmt->execute();
      $id = (int)$stmt->fetchColumn();
    }
    return $id;
  } // end of member function getStandardId



} // end of Language
?>
