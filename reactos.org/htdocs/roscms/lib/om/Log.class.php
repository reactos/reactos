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



  /**
   * wrapper for write
   *
   * @param string log_str 
   * @return 
   * @access public
   */
  public static function writeLow( $text )
  {
    return self::write($text, 1);
  } // end of member function writeLow



  /**
   * wrapper for write
   *
   * @param string log_str 
   * @return bool
   * @access public
   */
  public static function writeMedium( $text )
  {
    return self::write($text, 2);
  } // end of member function writeMedium



  /**
   * wrapper for write
   *
   * @param string text
   * @return bool
   * @access public
   */
  public static function writeHigh( $text )
  {
    return self::write($text, 3);
  } // end of member function writeHigh



  /**
   * wrapper for write
   *
   * @param string text
   * @param string lang 
   * @return bool
   * @access public
   */
  public static function writeLangMedium( $text,  $lang = '')
  {
    $lang = Language::validate($lang);
    return self::write($text, 2, 'log_website_'.$lang.'_');
  } // end of member function writeLangMedium



  /**
   * wrapper for write
   *
   * @param string text
   * @return bool
   * @access public
   */
  public static function writeGenerateLow( $text )
  {
    return self::write($text, 1, 'log_website_generate_');
  } // end of member function writeGenerateLow



  /**
   * wrapper for write
   *
   * @param string text
   * @return bool
   * @access public
   */
  public static function writeGenerateMedium( $text )
  {
    return self::write($text, 2, 'log_website_generate_');
  } // end of member function writeGenerateMedium



  /**
   * wrapper for write
   *
   * @param string text
   * @return bool
   * @access public
   */
  public static function writeGenerateHigh( $text )
  {
    return self::write($text, 3, 'log_website_generate_');
  } // end of member function writeGenerateHigh



  /**
   * updates the log for this week, or create a new one
   *
   * @param string log_str string which is append on top
   * @param short log_mode 1=low 2=medium 3=high
   * @param string log_entry 
   * @return bool
   * @access private
   */
  private function write( $text, $mode = 3, $entry = 'log_website_' )
  {
    $entry .= date('Y-W');

    // get current log id
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name =:data_name LIMIT 1");
    $stmt->bindParam('data_name',$entry,PDO::PARAM_STR);
    $stmt->execute();
    $data_id = $stmt->fetchColumn();

    // if no log is started yet, create a new entry and return new id
    if ($data_id === false) {

      // insert new entry
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ENTRIES." ( id, name, type, access_id ) VALUES ( NULL, :data_name, 'system', NULL )");
      $stmt->bindParam('data_name',$entry,PDO::PARAM_STR);
      $stmt->execute();

      // and use the new id as current id
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :data_name LIMIT 1");
      $stmt->bindParam('data_name',$entry,PDO::PARAM_STR);
      $stmt->execute();
      $data_id = $stmt->fetchColumn();
    }

    // revision (we should only have one)
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->execute();
    $rev_id = $stmt->fetchColumn();

    // if no log revision is created, insert a new log revision and return id
    if ($rev_id === false) {
      $rev_id = Revision::add($data_id,Language::getStandardId());

      // insert short text
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, 'description', :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindValue('content',$entry,PDO::PARAM_STR);
      $stmt->execute();

      // insert long text
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES (NULL, :rev_id, :name, :content)");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindValue('content',$entry,PDO::PARAM_STR);

      // insert all three
      $stmt->bindValue('name','low',PDO::PARAM_INT);
      $stmt->execute();
      $stmt->bindValue('name','medium',PDO::PARAM_INT);
      $stmt->execute();
      $stmt->bindValue('name','high',PDO::PARAM_INT);
      $stmt->execute();
    }

    // get name of selected mode
    switch ($mode) {
      case 1:
        $mode_name = 'low';
        break;
      case 2:
        $mode_name = 'medium';
        break;
      case 3:
        $mode_name = 'high';
        break;
    }

    // get a username, who is responsible for this log
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    $stmt->execute();
    $username = $stmt->fetchColumn();

    // finally update log, and tell this the function caller
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_TEXT." SET content = CONCAT(:content,content) WHERE rev_id = :rev_id AND name=:name");
    $stmt->bindValue('content',' * '.date('Y-m-d H:i:s').' - '.$username.': '.$text."\n",PDO::PARAM_STR);
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$mode_name,PDO::PARAM_STR);
    return $stmt->execute(); // should be true
  } // end of member function write



  /**
   * read a log
   *
   * @param string level
   * @param string log
   * @return string
   * @access public
   */
  public static function read($level = 'medium', $log = '' ) {
    return Entry::getContent('log_website_'.($log!=''?$log.'_':'').date('Y-W'), 'system', Language::getStandardId(), $level.'_security_log', 'text');
  } // end of member function read



} // end of Log
?>
