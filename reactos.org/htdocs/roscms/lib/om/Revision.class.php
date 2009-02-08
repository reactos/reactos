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
 * class Revision
 * 
 */
class Revision
{



  /**
   * returns requested stext content
   *
   * @param rev_id
   * @para name name of stext
   * @return string
   * @access public
   */
  public static function getSText( $rev_id, $name )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id AND name = :name LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    if ($stmt->execute()) {
      return $stmt->fetchColumn();
    }
    return false;
  } // end of member function getSText



  /**
   * returns requested text content
   *
   * @param rev_id
   * @param name name of text
   * @return string
   * @access public
   */
  public static function getText( $rev_id, $name )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id AND name = :name LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    if ($stmt->execute()) {
      return $stmt->fetchColumn();
    }
    return false;
  } // end of member function getText



  /**
   * updates the revision itself with given information
   *
   * @param int rev_id
   * @param int lang_id new language
   * @param string user_name new owner of that revision
   * @param string date new date
   * @param string time new time
   * @return bool
   * @access public
   */
  public static function update( $rev_id, $lang_id, $user_name, $date, $time )
  {
    $success = true;

    // get current state, so that we only update changed information
    $stmt=&DBConnection::getInstance()->prepare("SELECT lang_id, version, user_id, datetime FROM ".ROSCMST_REVISIONS." WHERE rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $success = $success && $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // language
    if ($lang_id > 0 && $lang_id != $revision['lang_id']) {

      // check if the choosen language do exist
      $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_LANGUAGES." WHERE id = :lang LIMIT 1");
      $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
      $success = $success && $stmt->execute();

      // update with new language
      if ($stmt->fetchColumn() !== false) {
        $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET lang_id = :lang WHERE id = :rev_id LIMIT 1");
        $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $success = $success && $stmt->execute();
        Log::writeMedium('entry language changed '.$revision['lang_id'].' =&gt; '.$lang_id);
      }
    }

    // user-name
    if ($user_name != '') {

      // check for existing user-name
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_USERS." WHERE name = :user_name LIMIT 1");
      $stmt->bindParam('user_name',$user_name,PDO::PARAM_STR);
      $success = $success && $stmt->execute();
      $user_id = $stmt->fetchColumn();

      // update with new user id
      if ($user_id > 0 && $user_id != $revision['user_id']) {
        $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET user_id = :user_id WHERE id = :rev_id LIMIT 1");
        $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $success = $success && $stmt->execute();
        Log::writeMedium('entry user-name changed: '.$revision['user_id'].' =&gt; '.$user_id.' ('.$user_name.')');
      }
    }

    // date + time (check for Y-m-d and valid-date) + (H:i:s)
    if (preg_match('/^([12][0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])$/', $date,$date_matches) && checkdate($date_matches[2], $date_matches[3], $date_matches[1]) && preg_match('/^([01][0-9]|2[0-3])(:[0-5][0-9]){2}$/',$time) && ($revision['datetime'] != $date.' '.$time) ) {

      // update with new datetime
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET datetime = :datetime WHERE id = :rev_id LIMIT 1");
      $stmt->bindValue('datetime',$date." ".$time,PDO::PARAM_STR);
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $success = $success && $stmt->execute();
      Log::writeMedium('entry date+time changed: '.$revision['datetime'].' =&gt; '.$date.' '.$time);
    }

    // report if Database calls went right
    return $success;
  } // end of member function update



  /**
   * delete a revision
   * removes also depencies to this revision
   *
   * @param int rev_id
   * @return bool
   * @access public
   */
  public static function delete( $rev_id )
  {
    Log::writeMedium('delete entry: rev-id [rev-id: '.$rev_id.']');

    // delete Depencies
    $success = Depencies::removeRevision($rev_id);

    // delete revision and texts
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_REVISIONS." WHERE id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    // we no longer need the tags
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id);
    $success = $success && $stmt->execute();

    // report if everything went right
    return $success;
  } // end of member function delete



  /**
   * managaes Text & SText fields
   * - add (name='new' and new_name != '' and delete=false)
   * - update (name=[existing] and new_name != '' and delete=false)
   * - delete (name=[existing] and delete=true)
   *
   * @param int rev_id
   * @param string stext list of stext name, new name, action
   * @param string text list of text name, new name, action
   * @access public
   */
  public static function updateText( $rev_id, $stext, $text )
  {
    // prepare for usage in foreach loop
      // check
      $stmt_check=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id AND name = :name LIMIT 1");
      $stmt_check->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // delete
      $stmt_delete=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id AND name = :name"); 
      $stmt_delete->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // add
      $stmt_insert=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) VALUES ( NULL , :rev_id, :name, '' )");
      $stmt_insert->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // update
      $stmt_update=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_STEXT." SET name = :name_new WHERE rev_id = :rev_id AND name = :name");
      $stmt_update->bindParam('rev_id',$rev_id,PDO::PARAM_INT);

    // Short Text Fields
    $entries = explode('|', $stext);
    foreach ($entries as $entry) {
      $entry_parts = explode('=', $entry);

      $stext_name = $entry_parts[0];
      $new_stext_name = $entry_parts[1];
      $stext_del = $entry_parts[2]=='true';

      // check if any work has to be done
      if ($stext_name == $new_stext_name && !$stext_del) {
        continue;
      }

      // get number of stext which match the criteria
      $stmt_check->bindParam('name',$stext_name,PDO::PARAM_STR);
      $stmt_check->execute();
      $stext_exist = $stmt_check->fetchColumn();

      // add new field
      if (!$stext_exist && $stext_name == 'new' && !$stext_del && $new_stext_name != '') {
        $stmt_insert->bindParam('name',$new_stext_name,PDO::PARAM_STR);
        $stmt_insert->execute();
      }
      
      // update field name
      elseif (!$stext_del && $new_stext_name != '') {
        $stmt_update->bindParam('name_new',$new_stext_name,PDO::PARAM_STR);
        $stmt_update->bindParam('name',$stext_name,PDO::PARAM_STR);
        $stmt_update->execute();
      }

      // delete field
      elseif ($stext_del) {
        $stmt_delete->bindParam('name',$stext_name,PDO::PARAM_STR);
        $stmt_delete->execute();
      }
    } // foreach

    // prepare for usage in foreach loop
      // check
      $stmt_check=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id AND name = :name LIMIT 1"); 
      $stmt_check->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // add
      $stmt_insert=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES ( NULL , :rev_id, :name, '' )");
      $stmt_insert->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // update
      $stmt_update=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_TEXT." SET name = :name_new WHERE rev_id = :rev_id AND name = :name");
      $stmt_update->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // delete
      $stmt_delete=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id AND name = :name"); 
      $stmt_delete->bindParam('rev_id',$rev_id,PDO::PARAM_INT);

    // Text Fields
    $entries = explode('|', $text);
    foreach ($entries as $entry) {
      $entry_parts = explode('=', $entry);

      $text_name = $entry_parts[0];
      $new_text_name = $entry_parts[1];
      $text_del = $entry_parts[2]=='true';

      // check if any work has to be done
      if ($text_name == $new_text_name && !$text_del) {
        continue;
      }

      $stmt_check->bindParam('name',$text_name,PDO::PARAM_STR);
      $stmt_check->execute();
      $text_exist = $stmt_check->fetchColumn();

      // add new field
      if (!$text_exist && $text_name == 'new' && !$text_del && $new_text_name != '') {
        $stmt_insert->bindParam('name',$new_text_name,PDO::PARAM_STR);
        $stmt_insert->execute();
      }

      // update field-name
      elseif (!$text_del && $new_text_name != '') {
        $stmt_update->bindParam('name_new',$new_text_name,PDO::PARAM_STR);
        $stmt_update->bindParam('name',$text_name,PDO::PARAM_STR);
        $stmt_update->execute();
      }

      // delete field
      else if ($text_del) {
        $stmt_delete->bindParam('name',$text_name,PDO::PARAM_STR);
        $stmt_delete->execute();
      }
    } // foreach

  } // end of member function updateText



  /**
   * deletes files from generated entries
   *
   * @param int rev_id
   * @access public
   */
  public static function deleteFile( $rev_id )
  {
    // only for admins
    if (!ThisUser::getInstance()->hasAccess('delete_file')) {
      return;
    }

    //@TODO implement adding to job queue
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_JOBS." (name, content) VALUES('stub','stub')");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
  } // end of member function deleteFile



  /**
   * updates status of revision
   *
   * @param int rev_id
   * @param string status 'new','stable','draft'
   * @access public
   */
  public static function setStatus( $rev_id, $status )
  {
    // updates the revisions status
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET status=:status WHERE id=:rev_id");
    $stmt->bindParam('status',$status,PDO::PARAM_STR);
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
  } // end of member function setStatus



  /**
   *
   *
   * @param int data_id
   * @param int lang_id
   * @return bool
   * @access public
   */
  public static function add( $data_id, $lang_id = 0 )
  {
    $thisuser_id = &ThisUser::getInstance()->id();

    // check if revision exists
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version = 0 AND lang_id = :lang AND user_id = :user_id AND status='draft' ORDER BY datetime DESC LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$thisuser_id,PDO::PARAM_INT);
    $stmt->execute();
    $rev_id = $stmt->fetchColumn();

    // create a new revision
    if ($rev_id === false) {

      // create new revision
      $stmt_ins=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_REVISIONS." ( id , data_id , version , lang_id , user_id , datetime, status ) VALUES ( NULL, :data_id, 0, :lang, :user_id, NOW(), 'draft')");
      $stmt_ins->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt_ins->bindParam('lang',$lang_id,PDO::PARAM_INT);
      $stmt_ins->bindParam('user_id',$thisuser_id,PDO::PARAM_INT);
      $stmt_ins->execute();

      // get new revision id (use old used statement again)
      $stmt->execute();

      // return new revision id
      return $stmt->fetchColumn();
    }

    return false;
  } // end of member function add



  /**
   * sends a revision to archive
   *
   * @param int rev_id
   * @return bool
   * @access public
   */
  public static function toArchive( $rev_id )
  {
    // remove depencies
    Depencies::removeRevision($rev_id);

    // move into archive
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET archive = TRUE WHERE id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    return $stmt->execute();
  } // end of member function toArchive



  /**
   * copies a revision to another language and saves it as draft
   *
   * @param int rev_id the revision id which has to be translated
   * @param int lang_id translated to this language
   * @return bool|int
   * @access public
   */
  public static function translate( $rev_id, $lang_id = 0 )
  {
    // can translate to this language ?
    if (!ThisUser::getInstance()->hasAccess('more_lang') && $lang_id != ThisUser::getInstance()->language()) {
      die ('You\'ve no rights to translate into this language'.$lang_id.'--'.ThisUser::getInstance()->language());
    }

    // original_revision
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, data_id, lang_id FROM ".ROSCMST_REVISIONS." WHERE id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // check if we can translate to the selected language
    if ($lang_id == $revision['lang_id'] || $lang_id == Language::getStandardId()) {
      die ('Can\'t translate to your language language');
    }

    // insert translated revision
    $new_rev_id = self::add($revision['data_id'],$lang_id);

    // check if copy process was successfull
    if ($new_rev_id === false) {
      die('Copy-process of data_revision not successful, maybe you\'ve that entry already as a draft.');
    }

    // copy short text
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) SELECT NULL, :new_rev_id, name, content FROM ".ROSCMST_STEXT." WHERE rev_id = :old_rev_id");
    $stmt->bindParam('new_rev_id',$new_rev_id,PDO::PARAM_INT);
    $stmt->bindParam('old_rev_id',$revision['id'],PDO::PARAM_INT);
    $stmt->execute();

    // copy_text
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) SELECT NULL, :new_rev_id, name, content FROM ".ROSCMST_TEXT." WHERE rev_id = :old_rev_id");
    $stmt->bindParam('new_rev_id',$new_rev_id,PDO::PARAM_INT);
    $stmt->bindParam('old_rev_id',$revision['id'],PDO::PARAM_INT);
    $stmt->execute();

    // copy data tags and update status
    Tag::copyFromRevision($revision['id'], $new_rev_id);

    // add original translator / translation date
    if (Tag::getValue($revision['id'],'pub_user',-1) != '') {
      Tag::add($new_rev_id,'trans_user',ThisUser::getInstance()->id(),-1);
      Tag::add($new_rev_id,'trans_date',date('Y-m-d'),-1);
    }

    return $new_rev_id;
  } // end of member function translate



} // end of Revision
?>
