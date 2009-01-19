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
    if (!ThisUser::getInstance()->hasAccess('delete_file')) {
      return;
    }

    //@TODO implement adding to job queue
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_JOBS." (name, content) VALUES('stub','stub')");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();

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

    // delete Depencies
    $depencies = new DataDepencies();
    $depencies->removeRevision($rev_id);

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

    // we no longer need the tags
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
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, type, access_id FROM ".ROSCMST_ENTRIES." WHERE id = :data_id LIMIT 1");
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
    if ($data_acl != '' && $data_acl != $data['access_id']) {
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRIES." SET access_id = :acl_new WHERE id = :data_id LIMIT 1");
      $stmt->bindParam('acl_new',$data_acl);
      $stmt->bindParam('data_id',$data_id);
      $stmt->execute();
      Log::writeMedium('data-acl changed: '.$data['access_id'].' =&gt; '.$data_acl.Log::prepareInfo($data_id).'{altersecurityfields}');
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
      if ($data_type == 'page' || $data_type == 'dynamic') {
        // add also a comment
        $stmt->bindValue('description','comment',PDO::PARAM_STR);
        $stmt->bindValue('content','',PDO::PARAM_STR);
        $stmt->execute();
        // add also a title
        $stmt->bindValue('description','title',PDO::PARAM_STR);
        $stmt->bindValue('content',$data_name,PDO::PARAM_STR);
        $stmt->execute();

        // add next dynamic number for dynamic entries
        if ($data_type == 'dynamic') {
          Tag::add($rev_id, 'next_number', 1, -1);
        }
      }
      elseif ($data_type == 'content' && $dynamic_content === true) {

        // get highest saved dynamic number for this data
        $next_number = intval(Tag::getValueByUser($rev_id,'number_next',-1));

        // add a title
        $stmt->bindValue('description','title',PDO::PARAM_STR);
        $stmt->bindValue('content',$data_name.'_'.$next_number,PDO::PARAM_STR);
        $stmt->execute();
      }

      // is a template requested
      if ($layout_template != '' && $layout_template != 'none') {
        $content = '[#templ_'.$layout_template.']';
      }
      else {
        $content = '';
      }

      // make a new page text
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, 'content', :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindParam('content',$content,PDO::PARAM_STR);
      $stmt->execute();

      // draft / new / stable ...
      Tag::add($rev_id, 'status', $entry_status, -1);

      // add dynamic content tags
      if (isset($next_number)) {

        // add Tags
        Tag::add($rev_id, 'number', $next_number, -1);
        Tag::add($rev_id, 'number_sort', str_pad($next_number, 5, '0', STR_PAD_LEFT), -1); // padding with '0'
        Tag::add($rev_id, 'pub_date', date('Y-m-d'), -1);
        Tag::add($rev_id, 'pub_user', $thisuser->id(), -1);

        // update next number
        Tag::update(Tag::getIdByUser($rev_id,'number_next',-1),$next_number+1);
      }

      if ($data_type == 'page' || $data_type == 'dynamic') {
        Tag::add($rev_id, 'extension', 'html', -1);
      }

      if ($show_output === true) {
        new Editor_Website($data_id, $rev_id, 'show');
      }
    }
  } // end of member function getCookieDomain



  /**
   * check before usage if there is a set of entries or not, only call when it
   *
   */
  public static function evalAction( $id_list, $action, $lang_id = 0, $label_name = null )
  {
    $thisuser = &ThisUser::getInstance();

    $id_list = preg_replace('/(^|-)[0-9]+\_([0-9]+)/','$2|',$id_list);
    $rev_id_list = explode('|',substr($id_list,0,-1));


    // preview
    if ($action == 'mp') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, r.rev_id, r.lang_id FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id = d.data_id WHERE r.id = :rev_id LIMIT 1");
      $stmt->bindParam('rev_id',$rev_id_list[0],PDO::PARAM_INT);
      $stmt->execute();
      $revision = $stmt->fetchOnce();

      //@TODO rework preview with caching and such shit
      $generate = new Generate();
      echo $generate->oneEntry($revision['name'], $revision['lang_id']);
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
            if (!$thisuser->hasAccess('more_lang')) {

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

            if ($revision['version'] == 0) {

              // get next rev num
              $stmt=&DBConnection::getInstance()->prepare("SELECT version FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang ORDER BY version DESC, id DESC LIMIT 1");
              $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
              $stmt->bindParam('lang',$revision['lang_id'],PDO::PARAM_INT);
              $stmt->execute();
              $version_num = $stmt->fetchColumn()+1;

              // get latest stable entry
              $stmt=&DBConnection::getInstance()->prepare("SELECT id, data_id, lang_id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang AND archive IS FALSE ORDER BY version DESC, id DESC LIMIT 1");
              $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
              $stmt->bindParam('lang',$revision['lang_id'],PDO::PARAM_INT);
              $stmt->execute();
              $stable_revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);


              // no stable entry exist, so skip move-process
              if ($stable_revision !== false) {

                // delete old tags
                $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id");
                $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
                $stmt->execute();

                // transfer 
                Tag::copyFromData($stable_revision['id'], $revision['id']);

                // move old revision to archive
                if (!Data::toArchive($stable_revision['id'])) {
                  Log::writeMedium('Data::copy() failed: data-id '.$stable_revision['data_id'].', rev-id '.$stable_revision['id'].Log::prepareInfo($stable_revision['data_id'], $stable_revision['rev_id']).'{changetags}');
                  echo 'Process not successful :S';
                }
              }

              // update the version number
              $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET version = :version WHERE id = :rev_id");
              $stmt->bindParam('version',$version_num,PDO::PARAM_INT);
              $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
              $stmt->execute();
              
              // get depencies
              $depency = new DataDepencies();
              $depency->addRevision($revision['id']);

              // generate content
              $generate = new Generate();
              $generate->update($revision['id']);
              echo 'Page generation process finished';
            }
            else {
              echo 'Only &quot;new&quot; entries can be made stable';
            }
            break;

          // mark as new
          case 'mn':
            if (!$thisuser->hasAccess('more_lang')) {

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
            if ($thisuser->hasAccess('more_lang') || $revision['lang_id'] == RosUser::getLanguage($thisuser->id(),true)) {
              if ($thisuser->hasAccess('del_entry') || $revision['user_id'] == $thisuser->id() && $revision['version']==0) {

                // copy to Archive if no admin
                if ($revision['version']) {
                  Data::deleteRevision($revision['id']);
                }
                elseif (!$thisuser->hasAccess('del_wo_archiv')) {
                  Data::toArchive($revision['id']);
                }
                else {
                  //Data::deleteFile($revision['id']);
                  Data::deleteRevision($revision['id']);
                }
              }
              else {
                echo 'Not enough rights for delete process.';
              }
            }
            else {
              echo 'You have no rights to delete entries from other languages';
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
   * @param int rev_id
   * @return bool
   * @access public
   */
  public static function toArchive( $rev_id )
  {
    // remove depencies
    DataDepencies::removeRevision($rev_id);

    // move into archive
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET archive = TRUE WHERE id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    return $stmt->execute();
  }



  /**
   *
   *
   * @param int rev_id
   * @return bool
   * @access public
   */
  public static function hasAccess( $data_id, $area )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_ACL." a ON d.access_id=a.access_id JOIN ".ROSCMST_GROUPS." g ON g.id=a.group_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id JOIN ".ROSCMST_RIGHTS." r ON r.id=a.right_id WHERE d.id=:data_id AND m.user_id =:user_id AND r.name_short=:area LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('area',$area,PDO::PARAM_STR);
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    $stmt->execute();
    return ($stmt->fetchColumn()!==false);
  }



  /**
   *
   *
   * @param int rev_id
   * @return bool
   * @access public
   */
  public static function hasAccessAsList( $area )
  {
    $acl = 'NULL,';
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT a.access_id FROM ".ROSCMST_ACL." a JOIN ".ROSCMST_GROUPS." g ON g.id=a.group_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id JOIN ".ROSCMST_RIGHTS." r ON r.id=a.right_id WHERE m.user_id =:user_id AND r.name_short=:area");
    $stmt->bindParam('area',$area,PDO::PARAM_STR);
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    $stmt->execute();
    while ($list = $stmt->fetch(PDO::FETCH_ASSOC)) {
      if ($acl != 'NULL,') $acl .= ',';
      $acl .= $list['access_id'];
    }
    return $acl;
  }



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
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, d.access_id, r.version, r.user_id, r.lang_id, r.datetime FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id=d.id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    if ($archive_mode === false) {
      $revision = array(
        'data_id' => $revision['data_id'],
        'version' => '0',
        'user_id' => ThisUser::getInstance()->id(),
        'lang_id' => $lang_id,
        'datetime' => date('Y-m-d H:i:s'));
    }
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_REVISIONS." ( id , data_id , version , lang_id , user_id , datetime ) VALUES ( NULL, :data_id, :version, :lang, :user_id, :datetime )");
    $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
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
