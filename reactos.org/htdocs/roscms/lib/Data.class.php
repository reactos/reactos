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
 * class Data
 * 
 */
class Data
{


  /**
   * 
   *
   * @return difference
   * @access public
   */
  public static function getContent( $data_name, $data_type, $lang_id, $content_name, $content_type = 'stext', $mode = '' )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.name = :name AND d.type = :type AND r.lang_id = :lang AND r.archive = :archive AND r.version > 0 ORDER BY r.version DESC LIMIT 1");
    $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
    $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
    $stmt->bindValue('archive',($mode == 'archive'),PDO::PARAM_BOOL);
    echo ($mode == 'archive');
    $stmt->execute();
    $rev_id = $stmt->fetchColumn();

    if ($rev_id > 0) {
      switch ($content_type) {
        case 'text':
          return self::getText($rev_id, $content_name);
          break;
        case 'tag':
          return Tag::getValue($rev_id, $content_name);
          break; 
        case 'stext':
        default:
          return self::getSText($rev_id, $content_name);
          break;
      }
    }
    return;
  } // end of member function compareGregorianDate


  /**
   *
   *
   * @return 
   * @access public
   */
  public static function getSText( $rev_id, $name )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id AND name = :name LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    $stmt->execute();
    return $stmt->fetchColumn();
  } // end of member function compareGregorianDate


  /**
   *
   *
   * @return 
   * @access public
   */
  public static function getText( $rev_id, $name )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id AND name = :name LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    $stmt->execute();
    return $stmt->fetchColumn();
  } // end of member function getCookieDomain


  /**
   *
   *
   * @param int rev_id
   * @access public
   */
  public static function updateRevision( $rev_id, $lang_id, $version, $user_name, $date, $time )
  {
    // get current state, so that we only update changed information
    $stmt=&DBConnection::getInstance()->prepare("SELECT data_id, lang_id, version, user_id, datetime FROM ".ROSCMST_REVISIONS." WHERE rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // language
    if ($lang_id > 0 && $lang_id != $revision['lang_id']) {

      // check if the choosen language do exist
      $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_LANGUAGES." WHERE id = :lang LIMIT 1");
      $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
      $stmt->execute();

      // update with new language
      if ($stmt->fetchColumn() !== false) {
        $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET lang_id = :lang WHERE id = :rev_id LIMIT 1");
        $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt->execute();
        Log::writeMedium('entry language changed '.$revision['lang_id'].' =&gt; '.$lang_id.Log::prepareInfo($revision['data_id'], $rev_id).'{alterentry}');
      }
    }

    // version
    if ($version != '' && $version != $revision['version']) {

      // check for existing revisons with same number
      $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".ROSCMST_REVISIONS." WHERE version = :version AND data_id = :data_id AND lang_id = :lang");
      $stmt->bindParam('version',$version,PDO::PARAM_INT);
      $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
      $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
      $stmt->bindParam('archive',$archive,PDO::PARAM_BOOL);
      $stmt->execute();

      // update with new version
      if ($stmt->fetchColumn() <= 0) {
        $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET version = :version WHERE rev_id = :rev_id LIMIT 1");
        $stmt->bindParam('version',$version,PDO::PARAM_INT);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt->execute();
        Log::writeMedium('entry version-number changed: '.$revision['version'].' =&gt; '.$version.Log::prepareInfo($revision['data_id'], $rev_id).'{alterentry}');
      }
    }

    // user-name
    if ($user_name != '') {

      // check for existing user-name
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_USERS." WHERE name = :user_name LIMIT 1");
      $stmt->bindParam('user_name',$user_name,PDO::PARAM_STR);
      $stmt->execute();
      $user_id = $stmt->fetchColumn();

      if ($user_id > 0 && $user_id != $revision['user_id']) {
        $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET user_id = :user_id WHERE id = :rev_id LIMIT 1");
        $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt->execute();
        Log::writeMedium('entry user-name changed: '.$revision['user_id'].' =&gt; '.$user_id.' ('.$user_name.')'.Log::prepareInfo($revision['data_id'], $rev_id).'{alterentry}');
      }
    }

    // date + time (check for Y-m-d and valid-date) + (H:i:s)
    if (preg_match('/^([12][0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])$/', $date,$date_matches) && checkdate($date_matches[2], $date_matches[3], $date_matches[1]) && preg_match('/^([01][0-9]|2[0-3])(:[0-5][0-9]){2}$/',$time) && ($revision['datetime'] != $date.' '.$time) ) {

      //
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET datetime = :datetime WHERE id = :rev_id LIMIT 1");
      $stmt->bindValue('datetime',$date." ".$time,PDO::PARAM_STR);
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();
      Log::writeMedium('entry date+time changed: '.$revision['datetime'].' =&gt; '.$date.' '.$time.Log::prepareInfo($revision['data_id'], $rev_id).'{alterentry}');
    }
  } // end of member function getCookieDomain


  /**
   * deletes files from generated entries
   *
   * @param int rev_id
   * @access public
   */
  public static function deleteFile( $rev_id )
  {
    // only for admins
    if (ThisUser::getInstance()->securityLevel() < 3) {
      return;
    }

    $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, d.type, r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id=d.id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $page = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // only for entries of type page
    if ($page === false) {
      return;
    }

    //if data_type is page -> delete file in all languages
    if ($page['type'] == 'page'){
      $stmt=&DBConnection::getInstance()->prepare("SELECT lang_id FROM ".ROSCMST_LANGUAGES);
    }

    //if data_type is content (only for dynamic) -> delete only selected one
    elseif ($page['type'] == ''){
      $dynamic_num = Tag::getValueByUser($page['id'],'number',-1);
      if ($dynamic_num > 0) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT name_short FROM ".ROSCMST_LANGUAGES." WHERE id = :lang_id LIMIT 1");
        $stmt->bindParam('lang_id',$page['lang_id'],PDO::PARAM_INT);
      }
      // entry is not dynamic
      else {
        return;
      }
    }

    // neither page or content -> nothing to do
    else {
      return;
    }
    
    // get file name
    $file_extension = Tag::getValueByUser($page['rev_id'], 'extension', -1);
    $file_name = $page['name'].(isset($dynamic_num) ? '_'.$dynamic_num : '').'.'.$file_extension;

    // delete entries for selected language packs
    $stmt->execute();
    while ($lang = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // delete file if it exists
      if ( file_exists('../'.$lang['name_short'].'/'.$file_name)) {
        unlink('../'.$lang['name_short'].'/'.$file_name);
      }
    }
  }


  /**
   *
   *
   * @param int rev_id
   * @access public
   */
  public static function deleteRevision( $rev_id )
  {
    Log::writeMedium("delete entry: rev-id [rev-id: ".$rev_id."] {deleteRevision}");

    // delete revision and texts
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_REVISIONS." WHERE id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();

    // as we have a result set, we no longer need the tags
    $stmt_delete=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id LIMIT 1");
    $stmt_delete->bindParam('rev_id',$rev_id);
    $stmt_delete->execute();
  } // end of member function getCookieDomain


  /**
   *
   *
   * @access public
   */
  public static function updateText( $rev_id, $stext, $text, $archive_mode)
  {

    // only allowed in non-archive mode
    if ($archive_mode == true) {
      return;
    }

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
      if ($stext_name == $new_stext_name && $stext_del == false) {
        continue;
      }

      // get number of stext which match the criteria
      $stmt_check->bindParam('name',$stext_name,PDO::PARAM_STR);
      $stmt_check->execute();
      $stext_exist = $stmt_check->fetchColumn();

      // add new field
      if ($stext_exist === false && $stext_name == 'new' && $stext_del == false && $new_stext_name != '') {
        $stmt_insert->bindParam('name',$new_stext_name,PDO::PARAM_STR);
        $stmt_insert->execute();
      }
      
      // update field name
      elseif ($stext_del == false && $new_stext_name != '') {
        $stmt_update->bindParam('name_new',$new_stext_name,PDO::PARAM_STR);
        $stmt_update->bindParam('name',$stext_name,PDO::PARAM_STR);
        $stmt_update->execute();
      }

      // delete field
      elseif ($stext_del == true) {
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
      if ($text_name == $new_text_name && $text_del == false) {
        continue;
      }

      $stmt_check->bindParam('name',$text_name,PDO::PARAM_STR);
      $stmt_check->execute();
      $text_exist = $stmt_check->fetchColumn();

      // add new field
      if ($text_exist === false && $text_name == 'new' && $text_del == false && $new_text_name != '') {
        $stmt_insert->bindParam('name',$new_text_name,PDO::PARAM_STR);
        $stmt_insert->execute();
      }

      // update field-name
      elseif ($text_del == false && $new_text_name != '') {
        $stmt_update->bindParam('name_new',$new_text_name,PDO::PARAM_STR);
        $stmt_update->bindParam('name',$text_name,PDO::PARAM_STR);
        $stmt_update->execute();
      }

      // delete field
      else if ($text_del == "true") {
        $stmt_delete->bindParam('name',$text_name,PDO::PARAM_STR);
        $stmt_delete->execute();
      }
    } // foreach

  } // end of member function getCookieDomain


  /**
   *
   *
   * @param int data_id target for update
   * @param string data_name new data name
   * @param string data_type new data_type
   * @param string data_acl new data_acl
   * @param bool update_links if true, all dependent
   * @access public
   */
  public static function update( $data_id, $data_name, $data_type, $data_acl, $update_links )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, type, acl_id FROM ".ROSCMST_ENTRIES." WHERE id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // check if we have a dataset to work with
    if ($data === false) {
      return;
    }

    // update data type
    if ($data_type != '' && $data_type != $data['type']) {
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRIES." SET type = :type_new WHERE id = :data_id LIMIT 1");
      $stmt->bindParam('type_new',$data_type,PDO::PARAM_STR);
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->execute();
      Log::writeMedium('data-type changed: '.$data['type'].' =&gt; '.$data_type.Log::prepareInfo($data_id).'{altersecurityfields}');
      $new_data_type = $data_type;
    }

    // update data name
    if ($data_name != '' && $data_name != $data['name']) {
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRIES." SET name = :name_new WHERE id = :id LIMIT 1");
      $stmt->bindParam('name_new',$data_name,PDO::PARAM_STR);
      $stmt->bindParam('id',$data_id,PDO::PARAM_INT);
      $stmt->execute();

      Log::writeMedium('data-name changed: '.$data['name'].' =&gt; '.$data_name.Log::prepareInfo($data_id).'{altersecurityfields}');

      // update dependent entries
      if ($update_links == true) {
        if ($new_data_type == '') {
          $new_data_type = $data['type'];
        }

        // old type
        switch ($data['type']) {
          case 'content':
            $new_type_short = 'cont';
            break;
          case 'template':
            $new_type_short = 'templ';
            break;
          case 'script':
            $new_type_short = 'inc';
            break;
          case 'system':
            $new_type_short = 'sys';
            break;
          case 'page':
          default:
            $new_type_short = 'no';
            break;
        }

        // new type
        switch ($new_data_type) {
          case 'content':
            $old_type_short = 'cont';
            break;
          case 'template':
            $old_type_short = 'templ';
            break;
          case 'script':
            $old_type_short = 'inc';
            break;
          case 'system':
            $old_type_short = 'sys';
            break;
          case 'page':
          default:
            $old_type_short = 'no';
            break;
        }

        // update text content with new name
        //@ADD check, for only updating dependent entries
        $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_TEXT." SET content = REPLACE(REPLACE(content, :old_type_name, :new_type_name), :old_link, :new_link) WHERE content LIKE :search1 OR content LIKE :search2");
        $stmt->bindParam('search1','%[#'.$old_type_short.'_'.$data['name'].']%',PDO::PARAM_STR);
        $stmt->bindParam('search2','%[#link_'.$data['name'].']%',PDO::PARAM_STR);
        $stmt->bindParam('old_type_name','[#'.$old_type_short.'_'.$data['name'].']',PDO::PARAM_STR);
        $stmt->bindParam('new_type_name','[#'.$new_type_short.'_'.$data_name.']',PDO::PARAM_STR);
        $stmt->bindParam('old_link','[#link_'.$data['name'].']',PDO::PARAM_STR);
        $stmt->bindParam('new_link','[#link_'.$data_name.']',PDO::PARAM_STR);
        $stmt->execute();

        Log::writeMedium('data-interlinks updated due data-name change'.Log::prepareInfo($data_id).'{altersecurityfields}');
      }
    } // end data_name changes

    // change ACL
    if ($data_acl != '' && $data_acl != $data['acl_id']) {
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRIES." SET acl_id = :acl_new WHERE id = :data_id LIMIT 1");
      $stmt->bindParam('acl_new',$data_acl);
      $stmt->bindParam('data_id',$data_id);
      $stmt->execute();
      Log::writeMedium('data-acl changed: '.$data['acl_id'].' =&gt; '.$data_acl.Log::prepareInfo($data_id).'{altersecurityfields}');
    } 

  } // end of member function getCookieDomain


  /**
   *
   *
   * @param int data_id
   * @param int rev_id
   * @param bool archive_mode
   * @param string lang
   * @return bool
   * @access public
   */
  public static function add($data_type = null, $lang_id = 0, $show_output = false, $dynamic_content = false, $entry_status = 'draft', $layout_template = '')
  {
    $thisuser = &ThisUser::getInstance();

    $data_name = trim(@htmlspecialchars($_GET['d_name']));

    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :name AND type = :type LIMIT 1");
    $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
    $stmt->execute();
    $data_id = $stmt->fetchColumn();

    if ($data_id === false) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ENTRIES." ( id , name , type ) VALUES ( NULL , :name, :type )");
      $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
      $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
      $stmt->execute();

      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :name AND type = :type LIMIT 1");
      $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
      $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
      $stmt->execute();
      $data_id = $stmt->fetchColumn();
    }

    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_REVISIONS." WHERE id = :data_id AND lang_id = :lang AND archive IS FALSE LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision_exists = $stmt->fetchColumn();

    if ($revision_exists === false || $dynamic_content === true) {
      // revision entry doesn't exist
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_REVISIONS." ( id , data_id , version , lang_id , user_id , datetime ) VALUES ( NULL, :data_id, 0, :lang, :user_id, NOW())");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->execute();
      
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version = 0 AND lang_id = :lang AND user_id = :user_id ORDER BY datetime DESC");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->execute();
      $rev_id = $stmt->fetchColumn();

      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, :description, :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindValue('description','description',PDO::PARAM_STR);
      $stmt->bindValue('content','',PDO::PARAM_STR);
      $stmt->execute();
      if ($data_type == 'page') {
        // add also a comment
        $stmt->bindValue('description','comment',PDO::PARAM_STR);
        $stmt->bindValue('content','',PDO::PARAM_STR);
        $stmt->execute();
        // add also a title
        $stmt->bindValue('description','title',PDO::PARAM_STR);
        $stmt->bindValue('content',$data_name,PDO::PARAM_STR);
        $stmt->execute();
      }
      elseif ($data_type == 'content' && $dynamic_content === true) {
        // add a title
        $stmt->bindValue('description','title',PDO::PARAM_STR);
        $stmt->bindValue('content',$data_name,PDO::PARAM_STR);
        $stmt->execute();
      }

      if ($layout_template != '' && $layout_template != 'none') {
        $content = '[#templ_'.$layout_template.']';
      }
      else {
        $content = '';
      }

      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, 'content', :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindParam('content',$content,PDO::PARAM_STR);
      $stmt->execute();

      Tag::add($rev_id, 'status', $entry_status, -1);

      // add dynamic content tags
      if ($dynamic_content === true) {

        // get highest saved dynamic number for this data
        $dynamic_number = 0;
        $stmt=&DBConnection::getInstance()->prepare("SELECT t.value FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_TAGS." t ON r.id = t.rev_id WHERE r.data_id = :data_id AND r.lang_id = :lang AND r.version > 0 AND t.user_id = -1 AND t.name = 'number' ORDER BY t.value DESC");
        $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
        $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
        $stmt->execute();
        while ( $tag_value = intval($stmt->fetchColumn())) {
          // get dynamic content number
          if ($tag_value > $dynamic_number) {
            $dynamic_number = $tag_value;
          }
        }
        $dynamic_number++;

        // add Tags
        Tag::add($rev_id, 'number', $dynamic_number, -1);
        Tag::add($rev_id, 'number_sort', str_pad($dynamic_number, 5, '0', STR_PAD_LEFT), -1); // padding with '0'
        Tag::add($rev_id, 'pub_date', date('Y-m-d'), -1);
        Tag::add($rev_id, 'pub_user', $thisuser->id(), -1);
      }

      if ($data_type == 'page') {
        Tag::add($rev_id, 'extension', 'html', -1);
      }

      if ($show_output === true) {
        new Editor_Website($data_id, $rev_id, 'show');
      }
    }
  } // end of member function getCookieDomain



  /**
   *
   *
   * @access public
   */
  public static function rebuildDepencies()
  {
    // remove old depencies
    DBConnection::getInstance()->exec("DELETE FROM ".ROSCMST_DEPENCIES);

    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." ORDER BY id ASC");
    $stmt->execute();
    while ($revision = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $content = Data::getText($revision['id'], 'content');
      $content = preg_replace_callback('/(\[#templ_[^][#[:space:]]+\])/', array($this,'handleTemplate'), $content);
      $content = preg_replace_callback('/(\[#cont_[^][#[:space:]]+\])/', array($this,'handleContent'), $content);
      $content = preg_replace_callback('/(\[#inc_[^][#[:space:]]+\])/', array($this,'handleScript'), $content);
      $content = preg_replace_callback('/(\[#link_[^][#[:space:]]+\])/', array($this,'handleLink'), $content);
    }
  }



  /**
   *
   *
   * @access public
   */
  public static function buildDepencies( $rev_id )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.type, d.name FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id=d.id WHERE r.id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id, PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    switch ($revision['type']) {
      case 'template':
        $type_short = 'templ';
        break;
      case 'content':
        $type_short = 'cont';
        break;
      case 'script':
        $type_short = 'inc';
        break;
      case 'page':
        $type_short = 'link';
        break;
      default:
        // skip system stuff
        return;
        break;
    }

    // search for depencies
    $stmt=&DBConnection::getInstance()->prepare("SELECT t.rev_id, d.type FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TEXT." t ON r.id = t.rev_id WHERE t.content LIKE :content_phrase AND r.version > 0");
    $stmt->bindValue('content_phrase','%[#'.$type_short.'_'.$revision['name'].']%',PDO::PARAM_STR);
    $stmt->execute();

    // for usage in loop
    $stmt_check=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_DEPENCIES." WHERE rev_id = :rev_id AND dependent_from = :depency_id LIMIT 1");
    $stmt_check->bindParam('depency_id',$rev_id,PDO::PARAM_INT);

    $stmt_ins=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENCIES." (rev_id, depency_id, depency_name, depeny_type) VALUES (:rev_id, :depency_id, :depency_name, :depency_type)");
    $stmt_ins->bindParam('depency_id',$rev_id,PDO::PARAM_INT);
    $stmt_ins->bindParam('depency_name',$revision['name'],PDO::PARAM_STR);
    $stmt_ins->bindParam('depency_type',$revision['type'],PDO::PARAM_STR);
    // write depencies
    while ($depency = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // check that entry doesn't exist
      $stmt_check->bindParam('rev_id',$depency['rev_id'],PDO::PARAM_INT);
      $stmt_check->execute();
      if ($stmt_check->fetchColumn() === false) {
        // write the depency to DB
        $stmt_ins->bindParam('rev_id',$depency['rev_id'],PDO::PARAM_INT);
        $stmt_ins->execute();
      }
    }

    if ($revision['type'] == 'template') {
      // templates need special dealing
      $stmt=&DBConnection::getInstance()->prepare("SELECT w.rev_id, d.name, REPLACE(REPLACE(t.content,'[#%NAME%]',d.name), '[#cont_%NAME%]', CONCAT('[#cont_',d.name,']')) AS content FROM roscms_rel_revisions_depencies w JOIN roscms_entries_revisions r ON w.rev_id=r.id JOIN roscms_entries d ON d.id=r.data_id JOIN roscms_entries_text t ON t.rev_id=w.dependent_from WHERE w.dependent_from = :depency_id AND w.dependent_type = 'template' LIMIT 1");
      $stmt->bindParam('depency_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();

      // for usage in loop
        $stmt_rev=DBConnection::getInstance()->prepare("SELECT d.type, r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id=d.id WHERE name = :name AND type = :type LIMIT 1");
        $stmt_check=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_DEPENCIES." WHERE rev_id = :rev_id AND dependent_from = :depency_id LIMIT 1");
        $stmt_ins=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENCIES." (rev_id, depency_id, depency_name, depency_type) VALUES (:rev_id, :depency_id, :depency_name, :depency_type)");

      while ($template = $stmt->fetch(PDO::FETCH_ASSOC)) {

        // for usage in inner loop
        $stmt_check->bindParam('rev_id',$template['rev_id'],PDO::PARAM_INT);
        $stmt_ins->bindParam('rev_id',$template['rev_id'],PDO::PARAM_INT);

        // get contained entries
        preg_match_all('/\[#([a-z_]+_'.$template['name'].'[^\]]*)\]/iU',$template['content'],$matches);
        for ($i=0; $i<count($matches[1]) ;$i++) {

          // get name & type
          if (strpos($matches[1][$i],'templ_') === 0) {
            $type = 'template';
            $name = substr($matches[1][$i], 6);
          }
          elseif (strpos($matches[1][$i],'cont_') === 0) {
            $type = 'content';
            $name = substr($matches[1][$i], 5);
          }
          elseif (strpos($matches[1][$i],'inc_') === 0) {
            $type = 'script';
            $name = substr($matches[1][$i], 4);
          }
          else {
            break;
          }

          $stmt_rev->bindParam('name',$name,PDO::PARAM_STR);
          $stmt_rev->bindParam('type',$type,PDO::PARAM_STR);
          $stmt_rev->execute();
          $revision = $stmt_rev->fetch(PDO::FETCH_ASSOC);

          if ($revision !== false) {
            $stmt_check->bindParam('depency_id',$revision['id'],PDO::PARAM_INT);
            $stmt_check->execute();
            if ($stmt_check->fetchColumn() === false) {
              // write the depency to DB
              $stmt_ins->bindParam('depency_id',$revision['id'],PDO::PARAM_INT);
              $stmt_ins->bindParam('depency_name',$revision['name'],PDO::PARAM_STR);
              $stmt_ins->bindParam('depency_type',$revision['type'],PDO::PARAM_STR);
              $stmt_ins->execute();
            }
          } else {echo $type;}
        }
      }
    }
  } // end of member function generate_page_output_update



  /**
   * check before usage if there is a set of entries or not, only call when it
   *
   */
  public static function evalAction( $id_list, $action, $lang_id = 0, $label_name = null )
  {
    $thisuser = &ThisUser::getInstance();

    $id_list = preg_replace('/(^|-)[0-9]+\_([0-9]+)/','$2|',$id_list);
    $rev_id_list = explode('|',substr($id_list,0,-1));

    $xport = new Export_HTML();

    // preview
    if ($action == 'mp') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, r.rev_id, r.lang_id FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id = d.data_id WHERE r.id = :rev_id LIMIT 1");
      $stmt->bindParam('rev_id',$rev_id_list[0],PDO::PARAM_INT);
      $stmt->execute();
      $revision = $stmt->fetchOnce();

      $dynamic_num = Tag::getValueByUser($revision['id'], 'number', -1);
      echo $xport->processTextByName($revision['name'], $revision['lang_id'], $dynamic_num, 'show');
    }

    else {

      // get user language
      $user_lang = ROSUser::getLanguage($thisuser->id(), true);

      foreach ($rev_id_list as $rev_id) {

        // get revision information
        $stmt_rev=&DBConnection::getInstance()->prepare("SELECT lang_id, version, data_id, id, user_id FROM ".ROSCMST_REVISIONS." WHERE id = :rev_id LIMIT 1");
        $stmt_rev->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt_rev->execute();
        $revision = $stmt_rev->fetchOnce();

        // don't go further if it's no valid rev-id
        if ($revision === false) {
          echo '<p>revision-id &quot;'.$rev_id.'&quot; is unknown</p>';
          continue;
        }

        switch ($action) {

          // mark as stable
          case 'ms':
            if ($thisuser->securityLevel() > 1 && $thisuser->isMemberOfGroup('transmaint')) {

              // check for user language
              if ($user_lang == '') {
                die('Set a valid language in your account settings!');
              }
              elseif ($user_lang != $revision['lang_id']) {
                echo 'As Language Maintainer you can only mark entries of "'.$user_lang.'" language as stable!';
                continue;
              }
            }

            Log::writeLow('mark entry as stable: data-id '.$revision['data_id'].', rev-id '.$revision['id'].Log::prepareInfo($revision['data_id'], $revision['id']).'{changetags}');

            // renew tag
            $tag_id = Tag::getIdByUser($revision['id'], 'status', -1);
            Tag::update($tag_id, 'stable');

            if ($revision['rev_version'] == 0) {
              $dynamic_num = Tag::getValueByUser($revision['rev_id'], 'number',  -1); 

              if ($dynamic_num > 0) {
                $stmt=&DBConnection::getInstance()->prepare("SELECT r.id, r.data_id, r.version, r.lang_id FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_TAGS." t ON r.id = t.rev_id WHERE r.data_id = :data_id AND r.version > 0 AND r.lang_id = :lang AND t.user_id = -1 AND t.name = 'number' AND t.value = :tag_value ORDER BY r.version DESC, r.id DESC LIMIT 1");
                $stmt->bindParam('tag_value',$dynamic_num,PDO::PARAM_STR);
              }
              else {
                $stmt=&DBConnection::getInstance()->prepare("SELECT id, data_id, version, lang_id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang ORDER BY version DESC, id DESC LIMIT 1");
              }
              $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
              $stmt->bindParam('lang',$revision['lang_id'],PDO::PARAM_INT);
              $stmt->execute();
              $stable_revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

              // setup a new version number
              $version_num = 1;

              // no stable entry exist, so skip move-process
              if ($stable_revision !== false) {
              
                // stable entry exist, so increase the version number
                $version_num = $stable_revision['version'] + 1;

                // delete old tags
                $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id");
                $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
                $stmt->execute();

                // transfer 
                Tag::copyFromData($stable_revision['id'], $revision['id']);

                // move old revision to archive
                if (Data::copy($stable_revision['id'], 0, $lang_id)) {
                  Data::deleteRevision($stable_revision['id']);
                }
                else {
                  Log::writeMedium('Data::copy() failed: data-id '.$stable_revision['data_id'].', rev-id '.$stable_revision['id'].Log::prepareInfo($stable_revision['data_id'], $stable_revision['rev_id']).'{changetags}');
                  echo 'Process not successful :S';
                }
              }

              // update the version number
              $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET version = :version WHERE id = :rev_id");
              $stmt->bindParam('version',$version_num,PDO::PARAM_INT);
              $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
              $stmt->execute();

              // get language to generation
              if ($revision['lang_id'] == null) {
                $generation_lang = Language::getStandardId();
              }
              else {
                $generation_lang = $revision['lang_id'];
              }

              $stmt=&DBConnection::getInstance()->prepare("SELECT d.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_ENTRIES." d2 ON d.name=d2.name WHERE d2.id = :data_id AND d.data_type = 'page' LIMIT 1");
              $stmt->bindParam('data_id',$revision['data_id']);
              $stmt->execute();
              $data_id = $stmt->fetchColumn();
              Log::writeGenerateLow('+++++ [generate_page_output_update('.$revision['data_id'].', '.$generation_lang.', '.$dynamic_num.')]');

              $xport->generate($data_id, $generation_lang, $dynamic_num);
              echo 'Page generation process finished';
            }
            else {
              echo 'Only &quot;new&quot; entries can be made stable';
            }
            break;

          // mark as new
          case 'mn':
            if ($thisuser->securityLevel() > 1 && $thisuser->isMemberOfGroup('transmaint')) {

              // check for user language
              if ($user_lang == 0) {
                die('Set a valid language in your Account settings!');
              }
              elseif ($user_lang != $revision['lang_id']) {
                echo 'As Language Maintainer you can only mark entries of "'.$user_lang.'" language as new!';
                continue;
              }
            }

            //update tag
            $tag_id = Tag::getIdByUser($revision['id'], 'status', -1);
            Tag::update($tag_id, 'new');

            $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET version = 0 WHERE id = :rev_id");
            $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
            $stmt->execute();
            break;

          // add star
          case 'as':
            Tag::deleteByName($revision['id'], 'star', $thisuser->id());
            Tag::add($revision['id'], 'star', 'on', $thisuser->id());
            break;

          // delete star
          case 'xs':
            Tag::deleteByName($revision['id'], 'star', $thisuser->id());
            break;

          // add label
          case 'tg':
            Tag::add($revision['id'], 'tag', $label_name, $thisuser->id());
            break;

          // delete entry
          case 'xe':
            if ($thisuser->securityLevel() > 1 || $revision['user_id'] == $thisuser->id()) {

              // copy to Archive if no admin
              if ($thisuser->securityLevel() < 3) {
                Data::copy($revision['id'], 0, $lang_id);
              }
              Data::deleteFile($revision['id']);
              Data::deleteRevision($revision['id']);
            }
            else {
              echo 'Not enough rights for delete process.';
            }
            break;

          // move to archiv
          case 'va':
            Data::copy($revision['id'], 0, $lang_id);
            Data::deleteFile($revision['id']);
            Data::deleteRevision($revision['id']);
            break;
        } // switch
      } // for
    }
  } // end of member function getCookieDomain


  /**
   *
   *
   * @param int data_id
   * @param int rev_id
   * @param bool archive_mode
   * @param string lang
   * @return bool
   * @access public
   */
  public static function copy($rev_id, $archive_mode, $lang_id = 0 )
  {
    // set archive mode dependent vars
    if ($archive_mode == 0) {
      // copy to archive
      $archive_mode = true;
    }
    elseif ($archive_mode == 1) {
      // create copy
      $archive_mode = false;
    }
    else {
      die('move_to_archive: wrong param');
    }

    // Log something
    if ($archive_mode === false) {
      Log::writeLow('copy entire entry (e.g. translate): rev-id '.$rev_id.Log::prepareInfo(null, $rev_id).'{move_to_archive}');
    }
    else {
      Log::writeMedium('move entire entry to archive: rev-id '.$rev_id.Log::prepareInfo(null, $rev_id).'{move_to_archive}');
    }

    // data_revision
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, d.acl_id, r.version, r.user_id, r.lang_id, r.datetime FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id=d.id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    if ($archive_mode === false) {
      $revision = array(
        'version' => '0',
        'user_id' => ThisUser::getInstance()->id(),
        'lang_id' => $lang_id,
        'datetime' => date('Y-m-d H:i:s'));
    }
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_REVISIONS." ( id , data_id , version , lang_id , user_id , datetime ) VALUES ( NULL, :data_id, :version, :lang, :user_id, :datetime )");
    $stmt->bindParam('data_id',$new_data_id,PDO::PARAM_INT);
    $stmt->bindValue('version',$revision['version'],PDO::PARAM_INT);
    $stmt->bindParam('lang',$revision['lang_id'],PDO::PARAM_INT);
    $stmt->bindParam('user_id',$revision['user_id'],PDO::PARAM_INT);
    $stmt->bindParam('datetime',$revision['datetime'],PDO::PARAM_STR);
    $stmt->execute();

    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND user_id=:user_id ORDER BY id DESC LIMIT 1");
    $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
    $stmt->bindParam('user_id',$revision['user_id'],PDO::PARAM_INT);
    $stmt->execute();
    $new_rev_id = $stmt->fetchColumn();

    if ($new_rev_id === false) {
      die('copy-process of data_revision not successful');
    }

    // copy stext
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) SELECT NULL, :new_rev_id AS rev_id, name, content FROM ".ROSCMST_STEXT." WHERE rev_id = :old_rev_id");
    $stmt->bindParam('new_rev_id',$new_rev_id,PDO::PARAM_INT);
    $stmt->bindParam('old_rev_id',$old_rev_id,PDO::PARAM_INT);
    $stmt->execute();

    // copy_text
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) SELECT NULL, :new_rev_id AS rev_id, name, content FROM ".ROSCMST_TEXT." WHERE rev_id = :old_rev_id");
    $stmt->bindParam('new_rev_id',$new_rev_id,PDO::PARAM_INT);
    $stmt->bindParam('old_rev_id',$old_rev_id,PDO::PARAM_INT);
    $stmt->execute();


    // data_tag
    Tag::copyFromData($rev_id, $new_rev_id);
    if ($archive_mode === false) {
      // change status to draft
      $tag_id = Tag::getIdByUser($new_rev_id, 'status', -1);
      Tag::update($tag_id, 'draft');
    }

    return true;
  } // end of member function compareGregorianDate




} // end of Data
?>
