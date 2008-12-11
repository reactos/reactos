<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>
                  2008  Danny Götte <dangerground@web.de>

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
 * class Editor_Website
 * 
 */
class Editor_Website extends Editor
{

  // Type of detail
  const METADATA = 0;
  const FIELDS   = 1;
  const HISTORY  = 2;
  const SECURITY = 3;
  const REVISION = 4;
  const DEPENCIES= 5;

  // types of new entries
  const SINGLE   = 0;
  const DYNAMIC  = 1;
  const TEMPLATE = 2;



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

    $this->rev_id = $revision['id'];
    $this->data_id = $revision['data_id'];
  }


  /**
   *
   *
   * @param string action action which has to be performed
   * @access protected
   */
  protected function evalAction( $action )
  {
    $thisuser = &ThisUser::getInstance();

    if (isset($_GET['d_r_id'])) {
      $this->setRevision($_GET['d_r_id']);
    }

    switch ($action) {

      // create entry - show interface
      case 'newentry':

        // add a new entry only with higher security level
        if ($thisuser->securityLevel() > 1) {
          switch ($_GET['d_val']) {
            case 'dynamic':
              $this->showAddEntry(self::DYNAMIC);
              break;
            case 'template':
              $this->showAddEntry(self::TEMPLATE);
              break;
            case 'single':
            default:
              $this->showAddEntry(self::SINGLE);
              break;
          } // end switch
        } else{
          die('No rights to add new entries.');
        }
        break;

      // single entry - save entry
      case 'newentry2': 
        Data::add(@$_GET['d_type'], $_GET['d_r_lang'],true);
        break;

      // dynamic entry - save entry
      case 'newentry4':
        Data::add('content', Language::getStandardId(),true, true);
        break;

      // page & content - save entry
      case 'newentry3': 
        Data::add('page', Language::getStandardId(), false, false, 'stable', htmlspecialchars(@$_GET['d_template']));
        Data::add('content', Language::getStandardId(), true);
        break;

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

      // show Field details
      case 'alterfields':
        $this->showEntryDetails(self::FIELDS);
        break;

      // update Field details
      case 'alterfields2':
        Data::updateText($_GET['d_r_id'], $_GET['d_val'], $_GET['d_val2'], @$_GET['d_arch']);
        $this->show();
        break;

      // show revision details
      case 'showentry':
        $this->showEntryDetails(self::REVISION);
        break;

      // update revision details
      case 'alterentry':
        Data::updateRevision($_GET['d_r_id'],$_GET['d_val'],$_GET['d_val2'],$_GET['d_val3'],$_GET['d_val4'],htmlspecialchars(@$_GET["d_val5"]));
        $this->show();
        break;

      // show security details
      case 'showsecurity':
        $this->showEntryDetails(self::SECURITY);
        break;

      // update Security details
      case 'altersecurity':
        Data::update($_GET['d_id'], $_GET['d_val'], $_GET['d_val2'], $_GET['d_val3'], $_GET['d_val4']);
        $this->show();
        break;

      // add new tag
      case 'addtag':
        Tag::add($_GET['d_r_id'], $_GET['d_val'], $_GET['d_val2'], $_GET['d_val3']);
        $this->showEntryDetails(self::METADATA);
        break;

      // delete tag
      case 'deltag':
        $this->setRevision(Tag::getRevIdById($_GET['d_val']));
        Tag::deleteById($_GET['d_val']);

        // reload Metadata
        $this->showEntryDetails(self::METADATA);
        break;

      // update tag by id
      case 'changetag':
        Tag::deleteById($_GET['d_val4']);
        Tag::add($_GET['d_r_id'], $_GET['d_val'], $_GET['d_val2'], $_GET['d_val3']);
        echo Tag::getIdByUser($_GET['d_r_id'], $_GET['d_val'], $thisuser->id());
        break;

      // update tag by name/user
      case 'changetag2':
      case 'changetag3':
        Tag::deleteByName($_GET['d_r_id'], $_GET['d_val'] , $_GET['d_val3']);
        Tag::add($_GET['d_r_id'], $_GET['d_val'] , $_GET['d_val2'], $_GET['d_val3']);
        echo Tag::getIdByUser($_GET['d_r_id'], $_GET['d_val'], $_GET['d_val3']);
        break;

      // Change Tags around Data entry
      case 'changetags':
        // only call function if some entries are given ($_GET['d_val'] holds number of id's)
        if ($_GET['d_val'] > 0) {
          Data::evalAction($_GET['d_val2'] /* entry rev_id's */, $_GET['d_val3'] /* action */, @$_GET['d_r_lang'], @$_GET['d_val4']);
        }
        break;

      // compare two entries
      //@FIXME broken logic, as both diff actions do the same
      case 'diff':
      // compare two entries; updates diff area
      case 'diff2':
        echo $this->showDifference($_GET['d_val'] /* rev_id entry 1 */, $_GET['d_val2'] /* rev_id entry 2 */);
        break;

      default:
        $this->performDefaultAction();
        break;
    }
  }


  /**
   *
   *
   * @access protected
   */
  protected function performDefaultAction()
  {
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
      if (Security::hasRight($revision['data_id'], 'trans')) {

        // copy existing entry to new language
        if (Data::copy($revision['id'], 1 /* copy mode */, $_GET['d_r_lang'])) {
          $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE data_id = :data_id AND user_id = :user_id AND version = 0 AND lang_id = :lang ORDER BY id DESC LIMIT 1");
          $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_STR);
          $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
          $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_STR);
          $stmt->execute();
          $translation = $stmt->fetchOnce(PDO::FETCH_ASSOC);
          
          $this->setRevision($translation['id']);
          $this->show();
        }
        else {
          die('Translation not successful, due entry-copy problem. If this happens more than once or twice please contact the website admin.');
        }
      }
      else {
        echo 'You have not enough rights to translate this entry.';
      }
    }
  }


  /**
   *
   *
   * @access private
   */
  protected function show( )
  {
    $this->showEntryData();
    $this->showEntryDetails(self::METADATA);
    echo '</div></div>';  // close elements opened in showEntryData
    $this->showEditor(true, true);
  }


  /**
   *
   *
   * @access private
   */
  protected function showEditor( $edit_stext = false, $edit_text = false)
  {
    echo_strip('
      <div class="editor" style="background:white; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb;">
        <div style="margin:10px;">
          <div style="width:95%;">
            <form method="post" action="#">
              <br />');

    // Input label
    if ($edit_stext === true) {
      $stext_num = 0;
      $stmt=&DBConnection::getInstance()->prepare("SELECT name, content FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id ORDER BY name ASC");
      $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
      $stmt->execute();
      while($stext = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $stext_num++;

        echo_strip('
          <label for="estext"'.$stext_num.'">'.$stext['name'].':</label>
          <span id="edstext'.$stext_num.'" style="display:none;">'.$stext['name'].'</span><br />
          <input name="estext"'.$stext_num.'" type="text" id="estext'.$stext_num.'" size="50" maxlength="250" value="');echo $stext['content'].'" /><br /><br />';
      }

      echo_strip('
        <span id="entrydataid" class="'.$this->data_id.'">&nbsp;</span>
        <span id="entrydatarevid" class="'.$this->rev_id.'">&nbsp;</span>
        <div id="estextcount" class="'.$stext_num.'">&nbsp;</div>');
    }

    // Input Text
    if ($edit_text == true) {
      $text_num = 0;
      $stmt=&DBConnection::getInstance()->prepare("SELECT name, content FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id ORDER BY name ASC");
      $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
      $stmt->execute();
      while($text = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $text_num++;

        echo_strip('
          <label for="elm'.$text_num.'" style="display: inline; margin-right: 20px;" id="textname'.$text_num.'">'.$text['name'].'</label>
          <button type="button" id="butRTE'.$text_num.'" onclick="'."toggleEditor('elm".$text_num."', this.id)".'">Rich Text</button>
          <span id="swraped'.$text_num.'"></span>
          <input id="wraped'.$text_num.'" type="checkbox" onclick="'."toggleWordWrap(this.id, 'elm".$text_num."');".'" checked="checked" style="padding-left: 10px;" />
          <label for="wraped'.$text_num.'" class="normal">Word wrap</label>
          <textarea name="elm'.$text_num.'" cols="80" rows="15" class="mceEditor" id="elm'.$text_num.'" style="width: 100%; background-color:#FFFFFF;" >');echo $text['content'];echo_strip('</textarea>
          <br />
          <br />');
      }

      echo '<span id="elmcount" class="'.$text_num.'">&nbsp;</span>';
    }

    if (Security::hasRight($this->data_id, 'write')) {
      echo_strip('
        <button type="button" id="bsavedraft" onclick="'."saveAsDraft(".$this->data_id.",".$this->rev_id.")".'">Save as Draft</button> &nbsp;
        <input name="editautosavemode" type="hidden" value="true" />');
    }
    else {
      echo_strip('
        <button type="button" id="bsavedraft" disabled="disabled">Save as Draft</button> &nbsp;
        <img src="images/locked.gif" width="11" height="12" /> (not enough rights) &nbsp;
        <input name="editautosavemode" type="hidden" value="false" />');
    }

    $stmt=&DBConnection::getInstance()->prepare("SELECT name, type FROM ".ROSCMST_ENTRIES." WHERE id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE d.name = :name AND r.version > 0 AND r.lang_id = :lang ORDER BY r.id DESC");
    $stmt->bindParam('name',$data['name'],PDO::PARAM_STR);
    $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
    $stmt->execute();
    $revisions_count = $stmt->fetchColumn();

    $dynamic_num = Tag::getValueByUser($this->rev_id, 'number', -1);

    if ($revisions_count <= 1) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_LANGUAGES." WHERE id=:lang_id");
      $stmt->bindParam('lang_id',Language::getStandardId(),PDO::PARAM_INT);
      $stmt->execute();
      $lang=$stmt->fetchColumn();

      echo_strip('
        <span id="bshowdiff" class="frmeditbutton" onclick="'."openOrCloseDiffArea(".$this->rev_id.",".$this->rev_id.")".'">
        <img id="bshowdiffi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Compare</span> (no related '.$lang['name'].' entry, choose yourself)&nbsp;');
    }
    else {

      if (isset($_GET['d_arch']) && $_GET['d_arch']) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE d.name = :name AND r.version > 0 AND r.lang_id = :lang AND r.archive IS TRUE ORDER BY r.id DESC LIMIT 2");
        $stmt->bindParam('name',$data['name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->execute();
        $diff2 = $stmt->fetch();
        $diff2 = 'ar'.$diff2['id'];
        $diff1 = $stmt->fetchOnce();
        $diff1 = 'ar'.$diff1['id'];
      }
      elseif ($dynamic_num === false) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE d.name = :name AND r.version > 0 AND r.lang_id = :lang AND r.archive IS FALSE ORDER BY r.id DESC LIMIT 1");
        $stmt->bindParam('name',$data['name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->execute();
        $diff2 = $stmt->fetchColumn();

        $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE d.name = :name AND r.version > 0 AND r.lang_id = :lang AND r.archive IS TRUE ORDER BY r.id DESC LIMIT 1");
        $stmt->bindParam('name',$data['name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->execute();
        $diff1 = 'ar'.$stmt->fetchColumn();
      }
      else {
        $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id JOIN ".ROSCMST_TAGS." t ON r.id=t.rev_id WHERE d.name = :name AND t.name = 'number' AND t.value = :dynamic_num AND t.user_id = -1 AND r.version > 0 AND r.lang_id = :lang AND r.archive IS FALSE ORDER BY r.id DESC LIMIT 1");
        $stmt->bindParam('name',$data['name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->bindParam('dynamic_num',$dynamic_num,PDO::PARAM_STR);
        $stmt->execute();
        $diff2 = $stmt->fetchColumn();

        $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id JOIN ".ROSCMST_TAGS." t ON r.id=t.rev_id WHERE d.name = :name AND t.name = 'number' AND t.value = :dynamic_num AND t.user_id = -1 AND r.version > 0 AND r.lang_id = :lang AND r.archive IS TRUE ORDER BY r.id DESC LIMIT 1");
        $stmt->bindParam('name',$data['name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',Language::getStandardId(),PDO::PARAM_INT);
        $stmt->bindParam('dynamic_num',$dynamic_num,PDO::PARAM_STR);
        $stmt->execute();
        $diff1 = 'ar'.$stmt->fetchColumn();
      }

      echo_strip('
        <span id="bshowdiff" class="frmeditbutton" onclick="'."openOrCloseDiffArea('".$diff1."','".$diff2."')".'">
          <img id="bshowdiffi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />
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
  }


  /**
   *
   *
   * @access protected
   */
  public function showAddEntry($tmode = self::SINGLE)
  {
    echo_strip('
      <div id="frmadd" class="editor" style="border-bottom: 1px solid #bbb; border-right: 1px solid #bbb; background: #FFFFFF none repeat scroll 0%;">
        <div style="margin:10px;">
          <div class="detailbody">
            <div class="detailmenubody" id="newentrymenu">');

    // is single
    if ($tmode == self::SINGLE) {
      echo '<strong>Single Entry</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changeNewEntryTab('single')".'">Single Entry</span>';
    }
    echo '&nbsp;|&nbsp;';

    // is dynamic
    if ($tmode == self::DYNAMIC) {
      echo '<strong>Dynamic Entry</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changeNewEntryTab('dynamic')".'">Dynamic Entry</span>';
    }
    echo '&nbsp;|&nbsp;';

    // is page & content
    if ($tmode == self::TEMPLATE) {
      echo '<strong>Page &amp; Content</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changeNewEntryTab('template')".'">Page &amp; Content</span>';
    }

    echo_strip('
        </div>
      </div>');

    // is single
    switch ($tmode) {

      // is single
      case self::SINGLE:
        echo_strip('
          <br />
          <label for="txtaddentryname">Name</label>
          <input type="text" id="txtaddentryname" name="txtaddentryname" />
          <br />
          <br />
          <br />
          <label for="txtaddentrytype">Type</label>
          <select id="txtaddentrytype" name="txtaddentrytype">
            <option value="page">Page</option>
            <option value="content">Content</option>
            <option value="template">Template</option>
            <option value="script">Script</option>'.(ThisUser::getInstance()->isMemberOfGroup('ros_sadmin') ? '
            <option value="dynamic">Dynamic Page Type</option>' : '').'
          </select>
          <br />
          <br />
          <br />
          <label for="txtaddentrylang">Language</label>
          <select id="txtaddentrylang" name="txtaddentrylang">');

        // language drop down
        $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE level > 0 ORDER BY name ASC");
        $stmt->execute();
        while($language=$stmt->fetch(PDO::FETCH_ASSOC)) {
          echo '<option value="'.$language['id'].'"'.(Language::getStandardId() == $language['id'] ? ' selected="selected"' : '').'>'.$language['name'].'</option>';
        }
        
        echo '</select>';
        break;

      // is dynamic
      case self::DYNAMIC:
        echo_strip('
          <br />
          <label for="txtadddynsource">Source</label>
          <select id="txtadddynsource" name="txtadddynsource">');

        // dynamic things
        $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_ENTRIES." WHERE type='dynamic' ORDER BY name ASC");
        $stmt->execute();
        while($data=$stmt->fetch(PDO::FETCH_ASSOC)) {
          echo '<option value="'.$data['id'].'">'.$data['name'].'</option>';
        }
        
        echo '</select>';
        break;

      // is page & content
      case self::TEMPLATE:
        echo_strip('
          <br />
          <label for="txtaddentryname3">Name</label>
          <input type="text" id="txtaddentryname3" name="txtaddentryname3" />
          <br />
          <br />
          <label for="txtaddtemplate">Template</label>
          <select id="txtaddtemplate" name="txtaddtemplate">
            <option value="none" selected="selected">no template</option>');

        // select templates (be sure that we have content to that template)
        $stmt=&DBConnection::getInstance()->prepare("SELECT d.name FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id = d.id WHERE r.version > 0 AND d.type = 'template' ORDER BY d.name ASC");
        $stmt->execute();
        while ($templates = $stmt->fetch(PDO::FETCH_ASSOC)) {
          echo '<option value="'. $templates['name'] .'">'. $templates['name'] .'</option>';
        }
        
        echo '</select>';
        break;
    }

    echo_strip('
          <br />
          <br />
          <button type="button" onclick="'."createNewEntry(".$tmode.")".'">Create</button>
        </div>
      </div>');
  }


  /**
   *
   *
   * @access protected
   */
  protected function showEntryData( )
  {
    $thisuser = &ThisUser::getInstance();

    // get Database Entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.id, d.name, d.type, r.version, l.name AS language, r.datetime, u.name AS user_name FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_USERS." u ON r.user_id = u.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id=l.id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$this->rev_id);
    $stmt->execute();
    $revision = $stmt->fetchOnce();

    echo_strip('
      <div style="padding-bottom: 3px;">
        <span class="revDetail">
          <span onclick="'."toggleBookmark(".$revision['id'].", ".$thisuser->id().", 'editstar')".'" style="cursor: pointer;">
           <img id="editstar" class="'.Tag::getIdByUser($revision['id'], 'star', $thisuser->id()).'" src="images/star_'.Tag::getValueByUser($revision['id'], 'star', $thisuser->id()).'_small.gif" alt="" style="width:13px; height:13px; border:0px;" alt="*" />
          </span>
          &nbsp;');
    echo $revision['name'];

    // give dynamic number (if exists)
    $dynamic_num = Tag::getValueByUser($revision['id'], 'number', -1);
    if ($revision['type'] == 'content' && $dynamic_num > 0) {
      echo_strip(
        '_'.$dynamic_num.'
        <div id="entryeditdynnbr" style="display:none;">'.$dynamic_num.'</div>');
    }
    else {
      echo '<div id="entryeditdynnbr" style="display:none;">no</div>';
    }

    echo_strip('
      </span> &nbsp;
      <span style="white-space: nowrap;">type: <span class="revDetail">'.$revision['type'].'</span></span> &nbsp; 
      <span style="white-space: nowrap;">version: <span id="mefrverid" class="revDetail">'.$revision['version'].'</span></span> &nbsp; 
      <span style="white-space: nowrap;">language: <span id="mefrlang" class="revDetail">'.$revision['language'].'</span></span> &nbsp; 
      <span style="white-space: nowrap;">user: <span id="mefrusrid" class="revDetail">'.$revision['user_name'].'</span></span> &nbsp; ');

    if (isset($_GET['d_arch']) && $_GET['d_arch']) {
      echo_strip('
        <span style="white-space: nowrap;">mode: 
          <span id="mefrusrid" class="revDetail">archive</span>
        </span> &nbsp; ');
    }

    echo_strip('
        <span id="frmedittags" class="frmeditbutton" onclick="TabOpenClose(this.id)" style="white-space: nowrap;">
          <img id="frmedittagsi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />
          &nbsp;Details
        </span>
      </div>
      <div id="frmedittagsc" class="edittagbody" style="display: none;">
        <div id="frmedittagsc2">');
  }


  protected function showEntryDetails( $mode = self::METADATA)
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
    echo '&nbsp;|&nbsp;';

    // History
    if ($mode == self::HISTORY) {
      echo '<strong>History</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."showEditorTabHistory(".$this->rev_id.")".'">History</span>';
    }
    echo '&nbsp;|&nbsp;';

    // Depencies
    if ($mode == self::DEPENCIES) {
      echo '<strong>Depencies</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."showEditorTabDepencies(".$this->rev_id.")".'">Depencies</span>';
    }

    // allowed only for someone with "add" rights
    if (Security::hasRight($this->data_id, 'add')) { 
      echo '&nbsp;|&nbsp;';

      // Fields
      if ($mode == self::FIELDS) {
        echo '<strong>Fields</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."showEditorTabFields(".$this->rev_id.")".'">Fields</span>';
      }
      echo '&nbsp;|&nbsp;';

      if ($mode == self::REVISION) {
        echo '<strong>Revision</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."showEditorTabRevisions(".$this->rev_id.")".'">Revision</span>';
      }
    }

    // allowed only for related super administrators
    if ($thisuser->isMemberOfGroup('ros_sadmin') || (Security::hasRight($this->data_id, 'add') && $thisuser->isMemberOfGroup('ros_admin'))) { 
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
        $this->showEntryDetailsSecurity();
        break;
      case self::FIELDS:
        $this->showEntryDetailsFields();
        break;
      case self::REVISION:
        $this->showEntryDetailsRevision();
        break;
    }

  }


  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsMetadata( )
  {
    $thisuser = &ThisUser::getInstance();

    // helper vars
    $last_user = null; // used in first while, to recognize the last type

    if ($thisuser->securityLevel() > 1) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, user_id, name, value FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id AND user_id IN(-1, 0,:user_id) ORDER BY user_id ASC, name ASC");
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, user_id, name, value FROM ".ROSCMST_TAGS." WHERE rev_id = :rev_id AND user_id IN(0,:user_id) ORDER BY user_id ASC, name ASC");
    }
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();
    while($tag = $stmt->fetch()) {
      if ($tag['user_id'] != $last_user) {

        // echo metadata type (metadata / label / private label)
        echo_strip('
          <h3>');
        switch ($tag['user_id']) {
          case -1:
            echo 'System Metadata';
            break;
          case 0:
            echo 'Labels';
            break;
          default:
            if ($tag['user_id'] == $thisuser->id()) {
              echo 'Private Labels';
            }
        } // end switch
        echo '</h3>';
      }

      // output name & current value
      echo '<strong>'.$tag['name'].':</strong>&nbsp;'.$tag['value'];

      // show delete button
        // allow to delete label if SecLev > 1
        // allow to delete sys metadata if user has the rights
        // allow someone to delete his metadata he set and the user-id > 0
      if (($thisuser->securityLevel() > 1 && $tag['user_id'] == 0) || (Security::hasRight($this->data_id, 'add') && $tag['user_id'] == -1) || ($tag['user_id'] == $thisuser->id() && $tag['user_id'] > 0)) {
        echo_strip('&nbsp;&nbsp;
          <span class="frmeditbutton" onclick="'."delLabelOrTag('".$tag['id']."')".'">
            <img src="images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />
            &nbsp;Delete
          </span>');
      }

      echo '<br />';
      $last_user = $tag['user_id'];
    }

    echo_strip('
      <br />
      <h3>Add Private Label</h3>
      <label for="addtagn" class="normal">Tag:</label>&nbsp;
      <input type="text" id="addtagn" size="15" maxlength="100" value="" />&nbsp;
      <button type="button" onclick="'."addLabelOrTag(".$this->rev_id.",'tag','addtagn', '".$thisuser->id()."')".'">Add</button>
      <br />');

    if ($thisuser->securityLevel() > 1) {
      echo_strip('
        <br />
        <h3>Add Label'.(Security::hasRight($this->data_id, 'add') ? ' or System Metadata' : '').'</h3>
        <label for="addtags1" class="normal">Name:</label>&nbsp;
        <input type="text" id="addtags1" size="15" maxlength="100" value="" />&nbsp;
        <label for="addtags2" class="normal">Value:</label>&nbsp;
        <input type="text" id="addtags2" size="15" maxlength="100" value="" /> &nbsp;
        <button type="button" onclick="'."addLabelOrTag(".$this->rev_id.",'addtags1','addtags2','0')".'">Add Label</button>&nbsp;');

      // add new system tags
      if (Security::hasRight($this->data_id, 'add')) { 
        echo '<button type="button" onclick="'."addLabelOrTag(".$this->rev_id.",'addtags1','addtags2',-1)".'">Add Sys</button>';
      }
    }
    
    echo '<br /><br />';
  }


  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsHistory( )
  {
    echo '<h3>Versions History</h3>';

    // get a perfect mixed entry set
    $dataset = $this->helperHistory(Tag::getValueByUser($this->rev_id, 'number', -1));

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
  }



  /**
   *
   *
   * @access private
   */
  private function showEntryDepencies()
  {
    echo '<h3>Dependent Entries</h3>';
    $this->buildDepencyTree($this->data_id);

    // required articles that don't exist
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT depency_name, is_include FROM ".ROSCMST_DEPENCIES." WHERE rev_id=:rev_id AND depency_id IS NULL ORDER BY is_include DESC, depency_name ASC");
    $stmt->bindParam('rev_id',$this->rev_id, PDO::PARAM_INT);
    $stmt->execute();
    $required_fail = $stmt->fetchAll(PDO::FETCH_ASSOC);

    // articles that exist
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT d.name, d.type, w.is_include FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON d.id=w.depency_id WHERE rev_id=:rev_id AND w.depency_name IS NULL ORDER BY w.is_include DESC, d.name ASC, d.type ASC");
    $stmt->bindParam('rev_id',$this->rev_id, PDO::PARAM_INT);
    $stmt->execute();
    $required_exist = $stmt->fetchAll(PDO::FETCH_ASSOC);

    if (count($required_fail) > 0 || count($required_exist) > 0) {
      echo '<h3>Required Entries</h3>';

      if (count($required_exist) > 0) {
        echo '<ul>';
        foreach($required_exist as $required) {
          echo '<li>'.$required['name'].' ['.$required['type'].'] ('.($required['is_include']==true ? 'include' : 'link').')</li>';
        }
        echo '</ul>';
      }

      if (count($required_fail) > 0) {
        echo_strip('
          <h4>Entries that don\'t exist</h4>
          <ul>');
        foreach($required_fail as $required) {
          echo '<li>'.$required['depency_name'].' ('.($required['is_include']==true ? 'include' : 'link').')</li>';
        }
        echo '</ul>';
      }

    }
  }



  /**
   *
   *
   * @access private
   */
  private function buildDepencyTree( $data_id )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, l.name AS language, d.type, r.data_id FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_REVISIONS." r ON w.rev_id = r.id JOIN ".ROSCMST_ENTRIES." d ON d.id=r.data_id JOIN ".ROSCMST_LANGUAGES." l ON l.id=r.lang_id WHERE w.depency_id=:data_id AND w.is_include IS TRUE ORDER BY language");
    $stmt->bindParam('data_id',$data_id, PDO::PARAM_INT);
    $stmt->execute();
    $depencies = $stmt->fetchAll(PDO::FETCH_ASSOC);

    if (count($depencies) > 0) {
      echo '<ul>';

      $last_language = null;
      // handle Depencies
      foreach ($depencies as $depency) {
        echo '<li>'.$depency['name'].' ('.$depency['type'].') ['.$depency['language'].']';
        if ($data_id != $depency['data_id']) $this->buildDepencyTree( $depency['data_id']);
        echo '</li>';
      }
      echo '</ul>';
    }
    elseif ($this->data_id === $data_id) {
      echo 'Looks like, no other entries depend on this entry.';
    }
  }



  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsSecurity( )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, type, acl_id FROM ".ROSCMST_ENTRIES." WHERE id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    echo_strip('
      <h3>Data-ID</h3>
      <div>'.$data['id'].'</div><br />
      <label for="secdataname">Name</label><br />
      <input type="text" name="secdataname" id="secdataname" size="25" maxlength="100" value="'.$data['name'].'" /> (ASCII lowercase, no space) 
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <input type="checkbox" name="chdname" id="chdname" value="update" checked="checked" />
      <label for="chdname">update all links to this data-name</label><br />
      <br />
      <label for="cbmdatatype">Type</label><br />
      <select id="cbmdatatype" name="cbmdatatype">
        <option value="page"'.(($data['type'] == 'page') ? ' selected="selected"' : '').'>Page</option>
        <option value="content"'.(($data['type'] == 'content') ? ' selected="selected"' : '').'>Content</option>
        <option value="template"'.(($data['type'] == 'template') ? ' selected="selected"' : '').'>Template</option>
        <option value="script"'.(($data['type'] == 'script') ? ' selected="selected"' : '').'>Script</option>
      </select><br />
      <br />
      <label for="cbmdataacl">ACL</label><br />
      <select id="cbmdataacl" name="cbmdataacl">');

    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_ACCESS." ORDER BY name ASC");
    $stmt->execute();
    while ($access = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$access['id'].'"'.(($access['id'] == $data['acl_id']) ? ' selected="selected"' : '').'>'.$access['name'].'</option>';
    }
    echo_strip('
      </select>
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <br />
      <button type="button" id="beditsavefields" onclick="'."saveSecurityData('".$this->data_id."','".$this->rev_id."')".'">Save Changes</button> &nbsp; 
      <button type="button" id="beditclear" onclick="'."showEditorTabSecurity(".$this->data_id.",".$this->rev_id.", '".ThisUser::getInstance()->id()."')".'">Clear</button>');
  }


  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsFields( )
  {
    echo '<h3>Short Text</h3>';

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
      <span class="filterbutton" onclick="addShortTextField()">
        <img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add
      </span>
      <br /><br />
      <h3>Text</h3>');

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
      <span class="filterbutton" onclick="addTextField()">
        <img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add
      </span>
      <br /><br /><br />
      <button type="button" id="beditsavefields" onclick="'."saveFieldData('".$this->data_id."','".$this->rev_id."')".'">Save Changes</button> &nbsp; 
      <button type="button" id="beditclear" onclick="'."saveFieldData(".$this->data_id.",".$this->rev_id.", '".ThisUser::getInstance()->id()."')".'">Clear</button>');
  }


  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsRevision( )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, d.type, DATE(r.datetime) AS date, TIME(r.datetime) AS time, r.id, r.lang_id, r.version, u.name AS user_name FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id JOIN ".ROSCMST_USERS." u ON u.id = r.user_id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$this->rev_id);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    echo_strip('
      <br />
      <h3>Rev-ID</h3>
      <div>'.$revision['id'].'</div><br />
      <label for="cbmentrylang">Language</label>
      <select id="cbmentrylang" name="cbmentrylang">');

    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." ORDER BY name ASC");
    $stmt->execute();
    while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$language['id'].'"'.(($language['id'] == $revision['lang_id']) ?' selected="selected"' : '').'>'.$language['name'].'</option>';
    }

    echo_strip('
      </select><br />
      <br />
      <label for="vernbr">Version</div>
      <input type="text" name="vernbr" id="vernbr" size="5" maxlength="11" value="'.$revision['version'].'" /><br />
      <br />
      <label for="verusr">User</label>
      <input type="text" name="verusr" id="verusr" size="20" maxlength="20" value="'.$revision['user_name'].'" /> (account name)
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <label for="verdate">Date</label>
      <input type="text" name="verdate" id="verdate" size="10" maxlength="10" value="'.$revision['date'].'" /> (year-month-day)
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <label for="vertime">Time</label>
      <input type="text" name="vertime" id="vertime" size="8" maxlength="8" value="'.$revision['time'].'" /> (hour:minute:second)
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <br />
      <button type="button" id="beditsaveentry" onclick="saveRevisionData('.$this->data_id.','.$this->rev_id.')">Save Changes</button> &nbsp;
      <button type="button" id="beditclear" onclick="'."showEditorTabRevisions(".$this->data_id.",".$this->rev_id.", '".ThisUser::getInstance()->id()."')".'">Clear</button>');
  }


  /**
   *
   *
   * @access private
   */
  private function selectRevision( $selected_rev, $dynamic_num )
  {

    // get a perfect mixed entry set
    $dataset = $this->helperHistory($dynamic_num);

    $last_lang = null;
    foreach($dataset as $revision) {
      if ($revision['rev_language'] != $last_lang) {
        if ($last_lang !== null) {
          echo '</optgroup>';
        }

        echo '<optgroup label="'.$revision['lang_name'].'">'; 
        $last_lang = $revision['rev_language'];
      }

      echo '<option value="'.($revision['archive'] ? 'ar' : '').$revision['rev_id'].'"'.(($revision['rev_id'] == $selected_rev) ? ' selected="selected"' : '').'>'.$revision['data_name'].' ('.$revision['rev_date'].') - v. '.$revision['rev_version'].'; '.$revision['user_name'].'</option>';
    }
    echo '</optgroup>';
  }


  /**
   *
   *
   * @access private
   */
  private function showDifference( $rev_id1, $rev_id2 )
  {
    // get archive mode for entry 1
    if (substr($rev_id1, 0, 2) == 'ar') {
      $h1_a = '_a';
      $h1_a2 = 'a';
      $rev_id1 = substr($rev_id1, 2);
    }
    else {
      $h1_a = '';
      $h1_a2 = '';
    }

    // get archive mode for entry 2
    if (substr($rev_id2, 0, 2) == 'ar') {
      $h2_a = '_a';
      $h2_a2 = 'a';
      $rev_id2 = substr($rev_id2, 2);
    }
    else {
      $h2_a = '';
      $h2_a2 = '';
    }

    // @TODO: add short text and optional long text additional entries
    // diff source 1
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, r.id, r.version, l.name AS language, r.datetime, u.name AS user_name FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_USERS." u ON r.user_id = u.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id1,PDO::PARAM_INT);
    $stmt->execute();
    $revision1 = $stmt->fetchOnce();

    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id AND name = 'content' ORDER BY name ASC");
    $stmt->bindParam('rev_id',$rev_id1,PDO::PARAM_INT);
    $stmt->execute();
    $text1 = $stmt->fetchColumn();

    // diff source 2
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, r.id, r.version, l.name AS language, r.datetime, u.name AS user_name FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_USERS." u ON r.user_id = u.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id2,PDO::PARAM_INT);
    $stmt->execute();
    $revision2 = $stmt->fetchOnce();

    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_TEXT." WHERE rev_id = :rev_id AND name = 'content' ORDER BY name ASC");
    $stmt->bindParam('rev_id',$rev_id2,PDO::PARAM_INT);
    $stmt->execute();
    $text2 = $stmt->fetchColumn();

    // get data id from any stable revision
    $dynamic_num = Tag::getValueByUser($revision2['id'], 'number', -1);
    $this->data_id = $revision2['data_id'];
    if ($h2_a2 != '') {
      $dynamic_num = Tag::getValueByUser($revision1['id'], 'number', -1);
      $this->data_id = $revision1['data_id'];
    }

    echo_strip('
      <div style="display: block; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb; background: white none repeat scroll 0%;">
        <div style="margin:10px;">
          <br />
          <span>Compare</span>');

    if ($rev_id1 == $rev_id2) {
      echo '<p>Please select two different entries to display the differences!</p>';
    }
    else {
      echo '<br />';
    }

    echo_strip( '
      <table width="100%" border="0">
        <tr>
          <td style="text-align:center;">
            <select name="cbmdiff1" id="cbmdiff1" onchange="'."getDiffEntries(this.value, document.getElementById('cbmdiff2').value)".'">');
    // history
    $this->selectRevision($rev_id1, $dynamic_num);
    echo_strip('
            </select>
          </td>
          <td style="width:50px;text-align:center;">
            <input type="submit" name="switchdiff" id="switchdiff" value="switch" onclick="'."getDiffEntries(document.getElementById('cbmdiff2').value, document.getElementById('cbmdiff1').value)".'" />
          </td>
          <td style="text-align:center;">
            <select name="cbmdiff2" id="cbmdiff2" onchange="'."getDiffEntries(document.getElementById('cbmdiff1').value, this.value)".'">');
    // history
    $this->selectRevision($rev_id2,$dynamic_num);
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
    if (ThisUser::getInstance()->securityLevel() > 1) {
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
    if (ThisUser::getInstance()->securityLevel() > 1) {
      echo '<li>ID: '.$revision2['id'].'</li>';
    }
    echo_strip('
            </ul>
          </td>
        </tr>
      </table>
      <div><pre id="frmeditdiff1" style="display: none;">');echo $text1;echo_strip('</pre></div>
      <div><pre id="frmeditdiff2" style="display: none;">');echo $text2;echo_strip('</pre></div>
      <div style="display: block;border-bottom: 1px solid #bbb;  border-right: 1px solid #bbb; border-top: 1px solid #e3e3e3; border-left: 1px solid #e3e3e3; background: #F2F2F2;">
        <pre style="margin:10px; font-size:9px; font-family:Arial, Helvetica, sans-serif;" id="frmeditdiff">&nbsp;</pre>
      </div>
      </div>');
  }



  /**
   * unifies two arrays into one, sort by language ASC, dateime DESC
   *
   * @param mixed data_normal should be already in right order 
   * @access private
   */
  private function helperHistory( $dynamic_num = false )
  {
    // check stable entries
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, type FROM ".ROSCMST_ENTRIES." WHERE id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    // no dynamic number
    if ($dynamic_num === false) {
      // select active entries
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, r.id, l.name AS language, r.version, r.datetime, u.name AS user_name FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id JOIN ".ROSCMST_USERS." u ON u.id = r.user_id WHERE d.name = :name AND d.type = :type AND r.version > 0 ORDER BY l.name  ASC, r.datetime DESC");
      $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
      $stmt->bindParam('type',$data['data_type'],PDO::PARAM_STR);
      $stmt->execute();
      return $stmt->fetchAll(PDO::FETCH_ASSOC);
    }
    
    // get only dynamic entries with our dynamic number
    else {
      // select active entries
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, CONCAT(d.name,'_',t.value) AS name, r.id, r.version, r.datetime, u.name AS user_name, l.name AS language FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TAGS." t ON t.rev_id = r.id JOIN ".ROSCMST_LANGUAGES." l ON r.lang_id = l.id JOIN ".ROSCMST_USERS." u ON u.id = r.user_id WHERE d.name = :name AND d.type = :type AND t.name = 'number' AND t.user_id = -1 AND t.value = :tag_value AND r.version > 0 ORDER BY l.name ASC, r.datetime DESC");
      $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
      $stmt->bindParam('type',$data['data_type'],PDO::PARAM_STR);
      $stmt->bindParam('tag_value',$dynamic_num,PDO::PARAM_INT);
      $stmt->execute();
      return $stmt->fetchAll(PDO::FETCH_ASSOC);
    }
  } // end of member function helperHistory
  
  
} // end of Editor_Website
?>
