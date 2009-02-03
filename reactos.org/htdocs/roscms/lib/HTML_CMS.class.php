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



  protected $branch = 'website';



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
    $this->register_css('cms_navigation.css');
    $this->register_js('cms_navigation.js');

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
   * @access private
   */
  private function navigation( )
  {
    $thisuser = &ThisUser::getInstance();

    // generate list of memberships
    $group_list = '';
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id WHERE m.user_id=:user_id ORDER BY g.security_level DESC, g.name");
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();
    while($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $group_list .= ($group_list!=''?',':'').$group['name'];
    }

    // get selected navigation entry
    echo_strip('
      <div id="myReactOS">
        <strong>'.$thisuser->name().'</strong> ('. $group_list .')
        |
        <span onclick="refreshPage()" style="color:#006090; cursor:pointer;">
          <img src="images/reload.gif" alt="reload page" width="16" height="16" />
          <span style="text-decoration:underline;">reload</span>
        </span>  
        |
        <a href="'.RosCMS::getInstance()->pathRosCMS().'?page=logout">Sign out</a>
      </div>
      <div id="roscms_page">
        <table id="mt" cellpadding="0" cellspacing="0">
          <tbody>
            <tr>
              <th class="int'.(($this->branch == 'welcome') ? '2' : '1').'" onclick="'."loadBranch('welcome')".'">
                <div class="tcL">
                  <div class="tcR">
                    <div class="text">Welcome</div>
                  </div>
                </div>
              </th>
              <td>&nbsp;&nbsp;</td>');

    // Website branch tab
    if ($thisuser->hasAccess('website')) {
      echo_strip('
              <th class="int'.(($this->branch == 'website') ? '2' : '1').'" onclick="'."loadBranch('website')".'">
                <div class="tcL">
                  <div class="tcR">
                    <div class="text">Website</div>
                  </div>
                </div>
              </th>
              <td>&nbsp;&nbsp;</td>');
    }

    // User branch tab
    if ($thisuser->hasAccess('user')) {
      echo_strip('
        <th class="int'.(($this->branch == 'user') ? '2' : '1').'" onclick="'."loadBranch('user')".'">
          <div class="tcL">
            <div class="tcR">
              <div class="text">User</div>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>');
    }

    // Maintain branch tab
    if ($thisuser->hasAccess('maintain')) {
      echo_strip('
        <th class="int'.(($this->branch == 'maintain') ? '2' : '1').'" onclick="'."loadBranch('maintain')".'">
          <div class="tcL">
            <div class="tcR">
              <div class="text">Maintain</div>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>');
    }

    // Administration branch tab
    if ($thisuser->hasAccess('admin')) {
      echo_strip('
        <th class="int'.(($this->branch == 'admin') ? '2' : '1').'" onclick="'."loadBranch('admin')".'">
          <div class="tcL">
            <div class="tcR">
              <div class="text">Administration</div>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>');
    }

    // Statistics branch tab
    if ($thisuser->hasAccess('stats')) {
      echo_strip('
        <th class="int'.(($this->branch == 'stats') ? '2' : '1').'" onclick="'."loadBranch('stats')".'">
          <div class="tcL">
            <div class="tcR">
              <div class="text">Statistics</div>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>');
    }

    echo_strip('
            <td style="width:100%">
              <div id="ajaxloadinginfo" style="visibility:hidden; text-align: center;">
                <img src="images/ajax_loading.gif" alt="loading ..." width="13" height="13" />
              </div>
            </td>
          </tr>
        </tbody>
      </table>

      <div class="tcR" style="background-color:#C9DAF8;">
        <div id="branchInfo">');

    // Branch Info List (below branch tabs)
    switch ($this->branch) {
      case 'welcome';
        echo 'RosCMS v3 - Welcome page';
        break;

      case 'website':
        echo_strip('Quick Links: <a href="'.RosCMS::getInstance()->pathRosCMS().'?page=data&amp;branch=welcome#web_news_langgroup">Translation Group News</a>
          | <a href="'.RosCMS::getInstance()->pathGenerated().'?page=tutorial_roscms">Text- &amp; Video-Tutorials</a>
          | <a href="'.RosCMS::getInstance()->pathGenerated().'/forum/viewforum.php?f=18">Website Forum</a>');
        break;
        
      case 'user':
        echo 'User Account Management Interface';
        break;

      case 'maintain': 
        echo 'RosCMS Maintainer Interface';
        break;

      case 'stats':
        echo 'RosCMS Website Statistics';
        break;
    } // end switch

    echo_strip('
        </div>
      </div>');
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
