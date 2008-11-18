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
 * class CMSWebsiteSaveEntry
 * 
 */
class CMSWebsiteSaveEntry
{


  public function __construct()
  {
    require('login.php');
  
    if (!isset($_GET['d_id']) || !isset($_GET['d_r_lang'])){
      echo 'Missing params';
      return;
    }
  
    $this->save();
  }


  /**
   * Saves a entry as draft, is also responsible for autosaving drafts
   *
   * @param int _GET['d_id'] data_id
   * @param string _GET['d_r_lang'] lang
   * @param string _GET['d_val3'] type may be 'draft' or 'submit'
   * @param string tag_value dynamic number
   * @return 
   * @access private
   */
  private function save( $tag_value = 'no' )
  {
    global $roscms_intern_account_id;

    $type = (isset($_GET['d_val3']) ? $_GET['d_val3'] : '');
    $tag_value = (isset($_GET['d_val4']) ? $_GET['d_val4'] : 'no');

    $rev_id = 0; // helper var, contains current rev_id in force

    // detect if theres already a autosave-draft saved, and get rev_id
    if ($type == 'draft') { // draft
      if ($tag_value != 'no') {
        $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id FROM data_tag a JOIN data_revision r ON (r.data_id = a.data_id AND r.rev_id = a.data_rev_id) JOIN data_tag_name n ON a.tag_name_id = n.tn_id JOIN data_tag_value v ON a.tag_value_id  = v.tv_id WHERE r.data_id = :data_id AND r.rev_usrid = :user_id AND r.rev_date = :date AND r.rev_language = :lang AND r.rev_id = a.data_rev_id AND a.tag_usrid = '-1' AND n.tn_name = 'number' AND v.tv_value = :tag_value ORDER BY r.rev_id DESC LIMIT 1");
        $stmt->bindParam('tag_value',$tag_value,PDO::PARAM_STR);
      }
      else {
        $stmt=DBConnection::getInstance()->prepare("SELECT rev_id FROM data_revision WHERE data_id = :data_id AND rev_usrid = :user_id AND rev_date = :date AND rev_language = :lang ORDER BY rev_id DESC LIMIT 1");
      }
      $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
      $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
      $stmt->bindValue('date',date("Y-m-d"),PDO::PARAM_STR);
      $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_STR);
      $stmt->execute();
      $draft_candidate = $stmt->fetchColumn();

      // detect if the most recent rev_id, which is tagged as draft
      if (Tag::getValueByUser($_GET['d_id'], $draft_candidate, 'status', -1) == 'draft') {
        $rev_id = $draft_candidate;
      }
    }

    // add new draft, if no autosave-draft exists or draft is submitted
    if ($type  =='submit' || ($type  == 'draft' && $rev_id == 0)) {

      // insert revision itself
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) VALUES ( NULL, :data_id, 0, :lang, :user_id, NOW(), CURDATE(), CURTIME() )");
      $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
      $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_STR);
      $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
      $stmt->execute();

      // get inserted rev_id
      $stmt=DBConnection::getInstance()->prepare("SELECT rev_id FROM data_revision WHERE data_id = :data_id AND rev_version = 0 AND rev_language = :lang AND rev_usrid = :user_id ORDER BY rev_datetime DESC;");
      $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
      $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_STR);
      $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
      $stmt->execute();
      $rev_id = $stmt->fetchColumn();

      // tag the revision as new or draft
      if ($type  == 'submit') {
        Tag::add($_GET['d_id'], $rev_id, 'status', 'new', -1);
      }
      else if ($type  == 'draft') {
        Tag::add($_GET['d_id'], $rev_id, 'status', 'draft', -1);
      }

      if ($tag_value != "no") {
        Tag::add($_GET['d_id'], $rev_id, 'number', $tag_value, -1);
      }
    }
    else if ($rev_id != 0 && $type  == 'draft') {
      // Update
      // first delete, then insert new (outside from this scope)

      // del short
      $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_stext WHERE data_rev_id = :rev_id");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();

      // del long
      $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_text WHERE data_rev_id = :rev_id");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();

    }
    else {

      // fail
      echo 'not enough data ...';
    }
    

    // insert/update the (new) data
    if ($type == 'draft' || $type == 'submit') {

      // insert short text
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) VALUES ( NULL, :rev_id, :name, :content)");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      for ($i=1; $i <= $_POST['stextsum']; $i++) {  
        $stmt->bindParam('name',$_POST['pdstext'.$i],PDO::PARAM_STR);
        $stmt->bindParam('content',$_POST['pstext'.$i],PDO::PARAM_STR);
        $stmt->execute();
      }

      // insert long text
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) VALUES ( NULL, :rev_id, :name, :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      for ($i=1; $i <= $_POST['plmsum']; $i++) { // text
        $stmt->bindParam('name',$_POST['pdtext'.$i],PDO::PARAM_STR);
        $stmt->bindParam('content',$_POST['plm'.$i],PDO::PARAM_STR);
        $stmt->execute();
      }
    }
  }


} // end of CMSWebsiteSaveEntry
?>
