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
  public static function getContent( $data_name, $data_type, $lang, $content_name, $content_type = 'stext', $mode = '' )
  {
    global $h_a;
    global $h_a2;


    // set to archive mode, if needed
    if ($mode == 'archive') {
      $_GET['d_arch']=true;
      $h_a = '_a';
      $h_a2 = 'a';
    }

    $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id, r.data_id FROM data_".$h_a2." d JOIN data_revision".$h_a." r ON r.data_id = d.data_id WHERE d.data_name = :name AND d.data_type = :type AND r.rev_language = :lang AND r.rev_version > 0 ORDER BY r.rev_version DESC LIMIT 1");
    $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
    $stmt->bindParam('lang',$lang,PDO::PARAM_STR);
    $stmt->execute();
    $data = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    if ($data['rev_id'] > 0) {
      switch ($content_type) {
        case 'text':
          return self::getText($data['rev_id'], $content_name);
          break;
        case 'tag':
          return Tag::getValue($data['data_id'], $data['rev_id'], $content_name);
          break; 
        case 'stext':
        default:
          return self::getSText($data['rev_id'], $content_name);
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
  public static function getSText( $rev_id, $entry_name )
  {
    global $h_a;


    $stmt=DBConnection::getInstance()->prepare("SELECT stext_content FROM data_stext".$h_a." WHERE data_rev_id = :rev_id AND stext_name = :name LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$entry_name,PDO::PARAM_STR);
    $stmt->execute();
    return $stmt->fetchColumn();
  } // end of member function compareGregorianDate


  /**
   *
   *
   * @return 
   * @access public
   */
  public static function getText( $rev_id, $entry_name )
  {
    global $h_a;

    $stmt=DBConnection::getInstance()->prepare("SELECT text_content FROM data_text".$h_a." WHERE data_rev_id = :rev_id AND text_name = :name LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('name',$entry_name,PDO::PARAM_STR);
    $stmt->execute();
    return $stmt->fetchColumn();
  } // end of member function getCookieDomain


  /**
   *
   *
   * @param int rev_id
   * @access public
   */
  public static function updateRevision( $data_id, $rev_id, $lang, $version, $user_name, $date, $time )
  {
    global $h_a;
    global $h_a2;

    // get current state, so that we only update changed information
    $stmt=DBConnection::getInstance()->prepare("SELECT rev_language, rev_version, rev_usrid, rev_date, rev_time FROM data_revision".$h_a." WHERE rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // language
    if ($lang != '' && $lang != $data['rev_language']) {

      // check if the choosen language do exist
      $stmt=DBConnection::getInstance()->prepare("SELECT 1 FROM languages WHERE lang_id = :lang LIMIT 1");
      $stmt->bindParam('lang',$lang,PDO::PARAM_INT);
      $stmt->execute();

      // update with new language
      if ($stmt->fetchColumn() !== false) {
        $stmt=DBConnection::getInstance()->prepare("UPDATE data_revision".$h_a." SET rev_language = :lang WHERE rev_id = :rev_id LIMIT 1");
        $stmt->bindParam('lang',$lang,PDO::PARAM_STR);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt->execute();
        Log::writeMedium('entry language changed '.$data['rev_language'].' =&gt; '.$lang.Log::prepareInfo($data_id, $rev_id).'{alterentry}');
      }
    }

    // version
    if ($version != '' && $version != $data['rev_version']) {

      // check for existing revisons with same number
      $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM data_revision".$h_a." WHERE rev_version = :version AND data_id = :data_id AND rev_language = :lang");
      $stmt->bindParam('version',$version,PDO::PARAM_INT);
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindParam('lang',$data['rev_language'],PDO::PARAM_STR);
      $stmt->execute();

      // update with new version
      if ($stmt->fetchColumn() <= 0) {
        $stmt=DBConnection::getInstance()->prepare("UPDATE data_revision".$h_a." SET rev_version = :version WHERE rev_id = :rev_id LIMIT 1");
        $stmt->bindParam('version',$version,PDO::PARAM_INT);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt->execute();
        Log::writeMedium('entry version-number changed: '.$data['rev_version'].' =&gt; '.$version.Log::prepareInfo($data_id, $rev_id).'{alterentry}');
      }
    }

    // user-name
    if ($user_name != '') {

      // check for existing user-name
      $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_name = :user_name LIMIT 1");
      $stmt->bindParam('user_name',$user_name,PDO::PARAM_STR);
      $stmt->execute();
      $user_id = $stmt->fetchColumn();

      if ($user_id > 0 && $user_id != $data['rev_usrid']) {
        $stmt=DBConnection::getInstance()->prepare("UPDATE data_revision".$h_a." SET rev_usrid = :user_id WHERE rev_id = :rev_id LIMIT 1");
        $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt->execute();
        Log::writeMedium('entry user-name changed: '.$data['rev_usrid'].' =&gt; '.$user_id.' ('.$user_name.')'.Log::prepareInfo($data_id, $rev_id).'{alterentry}');
      }
    }

    // date + time (check for Y-m-d and valid-date) + (H:i:s)
    if (preg_match('/^([12][0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])$/', $date,$date_matches) && checkdate($date_matches[2], $date_matches[3], $date_matches[1]) && preg_match('/^([01][0-9]|2[0-3])(:[0-5][0-9]){2}$/',$time) && ($data['rev_date'] != $date || $data['rev_time'] != $time) ) {

      //
      $stmt=DBConnection::getInstance()->prepare("UPDATE data_revision".$h_a." SET rev_datetime = :datetime, rev_date = :date, rev_time = :time WHERE rev_id = :rev_id LIMIT 1");
      $stmt->bindValue('datetime',$date." ".$time,PDO::PARAM_STR);
      $stmt->bindParam('date',$date,PDO::PARAM_STR);
      $stmt->bindParam('time',$time, PDO::PARAM_STR);
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();
      Log::writeMedium('entry date+time changed: '.$data['rev_date'].' '.$data['rev_time'].' =&gt; '.$date.' '.$time.Log::prepareInfo($data_id, $rev_id).'{alterentry}');
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

    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_language FROM data_ d JOIN data_revision r ON r.data_id=d.data_id WHERE r.rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $page = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // only for entries of type page
    if ($page === false) {
      return;
    }

    //if data_type is page -> delete file in all languages
    if ($page['data_type'] == 'page'){
      $stmt=DBConnection::getInstance()->prepare("SELECT lang_id FROM languages");
    }

    //if data_type is content (only for dynamic) -> delete only selected one
    elseif ($page['data_type'] == ''){
      $dynamic_num = Tag::getValueByUser($page['data_id'],$page['rev_id'],'number',-1);
      if ($dynamic_num > 0) {
        $stmt=DBConnection::getInstance()->prepare("SELECT lang_id FROM languages WHERE lang_id = :lang_id LIMIT 1");
        $stmt->bindParam('lang_id',$page['rev_language'],PDO::PARAM_STR);
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
    $file_extension = Tag::getValueByUser($page['data_id'], $page['rev_id'], 'extension', -1);
    $file_name = $page['data_name'].(isset($dynamic_num) ? '_'.$dynamic_num : '').'.'.$file_extension;

    // delete entries for selected language packs
    $stmt->execute();
    while ($lang = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // delete file if it exists
      if ( file_exists('../'.$lang['lang_id'].'/'.$file_name)) {
        unlink('../'.$lang['lang_id'].'/'.$file_name);
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
    $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_revision WHERE rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_stext WHERE data_rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_text WHERE data_rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();

    // delete tags, and check if tag name and value are anymore used
    $stmt=DBConnection::getInstance()->prepare("SELECT tag_name_id, tag_value_id FROM data_tag  WHERE data_rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id);
    $stmt->execute(); // used in while

    // as we have a result set, we no longer need the tags
    $stmt_delete=DBConnection::getInstance()->prepare("DELETE FROM data_tag WHERE data_rev_id = :rev_id LIMIT 1");
    $stmt_delete->bindParam('rev_id',$rev_id);
    $stmt_delete->execute();

    // prepare for usage in loop
      $stmt_name=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM data_tag WHERE tag_name_id = :tag_name_id");
      $stmt_delete_name=DBConnection::getInstance()->prepare("DELETE FROM data_tag_name WHERE tn_id = :tag_name_id LIMIT 1");
      $stmt_value=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM data_tag WHERE tag_value_id = :tag_value_id");
      $stmt_delete_value=DBConnection::getInstance()->prepare("DELETE FROM data_tag_value WHERE tv_id = :tag_value_id LIMIT 1");
    
    while ($tag = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // delete tag name
      $stmt_name->bindParam('tag_name_id',$tag['tag_name_id'],PDO::PARAM_INT);
      $stmt_name->execute();
      if ($stmt_name->fetchColumn() === 0) {
        $stmt_delete_name->bindParam('tag_name',$tag['tag_name_id'],PDO::PARAM_INT);
        $stmt_delete_name->execute();
      }

      // delete tag value
      $stmt_value->bindParam('tag_value_id',$tag['tag_value_id'],PDO::PARAM_INT);
      $stmt_value->execute();
      if ($stmt_value->fetchColumn() === 0) {
        $stmt_delete_value->bindParam('tag_value_id',$tag['tag_value_id'],PDO::PARAM_INT);
        $stmt_delete_value->execute();
      }
    } // while
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
      $stmt_check=DBConnection::getInstance()->prepare("SELECT 1 FROM data_revision r JOIN data_stext s ON s.data_rev_id = r.rev_id WHERE r.rev_id = :rev_id AND s.stext_name = :name LIMIT 1");
      $stmt_check->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // delete
      $stmt_delete=DBConnection::getInstance()->prepare("DELETE FROM data_stext WHERE data_rev_id = :rev_id AND stext_name = :name LIMIT 1"); 
      $stmt_delete->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // add
      $stmt_insert=DBConnection::getInstance()->prepare("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) VALUES ( NULL , :rev_id, :name, '' )");
      $stmt_insert->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // update
      $stmt_update=DBConnection::getInstance()->prepare("UPDATE data_stext SET stext_name = :name_new WHERE data_rev_id = :rev_id AND stext_name = :name LIMIT 1");
      $stmt_update->bindParam('rev_id',$rev_id,PDO::PARAM_INT);

    // Short Text Fields
    $entries = explode('|', $stext);
    var_dump($stext);
    foreach ($entries as $entry) {
      $entry_parts = explode('=', $entry);

      $stext_name = $entry_parts[0];
      $new_stext_name = $entry_parts[1];
      $stext_del = $entry_parts[2]=='true';

      // check if any work has to be done
      if ($stext_name == $new_stext_name && $stext_del == false) {
        continue;
      }

      // get number of stext which match the cri
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
      $stmt_check=DBConnection::getInstance()->prepare("SELECT 1 FROM data_revision r JOIN data_text t ON t.data_rev_id = r.rev_id WHERE r.rev_id = :rev_id AND t.text_name = :name LIMIT 1"); 
      $stmt_check->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // add
      $stmt_insert=DBConnection::getInstance()->prepare("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) VALUES ( NULL , :rev_id, :name, '' )");
      $stmt_insert->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // update
      $stmt_update=DBConnection::getInstance()->prepare("UPDATE data_text SET text_name = :name_new WHERE data_rev_id = :rev_id AND text_name = :name LIMIT 1");
      $stmt_update->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      // delete
      $stmt_delete=DBConnection::getInstance()->prepare("DELETE FROM data_text WHERE data_rev_id = :rev_id AND text_name = :name LIMIT 1"); 
      $stmt_delete->bindParam('rev_id',$rev_id,PDO::PARAM_INT);

    // Text Fields
    $entries = explode('|', $text);
    var_dump($text);
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
    global $h_a;
    global $h_a2;

    $stmt=DBConnection::getInstance()->prepare("SELECT data_name, data_type, data_acl FROM data_".$h_a2." WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // check if we have a dataset to work with
    if ($data === false) {
      return;
    }

    // update data type
    if ($data_type != '' && $data_type != $data['data_type']) {
      $stmt=DBConnection::getInstance()->prepare("UPDATE data_".$h_a2." SET data_type = :type_new WHERE data_id = :data_id LIMIT 1");
      $stmt->bindParam('type_new',$data_type,PDO::PARAM_STR);
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->execute();
      Log::writeMedium('data-type changed: '.$data['data_type'].' =&gt; '.$data_type.Log::prepareInfo($data_id).'{altersecurityfields}');
      $new_data_type = $data_type;
    }

    // update data name
    if ($data_name != '' && $data_name != $data['data_name']) {
      $stmt=DBConnection::getInstance()->prepare("UPDATE data_".$h_a2." SET data_name = :name_new WHERE data_id = :id LIMIT 1");
      $stmt->bindParam('name_new',$data_name,PDO::PARAM_STR);
      $stmt->bindParam('id',$data_id,PDO::PARAM_INT);
      $stmt->execute();

      Log::writeMedium('data-name changed: '.$data['data_name'].' =&gt; '.$data_name.Log::prepareInfo($data_id).'{altersecurityfields}');

      // update dependent entries
      if ($update_links == true) {
        if ($new_data_type == '') {
          $new_data_type = $data['data_type'];
        }

        // old type
        switch ($data['data_type']) {
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
        $stmt=DBConnection::getInstance()->prepare("UPDATE data_text".$h_a." SET text_content = REPLACE(REPLACE(text_content, :old_type_name, :new_type_name), :old_link, :new_link) WHERE text_content LIKE :search1 OR text_content LIKE :search2 ORDER BY text_id ASC");
        $stmt->bindParam('search1','%[#'.$old_type_short.'_'.$data['data_name'].']%',PDO::PARAM_STR);
        $stmt->bindParam('search2','%[#link_'.$data['data_name'].']%',PDO::PARAM_STR);
        $stmt->bindParam('old_type_name','[#'.$old_type_short.'_'.$data['data_name'].']',PDO::PARAM_STR);
        $stmt->bindParam('new_type_name','[#'.$new_type_short.'_'.$data_name.']',PDO::PARAM_STR);
        $stmt->bindParam('old_link','[#link_'.$data['data_name'].']',PDO::PARAM_STR);
        $stmt->bindParam('new_link','[#link_'.$data_name.']',PDO::PARAM_STR);
        $stmt->execute();

        Log::writeMedium('data-interlinks updated due data-name change'.Log::prepareInfo($data_id).'{altersecurityfields}');
      }
    } // end data_name changes

    // change ACL
    if ($data_acl != '' && $data_acl != $data['data_acl']) {
      $stmt=DBConnection::getInstance()->prepare("UPDATE data_".$h_a2." SET data_acl = :acl_new WHERE data_id = :data_id LIMIT 1");
      $stmt->bindParam('acl_new',$data_acl);
      $stmt->bindParam('data_id',$data_id);
      $stmt->execute();
      Log::writeMedium('data-acl changed: '.$data['data_acl'].' =&gt; '.$data_acl.Log::prepareInfo($data_id).'{altersecurityfields}');
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
  public static function add($data_type = null, $lang = null, $show_output = false, $dynamic_content = false, $entry_status = 'draft', $layout_template = '')
  {
    $thisuser = &ThisUser::getInstance();

    $data_name = trim(@htmlspecialchars($_GET['d_name']));

    $stmt=DBConnection::getInstance()->prepare("SELECT data_id FROM data_ WHERE data_name = :name AND data_type = :type LIMIT 1");
    $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
    $stmt->execute();
    $data_id = $stmt->fetchColumn();

    if ($data_id === false) {
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_ ( data_id , data_name , data_type ) VALUES ( NULL , :name, :type )");
      $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
      $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
      $stmt->execute();

      $stmt=DBConnection::getInstance()->prepare("SELECT data_id FROM data_ WHERE data_name = :name AND data_type = :type LIMIT 1");
      $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
      $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
      $stmt->execute();
      $data_id = $stmt->fetchColumn();
    }

    $stmt=DBConnection::getInstance()->prepare("SELECT 1 FROM data_revision WHERE data_id = :data_id AND rev_language = :lang LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('lang',$lang,PDO::PARAM_STR);
    $stmt->execute();
    $revision_exists = $stmt->fetchColumn();

    if ($revision_exists === false || $dynamic_content === true) {
      // revision entry doesn't exist
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) VALUES ( NULL, :data_id, 0, :lang, :user_id, NOW(), CURDATE(), CURTIME() )");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindParam('lang',$lang,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->execute();
      
      $stmt=DBConnection::getInstance()->prepare("SELECT rev_id FROM data_revision WHERE data_id = :data_id AND rev_version = '0' AND rev_language = :lang AND rev_usrid = :user_id ORDER BY rev_datetime DESC");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindParam('lang',$lang,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->execute();
      $rev_id = $stmt->fetchColumn();

      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) VALUES ( NULL, :rev_id, :description, :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindValue('description','description',PDO::PARAM_STR);
      $stmt->bindValue('content','',PDO::PARAM_STR);
      $stmt->execute();
      if ($data_type == "page") {
        // add also a comment
        $stmt->bindValue('description','comment',PDO::PARAM_STR);
        $stmt->bindValue('content','',PDO::PARAM_STR);
        $stmt->execute();
        // add also a title
        $stmt->bindValue('description','title',PDO::PARAM_STR);
        $stmt->bindValue('content',ucfirst($data_name),PDO::PARAM_STR);
        $stmt->execute();
      }
      elseif ($data_type == 'content' && $dynamic_content === true) {
        // add a title
        $stmt->bindValue('description','title',PDO::PARAM_STR);
        $stmt->bindValue('content',ucfirst($data_name),PDO::PARAM_STR);
        $stmt->execute();
      }

      if ($layout_template != '' && $layout_template != 'none') {
        $content = '[#templ_'.$layout_template.']';
      }
      else {
        $content = '';
      }

      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) VALUES ( NULL, :rev_id, 'content', :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindParam('content',$content,PDO::PARAM_STR);
      $stmt->execute();

      Tag::add($data_id, $rev_id, 'status', $entry_status, -1);

      // add dynamic content tags
      if ($dynamic_content === true) {

        // get highest saved dynamic number for data_name
        $dynamic_number = 0;
        $stmt=DBConnection::getInstance()->prepare("SELECT v.tv_value FROM data_revision r JOIN data_tag a ON (r.data_id = a.data_id AND r.rev_id = a.data_rev_id) JOIN data_tag_name n ON a.tag_name_id = n.tn_id JOIN  data_tag_value v ON a.tag_value_id  = v.tv_id WHERE r.data_id = :data_id AND r.rev_language = :lang AND r.rev_version > 0 AND a.tag_usrid = -1 AND n.tn_name = 'number' ORDER BY v.tv_value DESC");
        $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
        $stmt->bindParam('lang',$lang,PDO::PARAM_STR);
        $stmt->execute();
        while ( $tag_value = intval($stmt->fetchColumn())) {
          // get dynamic content number
          if ($tag_value > $dynamic_number) {
            $dynamic_number = $tag_value;
          }
        }
        $dynamic_number++;

        // add Tags
        Tag::add($data_id, $rev_id, 'number', $dynamic_number, -1);
        Tag::add($data_id, $rev_id, 'number_sort', str_pad($dynamic_number, 5, '0', STR_PAD_LEFT), -1); // padding with '0'
        Tag::add($data_id, $rev_id, 'pub_date', date('Y-m-d'), -1);
        Tag::add($data_id, $rev_id, 'pub_user', $thisuser->id(), -1);
      }

      if ($data_type == 'page') {
        Tag::add($data_id, $rev_id, 'extension', 'html', -1);
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
  public static function evalAction( $id_list, $action, $lang = null, $label_name = null )
  {
    $thisuser = &ThisUser::getInstance();

    global $roscms_standard_language;

    $id_list = preg_replace('/(^|-)[0-9]+\_([0-9]+)/','$2|',$id_list);
    $rev_id_list = explode('|',substr($id_list,0,-1));

    $xport = new Export_HTML();

    // preview
    if ($action == 'mp') {
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_name, d.data_id, r.rev_id, r.rev_language FROM data_revision r JOIN data_ d ON r.data_id = d.data_id WHERE r.rev_id = :rev_id LIMIT 1");
      $stmt->bindParam('rev_id',$rev_id_list[0],PDO::PARAM_INT);
      $stmt->execute();
      $revision = $stmt->fetchOnce();

      $dynamic_num = Tag::getValueByUser($revision['data_id'], $revision['rev_id'], 'number', -1);
      echo $xport->processTextByName($revision['data_name'], $revision['rev_language'], $dynamic_num, 'show');
    }

    else {
    
      // get user language
      $stmt_lang=DBConnection::getInstance()->prepare("SELECT user_language FROM users WHERE user_id = :user_id LIMIT 1");
      $stmt_lang->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt_lang->execute();
      $user_lang = $stmt_lang->fetchColumn();

      foreach ($rev_id_list as $rev_id) {

        // get revision information
        $stmt_rev=DBConnection::getInstance()->prepare("SELECT rev_language, rev_version, data_id, rev_id, rev_usrid FROM data_revision r WHERE r.rev_id = :rev_id LIMIT 1");
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
                die('Set a valid language in your myReactOS account settings!');
              }
              elseif ($user_lang != $revision['rev_language']) {
                echo 'As Language Maintainer you can only mark entries of "'.$user_lang.'" language as stable!';
                continue;
              }
            }

            Log::writeLow('mark entry as stable: data-id '.$revision['data_id'].', rev-id '.$revision['rev_id'].Log::prepareInfo($revision['data_id'], $revision['rev_id']).'{changetags}');

            // renew tag
            $tag_id = Tag::getIdByUser($revision['data_id'], $revision['rev_id'], 'status', -1);
            if ($tag_id > 0) {
              Tag::deleteById($tag_id, $thisuser->id());
            }
            Tag::add($revision['data_id'], $revision['rev_id'], 'status', 'stable', -1);

            if ($revision['rev_version'] == 0) {
              $dynamic_num = Tag::getValueByUser($revision['data_id'], $revision['rev_id'], 'number',  '-1'); 

              if ($dynamic_num > 0) {
                $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id, r.data_id, r.rev_version, r.rev_language FROM data_revision r JOIN data_tag a ON r.rev_id = a.data_rev_id JOIN data_tag_name n ON a.tag_name_id = n.tn_id JOIN data_tag_value v ON a.tag_value_id = v.tv_id WHERE r.data_id = :data_id AND r.rev_version > 0 AND r.rev_language = :lang AND a.tag_usrid = -1 AND n.tn_name = 'number' AND v.tv_value = :tag_value ORDER BY r.rev_version DESC, r.rev_id DESC LIMIT 1");
                $stmt->bindParam('tag_value',$dynamic_num,PDO::PARAM_STR);
              }
              else {
                $stmt=DBConnection::getInstance()->prepare("SELECT rev_id, data_id, rev_version, rev_language FROM data_revision WHERE data_id = :data_id AND rev_version > 0 AND rev_language = :lang ORDER BY rev_version DESC, rev_id DESC LIMIT 1");
              }
              $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
              $stmt->bindParam('lang',$revision['rev_language'],PDO::PARAM_STR);
              $stmt->execute();
              $stable_revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);
//var_dump($stable_revision);
              // setup a new version number
              $version_num = 1;

              // no stable entry exist, so skip move-process
              if ($stable_revision !== false) {
              
                // stable entry exist, so increase the version number
                $version_num = $stable_revision['rev_version'] + 1;

                // delete old tags
                $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_tag WHERE data_rev_id = :rev_id");
                $stmt->bindParam('rev_id',$revision['rev_id'],PDO::PARAM_INT);
                $stmt->execute();

                // transfer 
                Tag::copyFromData($stable_revision['data_id'], $stable_revision['rev_id'], $revision['data_id'], $revision['rev_id'], false);

                // move old revision to archive
                if (Data::copy($stable_revision['data_id'], $stable_revision['rev_id'], 0, $lang)) {
                  Data::deleteRevision($stable_revision['rev_id']);
                }
                else {
                  Log::writeMedium('Data::copy() failed: data-id '.$stable_revision['data_id'].', rev-id '.$stable_revision['rev_id'].Log::prepareInfo($stable_revision['data_id'], $stable_revision['rev_id']).'{changetags}');
                  echo 'Process not successful :S';
                }
              }

              // update the version number
              $stmt=DBConnection::getInstance()->prepare("UPDATE data_revision SET rev_version = :version WHERE rev_id = :rev_id LIMIT 1");
              $stmt->bindParam('version',$version_num,PDO::PARAM_INT);
              $stmt->bindParam('rev_id',$revision['rev_id'],PDO::PARAM_INT);
              $stmt->execute();

              // get language to generation
              if ($revision['rev_language'] === '') {
                $generation_lang = $roscms_standard_language;
              }
              else {
                $generation_lang = $revision['rev_language'];
              }

              $stmt=DBConnection::getInstance()->prepare("SELECT g.data_id FROM data_ g JOIN data_ h ON g.data_name=h.data_name WHERE h.data_id = :data_id AND g.data_type = 'page' LIMIT 1");
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
              if ($user_lang == '') {
                die('Set a valid language in your myReactOS account settings!');
              }
              elseif ($user_lang != $revision['rev_language']) {
                echo 'As Language Maintainer you can only mark entries of "'.$user_lang.'" language as new!';
                continue;
              }
            }

            //
            $tag_id = Tag::getIdByUser($revision['data_id'], $revision['rev_id'], 'status', -1);
            if ($tag_id > 0) {
              Tag::deleteById($tag_id, $thisuser->id());
            }
            Tag::add($revision['data_id'], $revision['rev_id'], 'status', 'new', -1);

            $stmt=DBConnection::getInstance()->prepare("UPDATE data_revision SET rev_version = 0 WHERE rev_id = :rev_id");
            $stmt->bindParam('rev_id',$revision['rev_id'],PDO::PARAM_INT);
            $stmt->execute();
            break;

          // add star
          case 'as':
            $tag_id = Tag::getIdByUser($revision['data_id'], $revision['rev_id'], 'star', $thisuser->id());
            if ($tag_id > 0) {
              Tag::deleteById($t_tagid, $thisuser->id());
            }
            Tag::add($revision['data_id'], $revision['rev_id'], 'star', 'on', $thisuser->id());
            break;

          // delete star
          case 'xs':
            $tag_id = Tag::getIdByUser($revision['data_id'], $revision['rev_id'], 'star', $thisuser->id());
            if ($tag_id > 0) {
              Tag::deleteById($tag_id, $thisuser->id());
            }
            break;

          // add label
          case 'tg':
            Tag::add($revision['data_id'], $revision['rev_id'], 'tag', $label_name, $thisuser->id());
            break;

          // delete entry
          case 'xe':
            if ($thisuser->securityLevel() > 1 || $revision['rev_usrid'] == $thisuser->id()) {

              // copy to Archive if no admin
              if ($thisuser->securityLevel() < 3) {
                Data::copy($revision['data_id'], $revision['rev_id'], 0, $lang);
              }
              Data::deleteFile($revision['rev_id']);
              Data::deleteRevision($revision['rev_id']);
            }
            else {
              echo 'Not enough rights for delete process.';
            }
            break;

          // move to archiv
          case 'va':
            Data::copy($revision['data_id'], $revision['rev_id'], 0, $lang);
            Data::deleteFile($revision['rev_id']);
            Data::deleteRevision($revision['rev_id']);
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
  public static function copy( $data_id, $rev_id, $archive_mode, $lang = '' )
  {
    // set archive mode dependent vars
    if ($archive_mode == 0) {
      // copy to archive
      $h_a = '_a';
      $archive_mode = true;
    }
    elseif ($archive_mode == 1) {
      // create copy
      $h_a = '';
      $archive_mode = false;
    }
    else {
      die('move_to_archive: wrong param');
    }

    // Log something
    if ($archive_mode === false) {
      Log::writeLow('copy entire entry (e.g. translate): data-id '.$data_id.', rev-id '.$rev_id.Log::prepareInfo($data_id, $rev_id).'{move_to_archive}');
    }
    else {
      Log::writeMedium('move entire entry to archive: data-id '.$data_id.', rev-id '.$rev_id.Log::prepareInfo($data_id, $rev_id).'{move_to_archive}');
    }

    //
    $stmt=DBConnection::getInstance()->prepare("SELECT data_id, data_name, data_type, data_acl FROM data_ WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // check if entry already has a archive entry
    if ($archive_mode === true) {
      $stmt_archive_data=DBConnection::getInstance()->prepare("SELECT data_id FROM data_a WHERE data_name = :name AND data_type = :type ORDER BY data_id DESC LIMIT 1");
      $stmt_archive_data->bindParam('name',$data['data_name'],PDO::PARAM_STR);
      $stmt_archive_data->bindParam('type',$data['data_type'],PDO::PARAM_STR);
      $stmt_archive_data->execute();
      $new_data_id = $stmt_archive_data->fetchColumn();

      // if not exist, copy "data_" entry to archive
      if ($new_data_id === false) { 
        $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_a ( data_id , data_name , data_type , data_acl ) VALUES ( NULL, :name, :type, :acl )");
        $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
        $stmt->bindParam('type',$data['data_type'],PDO::PARAM_STR);
        $stmt->bindParam('acl',$data['data_acl'],PDO::PARAM_STR);
        $stmt->execute();

        // and get new 
        $stmt_archive_data->execute(); // use statement from earlier scope
        $new_data_id = $stmt_archive_data->fetchColumn();
      }
    }
    else {
      $new_data_id = $data['data_id'];
    }

    // data_revision
    $stmt=DBConnection::getInstance()->prepare("SELECT rev_version, rev_usrid, rev_language, rev_datetime, rev_date, rev_time FROM data_revision WHERE rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    if ($archive_mode === false) {
      $revision = array(
        'rev_version' => '0',
        'rev_usrid' => ThisUser::getInstance()->id(),
        'rev_language' => $lang,
        'rev_datetime' => date('Y-m-d H:i:s'),
        'rev_date' => date('Y-m-d'),
        'rev_time' => date('H:i:s'));
    }
    $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_revision".$h_a." ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) VALUES ( NULL, :data_id, :version, :lang, :user_id, :datetime, :date, :time )");
    $stmt->bindParam('data_id',$new_data_id,PDO::PARAM_INT);
    $stmt->bindValue('version',$revision['rev_version'],PDO::PARAM_INT);
    $stmt->bindParam('lang',$revision['rev_language'],PDO::PARAM_STR);
    $stmt->bindParam('user_id',$revision['rev_usrid'],PDO::PARAM_INT);
    $stmt->bindParam('datetime',$revision['rev_datetime'],PDO::PARAM_STR);
    $stmt->bindParam('date',$revision['rev_date'],PDO::PARAM_STR);
    $stmt->bindParam('time',$revision['rev_time'],PDO::PARAM_STR);
    $stmt->execute();

    $stmt=DBConnection::getInstance()->prepare("SELECT rev_id FROM data_revision".$h_a." WHERE data_id = :data_id AND rev_datetime = :datetime ORDER BY rev_id DESC LIMIT 1");
    $stmt->bindParam('data_id',$new_data_id,PDO::PARAM_INT);
    $stmt->bindParam('datetime',$revision['rev_datetime'],PDO::PARAM_STR);
    $stmt->execute();
    $new_rev_id = $stmt->fetchColumn();

    if ($new_rev_id === false) {
      die("copy-process of data_revision not successful");
    }


    // data_stext
    $stmt=DBConnection::getInstance()->prepare("SELECT stext_name, stext_content FROM data_stext WHERE data_rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    // prepare for ussage in while loop
    $stmt_insert=DBConnection::getInstance()->prepare("INSERT INTO data_stext".$h_a." ( stext_id , data_rev_id , stext_name , stext_content ) VALUES ( NULL, :rev_id, :name, :content)");
    $stmt_insert->bindParam('rev_id',$new_rev_id,PDO::PARAM_INT);

    while($stext = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $stmt_insert->bindParam('name',$stext['stext_name'],PDO::PARAM_STR);
      $stmt_insert->bindParam('content',$stext['stext_content'],PDO::PARAM_STR);
      $stmt_insert->execute();
    }

    // data_text
    $stmt=DBConnection::getInstance()->prepare("SELECT text_name, text_content FROM data_text WHERE data_rev_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    // prepare for ussage in while loop
    $stmt_insert=DBConnection::getInstance()->prepare("INSERT INTO data_text".$h_a." ( text_id , data_rev_id , text_name , text_content ) VALUES ( NULL, :rev_id, :name, :content )");
    $stmt_insert->bindParam('rev_id',$new_rev_id,PDO::PARAM_INT);

    while($text = $stmt->fetch(PDO::FETCH_ASSOC)) {
     $stmt_insert->bindParam('name',$text['text_name']);
      $stmt_insert->bindParam('content',$text['text_content']);
      $stmt_insert->execute();
    }


    // data_tag
    Tag::copyFromData($data_id, $rev_id, $new_data_id, $new_rev_id, $archive_mode);
    if ($archive_mode === false) {
      // change status to draft
      Tag::deleteByName($new_data_id, $new_rev_id, 'status', -1);
      Tag::add($new_data_id, $new_rev_id, 'status', 'draft', -1);
    }

    return true;
  } // end of member function compareGregorianDate




} // end of Data
?>
