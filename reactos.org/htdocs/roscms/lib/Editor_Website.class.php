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
  const ENTRY    = 4;

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
  protected function evalAction( $action )
  {
    $thisuser = &ThisUser::getInstance();

    global $roscms_standard_language;
    global $RosCMS_GET_d_value, $RosCMS_GET_d_value2, $RosCMS_GET_d_value3, $RosCMS_GET_d_value4;
    global $RosCMS_GET_d_id, $RosCMS_GET_d_r_id;
    global $RosCMS_GET_d_r_lang;

    switch ($action) {

      // create entry - show interface
      case 'newentry':

        // add a new entry only with higher security level
        if ($thisuser->securityLevel() > 1) {
          switch ($RosCMS_GET_d_value) {
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
        Data::add(@$_GET['d_type'], @$_GET['d_r_lang'],true);
        break;

      // dynamic entry - save entry
      case 'newentry4':
        Data::add('content', $roscms_standard_language,true, true);
        break;

      // page & content - save entry
      case 'newentry3': 
        Data::add('page', $roscms_standard_language, false, false, 'stable', htmlspecialchars(@$_GET['d_template']));
        Data::add('content', $roscms_standard_language, true);
        break;

      // show Metadata details
      case 'showtag':
        $this->showEntryDetails(self::METADATA);
        break;

      // show History details
      case 'showhistory':
        $this->showEntryDetails(self::HISTORY);
        break;

      // show Field details
      case 'alterfields':
        $this->showEntryDetails(self::FIELDS);
        break;

      // update Field details
      case 'alterfields2':
        Data::updateText($RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value2, @$_GET['d_arch']);
        $this->show();
        break;

      // show entry details
      case 'showentry':
        $this->showEntryDetails(self::ENTRY);
        break;

      // update Entry details
      case 'alterentry':
        Data::updateRevision($RosCMS_GET_d_id, $RosCMS_GET_d_r_id,$RosCMS_GET_d_value,$RosCMS_GET_d_value2,$RosCMS_GET_d_value3,$RosCMS_GET_d_value4,htmlspecialchars(@$_GET["d_val5"]),htmlspecialchars(@$_GET["d_val6"]),htmlspecialchars(@$_GET["d_val7"]));
        $this->show();
        break;

      // show security details
      case 'showsecurity':
        $this->showEntryDetails(self::SECURITY);
        break;

      // update Security details
      case 'altersecurity':
        Data::update($RosCMS_GET_d_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value2, $RosCMS_GET_d_value3, $RosCMS_GET_d_value4);
        $this->show();
        break;

      // add new tag
      case 'addtag':
        Tag::add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value2, $RosCMS_GET_d_value3);
        $this->showEntryDetails(self::METADATA);
        break;

      // delete tag
      case 'deltag':
      
        // only delete, if user has a higher level than translator, or it's requested by the user itself
        if ($thisuser->securityLevel() > 1 || $RosCMS_GET_d_value2 == $thisuser->id()) {
          Tag::deleteById($RosCMS_GET_d_value, $RosCMS_GET_d_value2);
        }

        // reload Metadata
        $this->showEntryDetails(self::METADATA);
        break;

      // update tag by id
      case 'changetag':
        Tag::deleteById($RosCMS_GET_d_value4, $RosCMS_GET_d_value3);
        Tag::add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value2, $RosCMS_GET_d_value3);
        echo Tag::getIdByUser($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $thisuser->id());
        break;

      // update tag by name/user
      case 'changetag2':
      case 'changetag3':
        Tag::deleteByName($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value , $RosCMS_GET_d_value3);
        Tag::add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value , $RosCMS_GET_d_value2, $RosCMS_GET_d_value3);
        echo Tag::getIdByName($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value3);
        break;

      // Change Tags around Data entry
      case 'changetags':
        // only call function if some entries are given ($_GET['d_val'] holds number of id's)
        if ($_GET['d_val'] > 0) {
          Data::evalAction($RosCMS_GET_d_value2 /* entry rev_id's */, $RosCMS_GET_d_value3 /* action */, $RosCMS_GET_d_r_lang, $RosCMS_GET_d_value4);
        }
        break;

      // compare two entries
      //@FIXME broken logic, as both diff actions do the same
      case 'diff':
      // compare two entries; updates diff area
      case 'diff2':
        echo $this->showDifference($RosCMS_GET_d_value /* rev_id entry 1 */, $RosCMS_GET_d_value2 /* rev_id entry 2 */);
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
    global $RosCMS_GET_d_r_lang;

    // normal (contains NO "tr")
    if (!isset($_GET['d_r_id']) || strpos($_GET['d_r_id'], 'tr') === false) {
      $this->show();
    }

    // translation mode (contains "tr")
    else {
      $stmt=DBConnection::getInstance()->prepare("SELECT rev_id, data_id FROM data_revision WHERE rev_id = :rev_id LIMIT 1");
      $stmt->bindValue('rev_id',substr($_GET['d_r_id'], 2),PDO::PARAM_INT); // remove 'tr' from rev_id while query
      $stmt->execute();
      $revision = $stmt->fetchOnce();

      // check if user has translator access
      if (Security::hasRight($revision['data_id'], 'trans')) {

        // copy existing entry to new language
        if (Data::copy($revision['data_id'], $revision['rev_id'], 1 /* copy mode */, $RosCMS_GET_d_r_lang)) {
          $stmt=DBConnection::getInstance()->prepare("SELECT data_id, rev_id, rev_language FROM data_revision WHERE data_id = :data_id AND rev_usrid = :user_id AND rev_version = 0 AND rev_language = :lang AND rev_date = :date ORDER BY rev_id DESC LIMIT 1");
          $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_STR);
          $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
          $stmt->bindParam('lang',$_GET['d_r_lang'],PDO::PARAM_STR);
          $stmt->bindParam('date',date('Y-m-d'),PDO::PARAM_STR);
          $stmt->execute();
          $translation = $stmt->fetchOnce();
          
          $this->data_id = $translation['data_id'];
          $this->rev_id = $translation['rev_id'];
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
    global $roscms_standard_language;
    global $roscms_standard_language_full;
    global $h_a,$h_a2;

    echo_strip('
      <div style="background:white; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb;">
        <div style="margin:10px;">
          <div style="width:95%;">
            <form method="post" action="#">
              <br />');

    // Input label
    if ($edit_stext === true) {
      $stext_num = 0;
      $stmt=DBConnection::getInstance()->prepare("SELECT s.stext_name, s.stext_content FROM data_revision".$h_a." r, data_stext".$h_a." s WHERE r.rev_id = s.data_rev_id AND r.rev_id = :rev_id ORDER BY stext_name ASC");
      $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
      $stmt->execute();
      while($stext = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $stext_num++;

        echo_strip('
          <label for="estext"'.$stext_num.'" class="frmeditheadline" style="font-weight: bold;">'); echo ucfirst($stext['stext_name']);echo_strip(':</label>
          <span id="edstext'.$stext_num.'" style="display:none;">'.$stext['stext_name'].'</span><br />
          <input name="estext"'.$stext_num.'" type="text" id="estext'.$stext_num.'" size="50" maxlength="250" value="');echo $stext['stext_content'].'" /><br /><br />';
      }

      echo_strip('
        <span id="entrydataid" class="'.$this->data_id.'">&nbsp;</span>
        <span id="entrydatarevid" class="'.$this->rev_id.'">&nbsp;</span>
        <div id="estextcount" class="'.$stext_num.'">&nbsp;</div>');
    }

    // Input Text
    if ($edit_text == true) {
      $text_num = 0;
      $stmt=DBConnection::getInstance()->prepare("SELECT t.text_name, t.text_content, r.rev_language FROM data_revision".$h_a." r, data_text".$h_a." t WHERE r.rev_id = t.data_rev_id AND r.rev_id = :rev_id ORDER BY text_name ASC");
      $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
      $stmt->execute();
      while($text = $stmt->fetch()) {
        $text_num++;

        echo_strip('
          <label class="frmeditheadline" for="elm'.$text_num.'">');echo ucfirst($text['text_name']); echo_strip('</label>
          <button type="button" id="butRTE'.$text_num.'" onclick="'."toggleEditor('elm".$text_num."', this.id)".'">Rich Text</button>
          <span id="swraped'.$text_num.'">
          <input id="wraped'.$text_num.'" type="checkbox" onclick="'."toggleWordWrap(this.id, 'elm".$text_num."');".'" checked="checked" />
          <label for="wraped'.$text_num.'</label>">Word wrap</label></span>
          <span id="edtext'.$text_num.'" style="display:none;">'.$text['text_name'].'</span><br />
          <textarea name="elm'.$text_num.'" cols="80" rows="15" class="mceEditor" id="elm'.$text_num.'" style="width: 100%; background-color:#FFFFFF;" >');echo $text['text_content'];echo_strip('</textarea>
          <br />
          <br />');
      }

      echo '<span id="elmcount" class="'.$text_num.'">&nbsp;</span>';
    }

    if (Security::hasRight($this->data_id, 'write')) {
      echo_strip('
        <button type="button" id="bsavedraft" onclick="'."edit_form_submit_draft(".$this->data_id.",".$this->rev_id.")".'">Save as Draft</button> &nbsp;
        <input name="editautosavemode" type="hidden" value="true" />');
    }
    else {
      echo_strip('
        <button type="button" id="bsavedraft" disabled="disabled">Save as Draft</button> &nbsp;
        <img src="images/locked.gif" width="11" height="12" /> (not enough rights) &nbsp;
        <input name="editautosavemode" type="hidden" value="false" />');
    }

    $stmt=DBConnection::getInstance()->prepare("SELECT data_name, data_type FROM data_ WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    // try to find in archive
    if ($data === false) { 
      $stmt=DBConnection::getInstance()->prepare("SELECT data_name, data_type FROM data_a WHERE data_id = :data_id LIMIT 1");
      $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
      $stmt->execute();
      $data = $stmt->fetchOnce();
    }

    $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM data_a d JOIN data_revision_a r ON d.data_id = r.data_id WHERE d.data_name = :name AND r.rev_version > 0 AND r.rev_language = :lang ORDER BY r.rev_id DESC");
    $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
    $stmt->bindParam('lang',$roscms_standard_language,PDO::PARAM_STR);
    $stmt->execute();
    $revisions_count = $stmt->fetchColumn();

    $dynamic_num = Tag::getValueByUser($this->data_id, $this->rev_id, 'number', -1);

    if ($revisions_count <= 0) {
      echo_strip('
        <span id="bshowdiff" class="frmeditbutton" onclick="'."diffentries3(".$this->rev_id.",".$this->rev_id.")".'>
        <img id="bshowdiffi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Compare</span> (no related ');echo strtolower($roscms_standard_language_full).' entry, choose yourself)&nbsp;';
    }
    else {

      if (isset($_GET['d_arch']) && $_GET['d_arch']) {
        $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id FROM data_a d JOIN data_revision_a r ON d.data_id = r.data_id WHERE d.data_name = :name AND r.rev_version > 0 AND r.rev_language = :lang ORDER BY r.rev_id DESC LIMIT 2");
        $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',$roscms_standard_language,PDO::PARAM_STR);
        $stmt->execute();
        $diff2 = $stmt->fetch();
        $diff2 = 'ar'.$diff2['rev_id'];
        $diff1 = $stmt->fetchOnce();
        $diff1 = 'ar'.$diff1['rev_id'];
      }
      elseif ($dynamic_num === false) {
        $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id FROM data_ d JOIN data_revision r ON d.data_id = r.data_id WHERE d.data_name = :name AND r.rev_version > 0 AND r.rev_language = :lang ORDER BY r.rev_id DESC LIMIT 1");
        $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',$roscms_standard_language,PDO::PARAM_STR);
        $stmt->execute();
        $diff2 = $stmt->fetchColumn();

        $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id FROM data_a d JOIN data_revision_a r ON d.data_id = r.data_id WHERE d.data_name = :name AND r.rev_version > 0 AND r.rev_language = :lang ORDER BY r.rev_id DESC LIMIT 1");
        $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',$roscms_standard_language);
        $stmt->execute();
        $diff1 = 'ar'.$stmt->fetchColumn();
      }
      else {
        $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id FROM data_ d JOIN data_revision r ON d.data_id = r.data_id JOIN data_tag t ON r.rev_id=t.data_rev_id JOIN data_tag_name n ON t.tag_name_id=n.tn_id JOIN data_tag_value v ON v.tv_id=t.tag_value_id WHERE d.data_name = :name AND n.tn_name='number' AND tv_value = :dynamic_num AND t.tag_usrid = -1 AND r.rev_version > 0 AND r.rev_language = :lang ORDER BY r.rev_id DESC LIMIT 1");
        $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',$roscms_standard_language,PDO::PARAM_STR);
        $stmt->bindParam('dynamic_num',$dynamic_num,PDO::PARAM_STR);
        $stmt->execute();
        $diff2 = $stmt->fetchColumn();

        $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id FROM data_a d JOIN data_revision_a r ON d.data_id = r.data_id JOIN data_tag_a t ON r.rev_id=t.data_rev_id JOIN data_tag_name_a n ON t.tag_name_id=n.tn_id JOIN data_tag_value_a v ON v.tv_id=t.tag_value_id WHERE d.data_name = :name AND n.tn_name='number' AND tv_value = :dynamic_num AND t.tag_usrid = -1 AND r.rev_version > 0 AND r.rev_language = :lang ORDER BY r.rev_id DESC LIMIT 1");
        $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
        $stmt->bindParam('lang',$roscms_standard_language);
        $stmt->bindParam('dynamic_num',$dynamic_num,PDO::PARAM_STR);
        $stmt->execute();
        $diff1 = 'ar'.$stmt->fetchColumn();
      }

      echo_strip('
        <span id="bshowdiff" class="frmeditbutton" onclick="'."diffentries3('".$diff1."','".$diff2."')".'">
        <img id="bshowdiffi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Compare</span> &nbsp;');
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
      <div id="frmadd" style="border-bottom: 1px solid #bbb; border-right: 1px solid #bbb; background: #FFFFFF none repeat scroll 0%;">
        <div style="margin:10px;">
          <div class="detailbody">
            <div class="detailmenubody" id="newentrymenu">');

    // is single
    if ($tmode == self::SINGLE) {
      echo '<strong>Single Entry</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changecreateinterface('single')".'">Single Entry</span>';
    }
    echo '&nbsp;|&nbsp;';

    // is dynamic
    if ($tmode == self::DYNAMIC) {
      echo '<strong>Dynamic Entry</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changecreateinterface('dynamic')".'">Dynamic Entry</span>';
    }
    echo '&nbsp;|&nbsp;';

    // is page & content
    if ($tmode == self::TEMPLATE) {
      echo '<strong>Page &amp; Content</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changecreateinterface('template')".'">Page &amp; Content</span>';
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
          <div class="frmeditheadline">Name</div>
          <input type="text" id="txtaddentryname" name="txtaddentryname" />
          <br />
          <br />
          <br />
          <div class="frmeditheadline">Type</div>
          <select id="txtaddentrytype" name="txtaddentrytype">
            <option value="page">Page</option>
            <option value="content">Content</option>
            <option value="template">Template</option>
            <option value="script">Script</option>
            <option value="system">System</option>
          </select>
          <br />
          <br />
          <br />
          <div class="frmeditheadline">Language</div>
          <select id="txtaddentrylang" name="txtaddentrylang">');

        // language drop down
        $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages WHERE lang_level > 0 ORDER BY lang_name ASC");
        $stmt->execute();
        while($language=$stmt->fetch()) {
          echo '<option value="'.$language['lang_id'].'">'.$language['lang_name'].'</option>';
        }
        
        echo '</select>';
        break;

      // is dynamic
      case self::DYNAMIC:
        echo_strip('
          <br />
          <div class="frmeditheadline">Source</div>
          <select id="txtadddynsource" name="txtadddynsource">
            <option value="news_page">News</option>
            <option value="newsletter">Newsletter</option>
            <option value="interview">Interview</option>
          </select>');
        break;

      // is page & content
      case self::TEMPLATE:
        echo_strip('
          <br />
          <div class="frmeditheadline">Name</div>
          <input type="text" id="txtaddentryname3" name="txtaddentryname3" />
          <br />
          <br />
          <div class="frmeditheadline">Template</div>
          <select id="txtaddtemplate" name="txtaddtemplate">
            <option value="none" selected="selected">no template</option>');

        $stmt=DBConnection::getInstance()->prepare("SELECT d.data_name FROM data_revision r JOIN data_ d ON (r.rev_version > 0 AND r.data_id = d.data_id) WHERE d.data_type = 'template' ORDER BY d.data_name ASC");
        $stmt->execute();
        while ($templates = $stmt->fetch()) {
          echo '<option value="'. $templates['data_name'] .'">'. $templates['data_name'] .'</option>';
        }
        
        echo '</select>';
        break;
    }

    echo_strip('
          <br />
          <br />
          <button type="button" onclick="'."createentry('".$tmode."')".'">Create</button>
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
    global $h_a;
    global $h_a2;

    $thisuser = &ThisUser::getInstance();

    // get Database Entry
    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, u.user_name FROM data_".$h_a2." d JOIN data_revision".$h_a." r ON  r.data_id = d.data_id JOIN users u ON r.rev_usrid = u.user_id WHERE r.rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$this->rev_id);
    $stmt->execute();
    $revision = $stmt->fetchOnce();

    echo_strip('
      <div style="padding-bottom: 3px;">
        <span class="frmeditheader">
          <span onclick="'."bchangestar(".$revision['data_id'].",".$revision['rev_id'].",'star','addtagn', ".$thisuser->id().", 'editstar')".'" style="cursor: pointer;">
           <img id="editstar" class="'.Tag::getIdByUser($revision['data_id'], $revision['rev_id'], 'star', $thisuser->id()).'" src="images/star_'.Tag::getValueByUser($revision['data_id'], $revision['rev_id'], 'star', $thisuser->id()).'_small.gif" alt="" style="width:13px; height:13px; border:0px;" />
          </span>
          &nbsp;');
    echo $revision['data_name'];

    // give dynamic number (if exists)
    $dynamic_num = Tag::getValueByUser($revision['data_id'], $revision['rev_id'], 'number', -1);
    if ($revision['data_type'] == 'content' && $dynamic_num > 0) {
      echo_strip(
        '_'.$dynamic_num.'
        <div id="entryeditdynnbr" style="display:none;">'.$dynamic_num.'</div>');
    }
    else {
      echo '<div id="entryeditdynnbr" style="display:none;">no</div>';
    }

    echo_strip('
      </span> &nbsp;
      <span style="white-space: nowrap;">type: <span class="frmeditheader">'.$revision['data_type'].'</span></span> &nbsp; 
      <span style="white-space: nowrap;">version: <span id="mefrverid" class="frmeditheader">'.$revision['rev_version'].'</span></span> &nbsp; 
      <span style="white-space: nowrap;">language: <span id="mefrlang" class="frmeditheader">'.$revision['rev_language'].'</span></span> &nbsp; 
      <span style="white-space: nowrap;">user: <span id="mefrusrid" class="frmeditheader">'.$revision['user_name'].'</span></span> &nbsp; ');

    if (isset($_GET['d_arch']) && $_GET['d_arch']) {
      echo_strip('
        <span style="white-space: nowrap;">mode: 
          <span id="mefrusrid" class="frmeditheader">archive</span>
        </span> &nbsp; ');
    }

    echo_strip('
        <span id="frmedittags" class="frmeditbutton" onclick="TabOpenClose(this.id)" style="white-space: nowrap;">
          <img id="frmedittagsi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />
          &nbsp;Details
        </span>
      </div>
      <div id="frmedittagsc" class="edittagbody" style="display: none;">
        <div class="edittagbody2" id="frmedittagsc2">');
  }


  protected function showEntryDetails( $mode = self::METADATA)
  {
    global $h_a;
    global $h_a2;

    $thisuser = &ThisUser::getInstance();

    echo_strip('
      <div class="detailbody">
        <div class="detailmenubody">');

    // Metadata
    if ($mode == self::METADATA) {
      echo '<strong>Metadata</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."bshowtag(".$this->data_id.",".$this->rev_id.",'a','b', '".$thisuser->id()."')".'">Metadata</span>';
    }
    echo "&nbsp;|&nbsp;";

    // History
    if ($mode == self::HISTORY) {
      echo '<strong>History</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."bshowhistory(".$this->data_id.",".$this->rev_id.",'a','b', '".$thisuser->id()."')".'">History</span>';
    }

    // allowed only for someone with "add" rights
    if (Security::hasRight($this->data_id, 'add')) { 
      echo "&nbsp;|&nbsp;";

      // Fields
      if ($mode == self::FIELDS) {
        echo '<strong>Fields</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."balterfields(".$this->data_id.",".$this->rev_id.", '".$thisuser->id()."')".'">Fields</span>';
      }
      echo "&nbsp;|&nbsp;";

      if ($mode == self::ENTRY) {
        echo '<strong>Entry</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."bshowentry(".$this->data_id.",".$this->rev_id.", '".$thisuser->id()."')".'">Entry</span>';
      }
    }

    // allowed only for related super administrators
    if ($thisuser->isMemberOfGroup('ros_sadmin') || (Security::hasRight($this->data_id, 'add') && $thisuser->isMemberOfGroup('ros_admin'))) { 
      echo "&nbsp;|&nbsp;";

      if ($mode == self::SECURITY) {
        echo '<strong>Security</strong>';
      }
      else {
        echo '<span class="detailmenu" onclick="'."bshowsecurity(".$this->data_id.",".$this->rev_id.", '".$thisuser->id()."')".'">Security</span>';
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
      case self::SECURITY:
        $this->showEntryDetailsSecurity();
        break;
      case self::FIELDS:
        $this->showEntryDetailsFields();
        break;
      case self::ENTRY:
        $this->showEntryDetailsEntry();
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
    global $h_a,$h_a2;

    $thisuser = &ThisUser::getInstance();

    // helper vars
    $last_user = null; // used in first while, to recognize the last type

    if ($thisuser->securityLevel() > 1) {
      $stmt=DBConnection::getInstance()->prepare("SELECT a.tag_id, a.tag_usrid, n.tn_name, v.tv_value FROM data_".$h_a2." d, data_revision".$h_a." r, data_tag".$h_a." a, data_tag_name".$h_a." n, data_tag_value".$h_a." v WHERE (a.data_id = 0 OR (a.data_id = :data_id AND a.data_id = d.data_id) ) AND (a.data_rev_id = 0 OR (a.data_rev_id = :rev_id AND a.data_rev_id = r.rev_id) ) AND a.tag_usrid IN(-1, 0,:user_id) AND a.tag_name_id = n.tn_id AND a.tag_value_id  = v.tv_id ORDER BY tag_usrid ASC, tn_name ASC");
    }
    else {
      $stmt=DBConnection::getInstance()->prepare("SELECT a.tag_id, a.tag_usrid, n.tn_name, v.tv_value FROM data_".$h_a2." d, data_revision".$h_a." r, data_tag".$h_a." a, data_tag_name".$h_a." n, data_tag_value".$h_a." v WHERE (a.data_id = 0 OR (a.data_id = :data_id AND a.data_id = d.data_id) ) AND (a.data_rev_id = 0 OR (a.data_rev_id = :rev_id AND a.data_rev_id = r.rev_id) ) AND a.tag_usrid IN(0, :user_id) AND a.tag_name_id = n.tn_id AND a.tag_value_id  = v.tv_id ORDER BY tag_usrid ASC, tn_name ASC");
    }
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();
    while($tag = $stmt->fetch()) {
      if ($tag['tag_usrid'] != $last_user) {

        // echo metadata type (metadata / label / private label)
        echo_strip('
          <br />
          <div class="frmeditheadline">');
        switch ($tag['tag_usrid']) {
          case -1:
            echo 'System Metadata';
            break;
          case 0:
            echo 'Labels';
            break;
          default:
            if ($tag['tag_usrid'] == $thisuser->id()) {
              echo 'Private Labels';
            }
        } // end switch
        echo '</div>';
      }

      // output name & current value
      echo '<strong>'.$tag['tn_name'].':</strong>&nbsp;'.$tag['tv_value'];

      // show delete button
        // allow to delete label if SecLev > 1
        // allow to delete sys metadata if user has the rights
        // allow someone to delete his metadata he set and the user-id > 0
      if (($thisuser->securityLevel() > 1 && $tag['tag_usrid'] == 0) || (Security::hasRight($this->data_id, 'add') && $tag['tag_usrid'] == -1) || ($tag['tag_usrid'] == $thisuser->id() && $tag['tag_usrid'] > 0)) {
        echo_strip('&nbsp;&nbsp;
          <span class="frmeditbutton" onclick="'."bdeltag(".$this->data_id.",".$this->rev_id.",'".$tag['tag_id']."', '".$thisuser->id()."')".'">
            <img src="images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />
            &nbsp;Delete
          </span>');
      }

      echo '<br />';
      $last_user = $tag['tag_usrid'];
    }

    echo_strip('
      <br />
      <br />
      <div class="frmeditheadline">Add Private Label</div>
      <label for="addtagn"><b>Tag:</b></label>&nbsp;
      <input type="text" id="addtagn" size="15" maxlength="100" value="" />&nbsp;
      <button type="button" onclick="'."baddtag(".$this->data_id.",".$this->rev_id.",'tag','addtagn', '".$thisuser->id()."')".'">Add</button>
      <br />');

    if ($thisuser->securityLevel() > 1) {
      echo_strip('
        <br />
        <div class="frmeditheadline">Add Label'.(Security::hasRight($this->data_id, 'add') ? ' or System Metadata' : '').'</div>
        <label for="addtags1" style="font-weight:bold">Name:</label>&nbsp;
        <input type="text" id="addtags1" size="15" maxlength="100" value="" />&nbsp;
        <label for="addtags2" style="font-weight:bold">Value:</label>&nbsp;
        <input type="text" id="addtags2" size="15" maxlength="100" value="" /> &nbsp;
        <button type="button" onclick="'."baddtag(".$this->data_id.",".$this->rev_id.",'addtags1','addtags2','0')".'">Add Label</button>&nbsp;');

      // add new system tags
      if (Security::hasRight($this->data_id, 'add')) { 
        echo '<button type="button" onclick="'."baddtag(".$this->data_id.",".$this->rev_id.",'addtags1','addtags2',-1)".'">Add Sys</button>';
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
    $stmt=DBConnection::getInstance()->prepare("SELECT data_name, data_type FROM data_ WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    echo_strip('
      <br />
      <div class="frmeditheadline">Versions History</div>
      <br />');

    // get a perfect mixed entry set
    $dataset = $this->helperHistory(Tag::getValueByUser($this->data_id, $this->rev_id, 'number', -1));

    $last_language = null;
    foreach ($dataset as $revision) {
      if ($revision['rev_language'] != $last_language) {
        if ($last_language !== null) {
          echo '</ul>';
        }

        echo '<p style="font-weight:bold;">'.$revision['lang_name'].'</p><ul>'; 
        $last_language = $revision['rev_language'];
      }

      echo '<li'.($revision['rev_id'] == $this->rev_id ? ' style="text-decoration:underline;"' : '').'>'.$revision['data_name'].' ('.$revision['rev_datetime'].') - v. '.$revision['rev_version'].'; '.$revision['user_name'].'</li>';
    }
    echo '</ul>';
  }


  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsSecurity( )
  {
    global $h_a2;

    $stmt=DBConnection::getInstance()->prepare("SELECT data_id, data_name, data_type, data_acl FROM data_".$h_a2." WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    echo_strip('
      <br />
      <div class="frmeditheadline">Data-ID</div><br />
      <div>'.$data['data_id'].'</div><br />
      <div class="frmeditheadline">Name</div><br />
      <input type="text" name="secdataname" id="secdataname" size="25" maxlength="100" value="'.$data['data_name'].'" /> (ASCII lowercase, no space) 
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <input type="checkbox" name="chdname" id="chdname" value="update" checked="checked" />
      <label for="chdname">update all links to this data-name</label><br />
      <br />
      <div class="frmeditheadline">Type</div><br />
      <select id="cbmdatatype">
        <option value="page"'.(($data['data_type'] == 'page') ? ' selected="selected"' : '').'>Page</option>
        <option value="content"'.(($data['data_type'] == 'content') ? ' selected="selected"' : '').'>Content</option>
        <option value="template"'.(($data['data_type'] == 'template') ? ' selected="selected"' : '').'>Template</option>
        <option value="script"'.(($data['data_type'] == 'script') ? ' selected="selected"' : '').'>Script</option>
        <option value="system"'.(($data['data_type'] == 'system') ? ' selected="selected"' : '').'>System</option>
      </select><br />
      <br />
      <div class="frmeditheadline">ACL</div><br />
      <select id="cbmdataacl" name="cbmdataacl">');

    $stmt=DBConnection::getInstance()->prepare("SELECT sec_name, sec_fullname FROM data_security WHERE sec_branch = 'website' ORDER BY sec_fullname ASC");
    $stmt->execute();
    while ($acl = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$acl['sec_name'].'"'.(($acl['sec_name'] == $data['data_acl']) ? ' selected="selected"' : '').'>'.$acl['sec_fullname'].'</option>';
    }
    echo_strip('
      </select>
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <br />
      <button type="button" id="beditsavefields" onclick="'."editsavesecuritychanges('".$this->data_id."','".$this->rev_id."')".'">Save Changes</button> &nbsp; 
      <button type="button" id="beditclear" onclick="'."bshowsecurity(".$this->data_id.",".$this->rev_id.", '".$thisuser->id()."')".'">Clear</button>');
  }


  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsFields( )
  {
    global $h_a;
  
    echo_strip(
      '<br />
      <div class="frmeditheadline">Short Text</div>
      <br />');

    $stext_num = 0;
    $stmt=DBConnection::getInstance()->prepare("SELECT s.stext_name, s.stext_content FROM data_revision".$h_a." r, data_stext".$h_a." s WHERE r.rev_id = s.data_rev_id AND r.rev_id = :rev_id ORDER BY stext_name ASC");
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while($stext = $stmt->fetch()) {
      ++$stext_num;
    
      echo_strip('
        <input type="text" name="editstext'.$stext_num.'" id="editstext'.$stext_num.'" size="25" maxlength="100" value="');echo $stext['stext_name']; echo_strip('" /> 
        <input type="checkbox" name="editstextdel'.$stext_num.'" id="editstextdel'.$stext.'" value="del" />
        <label for="editstextdel'.$stext_num.'">delete?</label>
        <input name="editstextorg'.$stext_num.'" id="editstextorg'.$stext_num.'" type="hidden" value="');echo $stext['stext_name'];echo_strip('" />
        <br />
        <br />');
    }

    echo_strip('
      <div id="editaddstext"></div>
      <span id="editaddstextcount" style="display: none;">'.$stext_num.'</span>
      <span class="filterbutton" onclick="editaddshorttext()">
        <img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add
      </span>
      <br /><br /><br />
      <div class="frmeditheadline">Text</div>
      <br />');

    $text_num = 0;
    $stmt=DBConnection::getInstance()->prepare("SELECT t.text_name, t.text_content FROM data_revision".$h_a." r, data_text".$h_a." t WHERE r.rev_id = t.data_rev_id AND r.rev_id = :rev_id ORDER BY text_name ASC");
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while($text = $stmt->fetch()) {
      ++$text_num;

      echo_strip('
        <input type="text" name="edittext'.$text_num.'" id="edittext'.$text_num.'" size="25" maxlength="100" value="'.$text['text_name'].'" /> 
        <input type="checkbox" name="edittextdel'.$text_num.'" id="edittextdel'.$text_num.'" value="del" />
        <label for="edittextdel'.$text_num.'">delete?</label>
        <input name="edittextorg'.$text_num.'" id="edittextorg'.$text_num.'" type="hidden" value="'.$text['text_name'].'" />
        <br />
        <br />');
    }

    echo_strip('
      <div id="editaddtext"></div>
      <span id="editaddtextcount" style="display: none;">'.$text_num.'</span>
      <span class="filterbutton" onclick="editaddtext()">
        <img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add
      </span>
      <br /><br /><br />
      <button type="button" id="beditsavefields" onclick="'."editsavefieldchanges('".$this->data_id."','".$this->rev_id."')".'">Save Changes</button> &nbsp; 
      <button type="button" id="beditclear" onclick="'."balterfields(".$this->data_id.",".$this->rev_id.", '".$thisuser->id()."')".'">Clear</button>');
  }


  /**
   *
   *
   * @access private
   */
  private function showEntryDetailsEntry( )
  {
    global $h_a, $h_a2;

    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_name, d.data_type, r.rev_date, r.rev_time, r.rev_id, r.rev_language, r.rev_version, u.user_name FROM data_".$h_a2." d JOIN data_revision".$h_a." r ON d.data_id = r.data_id JOIN users u ON u.user_id = r.rev_usrid WHERE r.rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$this->rev_id);
    $stmt->execute();
    $revision = $stmt->fetchOnce();

    echo_strip('
      <br />
      <div class="frmeditheadline">Rev-ID</div><br />
      <div>'.$revision['rev_id'].'</div><br />
      <div class="frmeditheadline">Language</div><br />
      <select id="cbmentrylang" name="cbmentrylang">');

    $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages ORDER BY lang_name ASC");
    $stmt->execute();
    while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$language['lang_id'].'"'.(($language['lang_id'] == $revision['rev_language']) ?' selected="selected"' : '').'>'.$language['lang_name'].'</option>';
    }

    echo_strip('
      </select><br />
      <br />
      <div class="frmeditheadline">Version</div><br />
      <input type="text" name="vernbr" id="vernbr" size="5" maxlength="11" value="'.$revision['rev_version'].'" /><br />
      <br />
      <div class="frmeditheadline">User</div><br />
      <input type="text" name="verusr" id="verusr" size="20" maxlength="20" value="'.$revision['user_name'].'" /> (account name)
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <div class="frmeditheadline">Date</div><br />
      <input type="text" name="verdate" id="verdate" size="10" maxlength="10" value="'.$revision['rev_date'].'" /> (year-month-day)
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <div class="frmeditheadline">Time</div><br />
      <input type="text" name="vertime" id="vertime" size="8" maxlength="8" value="'.$revision['rev_time'].'" /> (hour:minute:second)
      <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <div class="frmeditheadline">Move Entry</div><br />
      <input type="text" name="chgdataname" id="chgdataname" size="25" maxlength="100" value="'.$revision['data_name'].'" />
      <select id="cbmchgdatatype">
        <option value="page"'.(($revision['data_type'] == 'page') ? ' selected="selected"' : '').'>Page</option>
        <option value="content"'.(($revision['data_type'] == 'content') ? ' selected="selected"' : '').'>Content</option>
        <option value="template"'.(($revision['data_type'] == 'template') ? ' selected="selected"' : '').'>Template</option>
        <option value="script"'.(($revision['data_type'] == 'script') ? ' selected="selected"' : '').'>Script</option>
        <option value="system"'.(($revision['data_type'] == 'system') ? ' selected="selected"' : '').'>System</option>
      </select> <img src="images/attention.gif" width="22" height="22" /><br />
      <br />
      <br />
      <button type="button" id="beditsaveentry" onclick="editsaveentrychanges('.$this->data_id.','.$this->rev_id.')">Save Changes</button> &nbsp;
      <button type="button" id="beditclear" onclick="'."bshowentry(".$this->data_id.",".$this->rev_id.", '".$thisuser->id()."')".'">Clear</button>');
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
    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, u.user_name, l.lang_name FROM data_".$h1_a2." d JOIN data_revision".$h1_a." r ON r.data_id = d.data_id JOIN users u ON r.rev_usrid = u.user_id JOIN languages l ON r.rev_language = l.lang_id WHERE r.rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id1,PDO::PARAM_INT);
    $stmt->execute();
    $revision1 = $stmt->fetchOnce();

    $stmt=DBConnection::getInstance()->prepare("SELECT t.text_content FROM data_revision".$h1_a." r JOIN data_text".$h1_a." t ON r.rev_id = t.data_rev_id WHERE r.rev_id = :rev_id AND t.text_name = 'content' ORDER BY text_name ASC");
    $stmt->bindParam('rev_id',$rev_id1,PDO::PARAM_INT);
    $stmt->execute();
    $text1 = $stmt->fetchColumn();

    // diff source 2
    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, u.user_name, l.lang_name FROM data_".$h2_a2." d JOIN data_revision".$h2_a." r ON r.data_id = d.data_id JOIN users u ON r.rev_usrid = u.user_id JOIN languages l ON r.rev_language = l.lang_id WHERE r.rev_id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id2,PDO::PARAM_INT);
    $stmt->execute();
    $revision2 = $stmt->fetchOnce();

    $stmt=DBConnection::getInstance()->prepare("SELECT t.text_content FROM data_revision".$h1_a." r JOIN data_text".$h1_a." t ON r.rev_id = t.data_rev_id WHERE r.rev_id = :rev_id AND t.text_name = 'content' ORDER BY text_name ASC");
    $stmt->bindParam('rev_id',$rev_id2,PDO::PARAM_INT);
    $stmt->execute();
    $text2 = $stmt->fetchColumn();

    // get data id from any stable revision
    $this->data_id = $revision2['data_id'];
    $dynamic_num = Tag::getValueByUser($revision2['data_id'], $revision2['rev_id'], 'number', -1);
    if ($dynamic_num === false) {
      $dynamic_num = Tag::getValueByUser($revision1['data_id'], $revision1['rev_id'], 'number', -1);
      $this->data_id = $revision1['data_id'];
    }

    echo_strip('
      <div style="display: block; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb; background: white none repeat scroll 0%;">
        <div style="margin:10px;">
        <br />
        <div class="frmeditheadline">Compare</div>');

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
            <select name="cbmdiff1" id="cbmdiff1" onchange="'."diffentries2(this.value, document.getElementById('cbmdiff2').value)".'">');
    // history
    $this->selectRevision($rev_id1, $dynamic_num);
    echo_strip('
            </select>
          </td>
          <td style="width:50px;text-align:center;">
            <input type="submit" name="switchdiff" id="switchdiff" value="switch" onclick="'."diffentries2(document.getElementById('cbmdiff2').value, document.getElementById('cbmdiff1').value)".'" />
          </td>
          <td style="text-align:center;">
            <select name="cbmdiff2" id="cbmdiff2" onchange="diffentries2(document.getElementById(\'cbmdiff1\').value, this.value)">');
    // history
    $this->selectRevision($rev_id2,$dynamic_num);
    echo_strip('
            </select>
          </td>
        </tr>
        <tr>
          <td>
            <ul style="font-size:9px;">
              <li>Type: '.$revision1['data_type'].'</li>
              <li>Language: '.$revision1['lang_name'].'</li>
              <li>User: '.$revision1['user_name'].'</li>');
    if (ThisUser::getInstance()->securityLevel() > 1) {
      echo '<li>ID: '.$revision1['rev_id'].'</li>';
    }
    echo_strip('
            </ul>
          </td>
          <td>&nbsp;</td>
          <td>
            <ul style="font-size:9px;">
              <li>Type: '.$revision2['data_type'].'</li>
              <li>Language: '.$revision2['lang_name'].'</li>
              <li>User: '.$revision2['user_name'].'</li>');
    if (ThisUser::getInstance()->securityLevel() > 1) {
      echo '<li>ID: '.$revision2['rev_id'].'</li>';
    }
    echo_strip('
            </ul>
          </td>
        </tr>
      </table>
      <div><pre id="frmeditdiff1" style="display: none;">');echo $text1;echo_strip('</pre></div>
      <div><pre id="frmeditdiff2" style="display: none;">');echo $text2;echo_strip('</pre></div>
      <div style="display: block; border-width: 1px; border-bottom: 1px solid #bbb;  border-right: 1px solid #bbb; border-top: 1px solid #e3e3e3; border-left: 1px solid #e3e3e3; background: #F2F2F2 none repeat scroll 0%;">
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
    $stmt=DBConnection::getInstance()->prepare("SELECT data_name, data_type FROM data_ WHERE data_id = :data_id LIMIT 1");
    $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();

    // check archive entries for data_id
    if ($data === false) { 
      $stmt=DBConnection::getInstance()->prepare("SELECT data_name, data_type FROM data_a WHERE data_id = :data_id LIMIT 1");
      $stmt->bindParam('data_id',$this->data_id,PDO::PARAM_INT);
      $stmt->execute();
      $data = $stmt->fetchOnce();
    }

    // no dynamic number
    if ($dynamic_num === false) {
      // select active entries
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, r.rev_id, r.rev_language, r.rev_version, r.rev_datetime, r.rev_date, u.user_name, l.lang_name FROM data_ d JOIN data_revision r ON r.data_id = d.data_id JOIN languages l ON r.rev_language = l.lang_id JOIN users u ON u.user_id = r.rev_usrid WHERE d.data_name = :name AND d.data_type = :type AND r.rev_version > 0 ORDER BY l.lang_name  ASC, r.rev_datetime DESC");
      $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
      $stmt->bindParam('type',$data['data_type'],PDO::PARAM_STR);
      $stmt->execute();
      $data_normal = $stmt->fetchAll(PDO::FETCH_ASSOC);

      // select entries from archive
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, r.rev_id, r.rev_language, r.rev_version, r.rev_datetime, r.rev_date, u.user_name, l.lang_name FROM data_a d JOIN data_revision_a r ON r.data_id = d.data_id JOIN languages l ON r.rev_language = l.lang_id JOIN users u ON u.user_id = r.rev_usrid WHERE d.data_name = :name AND d.data_type = :type AND r.rev_version > 0 ORDER BY l.lang_name  ASC, r.rev_datetime DESC");
      $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
      $stmt->bindParam('type',$data['data_type'],PDO::PARAM_STR);
      $stmt->execute();
      $data_archive = $stmt->fetchAll(PDO::FETCH_ASSOC);
    }
    
    // get only dynamic entries with our dynamic number
    else {
      // select active entries
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, CONCAT(d.data_name,'_',v.tv_value) AS data_name, r.rev_id, r.rev_language, r.rev_version, r.rev_datetime, r.rev_date, u.user_name, l.lang_name FROM data_ d JOIN data_revision r ON r.data_id = d.data_id JOIN data_tag t ON (r.data_id = t.data_id AND t.data_rev_id = r.rev_id) JOIN data_tag_name n ON n.tn_id=t.tag_name_id JOIN data_tag_value v ON v.tv_id=t.tag_value_id JOIN languages l ON r.rev_language = l.lang_id JOIN users u ON u.user_id = r.rev_usrid WHERE d.data_name = :name AND d.data_type = :type AND n.tn_name = 'number' AND t.tag_usrid = -1 AND v.tv_value = :tag_value AND r.rev_version > 0 ORDER BY l.lang_name ASC, r.rev_datetime DESC");
      $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
      $stmt->bindParam('type',$data['data_type'],PDO::PARAM_STR);
      $stmt->bindParam('tag_value',$dynamic_num,PDO::PARAM_INT);
      $stmt->execute();
      $data_normal = $stmt->fetchAll(PDO::FETCH_ASSOC);

      // select entries from archive
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, CONCAT(d.data_name,'_',v.tv_value) AS data_name, r.rev_id, r.rev_language, r.rev_version, r.rev_datetime, r.rev_date, u.user_name, l.lang_name FROM data_a d JOIN data_revision_a r ON r.data_id = d.data_id JOIN data_tag_a t ON (r.data_id = t.data_id AND t.data_rev_id = r.rev_id) JOIN data_tag_name_a n ON n.tn_id=t.tag_name_id JOIN data_tag_value_a v ON v.tv_id=t.tag_value_id JOIN languages l ON r.rev_language = l.lang_id JOIN users u ON u.user_id = r.rev_usrid WHERE d.data_name = :name AND d.data_type = :type AND n.tn_name = 'number' AND t.tag_usrid = -1 AND v.tv_value = :tag_value AND r.rev_version > 0 ORDER BY l.lang_name  ASC, r.rev_datetime DESC");
      $stmt->bindParam('name',$data['data_name'],PDO::PARAM_STR);
      $stmt->bindParam('type',$data['data_type'],PDO::PARAM_STR);
      $stmt->bindParam('tag_value',$dynamic_num,PDO::PARAM_INT);
      $stmt->execute();
      $data_archive = $stmt->fetchAll(PDO::FETCH_ASSOC);
    }

    // mix both arrays in correct order
    $i=0;
    $j=0;
    $dataset = array();
    while (isset($data_normal[$i]) && isset($data_archive[$j])) {
    
      // same language
      if ($data_normal[$i]['lang_name'] == $data_archive[$j]['lang_name']){
      
        // newer data
        if ($data_normal[$i]['rev_datetime'] > $data_archive[$j]['rev_datetime']) {
          $dataset[] = $data_normal[$i];
          ++$i;
        }
        else {
          $dataset[] = $data_archive[$j];
          ++$j;
        }
      }
      elseif ($data_normal[$i]['lang_name'] < $data_archive[$j]['lang_name']) {
        $dataset[] = $data_normal[$i];
        ++$i;
      }
      else {
        $dataset[] = $data_archive[$j];
        ++$j;
      }
    }

    // take the rest and put it to the end
    if (isset($data_normal[$i])) {
      while (isset($data_normal[$i])) {
        $dataset[] = $data_normal[$i];
        ++$i;
      }
    }
    else {
      while (isset($data_archive[$j])) {
        $dataset[] = $data_archive[$j];
        ++$j;
      }
    }

    return $dataset;
  }
  
  
} // end of Editor_Website
?>
