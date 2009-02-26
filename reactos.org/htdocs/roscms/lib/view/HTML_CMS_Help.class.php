<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2009  Klemens Friedl <frik85@reactos.org>

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
 * class HTML_CMS_Help
 * 
 * @package html
 * @subpackage cms
 */
class HTML_CMS_Help extends HTML_CMS
{


  /**
   * setup branch info, register css/js, check for access
   *
   * @access public
   */
  public function __construct( )
  {
    Login::required();

    $this->branch = 'help';

    // register css & js files
    $this->register_css('cms_bubble.css');
    $this->register_css('cms_help.css');
    $this->register_js('cms_help.js');

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
      <div id="roscms_container">
        <div class="leftMenu" style="position: absolute; top: 0px; width: 150px; left: 0px; z-index:1;">

          <div id="lmInfo" class="lmItemTopSelected" onclick="loadInfo()">
            <div class="lmItemBottom">Info</div>
          </div>
          <div id="lmFirst" class="lmItemTop" onclick="loadFirstSteps()">
            <div class="lmItemBottom">First Steps</div>
          </div>
          <div id="lmFAQ" class="lmItemTop" onclick="loadFAQ()">
            <div class="lmItemBottom">FAQ</div>
          </div>
          
          <div id="lmEntryTable" class="lmItemTop" onclick="loadEntryTable()">
            <div class="lmItemBottom">Entry Table</div>
          </div>
          <div id="lmWebEditor" class="lmItemTop" onclick="loadWebEditor()">
            <div class="lmItemBottom">Website Editor</div>
          </div>
          <div id="lmMaintain" class="lmItemTop" onclick="loadMaintain()">
            <div class="lmItemBottom">Maintain</div>
          </div>
          <div id="lmAdmin" class="lmItemTop" onclick="loadAdmin()">
            <div class="lmItemBottom">Administration</div>
          </div>
        </div>

        <div class="style1" id="impcont" style="margin-left: 150px; z-index:100;">
          <div id="bubble_bg">
            <div class="corner_TL">
              <div class="corner_TR"></div>
            </div>
            <div id="bubble">

              <div id="pageInfo" style="display:block;">
                someone has to write it.
              </div>

              <div id="pageEntryTable" style="display:block;">
              </div>

              <div id="pageWebEditor" style="display:none;">
              </div>

              <div id="pageMaintain" style="display:none">
              </div>

              <div id="pageAdmin" style="display:none">
              </div>

              <div id="pageFAQ" style="display:none">
              </div>

              <div id="pageFirst" style="display:none">
              </div>
            </div>

            <div class="corner_BL">
              <div class="corner_BR"></div>
            </div>
          </div>
        </div>

      </div>');
  } // end of member function body



} // end of HTML_CMS_Help
?>
