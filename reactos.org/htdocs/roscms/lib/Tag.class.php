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

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   * 
   *
   * @param long data_id

   * @param long rev_id 

   * @param long user_id 

   * @param string tag_name 

   * @return tag_id
   * @access public
   */
  public static function getIdByUser( $data_id, $rev_id, $tag_name, $user_id )
  {
    global $h_a;

    // sql stuff
    $stmt=DBConnection::getInstance()->prepare("SELECT t.tag_id FROM data_tag_name".$h_a." n JOIN data_tag".$h_a." t ON n.tn_id=t.tag_name_id WHERE t.data_id = :data_id AND t.data_rev_id = :rev_id AND n.tn_name = :tag_name AND tag_usrid = :user_id LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('tag_name',$tag_name,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute();

    return $stmt->fetchColumn();
  } // end of member function getIdByName


  /**
   * wrapper for deleteById
   *
   * @access public
   */
  public static function deleteByName( $data_id, $rev_id, $tag_name, $user_id )
  {
    $tag_id = self::getIdByUser($data_id, $rev_id, $tag_name, $user_id);
    return self::deleteById($tag_id, $user_id);
  } // end of member function getIdByName


  /**
   * deletes tag, and name+value if they're only used once by this tag
   *
   * @access public
   */
  public static function deleteById( $tag_id, $user_id )
  {
    global $h_a;

    // @unimplemented: account group membership check
    if ($user_id == ThisUser::getInstance()->id() || $user_id == 0 || $user_id == -1) {

      // get tag data
      $stmt=DBConnection::getInstance()->prepare("SELECT tag_name_id, tag_value_id FROM data_tag".$h_a." WHERE tag_id = :tag_id LIMIT 1");
      $stmt->bindParam('tag_id',$tag_id,PDO::PARAM_INT);
      $stmt->execute();
      $tag = $stmt->fetchOnce(PDO::FETCH_ASSOC);

      // delete tag name if, it's used only once
      $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(tag_name_id) FROM data_tag".$h_a." WHERE tag_name_id = :tag_name_id");
      $stmt->bindParam('tag_name_id',$tag['tag_name_id'],PDO::PARAM_INT);
      $stmt->execute();
      if ($stmt->fetchColumn() == 1) {
        $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_tag_name".$h_a." WHERE tn_id = :tag_name_id LIMIT 1");
        $stmt->bindParam('tag_name_id',$tag['tag_name_id'],PDO::PARAM_INT);
        $stmt->execute();
      }

      // delete tag value, if it's used only once
      $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(tag_value_id) FROM data_tag".$h_a." WHERE tag_value_id = :tag_value_id");
      $stmt->bindParam('tag_value_id',$tag['tag_value_id'],PDO::PARAM_INT);
      $stmt->execute();
      if ($stmt->fetchColumn() == 1) {
        $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_tag_value".$h_a." WHERE tv_id = :tag_value_id LIMIT 1");
        $stmt->bindParam('tag_value_id',$tag['tag_value_id']);
        $stmt->execute();
      }

      // finally delete tag
      $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_tag".$h_a." WHERE tag_id = :tag_id LIMIT 1");
      $stmt->bindParam('tag_id',$tag_id);
      return $stmt->execute();
    }
  } // end of member function getIdByName


  /**
   * 
   *
   * @param long data_id 

   * @param long rev_id 

   * @param long user_id 

   * @param string tag_name 

   * @return tag value
   * @access public
   */
  public static function add( $data_id, $rev_id, $tag_name, $tag_value, $user_id )
  {
    global $h_a;

    //@ADD group membership check
    if ($user_id != ThisUser::getInstance()->id() && $user_id != 0 && $user_id != -1) {
      die('ERROR: no rights to access this function');
    }

    // try to get tag name id, otherwise add new tag name
    $stmt=DBConnection::getInstance()->prepare("SELECT tn_id FROM data_tag_name".$h_a." WHERE tn_name = :name LIMIT 1");
    $stmt->bindParam('name',$tag_name,PDO::PARAM_STR);
    $stmt->execute();
    $tag_name_id = $stmt->fetchColumn();
    if ($tag_name_id === false) {

      // add tag name
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_tag_name ( tn_id , tn_name ) VALUES ( NULL , :name )");
      $stmt->bindParam('name',$tag_name,PDO::PARAM_STR);
      $stmt->execute();

      // try to get new tag name id
      $stmt=DBConnection::getInstance()->prepare("SELECT tn_id FROM data_tag_name".$h_a." WHERE tn_name = :name LIMIT 1");
      $stmt->bindParam('name',$tag_name,PDO::PARAM_STR);
      $stmt->execute();
      $tag_name_id = $stmt->fetchColumn();
      if ($tag_name_id === false) {
        die('ERROR: couldn\'t add tag_name');
      }
    }

    // try to get tag value id, otherwise add new tag value
    $stmt=DBConnection::getInstance()->prepare("SELECT tv_id FROM data_tag_value".$h_a." WHERE tv_value = :value LIMIT 1");
    $stmt->bindParam('value',$tag_value,PDO::PARAM_STR);
    $stmt->execute();
    $tag_value_id = $stmt->fetchColumn();
    if ($tag_value_id === false) {

      // add tag value
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_tag_value".$h_a." ( tv_id , tv_value ) VALUES ( NULL , :value)");
      $stmt->bindParam('value',$tag_value,PDO::PARAM_STR);
      $stmt->execute();

      // try to get new tag value id
      $stmt=DBConnection::getInstance()->prepare("SELECT tv_id FROM data_tag_value".$h_a." WHERE tv_value = :value LIMIT 1");
      $stmt->bindParam('value',$tag_value,PDO::PARAM_STR);
      $stmt->execute();
      $tag_value_id = $stmt->fetchColumn();
      if ($tag_value_id === false) {
        die('ERROR:  couldn\'t add tag value');
      }
    }

    // tag already exists ?
    $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM data_tag".$h_a." WHERE data_id = :data_id AND data_rev_id = :rev_id AND tag_name_id = :tag_name_id AND tag_value_id = :tag_value_id AND tag_usrid = :user_id");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('tag_name_id',$tag_name_id,PDO::PARAM_INT);
    $stmt->bindParam('tag_value_id',$tag_value_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute();
    $tag_exists = $stmt->fetchColumn();

    // add new tag if necessary
    if ($tag_exists <= 0) {
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_tag".$h_a." ( tag_id , data_id , data_rev_id , tag_name_id , tag_value_id , tag_usrid ) VALUES (NULL, :data_id, :rev_id, :tag_name_id, :tag_value_id, :user_id)");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindParam('tag_name_id',$tag_name_id,PDO::PARAM_INT);
      $stmt->bindParam('tag_value_id',$tag_value_id,PDO::PARAM_INT);
      $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
      $stmt->execute();
    }
  } // end of member function getIdByName


  /**
   * copies tags from old-data to new-data and inserts tags to archive if they don't exist anymore, and archive mode is on
   *
   * @param int old_data_id 
   * @param int old_rev_id 
   * @param int new_data_id 
   * @param int new_rev_id 
   * @param bool archive 
   * @access public
   */
  public static function copyFromData( $old_data_id, $old_rev_id, $new_data_id, $new_rev_id, $archive )
  {
    //
    if ($archive == true) {
      $h_a = '_a';
    }
    else {
      $h_a = '';
    }

    // prepare statements for while loop
      // get tag_name, tag_name_id
      $stmt_tn=DBConnection::getInstance()->prepare("SELECT tn_name FROM data_tag_name WHERE tn_id = :tag_name_id LIMIT 1");
      $stmt_tn_id=DBConnection::getInstance()->prepare("SELECT tn_id FROM data_tag_name".$h_a." WHERE tn_name = :name LIMIT 1");
      // get tag_value, tag_value_id
      $stmt_tv=DBConnection::getInstance()->prepare("SELECT tv_value FROM data_tag_value WHERE tv_id = :tag_value_id LIMIT 1");
      $stmt_tv_id=DBConnection::getInstance()->prepare("SELECT tv_id FROM data_tag_value".$h_a." WHERE tv_value = :value LIMIT 1");
      // insert tag, tag_name, tag_value
      $stmt_ins_tn=DBConnection::getInstance()->prepare("INSERT INTO data_tag_name".$h_a." ( tn_id , tn_name ) VALUES ( NULL, :name )");
      $stmt_ins_tv=DBConnection::getInstance()->prepare("INSERT INTO data_tag_value".$h_a." ( tv_id , tv_value ) VALUES ( NULL, :value )");
      $stmt_ins_tag=DBConnection::getInstance()->prepare("INSERT INTO data_tag".$h_a." ( tag_id , data_id , data_rev_id , tag_name_id , tag_value_id , tag_usrid ) VALUES ( NULL, :data_id, :rev_id, :tag_name, :tag_value, :user_id)");
      $stmt_ins_tag->bindParam('data_id',$new_data_id);
      $stmt_ins_tag->bindParam('rev_id',$new_rev_id);

    // foreach tag
    $stmt=DBConnection::getInstance()->prepare("SELECT tag_name_id, tag_value_id, tag_usrid FROM data_tag WHERE data_id = :data_id AND data_rev_id = :rev_id");
    $stmt->bindParam('data_id',$old_data_id,PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$old_rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while($tag = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // get tag_name
      $stmt_tn->bindParam('tag_name_id',$tag['tag_name_id'],PDO::PARAM_INT);
      $stmt_tn->execute();
      $tag_name = $stmt_tn->fetchColumn();

      // get requested tag_name_id (considering archive mode)
      $stmt_tn_id->bindParam('name',$tag_name,PDO::PARAM_STR);
      $stmt_tn_id->execute();
      $tag_name_id = $stmt_tn_id->fetchColumn();

      // insert new tag_name, if necessary
      if ($tag_name_id === false) {
        $stmt_ins_tn->bindParam('name',$tag_name,PDO::PARAM_STR);
        $stmt_ins_tn->execute();

        // and get the new tag_name_id
        $stmt_tn_id->execute();
        $tag_name_id = $stmt_tn_id->fetchColumn();
      }

      // get tag_value
      $stmt_tv->bindParam('tag_value_id',$tag['tag_value_id'],PDO::PARAM_INT);
      $stmt_tv->execute();
      $tag_value = $stmt_tv->fetchColumn();

      // get requested tag_value_id (considering archive mode)
      $stmt_tv_id->bindParam('value',$tag_value,PDO::PARAM_STR);
      $stmt_tv_id->execute();
      $tag_value_id = $stmt_tv_id->fetchColumn();

      // insert new tag_value
      if ($tag_value_id === false) {
        $stmt_ins_tv->bindParam('value',$tag_value,PDO::PARAM_STR);
        $stmt_ins_tv->execute();

        // and get the new tag_value_id
        $stmt_tv_id->execute();
        $tag_value_id = $stmt_tv_id->fetchColumn();
      }

      // insert tags to new data
      $stmt_ins_tag->bindParam('tag_name',$tag_name_id);
      $stmt_ins_tag->bindParam('tag_value',$tag_value_id);
      $stmt_ins_tag->bindParam('user_id',$tag['tag_usrid']);
      $stmt_ins_tag->execute();
    } // while

  } // end of member function getIdByName


  /**
   * 
   *
   * @param long data_id 

   * @param long rev_id 

   * @param long user_id 

   * @param string tag_name 

   * @return tag value
   * @access public
   */
  public static function getValueByUser( $data_id, $rev_id, $tag_name, $user_id )
  {
    global $h_a;

    // need, tag name id
    $stmt=DBConnection::getInstance()->prepare("SELECT v.tv_value FROM data_tag_name".$h_a." n JOIN data_tag".$h_a." t ON n.tn_id=t.tag_name_id JOIN data_tag_value".$h_a." v ON v.tv_id=t.tag_value_id WHERE data_id = :data_id AND data_rev_id = :rev_id AND tn_name = :tag_name AND tag_usrid = :user_id LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('tag_name',$tag_name,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute();

    return $stmt->fetchColumn();
  } // end of member function getValueByName


  /**
   * 
   *
   * @param long data_id 

   * @param long rev_id 

   * @param string tag_name 

   * @return 
   * @access public
   */
  public static function getValue( $data_id, $rev_id, $tag_name )
  {
    global $h_a;

    $stmt=DBConnection::getInstance()->prepare("SELECT v.tv_value FROM data_tag_name".$h_a." n JOIN data_tag".$h_a." t ON  n.tn_id=t.tag_name_id JOIN data_tag_value".$h_a." v ON v.tv_id=t.tag_value_id WHERE t.data_id = :data_id AND t.data_rev_id = :rev_id AND n.tn_name = :tag_name LIMIT 1");
    $stmt->bindParam('data_id',$data_id);
    $stmt->bindParam('rev_id',$rev_id);
    $stmt->bindParam('tag_name',$tag_name);
    $stmt->execute();

    return $stmt->fetchColumn();
  } // end of member function get_tag


} // end of Tag
?>
