<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007      Klemens Friedl <frik85@reactos.org>
                  2008-2009 Danny Götte <dangerground@web.de>

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
 * class Backend_EntryTable
 * 
 * @package Branch_Website
 * @subpackage Backend
 */
class Backend_EntryTable extends Backend
{



  /**
   * constructor
   *
   * @access public
   */
  public function __construct( )
  {
    // prevent caching, set content header for text
    parent::__construct();

    // update one tag
    if ($_GET['d_fl'] == 'setbookmark') {
      echo $this->setBookmark();
    }

    // update a list (if there a entries selected)
    elseif ($_GET['count'] > 0) {
      $this->evalAction();
    }
  } // end of constructor



  /**
   * goes through a list of revisions and performs the action on that revision
   *
   * @access private
   */
  private function evalAction( )
  {
    $thisuser = &ThisUser::getInstance();

    // get better id format
    $id_list = preg_replace('/(^|-)[0-9]+\_([0-9]+)/','$2|',$_GET['d_val2']);
    $rev_id_list = explode('|',substr($id_list,0,-1));

    // build a list with rev_ids
    $id_list = null;
    foreach ($rev_id_list as $rev_id) {
      if ($id_list !== null) {
        $id_list .= ',';
      }
      $id_list .= $rev_id;
    }

    // go through all selected revisions
    $stmt=&DBConnection::getInstance()->prepare("SELECT lang_id, version, data_id, id, user_id, status FROM ".ROSCMST_REVISIONS." WHERE id IN(".$id_list.")");
    $stmt->execute();
    while ($revision = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // eval action
      switch ($_GET['d_val3']) {

        // mark as stable
        case 'ms':
          $this->markStable($revision);
          break;

        // mark as new
        case 'mn':
          $this->markNew($revision);
          break;

        // add star
        case 'as':
          // we need to delete, as we can't update, because we don't know if the entry already exists
          Tag::deleteByName($revision['id'], 'star', $thisuser->id());
          Tag::add($revision['id'], 'star', 'on', $thisuser->id());
          break;

        // delete star
        case 'xs':
          Tag::deleteByName($revision['id'], 'star', $thisuser->id());
          break;

        // add label
        case 'tg':
          Tag::add($revision['id'], 'tag', $_GET['d_val4'], $thisuser->id());
          break;

        // delete entry
        case 'xe':
          $this->deleteEntry($revision);
          break;

        // move to archiv
        case 'va':
          Revision::toArchive($revision['id']);
          Revision::deleteFile($revision['id']);
          break;
      } // end switch
    } // end while
  } // end of member function evalAction



  /**
   * updates a revision (as we don't know if it already exists we do delete -> add) and return the tag id
   *
   * @return int
   * @access private
   */
  private function setBookmark( )
  {
    Tag::deleteByName($_GET['rev'], 'star', ThisUser::getInstance()->id() );
    Tag::add($_GET['rev'], 'star' , $_GET['tag_value'], ThisUser::getInstance()->id());
  }



  /**
   * delete a revision (and file) or moves to archive
   *
   * @param int rev_id
   * @return bool
   * @access private
   */
  private function deleteEntry( $revision )
  {
    $thisuser = &ThisUser::getInstance();

    // able to delete this entry ?
    if (!$thisuser->hasAccess('del_entry') && ($revision['user_id'] != $thisuser->id() || $revision['version']>0)) {
      echo 'Not enough rights for delete process.';
      return false;
    }

    // delete own entries ?
    if ($revision['user_id'] == $thisuser->id() && $revision['version'] == 0 ) {
      return Revision::delete($revision['id']);
    }
    
    // access to this entry ?
    if (!$thisuser->hasAccess('more_lang') && $revision['lang_id'] != $thisuser->language()) {
      echo 'You have no rights to delete entries from other languages';
      return false;
    }

    // entries need to be backuped
    if (!$thisuser->hasAccess('del_wo_archiv')) {
      return Revision::toArchive($revision['id']);
    }

    // delete everything
    else {
      //Data::deleteFile($revision['id']);
      return Revision::delete($revision['id']);
    }

  } // end of member function deleteEntry



  /**
   * set a revision as new
   *
   * @param int rev_id
   * @return bool
   * @access private
   */
  private function markNew( $revision )
  {
    // check if user has access to update other languages
    if (!ThisUser::getInstance()->hasAccess('more_lang')) {

      // check if user has set a language
      if (ThisUser::getInstance()->language() == 0) {
        die('Set a valid language in your Account settings!');
      }

      // block actions for user with access to one language
      elseif (ThisUser::getInstance()->language() != $revision['lang_id']) {
        echo 'You can\'t mark entries of other languages as new!';
        return false;
      }
    }

    // mark revision as new
    Revision::setStatus($revision['id'], 'new');

    // as new entries don't have a version, we set it explicit to 0
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET version = 0 WHERE id = :rev_id");
    $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
    $stmt->execute();
  } // end of member function markNew



  /**
   * set a new stable head revision
   *
   * @param int rev_id
   * @return bool
   * @access private
   */
  private function markStable( $revision )
  {
    // check if entry is not already stable
    if ($revision['status'] === 'stable') {
      echo 'Entry is already stable';
      return false;
    }

    // has user access to modify other languages?
    if (!ThisUser::getInstance()->hasAccess('more_lang')) {

      // check if user has set a language
      if (ThisUser::getInstance()->language() == 0) {
        die('Set a valid language in your account settings!');
      }

      // block actions for user with access to one language
      elseif (ThisUser::getInstance()->language() != $revision['lang_id']) {
        echo 'You can\'t mark entries of other languages as stable!';
        return false;
      }
    }

    // get next rev version number (also search archive)
    $stmt=&DBConnection::getInstance()->prepare("SELECT version FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang ORDER BY version DESC, id DESC LIMIT 1");
    $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
    $stmt->bindParam('lang',$revision['lang_id'],PDO::PARAM_INT);
    $stmt->execute();
    $version_num = $stmt->fetchColumn()+1;

    // get latest stable head entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, data_id, lang_id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang AND archive IS FALSE AND status='stable' ORDER BY version DESC, id DESC LIMIT 1");
    $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
    $stmt->bindParam('lang',$revision['lang_id'],PDO::PARAM_INT);
    $stmt->execute();
    $stable_revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // copy tags from stable and move old stable to archive
    if ($stable_revision !== false) {

      // transfer 
      Tag::copyFromRevision($stable_revision['id'], $revision['id']);

      // move old revision to archive
      if (!Revision::toArchive($stable_revision['id'])) {
        Log::writeMedium('Data::copy() failed: data-id '.$stable_revision['data_id'].', rev-id '.$stable_revision['id']);
        echo 'Process not successful :S';
        return false;
      }
    }

    // update the version number
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_REVISIONS." SET version = :version WHERE id = :rev_id");
    $stmt->bindParam('version',$version_num,PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
    $stmt->execute();

    // update depencies for new rev
    $depency = new Depencies();
    if (!$depency->addRevision($revision['id'])) {
      echo 'Error while updating depencies';
      return false;
    }

    // make entry stable
    Revision::setStatus($revision['id'],'stable');

    // generate content
    $generate = new Generate();
    if (!$generate->update($revision['id'])) {
      Revision::setStatus($revision['id'],$revision['status']);
      echo 'Can\'t generate updated entry.';
      return false;
    }

    Log::writeLow('mark entry as stable: data-id '.$revision['data_id'].', rev-id '.$revision['id']);

    return true;
  } // end of member function markStable



} // end of Backend_EntryTable
?>
