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
 * class HTML_CMS
 * 
 * @package html
 * @subpackage cms
 */
abstract class HTML_CMS extends HTML
{



  protected $branch = 'welcome';



  /**
   * check for access and register js & css
   *
   * @access public
   */
  public function __construct( $page_title = '' )
  {
    // need to have a logged in user
    Login::required();

    // because of security check for access to the CMS
    if (!ThisUser::getInstance()->hasAccess('CMS')) {
      header('location:?page=nopermission');
    }

    // register css & js files
    $this->register_css('cms.css');
    $this->register_js('cms.js');

    parent::__construct($page_title);
  }



  /**
   * hook header function to include an navigation
   *
   * @access private
   */
  protected function header()
  {
    parent::header();
    $this->navigation();
  }



  /**
   * navigation for our CMS branches
   *
   * @access protected
   */
  protected function navigation( )
  {
    $thisuser = &ThisUser::getInstance();

    // generate list of memberships
    $group_list = '';
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id WHERE m.user_id=:user_id ORDER BY g.security_level DESC, g.name");
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();
    while($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $group_list .= ($group_list!=''?', ':'').$group['name'];
    }

    // get selected navigation entry
    echo_strip('
      <div id="myReactOS">
        <strong>'.$thisuser->name().'</strong> ('. $group_list .')
        &nbsp;|&nbsp;
        <a href="'.RosCMS::getInstance()->pathInstance().'?page=logout">Log out</a>
      </div>
      <div id="roscms_page">
        <table id="tabMenu" cellpadding="0" cellspacing="0">
          <tbody>
            <tr>
              <th'.(($this->branch == 'welcome') ? ' class="active"' : '').'>
                <div class="corner_TL">
                  <div class="corner_TR">
                    <a class="text" href="'.RosCMS::getInstance()->pathInstance().'?page=data&branch=welcome" onclick="'."loadBranch()".'">Welcome</a>
                  </div>
                </div>
              </th>
              <td>&nbsp;&nbsp;</td>');

    // Website branch tab
    if ($thisuser->hasAccess('website')) {
      echo_strip('
        <th'.(($this->branch == 'website') ? ' class="active"' : '').'>
          <div class="corner_TL">
            <div class="corner_TR">
              <a class="text" href="'.RosCMS::getInstance()->pathInstance().'?page=data&branch=website" onclick="'."loadBranch()".'">Website</a>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>');
    }

    // Maintain branch tab
    if ($thisuser->hasAccess('maintain')) {
      echo_strip('
        <th'.(($this->branch == 'maintain') ? ' class="active"' : '').'>
          <div class="corner_TL">
            <div class="corner_TR">
              <a class="text" href="'.RosCMS::getInstance()->pathInstance().'?page=data&branch=maintain" onclick="'."loadBranch()".'">Maintain</a>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>');
    }

    // Statistics branch tab
    if ($thisuser->hasAccess('stats')) {
      echo_strip('
        <th'.(($this->branch == 'stats') ? ' class="active"' : '').'>
          <div class="corner_TL">
            <div class="corner_TR">
              <a class="text" href="'.RosCMS::getInstance()->pathInstance().'?page=data&branch=stats" onclick="'."loadBranch()".'">Statistics</a>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>');
    }

      echo_strip('
            <th'.(($this->branch == 'help') ? ' class="active"' : '').'>
              <div class="corner_TL">
                <div class="corner_TR">
                  <a class="text" href="'.RosCMS::getInstance()->pathInstance().'?page=data&branch=help" onclick="'."loadBranch()".'">Help</a>
                </div>
              </div>
            </th>
            <td style="width:100%">
              <div id="ajaxloadinginfo" style="visibility:hidden; text-align: center;">
                <img src="'.RosCMS::getInstance()->pathRosCMS().'images/ajax_loading.gif" alt="loading ..." width="13" height="13" />
              </div>
            </td>
          </tr>
        </tbody>
      </table>

      <div class="tcR" style="background-color:#C9DAF8; height: 5px;"></div>');
  } // end of member function navigation



  /**
   * hook footer get get an additional element attached, needed by our navigation
   *
   * @access protected
   */
  protected function footer( )
  {
    echo '</div>';
    parent::footer();
  } // end of member function footer



} // end of HTML_CMS
?>
