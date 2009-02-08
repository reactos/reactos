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
 * class Backend_SaveDraft
 * 
 * @package Branch_Website
 * @subpackage Backend
 */
class Backend_SaveDraft extends Backend
{


  /**
   * check data & language id to prevent problems
   *
   * @access public
   */
  public function __construct()
  {
    // login, prevent caching
    parent::__construct();

    if (!isset($_GET['data_id']) || !isset($_GET['lang_id'])){
      echo 'Missing params';
      return;
    }
  
    $this->save();
  } // end of constructor


  /**
   * Saves a entry as draft.
   * Function is also responsible for autosaving drafts.
   *
   * @access private
   */
  private function save( )
  {
    $thisuser = &ThisUser::getInstance();
    $rev_id = 0; // helper var, contains current rev_id in force
    
    if (!$thisuser->hasAccess('more_lang') && $_GET['lang_id'] != $thisuser->language()) {
      die ('Can\'t save drafts of other than your language, due to access restrictions');
    }

    // detect if theres already a autosave-draft saved, and get rev_id
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND user_id = :user_id AND lang_id = :lang AND archive IS FALSE AND status = 'draft' ORDER BY id DESC LIMIT 1");
    $stmt->bindParam('data_id',$_GET['data_id'],PDO::PARAM_INT);
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->bindParam('lang',$_GET['lang_id'],PDO::PARAM_INT);
    $stmt->execute();
    $draft_candidate = $stmt->fetchColumn();

    // if there is a valid value returned, use it as rev_id
    if ($draft_candidate !== false) {
      $rev_id = $draft_candidate;
    }

    // add new draft, if no autosave-draft exists or draft is submitted
    if ($rev_id === 0) {

      // add a new revision
      $rev_id = Revision::add($_GET['data_id'], $_GET['lang_id']);

      // get stable entry
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND lang_id = :lang_id AND status = 'stable' AND archive IS FALSE ORDER BY datetime DESC LIMIT 1");
      $stmt->bindParam('data_id',$_GET['data_id'],PDO::PARAM_INT);
      $stmt->bindParam('lang_id',$_GET['lang_id'],PDO::PARAM_STR);
      $stmt->execute();
      $stable = $stmt->fetchColumn();
      if ($stable !== false) {

        // transfer from stable entry
        Tag::copyFromRevision($stable, $rev_id);
      }
    }

    // delete old texts
    else {

      // del short
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();

      // del long
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->execute();
    }

    // insert/update short text
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_STEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, :name, :content)");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    for ($i=1; $i <= $_POST['stextsum']; ++$i) {  
      $stmt->bindParam('name',$_POST['pdstext'.$i],PDO::PARAM_STR);
      $stmt->bindParam('content',$_POST['pstext'.$i],PDO::PARAM_STR);
      $stmt->execute();
    }

    // insert/update long text
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_TEXT." ( id , rev_id , name , content ) VALUES ( NULL, :rev_id, :name, :content )");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    for ($i=1; $i <= $_POST['plmsum']; ++$i) {
      $stmt->bindParam('name',$_POST['pdtext'.$i],PDO::PARAM_STR);
      $stmt->bindParam('content',$_POST['plm'.$i],PDO::PARAM_STR);
      $stmt->execute();
    }
  } // end of member function save


} // end of Backend_SaveDraft
?>
