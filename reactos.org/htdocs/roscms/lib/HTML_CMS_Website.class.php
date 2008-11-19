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
 * class HTML_CMS_Website
 * 
 */
class HTML_CMS_Website extends HTML_CMS
{


  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '', $page_css = 'roscms' )
  {
    $this->branch = 'website';

    // register css
    $this->register_css('cms_website.css');
    $this->register_css('cms_website-ie.css','IE');

    $this->register_js('cms_website.js');
    $this->register_js('cms_website.js.php');
    $this->register_js('diffv3.js');
    $this->register_js('../editor/jscripts/tiny_mce/tiny_mce_src.js');
    $this->register_js('../editor/jscripts/mef.js');
    parent::__construct( $page_title, $page_css);
  }


  /**
   *
   *
   * @access protected
   */
  protected function body( )
  {
    global $roscms_intern_account_id;
    global $roscms_intern_login_check_username;
    global $roscms_security_level;
    global $roscms_standard_language;
    global $roscms_standard_language_trans;
    global $roscms_intern_webserver_roscms;
    global $roscms_intern_page_link;

    echo_strip('
      <noscript>
        <h3>RosCMS v3 requires Javascript, please activate/allow it.</h3>
        <p>It does work fine in Internet Explorer 5.5+, Mozilla Firefox 1.5+, Opera 9.1+, Safari 3.2+ and probably every client with basic Javascript (+AJAX) support.</p>
      </noscript>');
    echo '
      <script type="text/javascript">
      <!--'."
        // Navigation Data
        var roscms_current_page = 'new';
        var roscms_prev_page = 'new';
        var roscms_current_tbl_position = 0;
        var roscms_intern_entry_per_page = 25;

        // editor preferences
        var roscms_archive = 0;
        var autosave_coundown = 100000; // 10000; 100000
        var autosave_val = '';
        var autosave_cache = '';
        var autosave_timer;

        //
        var submenu_button = '';
        var nres=1;
        var smenutabs = 12; // sync this value with the tab-menu entry-count !!!
        markedrows = new Array(); //global

        //
        var filtstring1 = '';
        var filtstring2 = '';

        var roscms_page_load_finished = false;

        // map php vars
        var roscms_intern_account_id = ".$roscms_intern_account_id.";
        var roscms_standard_language = '".$roscms_standard_language."';
        var roscms_standard_language_trans = '".$roscms_standard_language_trans."';
        var roscms_intern_login_check_username = '".$roscms_intern_login_check_username."';
        var roscms_intern_webserver_roscms = '".$roscms_intern_webserver_roscms."';
        var roscms_intern_page_link = '".$roscms_intern_page_link."';
        var roscms_get_edit = '".(isset($_GET['edit']) ? $RosCMS_GET_cms_edit : '')."';
        var roscms_access_level = ".$roscms_security_level.";
        var roscms_cbm_hide = '".(($roscms_security_level > 1) ? '' : ' disabled="disabled" style="color:#CCCCCC;"')."'; // disable combobox entries for novice user

        // favorite user language
        ";

    $stmt=DBConnection::getInstance()->prepare("SELECT user_language FROM users WHERE user_id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
    $stmt->execute();
    $user_lang = $stmt->fetchColumn();

    if (!empty($user_language)) {
      echo "var userlang = '".$user_language."';";
    }
    else {
      echo "var userlang = roscms_standard_language;";
    }

    echo "
        var filtercounter = 0;
        var filterid = 0;
        var alertactiv = '';
        
        var timerquickinfo;
        
        var exitmsg = ".'"Click Cancel to continue with RosCMS, click OK to leave RosCMS.\n\nThanks for using RosCMS!"'."
      -->
      </script>";echo_strip('
      <div class="spacer">&nbsp;</div>
      <div style="padding-top: 8px; padding-bottom: 5px;text-align: center">
        <div id="alertb" class="infobox" style="visibility:hidden;">
          <div class="lab1">
            <div class="lab2">
              <div class="lab3">
                <div class="lab4">
                  <div id="alertbc" class="infoboxc">&nbsp;</div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div class="roscms_container" style="border: 1px dashed white; z-index: 2;">
        <div class="tabmenu" style="position: absolute; top: 0px; width: 150px; left: 0px; border: 0px; z-index:1;">
          <div id="smenutab1" class="submb" onclick="smenutab_open(this.id)"'.(($roscms_security_level == 1 || ROSUser::isMemberOfGroup("transmaint")) ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc1" class="subm2" style="font-weight: bold;">New Entry</div>
            </div>
          </div>');

    if ($roscms_security_level > 1) {
      echo '<div style="background: white none repeat scroll 0%;">&nbsp;</div>';
    }

    echo_strip('
          <div id="smenutab2" class="subma" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc2" class="subm2"><b>New</b></div>
            </div>
          </div>

          <div id="smenutab3" class="submb" onclick="smenutab_open(this.id)"'.(($roscms_security_level == 1 || ROSUser::isMemberOfGroup("transmaint")) ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc3" class="subm2">Page</div>
            </div>
          </div>
          <div id="smenutab4" class="submb" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc4" class="subm2">Content</div>
            </div>
          </div>
          <div id="smenutab5" class="submb" onclick="smenutab_open(this.id)"'.(($roscms_security_level == 1 || ROSUser::isMemberOfGroup("transmaint")) ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc5" class="subm2">Template</div>
            </div>
          </div>
          <div id="smenutab6" class="submb" onclick="smenutab_open(this.id)"'.(($roscms_security_level == 1 || ROSUser::isMemberOfGroup("transmaint")) ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc6" class="subm2">Script</div>
            </div>
          </div>
          <div id="smenutab7" class="submb" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc7" class="subm2">Translate</div>
            </div>
          </div>
          <div id="smenutab8" class="submb" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc8" class="subm2">All Entries</div>
            </div>
          </div>
          <div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>

          <div id="smenutab9" class="submb" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc9" class="subm2">Bookmark&nbsp;<img src="images/star_on_small.gif" alt="" style="width:13px; height:13px; border:0px;" /></div>
            </div>
          </div>
          <div id="smenutab10" class="submb" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc10" class="subm2">Drafts</div>
            </div>
          </div>
          <div id="smenutab11" class="submb" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc11" class="subm2">My Entries</div>
            </div>
          </div>
          <div id="smenutab12" class="submb" onclick="smenutab_open(this.id)">
            <div class="subm1">
              <div id="smenutabc12" class="subm2">Archive</div>
            </div>
          </div>
          <div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
          <div class="laba" style="background: #FFD4BC none repeat scroll 0%;">
            <div class="lab1">
              <div class="lab2">
                <div class="lab3">
                  <div class="lab4">
                    <div class="labtitel" id="labtitel1" onclick="TabOpenCloseEx(this.id)">
                      <img id="labtitel1i" src="images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Quick Info
                    </div>
                    <div class="labcontent" id="labtitel1c" style="display:block;">
                      <div id="qiload" style="display:none; text-align:right;" class="lablinks"><img src="images/ajax_loading.gif" alt="loading ..." width="13" height="13" /></div>
                      <div id="lablinks1" class="lablinks2"><span style="color:#FF6600;">Move the mouse over an item to get some details</span></div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
          <div class="laba" style="background: #B5EDA3 none repeat scroll 0%;">
            <div class="lab1">
              <div class="lab2">
                <div class="lab3">
                  <div class="lab4">
                    <div class="labtitel" id="labtitel2" onclick="TabOpenCloseEx(this.id)">
                      <img id="labtitel2i" src="images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Smart Filters
                    </div>
                    <div class="labcontent" id="labtitel2c">
                      <div id="lablinks2" class="lablinks2" style="color:#009900;">&nbsp;</div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div style="background: #FFFFFF none repeat scroll 0%;">&nbsp;</div>
          <div class="laba" style="background: #DFD3EC none repeat scroll 0%;">
            <div class="lab1">
              <div class="lab2">
                <div class="lab3">
                  <div class="lab4">
                    <div class="labtitel" id="labtitel3" onclick="TabOpenCloseEx(this.id)">
                      <img id="labtitel3i" src="images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Labels
                    </div>
                    <div class="labcontent" id="labtitel3c">
                      <div id="lablinks3" class="lablinks2" style="color:#8868AC;">&nbsp;</div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>

        <div class="style1" id="impcont" style="margin-left: 150px; z-index:100;">
          <div class="bubble_bg">
            <div class="rounded_ll">
              <div class="rounded_lr">
                <div class="rounded_ul">
                  <div class="rounded_ur">
                    <div class="bubble" id="bub">
                      <div id="frametable" style="border: 0px dashed white;">
                        <div class="filterbar">
                          <input id="txtfind" type="text" accesskey="f" tabindex="1" title="Search &amp; Filters" onfocus="'."textbox_hint(this.id, this.value, 'Search &amp; Filters', 1)".'" onblur="'."textbox_hint(this.id, this.value, 'Search &amp; Filters', 0)".'" onkeyup="filtsearchbox()" value="Search &amp; Filters" size="39" maxlength="250" class="tfind" />&nbsp;
                          <span id="filters" class="filterbutton" onclick="TabOpenClose(this.id)"><img id="filtersi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Filters</span>&nbsp;
                          <div id="filtersc" style="display:none;">
                            <div id="filtersct">&nbsp;</div>
                            <div id="filt2" class="filterbar2">
                              <span class="filterbutton" onclick="filtadd()"><img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add</span>
                              &nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="filtentryclear2()"><img src="images/clear.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Clear</span>
                              &nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="'."add_user_filter('filter', filtstring2)".'"><img src="images/save.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Save</span>
                              &nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="filtsearch()"><img src="images/search.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Search</span>
                            </div>
                          </div>');echo '
                          <script type="text/javascript">
                          <!--
                            // add first filter entry (default)
                            filtentryclear(); 
                          -->
                          </script>';echo_strip('
                        </div>
                      <div style="border: 0px dashed red; position: absolute; top: 9px; right: 13px; text-align:right; white-space: nowrap;">
                        <select name="favlangopt" id="favlangopt" style="vertical-align: top; width: 22ex;" onchange="setlang(this.value)">');

    $user_lang = ROSUser::getLanguage($roscms_intern_account_id, true);

    $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages WHERE lang_level > '0' ORDER BY lang_name ASC");
    $stmt->execute();
    while($language=$stmt->fetch()) {
      echo '<option value="'.$language['lang_id'].'"';

      if ($language['lang_id'] == $user_lang) {
        echo ' selected="selected"';
      }

      echo '>'.$language['lang_name'].'</option>';
    }

    echo_strip('
                        </select>
                      </div>
                      <div id="tablecmdbar" style="padding-top: 5px;"></div>
                        <div style="border: 0px dashed red; position: absolute; right: 10px; text-align:right; white-space: nowrap;">
                          <span id="mtblnav">&nbsp;</span>
                        </div>
                        <div class="tabselect">Select: <span id="tabselect1"></span></div>
                        <div id="tablist">&nbsp;</div>
                        <div style="border: 0px dashed red; position: absolute; right: 10px; text-align:right; white-space: nowrap;">
                          <span id="mtbl2nav">&nbsp;</span>
                        </div>
                        <div class="tabselect">Select: <span id="tabselect2"></span></div>
                      </div>
                      <div id="frameedit" style="display: block; border: 0px dashed red; ">

                        <div id="frmedithead" style="padding-bottom: 10px;">&nbsp;</div>
                        <div style="width:100%;">
                          <div id="editzone">&nbsp;</div>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
          <br />
          <p><strong>Table Legend</strong></p>
          <table id="legend" cellspacing="5">
            <tr>
              <td class="lbox" style="background-color:#ddd">&nbsp;</td>
              <td style="width:205px" rowspan="2">
                Standard<br />
                <span class="style2">(odd/even)</span>
              </td>
              <td style="width:50px" rowspan="5">&nbsp;</td>
              <td class="lbox" style="background-color:#ffcc99">&nbsp;</td>
              <td style="width:205px">Selected</td>
            </tr>
            <tr>
              <td class="lbox" style="background-color:#eeeeee">&nbsp;</td>
              <td class="lbox" style="background-color:#A3EDB4">&nbsp;</td>
              <td>Translation  up to date</td>
            </tr>
            <tr>
              <td class="lbox" style="background-color:#B5EDA3">&nbsp;</td>
              <td>New</td>
              <td class="lbox" style="background-color:#D6CAE4">&nbsp;</td>
              <td>Translation doesn\'t exist</td>
            </tr>
            <tr>
              <td class="lbox" style="background-color:#FFE4C1">&nbsp;</td>
              <td>Draft</td>
              <td class="lbox" style="background-color:#FAA5A5">&nbsp;</td>
              <td>Translation  outdated</td>
            </tr>
            <tr>
              <td class="lbox" style="background-color:#ffffcc">&nbsp;</td>
              <td>Mouse hover</td>
              <td class="lbox" style="background-color:#FFCCFF">&nbsp;</td>
              <td>Unknown</td>
            </tr>
          </table>
        </div>
      </div>
      <br />
      <script type="text/javascript" src="'.$roscms_intern_webserver_roscms.'js/cms_website-init.js"></script>');
  }


} // end of HTML_CMS_Website
?>
