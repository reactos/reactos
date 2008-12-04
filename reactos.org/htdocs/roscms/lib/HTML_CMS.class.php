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
 */
abstract class HTML_CMS extends HTML
{

  protected $branch = 'website';


  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '' )
  {
    // need to have a logged in user with minimum security level 1
    Login::required();
    if (ThisUser::getInstance()->securityLevel() == 0) {
      header('location:?page=nopermission');
    }

    // register css & js files
    $this->register_css('cms_navigation.css');
    $this->register_js('cms_navigation.js.php');

    parent::__construct($page_title);
  }
  
  protected function build()
  {
    $this->header();
    $this->navigation();
    $this->body();
    $this->footer();
  }


  /**
   *
   *
   * @access private
   */
  private function navigation( )
  {
    global $roscms_intern_page_link;
    global $roscms_intern_webserver_pages, $roscms_intern_page_link;

    $thisuser = &ThisUser::getInstance();

    // generate list of memberships
    $group_list = '';
    $stmt=DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id WHERE m.user_id=:user_id ORDER BY g.security_level DESC, g.name");
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();
    while($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $group_list .= ($group_list!=''?',':'').$group['name'];
    }

    // get security level
    $security_level = $thisuser->securityLevel();

    // get selected navigation entry
    echo_strip('
      <div id="myReactOS">
        <strong>'.$thisuser->name().'</strong>
        '.(($security_level > 1) ? '| SecLev: '.$security_level.' ('. $group_list .')' : '').'
        |
        <span onclick="refreshPage()" style="color:#006090; cursor:pointer;">
          <img src="images/reload.gif" alt="reload page" width="16" height="16" />
          <span style="text-decoration:underline;">reload</span>
        </span>  
        |
        <a href="'.$roscms_intern_page_link.'logout">Sign out</a>
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
              <td>&nbsp;&nbsp;</td>

              <th class="int'.(($this->branch == 'website') ? '2' : '1').'" onclick="'."loadBranch('website')".'">
                <div class="tcL">
                  <div class="tcR">
                    <div class="text">Website</div>
                  </div>
                </div>
              </th>
              <td>&nbsp;&nbsp;</td>');

    if ($thisuser->isMemberOfGroup('transmaint','ros_admin','ros_sadmin')) {
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

    if ($thisuser->securityLevel() == 3) {
      echo_strip('
        <th class="int'.(($this->branch == 'maintain') ? '2' : '1').'" onclick="'."loadBranch('maintain')".'">
          <div class="tcL">
            <div class="tcR">
              <div class="text">Maintain</div>
            </div>
          </div>
        </th>
        <td>&nbsp;&nbsp;</td>

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

    switch ($this->branch) {
      case 'welcome';
        echo 'RosCMS v3 - Welcome page';
        break;

      case 'website':
        echo_strip('Quick Links: <a href="'.$roscms_intern_page_link.'data&amp;branch=welcome#web_news_langgroup">Translation Group News</a>
          | <a href="'.$roscms_intern_webserver_pages.'?page=tutorial_roscms">Text- &amp; Video-Tutorials</a>
          | <a href="'.$roscms_intern_webserver_pages.'/forum/viewforum.php?f=18">Website Forum</a>');
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
    }
    echo_strip('
        </div>
      </div>');
  } // end of member function navigation


  /**
   *
   *
   * @access protected
   */
  protected function footer() {
    echo '</div>';
    parent::footer();
  }

} // end of HTML_CMS
?>
