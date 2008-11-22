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
 * class HTML_CMS_Maintain
 * 
 */
class HTML_CMS_Maintain extends HTML_CMS
{


  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '', $page_css = 'roscms' )
  {
    $this->branch = 'maintain';

    // register css & js files
    $this->register_css('cms_maintain.css');
    $this->register_js('cms_maintain.js.php');

    parent::__construct( $page_title, $page_css);
  }


  /**
   *
   *
   * @access protected
   */
  protected function body( )
  {
    global $roscms_intern_page_link;

    // check if user has rights for this area
    if (ThisUser::getInstance()->securityLevel() < 3) {
      return;
    }

    echo_strip('
      <br />
      <h2>Maintain</h2>
      <p><b>RosCMS Maintainer Interface</b></p>
      <br />
      <p><a href="javascript:optimizedb()">Optimize Database Tables</a></p>
      <br />
      <p><a href="javascript:ppreview()">Page Preview</a></p>

      <div>
        <label for="textfield">Entry-Name:</label>
        <input name="textfield" type="text" id="textfield" size="20" maxlength="100" />
        <select id="txtaddentrytype" name="txtaddentrytype">
          <option value="page" selected="selected">Page</option>
          <option value="content">Content</option>
          <option value="template">Template</option>
          <option value="script">Script</option>
          <option value="system">System</option>
        </select>
        <select id="txtaddentrylang" name="txtaddentrylang">');

    // display languages
    $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages WHERE lang_level > 0 ORDER BY lang_name ASC");
    $stmt->execute();
    while ($language=$stmt->fetch()) {
      echo '<option value="'.$language['lang_id'].'">'.$language['lang_name'].'</option>';
    }
    echo_strip('
        </select>
        <input name="dynnbr" type="text" id="dynnbr" size="3" maxlength="5" />
        <input name="entryupdate" type="button" value="generate" onclick="pupdate()" />
      </div>

      <p><a href="javascript:genpages()">Generate All Pages</a></p>
      <div id="maintainarea" style="border: 1px dashed red;display:none;"></div>
      <img id="ajaxloading" style="display:none;" src="images/ajax_loading.gif" width="13" height="13" alt="" />
      <br />');

    if (ThisUser::getInstance()->isMemberOfGroup('ros_sadmin')) {

      // display logs
      echo_strip('
        <br />
        <h2>RosCMS Global Log</h2>
        <h3>High Security Log - '.date('Y-W').'</h3>
        <textarea name="logviewerhigh" cols="75" rows="7">');echo Log::read('high');echo_strip('</textarea><br />
        <br />
        <h3>Medium Security Log - '.date('Y-W').'</h3>
        <textarea name="logviewermed" cols="75" rows="5">');echo Log::read('medium');echo_strip('</textarea><br />
        <br /><h3>Low Security Log - '.date('Y-W').'</h3>
        <textarea name="logviewerlow" cols="75" rows="3">');echo Log::read('low');echo_strip('</textarea><br />
        <br />
        <br />
        <br />
        <h2>RosCMS Generator Log</h2>
        <h3>High Security Log - '.date('Y-W').'</h3>
        <textarea name="logviewerhigh2" cols="75" rows="7">');echo Log::read('high','generate');echo_strip('</textarea><br />
        <br />
        <h3>Medium Security Log - '.date('Y-W').'</h3>
        <textarea name="logviewermed2" cols="75" rows="5">');echo Log::read('medium','generate');echo_strip('</textarea><br />
        <br />
        <h3>Low Security Log - '.date('Y-W').'</h3>
        <textarea name="logviewerlow2" cols="75" rows="3">');echo Log::read('low','generate');echo_strip('</textarea><br />
        <br />
        <br />
        <br />
        <h2>RosCMS Language Group Logs</h2>');

      // language specific logs
      $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages ORDER BY lang_name ASC");
      $stmt->execute();
      while ($language = $stmt->fetch()) {
        echo_strip('
          <h3>'.$language['lang_name'].'</h3>
          <h4>High Security Log - '.date('Y-W').'</h4>
          <textarea name="logviewerhigh'.$language['lang_id'].'" cols="75" rows="5">');echo Log::read('high', $language['lang_id']);echo_strip('</textarea><br />
          <br />
          <h4>Medium Security Log - '.date('Y-W').'</h4>
          <textarea name="logviewermed'.$language['lang_id'].'" cols="75" rows="4">');echo Log::read('medium', $language['lang_id']);echo_strip('</textarea><br />
          <br />
          <h4>Low Security Log - '.date('Y-W').'</h4>
        <textarea name="logviewerlow'.$language['lang_id'].'" cols="75" rows="3">');echo Log::read('low', $language['lang_id']);echo_strip('</textarea><br />
        <br />
        <br />');
      }
    } // end of ros_admin only

  }


} // end of HTML_CMS
?>
