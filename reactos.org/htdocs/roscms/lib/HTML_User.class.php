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
    $thisuser = &ThisUser::getInstance();
    $config = &RosCMS::getInstance();

    echo_strip('
      <table style="border:0" width="100%" cellpadding="0" cellspacing="0">
        <tr style="vertical-align: top;">
          <td style="width:147px; vertical-align: top;" id="leftNav"> 
            <h2>Navigation</h2>
            <ul>
              <li><a href="'.$config->pathGenerated().'en/index.html">Home</a></li>
              <li><a href="'.$config->pathGenerated().'en/about.html">Info</a></li>
              <li><a href="'.$config->pathGenerated().'en/community.html">Community</a></li>
              <li><a href="'.$config->pathGenerated().'en/dev.html">Development</a></li>
              <li><a href="'.$config->pathRosCMS().'?page=user">myReactOS</a></li>
            </ul>');

    if ($thisuser->id() > 0) {
      echo_strip('
        <h2>Account</h2>
        <ul>
          <li title="'.$thisuser->name().'">&nbsp;Nick:&nbsp;'.substr($thisuser->name(), 0, 9).'</li>
          <li><a href="'.$config->pathRosCMS().'?page=my">My Profile</a></li>
          <li><a href="'.$config->pathRosCMS().'?page=search">User Search</a></li>
          <li><a href="'.$config->pathGenerated().'peoplemap/">User Map</a></li>');
      if ($thisuser->hasAccess('CMS')) {
        echo '<li><a href="'.$config->pathRosCMS().'?page=data&amp;branch=welcome">RosCMS Interface</a></li>';
      }
      echo_strip('
          <li><a href="?page=logout">Logout</a></li>
        </ul>');
    }
    else {
      echo_strip('
        <h2>Account</h2>
        <ul> 
          <li><a href="'.$config->pathRosCMS().'?page=login">Login</a></li>
          <li><a href="'.$config->pathRosCMS().'?page=register">Register</a></li>
        </ul>');
    }

    // Quick links
    echo_strip('
      <h2>Quick Links</h2>
      <ul>
        <li><a href="'.$config->pathGenerated().'forum/">Forum</a></li>
        <li><a href="'.$config->pathGenerated().'wiki/">Wiki</a></li>
        <li><a href="'.$config->pathGenerated().'en/about_userfaq.html">FAQ</a></li>
        <li><a href="'.$config->pathGenerated().'en/about_press.html">Press</a></li>
        <li><a href="'.$config->pathGenerated().'bugzilla/">Bugzilla</a></li>
        <li><a href="'.$config->pathGenerated().'en/community_mailinglists.html">Mailing Lists</a></li>
        <li><a href="'.$config->pathGenerated().'getbuilds/">Trunk Builds</a></li>
      </ul>

      <h2>Language</h2>
      <ul>
        <li> 
          <div style="text-align:center;"> 
            <select id="select" size="1" name="select" class="selectbox" style="width:140px" onchange="'."window.location.href = '".$config->pathRosCMS().'?'.htmlentities($_SERVER['QUERY_STRING'])."&lang=' + this.options[this.selectedIndex].value".'">
              <optgroup label="current language">'); 
 
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE id = :lang_id");
    $stmt->bindParam('lang_id',$_GET['lang'],PDO::PARAM_INT);
    $stmt->execute();
    $current_lang = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    echo_strip('
        <option value="#">'.$current_lang['name'].'</option>
      </optgroup>
      <optgroup label="all languages">');
      
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, id, name_original FROM ".ROSCMST_LANGUAGES." WHERE id != :lang ORDER BY name ASC");
    $stmt->bindParam('lang',$current_lang['id'],PDO::PARAM_INT);
    $stmt->execute();
    while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // display original name in brackets, if a localized version is available
      if ($language['name_original'] != '') {
        echo '<option value="'.$language['id'].'">'.$language['name'].' ('.htmlentities($language['name_original']).')</option>';
      }
      else {
        echo '<option value="'.$language['id'].'">'.$language['name'].'</option>';
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
