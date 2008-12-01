<?php
    /*
    ReactOS DynamicFrontend (RDF)
    Copyright (C) 2008  Klemens Friedl <frik85@reactos.org>

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
 * class HTML_User
 * 
 */
abstract class HTML_User extends HTML
{


  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '')
  {
    $this->register_css('user.css');
    parent::__construct( $page_title );
  }
  
  protected function build()
  {
    parent::header();
    $this->navigation();
    $this->body();
    echo '</td></tr></table>';
    parent::footer();
  }


  /**
   *
   *
   * @access public
   */
  private function navigation( )
  {
    global $roscms_intern_webserver_pages;
    global $roscms_intern_webserver_roscms;
    global $roscms_intern_page_link;
    global $rpm_lang;
    global $roscms_langres;

    $thisuser = &ThisUser::getInstance();

    echo_strip('
      <table style="border:0" width="100%" cellpadding="0" cellspacing="0">
        <tr style="vertical-align: top;">
          <td style="width:147px; vertical-align: top;" id="leftNav"> 
            <h2>Navigation</h2>
            <ul>
              <li><a href="'.$roscms_intern_webserver_pages.'en/index.html">Home</a></li>
              <li><a href="'.$roscms_intern_webserver_pages.'en/about.html">Info</a></li>
              <li><a href="'.$roscms_intern_webserver_pages.'en/community.html">Community</a></li>
              <li><a href="'.$roscms_intern_webserver_pages.'en/dev.html">Development</a></li>
              <li><a href="'.$roscms_intern_webserver_roscms.'?page=user">myReactOS</a></li>
            </ul>');

    if ($thisuser->id() > 0) {
      echo_strip('
        <h2>'.$roscms_langres['Account'].'</h2>
        <ul>
          <li title="'.$thisuser->name().'">&nbsp;Nick:&nbsp;'.substr($thisuser->name(), 0, 9).'</li>
          <li><a href="'.$roscms_intern_page_link.'my">My Profile</a></li>
          <li><a href="'.$roscms_intern_page_link.'search">User Search</a></li>
          <li><a href="'.$roscms_intern_webserver_pages.'peoplemap/">User Map</a></li>');
      if ($thisuser->securityLevel() > 0) {
        echo '<li><a href="'.$roscms_intern_webserver_roscms.'?page=data&amp;branch=welcome">RosCMS Interface</a></li>';
      }
      echo_strip('
          <li><a href="?page=logout">'.$roscms_langres['Logout'].'</a></li>
        </ul>');
    }
    else {
      echo_strip('
        <h2>'.$roscms_langres['Account'].'</h2>
        <ul> 
          <li><a href="'.$roscms_intern_page_link.'login">Login</a></li>
          <li><a href="'.$roscms_intern_page_link.'register">Register</a></li>
        </ul>');
    }

    // Quick links
    echo_strip('
      <h2>Quick Links</h2>
      <ul>
        <li><a href="'.$roscms_intern_webserver_pages.'forum/">Forum</a></li>
        <li><a href="'.$roscms_intern_webserver_pages.'wiki/">Wiki</a></li>
        <li><a href="'.$roscms_intern_webserver_pages.'en/about_userfaq.html">FAQ</a></li>
        <li><a href="'.$roscms_intern_webserver_pages.'en/about_press.html">Press</a></li>
        <li><a href="'.$roscms_intern_webserver_pages.'bugzilla/">Bugzilla</a></li>
        <li><a href="'.$roscms_intern_webserver_pages.'en/community_mailinglists.html">Mailing Lists</a></li>
        <li><a href="'.$roscms_intern_webserver_pages.'getbuilds/">Trunk Builds</a></li>
      </ul>

      <h2>Language</h2>
      <ul>
        <li> 
          <div style="text-align:center;"> 
            <select id="select" size="1" name="select" class="selectbox" style="width:140px" onchange="'."window.location.href = '".$roscms_intern_webserver_roscms.'?'.htmlentities($_SERVER['QUERY_STRING'])."&lang=' + this.options[this.selectedIndex].value".'">
              <optgroup label="current language">'); 
 
    $stmt=DBConnection::getInstance()->prepare("SELECT lang_name FROM languages WHERE lang_id = :lang_id");
    $stmt->bindParam('lang_id',$rpm_lang,PDO::PARAM_STR);
    $stmt->execute();
    $current_lang = $stmt->fetchColumn();

    echo_strip('
        <option value="#">'.$current_lang.'</option>
      </optgroup>
      <optgroup label="all languages">');
      
    $stmt=DBConnection::getInstance()->prepare("SELECT lang_name, lang_id, lang_name_org FROM languages ORDER BY lang_level DESC");
    $stmt->execute();
    while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // display original name in brackets, if a localized version is available
      if ($language['lang_name'] != $language['lang_name_org']) {
        echo '<option value="'.$language['lang_id'].'">'.$language['lang_name_org'].' ('.$language['lang_name'].')</option>';
      }
      else {
        echo '<option value="'.$language['lang_id'].'">'.$language['lang_name'].'</option>';
      }
    }

    echo_strip('
                </optgroup>
              </select>
            </div>
          </li>
        </ul>
      </td>
      <td id="content" style="vertical-align: top;">');
  } // end of member function navigation

} // end of HTML_User
?>
