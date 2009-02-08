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
 * class Tag
 * 
 */
class Tag
{



  /**
   * returns tag id
   *
   * @param int rev_id 
   * @param string tag_name 
   * @param int user_id 
   * @return int|bool
   * @access public
   */
  public static function getId( $rev_id, $tag_name, $user_id )
  {
    // sql stuff
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id AND name = :tag_name AND user_id = :user_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('tag_name',$tag_name,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    if ($stmt->execute()) {
      return (int)$stmt->fetchColumn();
    }
    return false;
  } // end of member function getId



  /**
   * wrapper for deleteById
   *
   * @param int rev_id 
   * @param string tag_name 
   * @param int user_id 
   * @return bool
   * @access public
   */
  public static function deleteByName( $rev_id, $tag_name, $user_id )
  {
    $tag_id = self::getId($rev_id, $tag_name, $user_id);
    return self::deleteById($tag_id);
  } // end of member function deleteByName



  /**
   * deletes tag, and name+value if they're only used once by this tag
   *
   * @param int tag_id
   * @return bool
   * @access public
   */
  public static function deleteById( $tag_id )
  {
    // get tag data
    $stmt=&DBConnection::getInstance()->prepare("SELECT user_id FROM ".ROSCMST_TAGS." WHERE id = :tag_id LIMIT 1");
    $stmt->bindParam('tag_id',$tag_id,PDO::PARAM_INT);
    $stmt->execute();
    $tag = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // @unimplemented: account group membership check
    if ($tag['user_id'] == ThisUser::getInstance()->id() || ThisUser::getInstance()->hasAccess('deltag')) {

      // finally delete tag
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TAGS." WHERE id = :tag_id LIMIT 1");
      $stmt->bindParam('tag_id',$tag_id,PDO::PARAM_INT);
      return $stmt->execute();
    }
    return false;
  } // end of member function deleteById



  /**
   * add a new tag
   *
   * @param int rev_id 
   * @param string tag_name 
   * @param string tag_value 
   * @param int user_id 
   * @return bool
   * @access public
   */
  public static function add( $rev_id, $tag_name, $tag_value, $user_id )
  {
    // check if user has rights to add this type of tag
    if ($user_id != ThisUser::getInstance()->id() && ($user_id != -1 || !ThisUser::getInstance()->hasAccess('system_tags'))) {
      die('ERROR: no rights to access this function');
    }

    // tag already exists ?
    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id AND name = :tag_name AND value = :tag_value AND user_id = :user_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('tag_name',$tag_name_id,PDO::PARAM_STR);
    $stmt->bindParam('tag_value',$tag_value_id,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute();
    $tag_exists = $stmt->fetchColumn();

    // add new tag if necessary
    if ($tag_exists === false) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TAGS." ( id , rev_id , name , value , user_id ) VALUES (NULL, :rev_id, :tag_name, :tag_value, :user_id)");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindParam('tag_name',$tag_name,PDO::PARAM_STR);
      $stmt->bindParam('tag_value',$tag_value,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
      return $stmt->execute();
    }

    return false;
  } // end of member function add



  /**
   * updates a given tag with new value
   *
   * @param int rev_id 
   * @param string new_value 
   * @return bool
   * @access public
   */
  public static function update( $tag_id, $new_value )
  {
    // tag already exists ?
    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_TAGS." WHERE id = :tag_id AND user_id IN(-1, 0, :user_id) LIMIT 1");
    $stmt->bindParam('tag_id',$tag_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    $stmt->execute();
    if ($stmt->fetchColumn()) {

      // update value
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_TAGS." SET value = :new_value WHERE id=:tag_id");
      $stmt->bindParam('tag_id',$tag_id,PDO::PARAM_INT);
      $stmt->bindParam('new_value',$new_value,PDO::PARAM_STR);
      return $stmt->execute();
    }

    return false;
  } // end of member function update



  /**
   * copies tags from old-data to new-data
   *
   * @param int old_rev_id from revision
   * @param int new_rev_id to revision
   * @return bool
   * @access public
   */
  public static function copyFromRevision( $old_rev_id, $new_rev_id )
  {
    // prepare insert for usage in loop
      $stmt_ins=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TAGS." ( id , rev_id , name , value , user_id ) VALUES (NULL, :rev_id, :tag_name, :tag_value, :user_id)");
      $stmt_ins->bindParam('rev_id',$new_rev_id,PDO::PARAM_INT);

    // check each old tag
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, value, user_id FROM ".ROSCMST_TAGS." WHERE rev_id = :old_rev_id");
    $stmt->bindParam('old_rev_id',$old_rev_id, PDO::PARAM_INT);
    $success = $stmt->execute();
    while ($old = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $tag_id = Tag::getId($new_rev_id, $old['name'], $old['user_id']);

      // check if tag already exists
      if ($tag_id == false) { // could be 0

        // tag doesn't exist -> insert new
        $stmt_ins->bindParam('tag_name',$old['name'],PDO::PARAM_STR);
        $stmt_ins->bindParam('tag_value',$old['value'],PDO::PARAM_STR);
        $stmt_ins->bindParam('user_id',$old['user_id'],PDO::PARAM_INT);
        $success = $success && $stmt_ins->execute();
      }
      else {

        // update old tag with new value
        $success = $success && Tag::update($tag_id, $old['value']);
      }
    }

    return ;
  } // end of member function copyFromRevision



  /**
   * returns tag value 
   *
   * @param int rev_id 
   * @param string tag_name 
   * @param int user_id 
   * @return string
   * @access public
   */
  public static function getValue( $rev_id, $tag_name, $user_id )
  {
    // need, tag name id
    $stmt=&DBConnection::getInstance()->prepare("SELECT value FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id AND name = :tag_name AND user_id = :user_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('tag_name',$tag_name,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    if ($stmt->execute()) {
      return $stmt->fetchColumn();
    }
    return false;
  } // end of member function getValue



  /**
   * returns revision id
   *
   * @param int tag_id 
   * @return int
   * @access public
   */
  public static function getRevision( $tag_id )
  {
    // need, tag name id
    $stmt=&DBConnection::getInstance()->prepare("SELECT rev_id FROM ".ROSCMST_TAGS." WHERE id = :tag_id");
    $stmt->bindParam('tag_id',$tag_id,PDO::PARAM_INT);
    if ($stmt->execute()) {
      return (int)$stmt->fetchColumn();
    }
    return false;
  } // end of member function getRevision



} // end of Tag
?>
