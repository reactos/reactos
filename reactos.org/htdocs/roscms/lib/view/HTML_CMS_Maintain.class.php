<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>
                  2009  Danny Götte <dangerground@web.de>

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
 * @package html
 * @subpackage cms
 */
class HTML_CMS_Maintain extends HTML_CMS
{


  /**
   * setup branch info, register css/js, check for access
   *
   * @access public
   */
  public function __construct( )
  {
    Login::required();

    $this->branch = 'maintain';

    // register css & js files
    $this->register_css('cms_maintain.css');
    $this->register_js('cms_maintain.js');
    $this->register_js('cms_maintain.js.php');

    // check if user has rights for this area
    if (!ThisUser::getInstance()->hasAccess('maintain')) {
      die('Not enough rights to get into this area');
    }

    parent::__construct();
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
        var roscms_intern_webserver_roscms = '".RosCMS::getInstance()->pathRosCMS()."';

        // access restrictions
        var roscms_access = new Object();
            roscms_access['more_filter'] = ".($thisuser->hasAccess('more_filter') ? 'true' : 'false').";
            roscms_access['admin_filter'] = ".($thisuser->hasAccess('admin_filter') ? 'true' : 'false').";
            roscms_access['dont_hide_filter'] = ".($thisuser->hasAccess('dont_hide_filter') ? 'true' : 'false').";
            roscms_access['more_lang'] = ".($thisuser->hasAccess('more_lang') ? 'true' : 'false').";
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

          <div id="lmUser" class="lmItemTop" onclick="loadUserSearch()"'.(!$thisuser->hasAccess('user') ? ' style="display: none;"' : '').'>
            <div class="lmItemBottom">User Search</div>
          </div>
          <div id="lmGenerate" class="lmItemTop" onclick="loadGenerate()"'.(!$thisuser->hasAccess('maintain') ? ' style="display: none;"' : '').'>
            <div class="lmItemBottom">Generate</div>
          </div>
          <div id="lmLogs" class="lmItemTop" onclick="loadLogs()"'.(!$thisuser->hasAccess('logs') ? ' style="display: none;"' : '').'>
            <div class="lmItemBottom">Logs</div>
          </div>

          <div id="lmAccess" class="lmItemTop" onclick="loadAccess()"'.(!$thisuser->hasAccess('admin') ? ' style="display: none;"' : '').'>
            <div class="lmItemBottom">per Entry Access</div>
          </div>

          <div id="lmSystem" class="lmItemTop" onclick="loadSystem()"'.(!$thisuser->hasAccess('admin') ? ' style="display: none;"' : '').'>
            <div class="lmItemBottom">systemwide Access</div>
          </div>
          <div id="lmGroups" class="lmItemTop" onclick="loadGroups()"'.(!$thisuser->hasAccess('admin') ? ' style="display: none;"' : '').'>
            <div class="lmItemBottom">Groups</div>
          </div>
          <div id="lmLanguages" class="lmItemTop" onclick="loadLanguages()"'.(!$thisuser->hasAccess('admin') || !RosCMS::getInstance()->multiLanguage() ? ' style="display: none;"' : '').'>
            <div class="lmItemBottom">Languages</div>
          </div>

          <div class="leftBubble" id="userfilter">
            <div class="corner_TL">
              <div class="corner_TR"></div>
            </div>
            <div class="title" id="userfilter2" onclick="TabOpenCloseEx(this.id)">
              <img id="userfilter2i" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_open.gif" alt="" style="width:11px; height:11px;" />&nbsp;Search Filters
            </div>
            <div id="userfilter2c" class="content">&nbsp;</div>
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

              <div id="pageUser" style="display:block;">
                <div id="pageUserTable">
                  <input type="text" name="searchfield" id="searchfield" onkeyup="getUser()" onfocus="clearSearchBox(this)" onblur="clearSearchBox(this)" value="search user" /> 
                  <span class="virtualButton" onclick="getUser()"><img src="'.RosCMS::getInstance()->pathRosCMS().'images/search.gif" alt="" style="width:14px; height:14px;" />&nbsp;Search</span>
                  <br />
                  <span id="filters" class="virtualButton" onclick="TabOpenClose(this.id)"><img id="filtersi" src="'.RosCMS::getInstance()->pathRosCMS().'images/tab_closed.gif" alt="" style="width:11px; height:11px;" />&nbsp;Show Filters</span>
                  <div id="filtersc" style="display:none;">
                    <div id="filtersct"></div>
                    <div id="filterOptionsfilt2" class="filterbar2">
                      <span class="virtualButton" onclick="addFilter()">
                        <img src="'.RosCMS::getInstance()->pathRosCMS().'images/add.gif" alt="" />
                        &nbsp;Add
                      </span>
                      <span class="virtualButton" onclick="clearAllFilter()">
                        <img src="'.RosCMS::getInstance()->pathRosCMS().'images/clear.gif" alt=""  />
                        &nbsp;Clear
                      </span>
                      <span class="virtualButton" onclick="'."addUserFilter(filtstring2)".'">
                        <img src="'.RosCMS::getInstance()->pathRosCMS().'images/save.gif" alt="" />
                        &nbsp;Save
                      </span>
                    </div>
                  </div>

                  <div style="text-align: right; white-space: nowrap;padding-top: 1em;">
                    <span id="mtblnav">&nbsp;</span>
                  </div>
                  <div id="usertable"></div>
                  <div style="text-align: right; white-space: nowrap;padding-top: 1em;">
                    <span id="mtbl2nav">&nbsp;</span>
                  </div>
                </div>

                <div id="pageUserDetails"></div>
              </div>


              <div id="pageGenerate" style="display:none;">
                <div class="toolbar">
                  <div class="button" onclick="optimizeDB()">
                    <img src="'.RosCMS::getInstance()->pathRosCMS().'images/clear.gif" alt="" />
                    <span class="text">Optimize Tables</span>
                  </div>
                  <div class="button" onclick="rebuildDependencies()">
                    <img src="'.RosCMS::getInstance()->pathRosCMS().'images/tool.gif" alt="" />
                    <span class="text">Rebuild Dependency Tree</span>
                  </div>
                  <div class="button" onclick="generateAllPages()">
                    <img src="'.RosCMS::getInstance()->pathRosCMS().'images/active.gif" alt="" />
                    <span class="text">Generate All</span>
                  </div>
                </div>
                <br style="clear: both;" />
                <br />

                <div>
                  <label for="textfield">Entry-Name:</label>
                  <input name="textfield" type="text" id="textfield" size="20" maxlength="100" />
                  <select id="txtaddentrytype" name="txtaddentrytype">
                    <option value="page" selected="selected">Page</option>
                    <option value="dynamic">Dynamic Page</option>
                    <option value="content">Content</option>
                    <option value="script">Script</option>
                    <option value="system">System</option>
                  </select>
                  <select id="txtaddentrylang" name="txtaddentrylang">');

              // display languages
              if ($thisuser->hasAccess('more_lang')) {
                $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." ORDER BY name ASC");
              }
              else {
                $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE id IN(:standard_lang,:lang_id) ORDER BY name ASC");
                $stmt->bindParam('standard_lang',Language::getStandardId(),PDO::PARAM_INT);
                $stmt->bindParam('lang_id',$thisuser->language(),PDO::PARAM_INT);
              }
              $stmt->execute();
              while ($language=$stmt->fetch(PDO::FETCH_ASSOC)) {
                echo '<option value="'.$language['id'].'"'.($language['id']==Language::getStandardId() ? ' selected="selected"' : '').'>'.$language['name'].'</option>';
              }
              echo_strip('
                  </select>
                  <button name="entryupdate" onclick="generatePage()">generate</button>
                </div>

                <div id="maintainarea">Nothing done yet.</div>
                <br />
              </div>
              
              <div id="pageLogs" style="display:none">
              
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
          $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." ORDER BY name ASC");
          $stmt->execute();
          while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {
            $low = Log::read('low', $language['id']);
            $medium = Log::read('medium', $language['id']);
            $high = Log::read('high', $language['id']);

            if ($low != '' || $medium != '' || $high != '') {
              echo '<h3>'.$language['name'].'</h3>';
              
              if ($low != '') {
                echo '
                <h4>Low Security Log - '.date('Y-W').'</h4>
                <textarea name="logviewerhigh'.$language['id'].'" cols="75" rows="5">'.$low.'</textarea><br />
                <br />';
              }
              if ($medium != '') {
                echo '
                <h4>Medium Security Log - '.date('Y-W').'</h4>
                <textarea name="logviewerhigh'.$language['id'].'" cols="75" rows="5">'.$medium.'</textarea><br />
                <br />';
              }
              if ($high != '') {
                echo '
                <h4>High Security Log - '.date('Y-W').'</h4>
                <textarea name="logviewerhigh'.$language['id'].'" cols="75" rows="5">'.$high.'</textarea><br />
                <br />';
              }
            }
          }
          echo_strip('
              </div>

              <div id="pageAccess" style="display: none;">
                <div class="toolbar">
                  <div class="button" onclick="addAccess()">
                    <img src="'.RosCMS::getInstance()->pathRosCMS().'images/clear.gif" alt="" />
                    <span class="text">add new</span>
                  </div>
                </div>
                <br style="clear: both;" />
                <div id="accessList" style="min-height:200px;background-color: white;">
                  <table class="roscmsTable">
                    <thead>
                      <th>&nbsp;</th>
                      <th>name</th>
                      <th>description</th>
                    </thead>
                    <tbody id="accessListBody">
                    </tbody>
                  </table>
                </div>
                <div id="accessDetails"></div>
              </div>

              <div id="pageSystem" style="display: none;">
              </div>

              <div id="pageGroups" style="display: none;">
                <div class="toolbar">
                  <div class="button" onclick="addGroup()">
                    <img src="'.RosCMS::getInstance()->pathRosCMS().'images/clear.gif" alt="" />
                    <span class="text">add new</span>
                  </div>
                </div>
                <br style="clear: both;" />
                <div id="groupList" style="min-height:200px;background-color: white;">
                  <table class="roscmsTable">
                    <thead>
                      <th>&nbsp;</th>
                      <th>name</th>
                      <th>description</th>
                    </thead>
                    <tbody id="groupListBody">
                    </tbody>
                  </table>
                </div>
                <div id="groupDetails"></div>
              </div>

              <div id="pageLanguages" style="display: none;">
                <div class="toolbar">
                  <div class="button" onclick="addLanguage()">
                    <img src="'.RosCMS::getInstance()->pathRosCMS().'images/clear.gif" alt="" />
                    <span class="text">add new</span>
                  </div>
                </div>
                <br style="clear: both;" />
                <div id="languageList" style="min-height:200px;background-color: white;">
                  <table class="roscmsTable">
                    <thead>
                      <th>&nbsp;</th>
                      <th>name</th>
                      <th>native name</th>
                    </thead>
                    <tbody id="languageListBody">
                    </tbody>
                  </table>
                </div>
                <div id="languageDetails"></div>
              </div>
            </div>


            <div class="corner_BL">
              <div class="corner_BR"></div>
            </div>
          </div>
        </div>

      </div>
      <script type="text/javascript" src="'.RosCMS::getInstance()->pathRosCMS().'js/cms_maintain-init.js"></script>');
  } // end of member function body



} // end of HTML_CMS_Maintain
?>
