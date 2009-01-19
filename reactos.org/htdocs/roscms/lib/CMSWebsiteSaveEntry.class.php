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
    Login::required();

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
  private function save( )
  {
    $thisuser = &ThisUser::getInstance();

    $type = (isset($_GET['d_val3']) ? $_GET['d_val3'] : '');
    $rev_id = 0; // helper var, contains current rev_id in force

    // detect if theres already a autosave-draft saved, and get rev_id
    if ($type == 'draft') { // draft
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND user_id = :user_id AND lang_id = :lang AND archive IS FALSE ORDER BY id DESC LIMIT 1");
      $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_INT);
      $stmt->execute();
      $draft_candidate = $stmt->fetchColumn();

      // detect if the most recent rev_id, which is tagged as draft
      if (Tag::getValueByUser($draft_candidate, 'status', -1) == 'draft') {
        $rev_id = $draft_candidate;
      }
    }

    // add new draft, if no autosave-draft exists or draft is submitted
    if ($type  =='submit' || ($type  == 'draft' && $rev_id == 0)) {

      // insert revision itself
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_REVISIONS." ( id , data_id , version , lang_id , user_id , datetime ) VALUES ( NULL, :data_id, 0, :lang, :user_id, NOW())");
      $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
      $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->execute();

      // get inserted rev_id
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version = 0 AND lang_id = :lang AND user_id = :user_id AND archive IS FALSE ORDER BY datetime DESC;");
      $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
      $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->execute();
      $rev_id = $stmt->fetchColumn();

      // get stable entry
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_TAGS." t JOIN ".ROSCMST_REVISIONS." r ON r.id = t.rev_id WHERE r.data_id = :data_id AND r.lang_id = :lang AND t.user_id = -1 AND t.name = 'status' AND t.value = 'stable' AND r.archive IS FALSE ORDER BY r.id DESC LIMIT 1");
      $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
      $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_STR);
      $stmt->execute();
      $stable = $stmt->fetchColumn();
      if ($stable !== false) {

        // transfer from stable entry
        Tag::copyFromData($stable, $rev_id);
      }

      // tag the revision as new or draft
      if ($type  == 'submit') {
        Tag::update(Tag::getIdByUser($rev_id, 'status', -1),'new');
      }
      else {
        Tag::update(Tag::getIdByUser($rev_id, 'status', -1),'draft');
      }
    }
    elseif ($rev_id != 0 && $type  == 'draft') {
      // Update
      // first delete, then insert new (outside from this scope)

      // del short
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();

      // del long
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id");
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
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, :name, :content)");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      for ($i=1; $i <= $_POST['stextsum']; $i++) {  
        $stmt->bindParam('name',$_POST['pdstext'.$i],PDO::PARAM_STR);
        $stmt->bindParam('content',$_POST['pstext'.$i],PDO::PARAM_STR);
        $stmt->execute();
      }

      // insert long text
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, :name, :content )");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      for ($i=1; $i <= $_POST['plmsum']; $i++) { // text
      Log::writeMedium($_POST['pdtext'.$i]);
        $stmt->bindParam('name',$_POST['pdtext'.$i],PDO::PARAM_STR);
        $stmt->bindParam('content',$_POST['plm'.$i],PDO::PARAM_STR);
        $stmt->execute();
      }
    }
  }


} // end of CMSWebsiteSaveEntry
?>
