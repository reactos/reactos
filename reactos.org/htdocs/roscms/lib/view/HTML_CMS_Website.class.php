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
 * @package html
 * @subpackage cms
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
    Login::required();

    $this->branch = 'website';

    // register css
    $this->register_css('cms_website.css');
    $this->register_css('cms_website-ie.css','IE');

    $this->register_js('diff.js');
    $this->register_js('cms_website.js');
    $this->register_js('cms_website.js.php');
    $this->register_js('mef.js');
    $this->register_js('editor/tiny_mce_src.js');

    // check if user has rights to view this branch
    if (!ThisUser::getInstance()->hasAccess('website')) {
      die('Not enough rights to get into this area');
    }

    parent::__construct( $page_title, $page_css);
  } // end of constructor



  protected function body( )
  {
    $thisuser = &ThisUser::getInstance();

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
        var roscms_archive = false;
        var autosave_coundown = 100000; // 10000; 100000
        var autosave_val = '';
        var autosave_cache = '';
        var autosave_timer;

        //
        var submenu_button = false;
        var nres=1;
        var smenutabs = 13; // sync this value with the tab-menu entry-count !!!

        //
        var filtstring1 = '';
        var filtstring2 = '';

        var roscms_page_load_finished = false;

        // map php vars
        var roscms_intern_account_id = ".$thisuser->id().";
        var roscms_standard_language = '".Language::getStandardId()."';
        var roscms_intern_login_check_username = '".$thisuser->name()."';
        var roscms_intern_webserver_roscms = '".RosCMS::getInstance()->pathRosCMS()."';
        var roscms_intern_page_link = '".RosCMS::getInstance()->pathInstance()."?page=';
        var roscms_get_edit = '".(isset($_GET['edit']) ? $_GET['edit'] : '')."';
        
        // access restrictions
        var roscms_access = new Object();
            roscms_access['more_filter'] = ".($thisuser->hasAccess('more_filter') ? 'true' : 'false').";
            roscms_access['admin_filter'] = ".($thisuser->hasAccess('admin_filter') ? 'true' : 'false').";
            roscms_access['dont_hide_filter'] = ".($thisuser->hasAccess('dont_hide_filter') ? 'true' : 'false').";
            roscms_access['make_stable'] = ".($thisuser->hasAccess('make_stable') ? 'true' : 'false').";
            roscms_access['del_entry'] = ".($thisuser->hasAccess('del_entry') ? 'true' : 'false').";

        // favorite user language
        ";

    // set user language for js
    $user_lang = $thisuser->language();
    if (!empty($user_lang)) {
      echo "var userlang = '".$user_lang."';";
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
      <div id="alertbox">
        <div class="corner_TL">
          <div class="corner_TR"></div>
        </div>
        <div id="alertboxc">&nbsp;</div>
        <div class="corner_BL">
          <div class="corner_BR"></div>
        </div>
      </div>

      <div id="roscms_container">
        <div class="leftMenu" style="position: absolute; top: 0px; width: 150px; left: 0px; border: 0px; z-index:1;">
          <div id="smenutab1" class="submb" style="margin-bottom: 1.5em;" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('new_entry') ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc1" class="subm2" style="font-weight: bold;">New Entry</div>
            </div>
          </div>

          <div id="smenutab2" class="subma" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc2" class="subm2"><b>New</b></div>
            </div>
          </div>

          <div id="smenutab3" class="submb" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('pages') ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc3" class="subm2">Page</div>
            </div>
          </div>
          <div id="smenutab13" class="submb" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('dynamic_pages') ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc13" class="subm2">Dynamic&nbsp;Page</div>
            </div>
          </div>
          <div id="smenutab4" class="submb" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc4" class="subm2">Content</div>
            </div>
          </div>
          <div id="smenutab5" class="submb" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('templates') ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc5" class="subm2">Template</div>
            </div>
          </div>
          <div id="smenutab6" class="submb" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('scripts') ? ' style="display:none;"' : '').'>
            <div class="subm1">
              <div id="smenutabc6" class="subm2">Script</div>
            </div>
          </div>
          <div id="smenutab7" class="submb" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc7" class="subm2">Translate</div>
            </div>
          </div>
          <div id="smenutab8" class="submb" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc8" class="subm2">All Entries</div>
            </div>
          </div>
          <br />

          <div id="smenutab9" class="submb" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc9" class="subm2">Bookmark&nbsp;<img src="'.RosCMS::getInstance()->pathRosCMS().'images/star_on_small.gif" alt="" style="width:13px; height:13px; border:0px;" /></div>
            </div>
          </div>
          <div id="smenutab10" class="submb" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc10" class="subm2">Drafts</div>
            </div>
          </div>
          <div id="smenutab11" class="submb" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc11" class="subm2">My Entries</div>
            </div>
          </div>
          <div id="smenutab12" class="submb" onclick="loadMenu(this.id)">
            <div class="subm1">
              <div id="smenutabc12" class="subm2">Archive</div>
            </div>
          </div>
          
          <div class="leftBubble" id="quickinfo">
            <div class="corner_TL">
              <div class="corner_TR"></div>
            </div>
            <div class="title" id="labtitel1" onclick="TabOpenCloseEx(this.id)">
              <img id="labtitel1i" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Quick Info
            </div>
            <div class="content" id="labtitel1c" style="display:block;">
              <div id="qiload" style="display:none;">
                <img src="'.RosCMS::getInstance()->pathRosCMS().'images/ajax_loading.gif" alt="loading ..." width="13" height="13" />
              </div>
              <div id="lablinks1" class="text">
                <span>Move the mouse over an item to get some details</span>
              </div>
            </div>
            <div class="corner_BL">
              <div class="corner_BR"></div>
            </div>
          </div>

          <div class="leftBubble" id="smartfilter">
            <div class="corner_TL">
              <div class="corner_TR"></div>
            </div>
            <div class="title" id="labtitel2" onclick="TabOpenCloseEx(this.id)">
              <img id="labtitel2i" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Smart Filters
            </div>
            <div class="content" id="labtitel2c">&nbsp;</div>
            <div class="corner_BL">
              <div class="corner_BR"></div>
            </div>
          </div>

          <div class="leftBubble" id="userlabel">
            <div class="corner_TL">
              <div class="corner_TR"></div>
            </div>
            <div class="title" id="labtitel3" onclick="TabOpenCloseEx(this.id)">
              <img id="labtitel3i" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_open.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Labels
            </div>
            <div class="content" id="labtitel3c">&nbsp;</div>
            <div class="corner_BL">
              <div class="corner_BR"></div>
            </div>
          </div>
        </div>

        <div class="style1" id="impcont" style="margin-left: 150px; z-index:100;">
          <div id="bubble_bg">
            <div class="corner_TL">
              <div class="corner_TR"></div>
            </div>
            <div id="bubble">
              <div id="frametable" style="border: 0px dashed white;">
                <div class="filterbar">
                  <input id="txtfind" type="text" accesskey="f" tabindex="1" title="Search &amp; Filters" onfocus="'."searchFilter(this.id, this.value, 'Search &amp; Filters', true)".'" onblur="'."searchFilter(this.id, this.value, 'Search &amp; Filters', false)".'" onkeyup="getAllActiveFilters()" value="Search &amp; Filters" size="39" maxlength="250" />&nbsp;
                  <span id="filters" class="filterbutton" onclick="TabOpenClose(this.id)"><img id="filtersi" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Filters</span>&nbsp;
                  <div id="filtersc" style="display:none;">
                    <div id="filtersct">&nbsp;</div>
                    <div id="filterOptionsfilt2" class="filterbar2">
                      <span class="filterbutton" onclick="addFilter()"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add</span>
                      &nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="clearAllFilter()"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/clear.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Clear</span>
                      &nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="'."addUserFilter('filter', filtstring2)".'"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/save.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Save</span>
                      &nbsp;&nbsp;&nbsp;<span class="filterbutton" onclick="searchByFilters()"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/search.gif" alt="" style="width:14px; height:14px; border:0px;" />&nbsp;Search</span>
                    </div>
                  </div>
                </div>
                <div style="border: 0px dashed red; position: absolute; top: 9px; right: 13px; text-align:right; white-space: nowrap;">
                  <select name="favlangopt" id="favlangopt" style="vertical-align: top; width: 22ex;" onchange="setLang(this.value)">');

    // preselect current user language
    if ($thisuser->hasAccess('more_lang')) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE level > 0 ORDER BY name ASC");
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE id IN(:lang_id,:standard_lang)");
      $stmt->bindParam('lang_id',$thisuser->language(),PDO::PARAM_INT);
      $stmt->bindParam('standard_lang',Language::getStandardId(),PDO::PARAM_INT);
    }
    $stmt->execute();
    while($language=$stmt->fetch()) {
      echo '<option value="'.$language['id'].'"';

      if ($language['id'] == $user_lang) {
        echo ' selected="selected"';
      }

      echo '>'.$language['name'].'</option>';
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
                  <div id="frmdiff"></div>
                </div>
              </div>
              <div id="previewarea" style="display:none;">
                <div id="previewhead" style="padding-bottom: 10px;">&nbsp;</div>
                <div id="previewzone">&nbsp;</div>
              </div>
              <div id="newentryarea" style="display:none;">
                <div id="newentryhead" style="padding-bottom: 10px;">&nbsp;</div>
                <div id="newentryzone">&nbsp;</div>
              </div>

            </div>
            <div class="corner_BL">
              <div class="corner_BR"></div>
            </div>
          </div>
          <br />
          <table id="legend" cellspacing="5">
            <caption style="font-weight:bold;text-align:left;">Table Legend</caption>
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
              <td>Translation up to date</td>
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
              <td>Translation outdated</td>
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
      <script type="text/javascript" src="'.RosCMS::getInstance()->pathRosCMS().'js/cms_website-init.js"></script>');
  } // end of member function body



} // end of HTML_CMS_Website
?>
