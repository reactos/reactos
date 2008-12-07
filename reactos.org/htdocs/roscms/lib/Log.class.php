<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/**
 * class Log
 * 
 */
class Log
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   * FILLME
   *
   * @param long data_id 

   * @param long rev_id 

   * @return 
   * @access public
   */
  public static function prepareInfo( $data_id = 'unknown', $rev_id = 'unknown' )
  {
    return ' [dataid: '.$data_id.'; revid: '.$rev_id.'; userid: '.ThisUser::getInstance()->id().'; security: '.Security::rightsOverview($rev_id).'] ';
  } // end of member function prepare_info


  /**
   * 
   *
   * @param string log_str 

   * @return 
   * @access public
   */
  public static function writeLow( $log_str )
  {
    return self::write($log_str, 1);
  } // end of member function write_low


  /**
   * 
   *
   * @param string log_str 

   * @return 
   * @access public
   */
  public static function writeMedium( $log_str )
  {
    return self::write($log_str, 2);
  } // end of member function write_medium


  /**
   * 
   *
   * @param string log_str 

   * @return 
   * @access public
   */
  public static function writeHigh( $log_str )
  {
    return self::write($log_str, 3);
  } // end of member function write_high


  /**
   * 
   *
   * @param string log_str 

   * @param string log_lang 

   * @return 
   * @access public
   */
  public static function writeLangMedium( $log_str,  $log_lang = '')
  {
    if ($log_lang == '') {
      $log_lang = RosCMS::getInstance()->siteLanguage();
    }

    return self::write($log_str, 2, 'log_website_'.$log_lang.'_');
  } // end of member function write_lang_medium


  /**
   * 
   *
   * @param string log_str 

   * @return 
   * @access public
   */
  public static function writeGenerateLow( $log_str )
  {
    return self::write($log_str, 1, 'log_website_generate_');
  } // end of member function write_generate_low


  /**
   * 
   *
   * @param string log_str 

   * @return 
   * @access public
   */
  public static function writeGenerateMedium( $log_str )
  {
    return self::write($log_str, 2, 'log_website_generate_');
  } // end of member function write_generate_medium


  /**
   * 
   *
   * @param string log_str 

   * @return 
   * @access public
   */
  public static function writeGenerateHigh( $log_str )
  {
    return self::write($log_str, 3, 'log_website_generate_');
  } // end of member function write_generate_high


  /**
   * 
   *
   * @param string log_str string which is append on top

   * @param short log_mode 1=low 2=medium 3=high

   * @param string log_entry 

   * @return 
   * @access private
   */
  private function write( $log_str,  $log_mode = 3,  $log_entry = 'log_website_' )
  {
    // get current log id
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name =:data_name LIMIT 1");
    $stmt->bindValue('data_name',$log_entry.date('Y-W'),PDO::PARAM_STR);
    $stmt->execute();
    $log_id = $stmt->fetchColumn();

    // if no log is started yet, create a new log id
    if ($log_id <= 0) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ENTRIES." ( id, name, type, acl_id ) VALUES ( NULL, :data_name, 'system', NULL )");
      $stmt->bindValue('data_name',$log_entry.date('Y-W'),PDO::PARAM_STR);
      $stmt->execute();

      // and use the new id as current id
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :data_name LIMIT 1");
      $stmt->bindValue('data_name',$log_entry.date('Y-W'),PDO::PARAM_STR);
      $stmt->execute();
      $log_id = $stmt->fetchColumn();
    }


    // revision (we should only have one)
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$log_id,PDO::PARAM_INT);
    $stmt->execute();
    $log_rev_id = $stmt->fetchColumn();

    // if no log revision is created, insert a new log revision id
    if ($log_rev_id <= 0) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_REVISIONS." ( id , data_id , version , lang_id , user_id , datetime ) VALUES ( NULL, :data_id, 1, :lang, :user_id, NOW())");
      $stmt->bindParam('data_id',$log_id,PDO::PARAM_INT);
      $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
      $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
      $stmt->execute();

      // get the new log revison id
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id LIMIT 1");
      $stmt->bindParam('data_id',$log_id,PDO::PARAM_INT);
      $stmt->execute();
      $log_rev_id = $stmt->fetchColumn();

      // insert short text
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, 'description', :content )");
      $stmt->bindParam('rev_id',$log_rev_id,PDO::PARAM_INT);
      $stmt->bindValue('content',$log_entry.date('Y-W'),PDO::PARAM_INT);
      $stmt->execute();

      // insert long text
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES (NULL, :rev_id, :name, :content)");
      $stmt->bindParam('rev_id',$log_rev_id,PDO::PARAM_INT);
      $stmt->bindValue('content',$log_entry.date('Y-W'),PDO::PARAM_INT);

      // insert all threee
      $stmt->bindValue('name','low_security_log',PDO::PARAM_INT);
      $stmt->execute();
      $stmt->bindValue('name','medium_security_log',PDO::PARAM_INT);
      $stmt->execute();
      $stmt->bindValue('name','high_security_log',PDO::PARAM_INT);
      $stmt->execute();

      // tag as stable
      Tag::add($log_rev_id, 'status', 'stable', -1);
    }

    //@CHECKME should maybe $log_entry ?
    switch ($log_mode) {
      case 1:
        $tmp_mode_str = 'low_security_log';
        break;
      case 2:
        $tmp_mode_str = 'medium_security_log';
        break;
      case 3:
        $tmp_mode_str = 'high_security_log';
        break;
    }

    // get current content
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, content FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id AND name = :name LIMIT 1");
    $stmt->bindParam('rev_id',$log_rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$tmp_mode_str,PDO::PARAM_STR);
    $stmt->execute();
    $log_text = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // if content exists (should do as we've done this in our previous if-block)
    if ($log_text !== false) {

      // get a username, who is responsible for this log
      $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
      $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
      $stmt->execute();
      $username = $stmt->fetchColumn();

      // prepare content with appended new entry on top
      $log_message = ' * '.date('Y-m-d H:i:s').' - '.$username.': '.$log_str."\n".$log_text['content'];

      // finally update log, and tell this the function caller
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_TEXT." SET content = :content WHERE id = :text_id");
      $stmt->bindParam('content',$log_message,PDO::PARAM_STR);
      $stmt->bindParam('text_id',$log_text['id'],PDO::PARAM_INT);
      return $stmt->execute(); // should be true
    }
    return false;
  } // end of member function write


  /**
   * 
   *
   * @param string log_str string which is append on top

   * @param short log_mode 1=low 2=medium 3=high

   * @param string log_entry 

   * @return 
   * @access private
   */
  public static function read($level = 'medium', $log = '' ) {
    return Data::getContent('log_website_'.($log!=''?$log.'_':'').date('Y-W'), 'system',  Language::getStandardId(), $level.'_security_log', 'text', 'archive');
  }


} // end of Log
?>
