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

  protected function navigation() {
    echo '<div id="tooltip" style="display: none;"></div>';
    parent::navigation();
  }


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

        // editor preferences
        var roscms_archive = false;
        var autosave_coundown = 100000; // 10000; 100000
        var autosave_val = '';
        var autosave_cache = '';
        var autosave_timer;

        //
        var smenutabs = 12; // sync this value with the tab-menu entry-count !!!

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
            roscms_access['del_entry'] = ".($thisuser->hasAccess('del_entry') ? 'true' : 'false').";

        // favorite user language
        var userlang = ".$thisuser->language().";

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
        <div class="leftMenu" style="position: absolute; top: 0px; width: 150px; left: 0px; z-index:1;">
          <div id="smenutab1" class="lmItemTop" onclick="loadMenu(this.id)" style="margin-bottom: 1.5em;'.(!$thisuser->hasAccess('new_entry') ? 'display:none;' : '').'">
            <div id="smenutabc1" class="lmItemBottom" style="font-weight: bold;">Create new entry</div>
          </div>

          <div style="margin-bottom: 1.5em;'.(!$thisuser->hasAccess('pages') && !$thisuser->hasAccess('dynamic_pages') && !$thisuser->hasAccess('scripts') ? 'display:none;' : '').'">
            <div id="smenutab3" class="lmItemTop" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('pages') ? ' style="display:none;"' : '').'>
              <div id="smenutabc3" class="lmItemBottom">Page</div>
            </div>
            <div id="smenutab5" class="lmItemTop" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('dynamic_pages') ? ' style="display:none;"' : '').'>
              <div id="smenutabc5" class="lmItemBottom">Dynamic&nbsp;Page</div>
            </div>
            <div id="smenutab4" class="lmItemTop" onclick="loadMenu(this.id)">
              <div id="smenutabc4" class="lmItemBottom">Content</div>
            </div>
            <div id="smenutab6" class="lmItemTop" onclick="loadMenu(this.id)"'.(!$thisuser->hasAccess('scripts') ? ' style="display:none;"' : '').'>
              <div id="smenutabc6" class="lmItemBottom">Script</div>
            </div>
          </div>

          <div id="smenutab2" class="lmItemTop" onclick="loadMenu(this.id)">
            <div id="smenutabc2" class="lmItemBottom">Pending</div>
          </div>
          <div id="smenutab7" class="lmItemTop" onclick="loadMenu(this.id)"'.($thisuser->language() == Language::getStandardId() ? ' style="display:none;"' : '').'>
            <div id="smenutabc7" class="lmItemBottom">Translate</div>
          </div>
          <div id="smenutab8" class="lmItemTopSelected" onclick="loadMenu(this.id)">
            <div id="smenutabc8" class="lmItemBottom">All Entries</div>
          </div>
          <br />

          <div id="smenutab9" class="lmItemTop" onclick="loadMenu(this.id)">
            <div id="smenutabc9" class="lmItemBottom">
              Bookmark&nbsp;
              <img src="'.RosCMS::getInstance()->pathRosCMS().'images/star_on_small.gif" alt="" style="width:13px; height:13px;" />
            </div>
          </div>
          <div id="smenutab10" class="lmItemTop" onclick="loadMenu(this.id)">
            <div id="smenutabc10" class="lmItemBottom">Drafts</div>
          </div>
          <div id="smenutab11" class="lmItemTop" onclick="loadMenu(this.id)">
            <div id="smenutabc11" class="lmItemBottom">My Entries</div>
          </div>
          <div id="smenutab12" class="lmItemTop" onclick="loadMenu(this.id)">
            <div id="smenutabc12" class="lmItemBottom">Archive</div>
          </div>

          <div class="leftBubble" id="smartfilter">
            <div class="corner_TL">
              <div class="corner_TR"></div>
            </div>
            <div class="title" id="labtitel2" onclick="TabOpenCloseEx(this.id)">
              <img id="labtitel2i" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_open.gif" alt="" style="width:11px; height:11px;" />&nbsp;Smart Filters
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
              <img id="labtitel3i" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_open.gif" alt="" style="width:11px; height:11px;" />&nbsp;Labels
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
              <div id="frametable">
                <div class="filterbar">
                  <input id="txtfind" type="text" accesskey="f" tabindex="1" title="Searchbox" onfocus="clearSearchBox(this)" onblur="clearSearchBox(this)" onkeyup="getAllActiveFilters()" value="Enter name to search for" size="39" maxlength="250" />
                  <span class="virtualButton" onclick="searchByFilters()"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/search.gif" alt="" style="width:14px; height:14px;" />&nbsp;Search</span>
                  <br />
                  <span id="filters" class="virtualButton" onclick="TabOpenClose(this.id)"><img id="filtersi" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_closed.gif" alt="" style="width:11px; height:11px;" />&nbsp;Show Filters</span>&nbsp;
                  <div id="filtersc" style="display:none;">
                    <div id="filtersct">&nbsp;</div>
                    <div id="filterOptionsfilt2" class="filterbar2">
                      <span class="virtualButton" onclick="addFilter()"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/add.gif" alt="" style="width:11px; height:11px; " />&nbsp;Add</span>
                      &nbsp;&nbsp;&nbsp;<span class="virtualButton" onclick="clearAllFilter()"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/clear.gif" alt="" style="width:14px; height:14px;" />&nbsp;Clear</span>
                      &nbsp;&nbsp;&nbsp;<span class="virtualButton" onclick="'."addUserFilter(filtstring2)".'"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/save.gif" alt="" style="width:14px; height:14px;" />&nbsp;Save</span>
                    </div>
                  </div>
                </div>');

    if (RosCMS::getInstance()->multiLanguage()) {
      echo_strip('
                <div style="position: absolute; top: 9px; right: 13px; text-align:right; white-space: nowrap;">
                  <label for="favlangopt" style="float:left;color: #666;">Working language:</label><br />
                  <select name="favlangopt" id="favlangopt" style="vertical-align: top; width: 22ex;" onchange="setLang(this.value)">');

      // preselect current user language
      if ($thisuser->hasAccess('more_lang') ) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE level > 0 ORDER BY name ASC");
      }
      else {
        $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE id IN(:lang_id,:standard_lang) ORDER BY name ASC");
        $stmt->bindParam('lang_id',$thisuser->language(),PDO::PARAM_INT);
        $stmt->bindParam('standard_lang',Language::getStandardId(),PDO::PARAM_INT);
      }
      $stmt->execute();
      while($language=$stmt->fetch()) {
        echo '<option value="'.$language['id'].'"';

        if ($language['id'] == $thisuser->language()) {
          echo ' selected="selected"';
        }

        echo '>'.$language['name'].'</option>';
      }

      echo_strip('
          </select>
        </div>');
    }

    echo_strip('
                <div style="float: right; white-space: nowrap;padding-top: 1em;">
                  <span id="mtblnav">&nbsp;</span>
                </div>
                <div class="toolbar">
                  <div class="button" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)">
                    <img src="'.RosCMS::getInstance()->pathRosCMS().'images/reload.gif" alt="" />
                    <span class="text">Refresh</span>
                  </div>
                  <div id="toolbarExtension"></div> 
                  <div style="padding: 15px 0px 0px 5px;" id="tabselect1"></div>
                </div>
                <div id="tablist" style="clear: left;">&nbsp;</div>
                <div style="position: absolute; right: 10px; text-align:right; white-space: nowrap;">
                  <span id="mtbl2nav">&nbsp;</span>
                </div>
                <div class="tabselect"><span id="tabselect2"></span></div>

                <span id="legend" class="virtualButton" onclick="TabOpenCloseEx(this.id)">
                  <img id="legendi" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_'.((isset($_COOKIE['legend']) && $_COOKIE['legend']) ? 'open' : 'closed').'.gif" alt="" style="width:11px; height:11px;" /> Show legend
                </span>
                <div id="legendc" style="display:'.((isset($_COOKIE['legend']) && $_COOKIE['legend']) ? 'block' : 'none').';">
                  
                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#dddddd;margin-right: 0px; width: 8px;border-right: none;">&nbsp;</span>
                    <span class="lbox" style="background-color:#eeeeee;margin-left: 0px;border-left: none; width: 8px;">&nbsp;</span>
                    Published (odd/even)
                  </div>
                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#A3EDB4">&nbsp;</span>
                    Translation up to date
                  </div>
                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#ffcc99">&nbsp;</span>
                    Selected
                  </div>

                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#B5EDA3">&nbsp;</span>
                    Pending
                  </div>
                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#D6CAE4">&nbsp;</span>
                    Translation doesn\'t exist
                  </div>
                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#FFCCFF">&nbsp;</span>
                    Unknown
                  </div>

                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#FFE4C1">&nbsp;</span>
                    Draft
                  </div>
                  <div style="width: 33%; float: left;">
                    <span class="lbox" style="background-color:#FAA5A5">&nbsp;</span>
                    Translation outdated
                  </div>

                  <br style="clear: both;" />
                </div>
              </div>
              <div id="frameedit" style="display: block;">

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
        </div>
      </div>
      <script type="text/javascript" src="'.RosCMS::getInstance()->pathRosCMS().'js/cms_website-init.js"></script>');
  } // end of member function body



} // end of HTML_CMS_Website
?>
