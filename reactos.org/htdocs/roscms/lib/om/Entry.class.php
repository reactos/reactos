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
 * class Entry
 * 
 */
class Entry
{



  /**
   * breaks requested entry down to a revision and calls other functions to get specific contents for that revision and selected content type
   *
   * @return string
   * @access public
   */
  public static function getContent( $data_name, $data_type, $lang_id, $content_name, $content_type = 'stext' )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.name = :name AND d.type = :type AND r.lang_id = :lang AND r.archive = FALSE AND r.version > 0 ORDER BY r.version DESC LIMIT 1");
    $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
    $stmt->bindParam('lang',$lang_id,PDO::PARAM_INT);
    $stmt->execute();
    $rev_id = $stmt->fetchColumn();

    if ($rev_id > 0) {
      switch ($content_type) {
        case 'text':
          return Revision::getText($rev_id, $content_name);
          break;
        case 'tag':
          return Tag::getValue($rev_id, $content_name,-1);
          break; 
        case 'stext':
        default:
          return Revision::getSText($rev_id, $content_name);
          break;
      }
    }
    return;
  } // end of member function getContent



  /**
   * update data_id with new params and update links
   *
   * @param int data_id target for update
   * @param string name new data name
   * @param string type new data_type
   * @param string acl new data_acl
   * @param bool update_links if true, all dependent entries will be updated with the new name
   * @access public
   */
  public static function update( $data_id, $name, $type, $acl )
  {
    $type_update = false;
    $name_update = false;
  
    // get entry
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
      $type_update = true;

      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRIES." SET type = :type_new WHERE id = :data_id LIMIT 1");
      $stmt->bindParam('type_new',$data_type,PDO::PARAM_STR);
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->execute();
      Log::writeMedium('data-type changed: '.$data['type'].' =&gt; '.$data_type);
      $new_data_type = $data_type;
    }

    // update data name
    if ($data_name != '' && $data_name != $data['name']) {
      $name_update = true;

      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRIES." SET name = :name_new WHERE id = :id LIMIT 1");
      $stmt->bindParam('name_new',$data_name,PDO::PARAM_STR);
      $stmt->bindParam('id',$data_id,PDO::PARAM_INT);
      $stmt->execute();

      Log::writeMedium('data-name changed: '.$data['name'].' =&gt; '.$data_name);
    }

    // update dependent entries
    if ($type_update || $name_update) {

      // call old data types as array index to get the short version
      $convert = array(
        'content'=>'cont',
        'script'=>'inc');

      // if the datatype has not changed, use the old one
      if ($new_data_type == '') {
        $new_data_type = $data['type'];
      }

      // update text content with new name and or types
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_TEXT." SET content = REPLACE(content, :old_type_name, :new_type_name) WHERE content LIKE :search");
      $stmt->bindParam('search','%[#'.$convert[$data['type']].'_'.$data['name'].']%',PDO::PARAM_STR);
      $stmt->bindParam('old_type_name','[#'.$convert[$data['type']].'_'.$data['name'].']',PDO::PARAM_STR);
      $stmt->bindParam('new_type_name','[#'.$convert[$new_type_short].'_'.$data_name.']',PDO::PARAM_STR);
      $stmt->execute();

      Log::writeMedium('data-interlinks updated due data-name change');
    }

    // update page links
    if ($name_update && ($data['type'] == 'page' || $data['type'] == 'dynamic')) {
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_TEXT." SET content = REPLACE(content, :old_link, :new_link) WHERE content LIKE :search");
      $stmt->bindParam('search','%[#link_'.$data['name'].']%',PDO::PARAM_STR);
      $stmt->bindParam('old_link','[#link_'.$data['name'].']',PDO::PARAM_STR);
      $stmt->bindParam('new_link','[#link_'.$data_name.']',PDO::PARAM_STR);
      $stmt->execute();
    }

    // change ACL
    if ($data_acl != '' && $data_acl != $data['access_id']) {
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRIES." SET access_id = :acl_new WHERE id = :data_id LIMIT 1");
      $stmt->bindParam('acl_new',$data_acl);
      $stmt->bindParam('data_id',$data_id);
      $stmt->execute();
      Log::writeMedium('data-acl changed: '.$data['access_id'].' =&gt; '.$data_acl);
    } 

  } // end of member function update



  /**
   * add a new entry, and new revision
   * in that case, the entry already exists: it tries to add a new revision only (need to be another language for success)
   *
   * @param int rev_id
   * @param int lang_id
   * @return bool
   * @access public
   */
  public static function add($data_name, $data_type = null, $template = '')
  {
    $data_name = trim($data_name);

    // check if entry already exists
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :name AND type = :type LIMIT 1");
    $stmt->bindParam('name',$data_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$data_type,PDO::PARAM_STR);
    $stmt->execute();
    $data_id = $stmt->fetchColumn();

    // if entry does not exist -> create a new one
    if ($data_id === false) {
    
      $stmt_ask=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ACCESS." WHERE standard IS TRUE");
      $stmt_ask->execute();
      $access_id = $stmt_ask->fetchColumn();

      // insert new data
      $stmt_ins=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ENTRIES." ( id , name , type , access_id ) VALUES ( NULL , :name, :type, :access_id )");
      $stmt_ins->bindParam('name',$data_name,PDO::PARAM_STR);
      $stmt_ins->bindParam('type',$data_type,PDO::PARAM_STR);
      $stmt_ins->bindParam('access_id',$access_id,PDO::PARAM_INT);
      $stmt_ins->execute();

      // and get new data_id (use old used statement again)
      $stmt->execute();
      $data_id = $stmt->fetchColumn();
    }

    // report if everything went right
    if ($data_id !== false) {
      $rev_id = Revision::add($data_id, Language::getStandardId());
    }

    // only go on, if we got a new revision
    if ($rev_id === false) {
      echo 'Could not create new revision, maybe the entry already exists.';
      return false;
    }

    // create new stext contents for (dynamic) pages
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, :description, :content )");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindValue('description','description',PDO::PARAM_STR);
    $stmt->bindValue('content','',PDO::PARAM_STR);
    $stmt->execute();
    if ($data_type == 'page' || $data_type == 'dynamic') {

      // add a comment as short text
      $stmt->bindValue('description','comment',PDO::PARAM_STR);
      $stmt->bindValue('content','',PDO::PARAM_STR);
      $stmt->execute();

      // add a title as short text
      $stmt->bindValue('description','title',PDO::PARAM_STR);
      $stmt->bindValue('content',$data_name,PDO::PARAM_STR);
      $stmt->execute();

      // add next dynamic number for dynamic entries
      if ($data_type == 'dynamic') {
        Tag::add($rev_id, 'next_number', 1, -1);
      }
    }

    // set page content to template, if selected
    if ($template != '' && $template != 'none') {
      $content = '[#cont_'.htmlspecialchars($template).']';
    }
    else {
      $content = '';
    }

    // create new revision content text
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, 'content', :content )");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('content',$content,PDO::PARAM_STR);
    $stmt->execute();

    // add dynamic content tags
    if (isset($next_number)) {

      // add Tags
      Tag::add($rev_id, 'number', $next_number, -1);
      Tag::add($rev_id, 'pub_date', date('Y-m-d'), -1);
      Tag::add($rev_id, 'pub_user', ThisUser::getInstance()->id(), -1);

      // update next number
      Tag::update(Tag::getId($rev_id,'number_next',-1),$next_number+1);
    }

    if ($data_type == 'page' || $data_type == 'dynamic') {
      Tag::add($rev_id, 'extension', 'html', -1);
    }
    return $rev_id;
  } // end of member function add



  /**
   * checks if the user has access rights for a specific area for that entry
   *
   * @param int data_id
   * @param string area
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
  } // end of member function hasAccess



  /**
   * returns a list of access_id suitable for direct use in SQL statements
   *
   * @param string area
   * @return string
   * @access public
   */
  public static function hasAccessAsList( $area )
  {
    $acl = 'NULL';
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT a.access_id FROM ".ROSCMST_ACL." a JOIN ".ROSCMST_GROUPS." g ON g.id=a.group_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id JOIN ".ROSCMST_RIGHTS." r ON r.id=a.right_id WHERE m.user_id =:user_id AND r.name_short=:area");
    $stmt->bindParam('area',$area,PDO::PARAM_STR);
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    if ($stmt->execute()) {
      while ($list = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $acl .= ','.$list['access_id'];
      }
      return $acl;
    }
    return false;
  } // end of member function hasAccessAsList



} // end of Entry
?>
