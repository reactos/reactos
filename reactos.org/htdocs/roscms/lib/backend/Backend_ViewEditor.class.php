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
 * class Backend_ViewEditor
 * 
 * @package Branch_Website
 * @subpackage Backend
 */
class Backend_ViewEditor extends Backend
{



  // Type of detail
  const METADATA = 0;
  const FIELDS   = 1;
  const HISTORY  = 2;
  const SECURITY = 3;
  const REVISION = 4;
  const DEPENCIES= 5;

  // 
  private $data_id;
  private $rev_id;



  /**
   * constructor
   *
   * @access public
   */
  public function __construct( $rev_id = null )
  {
    // prevent caching, set content header for text
    parent::__construct();
    
    if ($rev_id !== null) {
      $this->setRevision($rev_id);
    }

    $this->evalAction();
  } // end of constructor



  /**
   *
   *
   * @param string action action which has to be performed
   * @access protected
   */
  private function setRevision( $rev_id ) {
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, data_id FROM ".ROSCMST_REVISIONS." WHERE id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    if ($revision !== false) {
      $this->rev_id = $revision['id'];
      $this->data_id = $revision['data_id'];
    }
  } // end of member function setRevision



  /**
   * selects the action which was requested
   *
   * @access protected
   */
  protected function evalAction( )
  {
    $thisuser = &ThisUser::getInstance();

    // set attribute for rev_id & data_id
    if (isset($_GET['d_r_id'])) {
      $this->setRevision($_GET['d_r_id']);
    }

    switch (@$_GET['d_fl']) {

      // show Metadata details
      case 'showtag':
        $this->showEntryDetails(self::METADATA);
        break;

      // show History details
      case 'showhistory':
        $this->showEntryDetails(self::HISTORY);
        break;

      // show Depencies
      case 'showdepencies':
        $this->showEntryDetails(self::DEPENCIES);
        break;

      // add Depencies
      case 'adddepency':
        if (Depencies::addManual($_GET['rev_id'],$_GET['dep_name'],$_GET['dep_type'])) {
          echo 'Adding user defined depency was successful.';
        }
        else {
          echo 'Error while adding user defined depency.';
        }
        break;

      // delete Depencies
      case 'deletedepency':
        if (Depencies::deleteManual($_GET['dep_id'])) {
          echo 'Deleting user defined depency was successful.';
        }
        else {
          echo 'Error while deleting user defined depency.';
        }
        break;

      // show Field details
      case 'alterfields':
        $this->showEntryDetails(self::FIELDS);
        break;

      // update Field details
      case 'alterfields2':
        Revision::updateText($_GET['d_r_id'], $_GET['d_val'], $_GET['d_val2']);
        $this->show();
        break;

      // show revision details
      case 'showentry':
        $this->showEntryDetails(self::REVISION);
        break;

      // update revision details
      case 'alterentry':
        Revision::update($_GET['d_r_id'],$_GET['d_val'],$_GET['d_val3'],$_GET['d_val4'],htmlspecialchars(@$_GET["d_val5"]));
        $this->show();
        break;

      // show security details
      case 'showsecurity':
        $this->showEntryDetails(self::SECURITY);
        break;

      // update Security details
      case 'altersecurity':
        Entry::update($_GET['d_id'], $_GET['d_val'], $_GET['d_val2'], $_GET['d_val3'], $_GET['d_val4']=='true');
        $this->show();
        break;

      // add new tag
      case 'addtag':
        Tag::add($_GET['d_r_id'], $_GET['d_val'], $_GET['d_val2'], $_GET['d_val3']);
        $this->showEntryDetails(self::METADATA);
        break;

      // delete tag
      case 'deltag':
        $this->setRevision(Tag::getRevision($_GET['d_val']));
        Tag::deleteById($_GET['d_val']);

        // reload Metadata
        $this->showEntryDetails(self::METADATA);
        break;

      // compare two entries
      //@FIXME broken logic, as both diff actions do the same
      case 'diff':
      // compare two entries; updates diff area
      case 'diff2':
        echo $this->showDifference($_GET['d_val'] /* rev_id entry 1 */, $_GET['d_val2'] /* rev_id entry 2 */);
        break;

      // Default
      default:

        // normal (contains NO "tr")
        if (!isset($_GET['d_r_id']) || strpos($_GET['d_r_id'], 'tr') === false) {
          $this->show();
        }

        // translation mode (contains "tr")
        else {
          $stmt=&DBConnection::getInstance()->prepare("SELECT id, data_id FROM ".ROSCMST_REVISIONS." WHERE id = :rev_id LIMIT 1");
          $stmt->bindValue('rev_id',substr($_GET['d_r_id'], 2),PDO::PARAM_INT); // remove 'tr' from rev_id while query
          $stmt->execute();
          $revision = $stmt->fetchOnce();

          // check if user has translator access
          if (!Entry::hasAccess($revision['data_id'], 'translate')) {
            die('You have not enough rights to translate this entry.');
          }

          // copy existing entry to new language
          $rev_id = Revision::translate($revision['id'], $_GET['d_r_lang']);
          if ($rev_id !== false) {
            $this->setRevision($rev_id);
            $this->show();
          }
          else {
            die('Translation not successful, due entry-copy problem. If this happens more than once or twice please contact the website admin.');
          }
        }
        break;
    } // end switch
  } // end of member function evalAction



  /**
   * shows the editor + header + details
   *
   * @access private
   */
  private function show( )
  {
    $this->showEntryHeader();
    $this->showEntryDetails(self::METADATA);
    echo '</div></div>';  // close elements opened in showEntryData
    $this->showEditor();
  } // end of member function show



  /**
   * shows the editor itself (input fields, textareas, save button, compare button)
   *
   * @access private
   */
  private function showEditor( )
  {
    echo_strip('
      <div class="editor" style="background:white; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb;">
        <div style="margin:10px;">
          <div style="width:95%;">
            <form method="post" action="#">
              <br />');

    // show short texts
    $stext_num = 0;
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, content FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id ORDER BY name ASC");
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while($stext = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$stext_num;

      echo_strip('
        <label for="estext"'.$stext_num.'">'.$stext['name'].':</label>
        <span id="edstext'.$stext_num.'" style="display:none;">'.$stext['name'].'</span>
        <input name="estext"'.$stext_num.'" type="text" id="estext'.$stext_num.'" size="50" maxlength="250" value="');echo htmlspecialchars($stext['content']).'" /><br /><br />';
    }

    echo_strip('
      <span id="entrydataid" class="'.$this->data_id.'">&nbsp;</span>
      <span id="entrydatarevid" class="'.$this->rev_id.'">&nbsp;</span>
      <div id="estextcount" class="'.$stext_num.'">&nbsp;</div>');

    // show long texts
    $text_num = 0;
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, content FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id ORDER BY name ASC");
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while($text = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$text_num;

      echo_strip('
        <label for="elm'.$text_num.'" style="display: inline; margin-right: 20px;" id="textname'.$text_num.'">'.$text['name'].'</label>
        <button type="button" id="butRTE'.$text_num.'" onclick="'."toggleEditor('elm".$text_num."', this.id)".'">Rich Text</button>
        <span id="swraped'.$text_num.'"></span>
        <input id="wraped'.$text_num.'" type="checkbox" onclick="'."toggleWordWrap(this.id, 'elm".$text_num."');".'" checked="checked" style="padding-left: 10px;" />
        <label for="wraped'.$text_num.'" class="normal">Word wrap</label>
        <textarea name="elm'.$text_num.'" cols="80" rows="15" class="mceEditor" id="elm'.$text_num.'" style="width: 100%; background-color:#FFFFFF;" >');echo htmlspecialchars($text['content']);echo_strip('</textarea>
        <br />
        <br />');
    }

    echo '<span id="elmcount" class="'.$text_num.'">&nbsp;</span>';

    // show save button
    if (Entry::hasAccess($this->data_id, 'edit')) {
      echo_strip('
        <button type="button" id="bsavedraft" onclick="'."saveAsDraft(".$this->data_id.",".$this->rev_id.")".'">Save as Draft</button> &nbsp;
        <input name="editautosavemode" type="hidden" value="true" />');
    }
    else {
      echo_strip('
        <button type="button" id="bsavedraft" disabled="disabled">Save as Draft</button> &nbsp;
        <img src="'.RosCMS::getInstance()->pathRosCMS().'images/locked.gif" width="11" height="12" /> (not enough rights) &nbsp;
        <input name="editautosavemode" type="hidden" value="false" />');
    }

    // get count similiar revs
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(id) FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang AND id != :rev_id");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revisions_count = $stmt->fetchColumn();

    // No compare
    if ($revisions_count == 0) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_LANGUAGES." WHERE id=:lang_id");
      $stmt->bindParam('lang_id',Language::getStandardId(),PDO::PARAM_INT);
      $stmt->execute();
      $lang=$stmt->fetchColumn();

      echo_strip('
        <span id="bshowdiff" class="virtualButton" onclick="'."openOrCloseDiffArea(".$this->rev_id.",".$this->rev_id.")".'">
        <img id="bshowdiffi" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Compare</span> (no related '.$lang.' entry, choose yourself)&nbsp;');
    }

    // Compare button
    else {

      if (isset($_GET['d_arch']) && $_GET['d_arch']) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang AND archive IS TRUE ORDER BY datetime DESC LIMIT 2");
        $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->execute();
        $diff2 = $stmt->fetch();
        $diff2 = $diff2['id'];
        $diff1 = $stmt->fetchOnce();
        $diff1 = $diff1['id'];
      }
      else {
        $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang AND archive IS FALSE ORDER BY datetime DESC LIMIT 1");
        $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->execute();
        $diff2 = $stmt->fetchColumn();

        $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND version > 0 AND lang_id = :lang AND archive IS TRUE ORDER BY datetime DESC LIMIT 1");
        $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->execute();
        $diff1 = $stmt->fetchColumn();
      }

      echo_strip('
        <span id="bshowdiff" class="virtualButton" onclick="'."openOrCloseDiffArea('".$diff1."','".$diff2."')".'">
          <img id="bshowdiffi" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />
          &nbsp;Compare
        </span>');
    }

    echo_strip('
              </form>
            &nbsp;&nbsp;<span id="mefasi">&nbsp;</span>
          </div>
        </div>
      </div>
      <div id="frmdiff" style="display:none;"></div>');
  } // end of member function showEditor



  /**
   * show some basic entry details, like name, type, version, author
   *
   * @access private
   */
  private function showEntryHeader( )
  {
    $thisuser = &ThisUser::getInstance();

    // get Database Entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.id, d.name, d.type, r.version, r.lang_id,l.name AS language, r.datetime, u.name AS user_name, r.archive FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_USERS." u ON r.user_id = u.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id=l.id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$this->rev_id);
    $stmt->execute();
    $revision = $stmt->fetchOnce();

    echo_strip('
      <div style="padding-bottom: 3px;">
        <span class="revDetail">
          <span onclick="'."toggleBookmark(".$revision['id'].", ".$thisuser->id().", 'editstar')".'" style="cursor: pointer;">
           <img id="editstar" class="'.Tag::getId($revision['id'], 'star', $thisuser->id()).'" src="'.RosCMS::getInstance()->pathRosCMS().'images/star_'.Tag::getValue($revision['id'], 'star', $thisuser->id()).'_small.gif" alt="" style="width:13px; height:13px; border:0px;" alt="*" />
          </span>
          &nbsp;'.$revision['name'].'</span>
          &nbsp;
          <span style="display: none;" id="mefrrevid">'.$revision['id'].'</span>
          <span style="white-space: nowrap;">type: <span class="revDetail">'.$revision['type'].'</span></span> &nbsp; 
          <span style="white-space: nowrap;">version: <span id="mefrverid" class="revDetail">'.$revision['version'].'</span></span> &nbsp; 
          <span style="white-space: nowrap;">language: <span class="revDetail">'.$revision['language'].'</span><span id="mefrlang" style="display:none;">'.$revision['lang_id'].'</span></span> &nbsp; 
          <span style="white-space: nowrap;">user: <span id="mefrusrid" class="revDetail">'.$revision['user_name'].'</span></span> &nbsp; ');

    if ($revision['archive']) {
      echo_strip('
        <span style="white-space: nowrap;">mode: 
          <span id="mefrusrid" class="revDetail">archive</span>
        </span> &nbsp; ');
    }

    echo_strip('
        <span id="frmedittags" class="virtualButton" onclick="TabOpenClose(this.id)" style="white-space: nowrap;">
          <img id="frmedittagsi" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />
          &nbsp;Details
        </span>
      </div>
      <div id="frmedittagsc" class="edittagbody" style="display: none;">
        <div id="frmedittagsc2">');
  } // end of member function  showEntryData



  /**
   * shows available Details and unfolds the selected one
   *
   * @param int mode 0-5 but you should use the class constants
   * @access private
   */
  private function showEntryDetails( $mode = self::METADATA)
  {
    $thisuser = &ThisUser::getInstance();

    echo_strip('
      <div class="detailbody">
        <div class="detailmenubody">');

    // Metadata
    if ($mode == self::METADATA) {
      echo '<strong>Metadata</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."showEditorTabMetadata(".$this->rev_id.")".'">Metadata</span>';
    }

    // History
    if (Entry::hasAccess($this->data_id,'history')) {
      echo '&nbsp;|&nbsp;';

      if ($mode == self::HISTORY) {
        echo '<strong>History</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."showEditorTabHistory(".$this->rev_id.")".'">History</span>';
      }
    }

    // Depencies
    if (Entry::hasAccess($this->data_id,'depencies')) {
      echo '&nbsp;|&nbsp;';

      if ($mode == self::DEPENCIES) {
        echo '<strong>Depencies</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."showEditorTabDepencies(".$this->rev_id.")".'">Depencies</span>';
      }
    }

    // allowed only for someone with "add" rights
    if (Entry::hasAccess($this->data_id,'fields')) {
      echo '&nbsp;|&nbsp;';

      // Fields
      if ($mode == self::FIELDS) {
        echo '<strong>Fields</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."showEditorTabFields(".$this->rev_id.")".'">Fields</span>';
      }
    }

    // Revision Data
    if (Entry::hasAccess($this->data_id,'revision')) {
      echo '&nbsp;|&nbsp;';

      // Revision
      if ($mode == self::REVISION) {
        echo '<strong>Revision</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."showEditorTabRevisions(".$this->rev_id.")".'">Revision</span>';
      }
    }

    // allowed only for related super administrators
    if ($thisuser->hasAccess('entry_security')) { 
      echo '&nbsp;|&nbsp;';

      // Security
      if ($mode == self::SECURITY) {
        echo '<strong>Security</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."showEditorTabSecurity(".$this->rev_id.")".'">Security</span>';
      }
    }

    echo_strip('
        </div>
      </div>');

    // select detail type
    switch ($mode) {
      case self::METADATA:
        $this->showEntryDetailsMetadata();
        break;
      case self::HISTORY:
        $this->showEntryDetailsHistory();
        break;
      case self::DEPENCIES:
        $this->showEntryDepencies($this->data_id);
        break;
      case self::SECURITY:
        $this->showEntryDetailsEntry();
        break;
      case self::FIELDS:
        $this->showEntryDetailsFields();
        break;
      case self::REVISION:
        $this->showEntryDetailsRevision();
        break;
    } // end switch
  } // end of member function showEntryDetails



  /**
   * Interface for metadata
   *
   * @access private
   */
  private function showEntryDetailsMetadata( )
  {
    $thisuser = &ThisUser::getInstance();

    // helper vars
    $last_user = null; // used in while loop, to recognize the last type

    // are we able to view Metadata
    if ($thisuser->hasAccess('system_tags')) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, user_id, name, value FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id AND user_id IN(-1, :user_id) ORDER BY user_id ASC, name ASC");
    }

    // Display only privat labels
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, user_id, name, value FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id AND user_id =:user_id ORDER BY user_id ASC, name ASC");
    }
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();

    // List Tags
    while($tag = $stmt->fetch()) {
      if ($tag['user_id'] != $last_user) {

        // echo metadata type (metadata / label / private label)
        echo '<h3>'.(($tag['user_id'] == -1) ? 'System Metadata' : 'Private Labels').'</h3>';
      }

      // output name & current value
      echo '<strong>'.$tag['name'].':</strong>&nbsp;'.$tag['value'];

      // show delete button
        // allow to delete label if SecLev > 1
        // allow to delete sys metadata if user has the rights
        // allow someone to delete his metadata he set and the user-id > 0
      if (($thisuser->hasAccess('system_tags') && $tag['user_id'] == -1) || $tag['user_id'] == $thisuser->id()) {
        echo_strip('&nbsp;&nbsp;
          <span class="virtualButton" onclick="'."delLabelOrTag('".$tag['id']."')".'">
            <img src="'.RosCMS::getInstance()->pathRosCMS().'images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />
            &nbsp;Delete
          </span>');
      }

      echo '<br />';
      $last_user = $tag['user_id'];
    }

    // insert new private label
    echo_strip('
      <br />
      <h3>Add Private Label</h3>
      <label for="addtagn" class="normal">Tag:</label>&nbsp;
      <input type="text" id="addtagn" size="15" maxlength="100" value="" />&nbsp;
      <button type="button" onclick="'."addLabelOrTag(".$this->rev_id.",'tag','addtagn', '".$thisuser->id()."')".'">Add</button>
      <br />');

    // insert new metadata
    if ($thisuser->hasAccess('system_tags') && Entry::hasAccess($this->data_id, 'system_meta')) {
      echo_strip('
        <br />
        <h3>Add System Metadata</h3>
        <label for="addtags1" class="normal">Name:</label>&nbsp;
        <input type="text" id="addtags1" size="15" maxlength="100" value="" />&nbsp;
        <label for="addtags2" class="normal">Value:</label>&nbsp;
        <input type="text" id="addtags2" size="15" maxlength="100" value="" /> &nbsp;
        <button type="button" onclick="'."addLabelOrTag(".$this->rev_id.",'addtags1','addtags2',-1)".'">Add Sys</button>');
    }

    echo '<br /><br />';
  } // end of member function showEntryDetailsMetadata



  /**
   * Display list of older versions
   *
   * @access private
   */
  private function showEntryDetailsHistory( )
  {
    echo '<h3>Versions History</h3>';

    // get a perfect mixed entry set
    if (ThisUser::getInstance()->hasAccess('more_lang')) {
      $dataset = $this->helperHistory();
    }
    else {
      $dataset = $this->helperHistory(ThisUser::getInstance()->language());
    }

    // list them grouped by language
    $last_language = null;
    foreach ($dataset as $revision) {
      if ($revision['language'] != $last_language) {
        if ($last_language !== null) {
          echo '</ul>';
        }

        echo '<p style="font-weight:bold;">'.$revision['language'].'</p><ul>'; 
        $last_language = $revision['language'];
      }

      echo '<li'.($revision['id'] == $this->rev_id ? ' style="text-decoration:underline;"' : '').'>'.$revision['name'].' ('.$revision['datetime'].') - v. '.$revision['version'].'; '.$revision['user_name'].'</li>';
    }
    echo '</ul>';
  } // end of member function showEntryDetailsHistory



  /**
   * Interface for depenciess
   *
   * @access private
   */
  private function showEntryDepencies()
  {
  
    // add manual depency
    if (ThisUser::getInstance()->hasAccess('add_depencies')) {
      echo_strip('
        <h3>Add Depency</h3>
        <fieldset>
          <label for="dep_name">Name:</label> 
          <input type="text" name="dep_name" id="dep_name" /><br />
          
          <label for="dep_type">Type:</label> 
          <select name="dep_type" id ="dep_type">
            <option value="content">Content</option>
            <option value="script">Script</option>
            '.(ThisUser::getInstance()->hasAccess('dynamic_pages') ? '<option value="dynamic">Dynamic Page</option>' : '').'
          </select><br />
          <button type="submit" onclick="'."addDepency(".$this->rev_id.")".'">add manual depency</button>
        </fieldset>');
    }

    // print depency tree
    echo '<h3>Dependent Entries</h3>';
    $this->buildDepencyTree($this->data_id);

    // required articles that don't exist
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT child_name, include, id, user_defined FROM ".ROSCMST_DEPENCIES." WHERE rev_id=:rev_id AND child_id IS NULL ORDER BY include DESC, child_name ASC");
    $stmt->bindParam('rev_id',$this->rev_id, PDO::PARAM_INT);
    $stmt->execute();
    $required_fail = $stmt->fetchAll(PDO::FETCH_ASSOC);

    // articles that exist
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT d.name, d.type, w.include, w.id, w.user_defined FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON d.id=w.child_id WHERE rev_id=:rev_id AND w.child_name IS NULL ORDER BY w.include DESC, d.name ASC, d.type ASC");
    $stmt->bindParam('rev_id',$this->rev_id, PDO::PARAM_INT);
    $stmt->execute();
    $required_exist = $stmt->fetchAll(PDO::FETCH_ASSOC);

    // list entries, which do not exist yet
    if (count($required_fail) > 0 || count($required_exist) > 0) {
      echo '<h3>Required Entries</h3>';

      if (count($required_exist) > 0) {
        echo '<ul>';
        foreach($required_exist as $required) {
          echo '<li>['.$required['type'].'] '.$required['name'].' ('.($required['include']==true ? 'include' : 'link').')';

          // delete manual depency
          if (ThisUser::getInstance()->hasAccess('add_depencies') && $required['user_defined']) {
            echo ' <span class="deletebutton" onclick="'."deleteDepency(".$required['id'].")".'"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/remove.gif" alt="" /> Delete</span>';
          }

          echo '</li>';
        }
        echo '</ul>';
      }

      if (count($required_fail) > 0) {
        echo_strip('
          <h4>Required Entries that don\'t exist</h4>
          <ul>');
        foreach($required_fail as $required) {
          echo '<li>'.$required['child_name'].' ('.($required['include']==true ? 'include' : 'link').')';

          // delete manual depency
          if (ThisUser::getInstance()->hasAccess('add_depencies') && $required['user_defined']) {
            echo ' <span class="deletebutton" onclick="'."deleteDepency(".$required['id'].")".'"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/remove.gif" alt="" /> Delete</span>';
          }

          echo '</li>';
        }
        echo '</ul>';
      }
    }
  } // end of member function showEntryDepencies



  /**
   * recursive function to build our depency tree
   *
   * @access private
   */
  private function buildDepencyTree( $data_id )
  {
    // get current childs
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, l.name AS language, d.type, r.data_id, w.user_defined, w.id FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_REVISIONS." r ON w.rev_id = r.id JOIN ".ROSCMST_ENTRIES." d ON d.id=r.data_id JOIN ".ROSCMST_LANGUAGES." l ON l.id=r.lang_id WHERE w.child_id=:data_id AND w.include IS TRUE ORDER BY l.name ASC, d.name ASC");
    $stmt->bindParam('data_id',$data_id, PDO::PARAM_INT);
    $stmt->execute();
    $depencies = $stmt->fetchAll(PDO::FETCH_ASSOC);


    // output childs
    if (count($depencies) > 0) {
      echo '<ul>';

      // show Depencies
      $x=0;
      foreach ($depencies as $depency) {
        $x++;
        echo '<li style="color: #'.($x%2 ? '000' : '777').';">['.$depency['type'].'] '.$depency['name'].' <span style="color: #'.($x%2 ? 'AAA' : 'CCC').';">('.$depency['language'].')</span>';

        // get childs
        if ($data_id != $depency['data_id']) {
          $this->buildDepencyTree( $depency['data_id']);
        }
        echo '</li>';
      } // end foreach

      echo '</ul>';
    }
    elseif ($this->data_id === $data_id) {
      echo 'Looks like, no other entries depend on this entry.';
    }
  } // end of member function buildDepencyTree



  /**
   * Interface to modify entry settings
   *
   * @access private
   */
  private function showEntryDetailsEntry( )
  {
    // entry details
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, type, access_id FROM ".ROSCMST_ENTRIES." WHERE id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    echo_strip('
      <h3>Data-ID</h3>
      <div>'.$data['id'].'</div><br />
      <label for="secdataname">Name</label><br />
      <input type="text" name="secdataname" id="secdataname" size="25" maxlength="100" value="'.$data['name'].'" /> (ASCII lowercase, no space) 
      <img src="'.RosCMS::getInstance()->pathRosCMS().'images/attention.gif" width="22" height="22" /><br />
      <small>all links to this entry will be updated</small><br />
      <br />
      <label for="cbmdatatype">Type</label><br />
      <select id="cbmdatatype" name="cbmdatatype">
        <option value="page"'.(($data['type'] == 'page') ? ' selected="selected"' : '').'>Page</option>
        <option value="page"'.(($data['type'] == 'dynamic') ? ' selected="selected"' : '').'>Dynamic Page</option>
        <option value="content"'.(($data['type'] == 'content') ? ' selected="selected"' : '').'>Content</option>
        <option value="script"'.(($data['type'] == 'script') ? ' selected="selected"' : '').'>Script</option>
      </select><br />
      <br />
      <label for="cbmdataacl">ACL</label><br />
      <select id="cbmdataacl" name="cbmdataacl">');

    // possible ACL settings to choose for this entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_ACCESS." ORDER BY name ASC");
    $stmt->execute();
    while ($access = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$access['id'].'"'.(($access['id'] == $data['access_id']) ? ' selected="selected"' : '').'>'.$access['name'].'</option>';
    }
    echo_strip('
      </select>
      <img src="'.RosCMS::getInstance()->pathRosCMS().'images/attention.gif" width="22" height="22" /><br />
      <br />
      <br />
      <button type="button" id="beditsavefields" onclick="'."saveSecurityData('".$this->data_id."','".$this->rev_id."')".'">Save Changes</button> &nbsp; 
      <button type="button" id="beditclear" onclick="'."showEditorTabSecurity(".$this->data_id.",".$this->rev_id.", '".ThisUser::getInstance()->id()."')".'">Clear</button>');
  } // end of member function showEntryDetailsSecurity



  /**
   * Interface to add remove text fields
   *
   * @access private
   */
  private function showEntryDetailsFields( )
  {
    echo '<h3>Short Text</h3>';

    // list current short texts
    $stext_num = 0;
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id ORDER BY name ASC");
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while($stext = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$stext_num;
    
      echo_strip('
        <input type="text" name="editstext'.$stext_num.'" id="editstext'.$stext_num.'" size="25" maxlength="100" value="');echo $stext['name']; echo_strip('" /> 
        <input type="checkbox" name="editstextdel'.$stext_num.'" id="editstextdel'.$stext_num.'" value="del" />
        <label for="editstextdel'.$stext_num.'">delete?</label>
        <input name="editstextorg'.$stext_num.'" id="editstextorg'.$stext_num.'" type="hidden" value="');echo $stext['name'];echo_strip('" />
        <br />
        <br />');
    }

    echo_strip('
      <div id="editaddstext"></div>
      <span id="editaddstextcount" style="display: none;">'.$stext_num.'</span>
      <span class="virtualButton" onclick="addShortTextField()">
        <img src="'.RosCMS::getInstance()->pathRosCMS().'images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add
      </span>
      <br /><br />
      <h3>Text</h3>');

    // list current long texts
    $text_num = 0;
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id ORDER BY name ASC");
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while($text = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$text_num;

      echo_strip('
        <input type="text" name="edittext'.$text_num.'" id="edittext'.$text_num.'" size="25" maxlength="100" value="'.$text['name'].'" /> 
        <input type="checkbox" name="edittextdel'.$text_num.'" id="edittextdel'.$text_num.'" value="del" />
        <label for="edittextdel'.$text_num.'">delete?</label>
        <input name="edittextorg'.$text_num.'" id="edittextorg'.$text_num.'" type="hidden" value="'.$text['name'].'" />
        <br />
        <br />');
    }

    echo_strip('
      <div id="editaddtext"></div>
      <span id="editaddtextcount" style="display: none;">'.$text_num.'</span>
      <span class="virtualButton" onclick="addTextField()">
        <img src="'.RosCMS::getInstance()->pathRosCMS().'images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add
      </span>
      <br /><br /><br />
      <button type="button" id="beditsavefields" onclick="'."saveFieldData('".$this->data_id."','".$this->rev_id."')".'">Save Changes</button> &nbsp; 
      <button type="button" id="beditclear" onclick="'."saveFieldData(".$this->data_id.",".$this->rev_id.", '".ThisUser::getInstance()->id()."')".'">Clear</button>');
  } // end of member function showEntryDetailsFields



  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsRevision( )
  {
    // get revision information
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, d.type, DATE(r.datetime) AS date, TIME(r.datetime) AS time, r.id, r.lang_id, u.name AS user_name FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id JOIN ".ROSCMST_USERS." u ON u.id = r.user_id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$this->rev_id);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    echo_strip('
      <br />
      <h3>Rev-ID</h3>
      <div>'.$revision['id'].'</div><br />
      <label for="cbmentrylang">Language</label>
      <select id="cbmentrylang" name="cbmentrylang">');

    // list of possible language, we can switch to
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." ORDER BY name ASC");
    $stmt->execute();
    while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$language['id'].'"'.(($language['id'] == $revision['lang_id']) ?' selected="selected"' : '').'>'.$language['name'].'</option>';
    }

    echo_strip('
      </select><br />
      <br />
      <label for="verusr">User</label>
      <input type="text" name="verusr" id="verusr" size="20" maxlength="20" value="'.$revision['user_name'].'" /> (account name)
      <img src="'.RosCMS::getInstance()->pathRosCMS().'images/attention.gif" width="22" height="22" /><br />
      <br />
      <label for="verdate">Date</label>
      <input type="text" name="verdate" id="verdate" size="10" maxlength="10" value="'.$revision['date'].'" /> (year-month-day)
      <img src="'.RosCMS::getInstance()->pathRosCMS().'images/attention.gif" width="22" height="22" /><br />
      <br />
      <label for="vertime">Time</label>
      <input type="text" name="vertime" id="vertime" size="8" maxlength="8" value="'.$revision['time'].'" /> (hour:minute:second)
      <img src="'.RosCMS::getInstance()->pathRosCMS().'images/attention.gif" width="22" height="22" /><br />
      <br />
      <br />
      <button type="button" id="beditsaveentry" onclick="saveRevisionData('.$this->data_id.','.$this->rev_id.')">Save Changes</button> &nbsp;
      <button type="button" id="beditclear" onclick="'."showEditorTabRevisions(".$this->data_id.",".$this->rev_id.", '".ThisUser::getInstance()->id()."')".'">Clear</button>');
  } // end of member function showEntryDetailsRevision



  /**
   * list of revisions for compare feature (current revision is selected by default)
   *
   * @access private
   */
  private function selectRevision( $selected_rev )
  {

    // get a perfect mixed entry set
    $dataset = $this->helperHistory();

    $last_lang = null;
    foreach($dataset as $revision) {
      if ($revision['lang_id'] != $last_lang) {
        if ($last_lang !== null) {
          echo '</optgroup>';
        }

        echo '<optgroup label="'.$revision['language'].'">'; 
        $last_lang = $revision['lang_id'];
      }

      echo '<option value="'.$revision['id'].'"'.(($revision['id'] == $selected_rev) ? ' selected="selected"' : '').'>'.$revision['name'].' ('.$revision['date'].') - v. '.$revision['version'].'; '.$revision['user_name'].'</option>';
    }
    echo '</optgroup>';
  } // end of member function selectRevision



  /**
   * Compare interface
   *
   * @access private
   */
  private function showDifference( $rev_id1, $rev_id2 )
  {
    // diff source 1
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, r.id, r.version, l.name AS language, r.datetime, u.name AS user_name, t.content FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_USERS." u ON r.user_id = u.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id JOIN ".ROSCMST_TEXT." t ON t.rev_id=r.id WHERE r.id = :rev_id AND t.name='content' LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id1,PDO::PARAM_INT);
    $stmt->execute();
    $revision1 = $stmt->fetchOnce();

    // diff source 2
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, r.id, r.version, l.name AS language, r.datetime, u.name AS user_name, t.content FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_USERS." u ON r.user_id = u.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id JOIN ".ROSCMST_TEXT." t ON t.rev_id=r.id WHERE r.id = :rev_id AND t.name='content' LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id2,PDO::PARAM_INT);
    $stmt->execute();
    $revision2 = $stmt->fetchOnce();

    // get data id from any stable revision
    $this->data_id = $revision1['data_id'];

    echo_strip('
      <div style="display: block; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb; background: white none repeat scroll 0%;">
        <div style="margin:10px;">');

    // do we have different entries ?
    if ($rev_id1 == $rev_id2) {
      echo '<p>Please select two different entries to display the differences!</p>';
      return false;
    }
    else {
      echo '<br />';
    }

    echo_strip( '
      <table width="100%" border="0">
        <tr>
          <td style="text-align:center;">
            <select name="cbmdiff1" id="cbmdiff1" onchange="'."getDiffEntries(this.value, document.getElementById('cbmdiff2').value)".'">');
    $this->selectRevision($rev_id1);
    // history
    echo_strip('
            </select>
          </td>
          <td style="width:50px;text-align:center;">
            <button name="switchdiff" id="switchdiff" onclick="'."getDiffEntries(document.getElementById('cbmdiff2').value, document.getElementById('cbmdiff1').value)".'">switch</button>
          </td>
          <td style="text-align:center;">
            <select name="cbmdiff2" id="cbmdiff2" onchange="'."getDiffEntries(document.getElementById('cbmdiff1').value, this.value)".'">');
    $this->selectRevision($rev_id2);
    // history
    echo_strip('
            </select>
          </td>
        </tr>
        <tr>
          <td>
            <ul style="font-size:9px;">
              <li>Type: '.$revision1['type'].'</li>
              <li>Language: '.$revision1['language'].'</li>
              <li>User: '.$revision1['user_name'].'</li>');
    if (ThisUser::getInstance()->hasAccess('entry_details')) {
      echo '<li>Rev-ID: '.$revision1['id'].'</li>';
    }
    echo_strip('
            </ul>
          </td>
          <td>&nbsp;</td>
          <td>
            <ul style="font-size:9px;">
              <li>Type: '.$revision2['type'].'</li>
              <li>Language: '.$revision2['language'].'</li>
              <li>User: '.$revision2['user_name'].'</li>');
    if (ThisUser::getInstance()->hasAccess('entry_details')) {
      echo '<li>Rev-ID: '.$revision2['id'].'</li>';
    }
    echo_strip('
            </ul>
          </td>
        </tr>
      </table>
      <div id="frmeditdiff1" style="display: none;">');echo $revision1['content'];echo_strip('</div>
      <div id="frmeditdiff2" style="display: none;">');echo $revision2['content'];echo_strip('</div>
      <div style="display: block;border-bottom: 1px solid #bbb;  border-right: 1px solid #bbb; border-top: 1px solid #e3e3e3; border-left: 1px solid #e3e3e3; background: #F2F2F2;">
        <pre style="margin:10px; font-size:9px; font-family:Arial, Helvetica, sans-serif;" id="frmeditdiff">&nbsp;</pre>
      </div>
      </div>');
  } // end of member function showDifference



  /**
   * get a array of entries, that are older versions of the same revision, by default also for other languages
   *
   * @param int lang_id if this param is set, we get only entries of the selected language
   * @access private
   */
  private function helperHistory( $lang_id = null )
  {
    // select all related entries
    if ($lang_id === null) {
      if (ThisUser::getInstance()->hasAccess('more_lang')) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, r.id, r.lang_id, l.name AS language, r.version, DATE(r.datetime) as date, r.datetime, u.name AS user_name, r.archive FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id JOIN ".ROSCMST_USERS." u ON u.id = r.user_id WHERE d.id = :data_id AND r.version > 0 ORDER BY l.name  ASC, r.datetime DESC");
      }
      else {
        $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, r.id, r.lang_id, l.name AS language, r.version, DATE(r.datetime) as date, r.datetime, u.name AS user_name, r.archive FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id JOIN ".ROSCMST_USERS." u ON u.id = r.user_id WHERE d.id = :data_id AND r.version > 0 AND l.id IN(:standard_lang,:lang_id) ORDER BY l.name  ASC, r.datetime DESC");
        $stmt->bindParam('standard_lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->bindParam('lang_id',ThisUser::getInstance()->language(),PDO::PARAM_INT);
      }
    }

    // select only one language
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, r.id, r.lang_id, l.name AS language, r.version, DATE(r.datetime) as date, r.datetime, u.name AS user_name, r.archive FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id JOIN ".ROSCMST_USERS." u ON u.id = r.user_id WHERE d.id = :data_id AND r.version > 0 AND r.lang_id=:lang_id ORDER BY l.name  ASC, r.datetime DESC");
      $stmt->bindParam('lang_id',$lang_id,PDO::PARAM_INT);
    }
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    return $stmt->fetchAll(PDO::FETCH_ASSOC);
  } // end of member function helperHistory



} // end of Backend_ViewEditor
?>
