<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

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


class Rank extends HTML
{



  protected function body( )
  {
    global $RSDB_intern_link_db_sec;
    global $RSDB_langres;
    global $RSDB_intern_link_name_letter;
    global $RSDB_intern_index_php;
    global $RSDB_intern_link_rank_rank2;

    echo '
      <h1><a href="'.$RSDB_intern_link_db_sec.'home">'.$RSDB_langres['TEXT_compdb_short'].'</a> &gt; <a href="'.$RSDB_intern_link_name_letter.'all">Browse Database</a> &gt; By Rank</h1>
      <style type="text/css">
      <!--'."
      /* tab colors */
      .tab                { background-color : #ffffff; }
      .tab_s              { background-color : #5984C3; }
      .tab_u              { background-color : #A0B7C9; }
      
      /* tab link colors */
      a.tabLink           { text-decoration : none; }
      a.tabLink:link      { text-decoration : none; }
      a.tabLink:visited   { text-decoration : none; }
      a.tabLink:hover     { text-decoration : underline; }
      a.tabLink:active    { text-decoration : underline; }
      
      /* tab link size */
      p.tabLink_s         { color: navy; font-size : 10pt; font-weight : bold; padding : 0 8px 1px 2px; margin : 0; }
      p.tabLink_u         { color: black; font-size : 10pt; padding : 0 8px 1px 2px; margin : 0; }
      
      /* text styles */
      .strike 	       { text-decoration: line-through; }
      .bold              { font-weight: bold; }
      .newstitle         { font-weight: bold; color: purple; }
      .title_group       { font-size: 16px; font-weight: bold; color: #5984C3; text-decoration: none; }
      .bluetitle:visited { color: #323fa2; text-decoration: none; }
      
      .Stil1 {font-size: xx-small}
      .Stil2 {font-size: x-small}
      .Stil3 {color: #FFFFFF}
      .Stil4 {font-size: xx-small; color: #FFFFFF; }
      
      ".'-->
      </style>
      <table style="text-align: center;width:100%;border:none;" cellpadding="0" cellspacing="0">
        <tr style="text-align:left;vertical-align:top;">
          <!-- title -->
          <td valign="bottom" width="100%">
            <table border="0" cellpadding="0" cellspacing="0" width="100%">
              <tr>
                <td class="title_group" nowrap="nowrap">';

    switch (@$_GET['rank2']) {
      case 'vendors':
        echo 'Vendors';
        break;
      case 'screenshots':
        echo 'Screenshots';
        break;
      case 'forums':
        echo 'Forums';
        break;
      case 'ratings':
        echo 'User Ratings';
        break;
      case 'awards':
        echo 'Awards';
        break;
      case 'new':
      default:
        echo 'New';
        break;
    } // end switch

    echo '
                </td>
              </tr>
              <tr valign="bottom">
                <td class="tab_s">
                  <img src="images/white_pixel.gif" alt="" height="1" width="1" />
                </td>
              </tr>
            </table>
          </td>

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
              <tr align="left" valign="top">
                <td width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr align="left" valign="top">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/white_pixel.gif" alt="" height="4" width="1" />
                </td>
                <td width="4">
                  <img src="images/tab_corner_'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'active' : 'inactive').'.gif" alt="" height="4" width="4" />
                </td>
                <td>
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="middle">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="4" />
                </td>
                <td nowrap="nowrap" class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tabLink_s' : 'tabLink_u').'">
                  <a href="'.$RSDB_intern_link_rank_rank2.'new" class="tabLink">New</a>
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="bottom">
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'" width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'new' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
            </table>
          </td>
          <!-- end tab -->

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
              <tr align="left" valign="top">
                <td width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr align="left" valign="top">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/white_pixel.gif" alt="" height="4" width="1" />
                </td>
                <td width="4">
                  <img src="images/tab_corner_'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'active' : 'inactive').'.gif" alt="" height="4" width="4" />
                </td>
                <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="middle">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="4" />
                </td>
                <td nowrap="nowrap" class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tabLink_s' : 'tabLink_u').'">
                  <a href="'.$RSDB_intern_link_rank_rank2.'awards" class="tabLink">Awards</a>
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="bottom">
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'" width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'awards' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
            </table>
          </td>
          <!-- end tab -->

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
              <tr align="left" valign="top">
                <td width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr align="left" valign="top">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/white_pixel.gif" alt="" height="4" width="1" />
                </td>
                <td width="4">
                  <img src="images/tab_corner_'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'active' : 'inactive').'.gif" alt="" height="4" width="4" />
                </td>
                <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="middle">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="4" />
                </td>
                <td nowrap="nowrap" class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tabLink_s' : 'tabLink_u').'">
                  <a href="'.$RSDB_intern_link_rank_rank2.'ratings" class="tabLink">Ratings</a>
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="bottom">
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'" width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
            </table>
          </td>
          <!-- end tab -->

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
              <tr align="left" valign="top">
                <td width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr align="left" valign="top">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/white_pixel.gif" alt="" height="4" width="1" />
                </td>
                <td width="4">
                  <img src="images/tab_corner_'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'active' : 'inactive').'.gif" alt="" height="4" width="4" />
                </td>
                <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="middle">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="4" />
                </td>
                <td nowrap="nowrap" class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tabLink_s' : 'tabLink_u').'">
                  <a href="'.$RSDB_intern_link_rank_rank2.'forums" class="tabLink">Forums</a>
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="bottom">
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'" width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'ratings' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
            </table>
          </td>
          <!-- end tab -->

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
              <tr align="left" valign="top">
                <td width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr align="left" valign="top">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/white_pixel.gif" alt="" height="4" width="1" />
                </td>
                <td width="4">
                  <img src="images/tab_corner_'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'active' : 'inactive').'.gif" alt="" height="4" width="4" />
                </td>
                <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="middle">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="4" />
                </td>
                <td nowrap="nowrap" class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tabLink_s' : 'tabLink_u').'">
                  <a href="'.$RSDB_intern_link_rank_rank2.'screenshots" class="tabLink">Screenshots</a>
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="bottom">
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'" width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'screenshots' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
            </table>
          </td>
          <!-- end tab -->

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
              <tr align="left" valign="top">
                <td width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr align="left" valign="top">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/white_pixel.gif" alt="" height="4" width="1" />
                </td>
                <td width="4">
                  <img src="images/tab_corner_'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'active' : 'inactive').'.gif" alt="" height="4" width="4" />
                </td>
                <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="middle">
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="4">
                  <img src="images/blank.gif" alt="" height="1" width="4" />
                </td>
                <td nowrap="nowrap" class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tabLink_s' : 'tabLink_u').'">
                  <a href="'.$RSDB_intern_link_rank_rank2.'vendors" class="tabLink">Vendors</a>
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab_s' : 'tab_u').'" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
              <tr valign="bottom">
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'" width="4">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="'.(isset($_GET['rank2']) && ($_GET['rank2'] == 'vendors' || $_GET['rank2'] == '') ? 'tab' : 'tab_s').'">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="1">
                  <img src="images/blank.gif" alt="" height="1" width="1" />
                </td>
                <td class="tab_s" width="2">
                  <img src="images/blank.gif" alt="" height="1" width="2" />
                </td>
              </tr>
            </table>
          </td>
          <!-- end tab -->		

          <!-- fill the remaining space -->
          <td valign="bottom" width="10">
            <table border="0" cellpadding="0" cellspacing="0" width="100%">
              <tr valign="bottom">
                <td class="tab_s">
                  <img src="images/white_pixel.gif" alt="" height="1" width="10" />
                </td>
              </tr>
            </table>
          </td>
        </tr>
      </table>
      <br />
      <br />';

    switch (@$_GET['rank2']) {
      case 'awards':
      case 'ratings':
        self::showAwards();
        break;
      case 'vendors':
        self::showVendors();
        break;
      case 'screenshots':
        self::showScreenshots();
        break;
      case 'new':
      default:
        self::showNew();
        break;
    }

  } // end of member function body
  
  
  
  private static function showNew()
  {
    global $RSDB_intern_link_item_comp;
  
    $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_comp = '1'");
    $stmt->execute();
    $app_count = $stmt->fetchColumn();

    echo '
      <h3>Recent submissions</h3>
      <p>There are <strong>'.$app_count.' applications and drivers</strong> currently in the database.</p>

      <div style="margin:0; margin-top:10px; margin-right:10px; border:1px solid #dfdfdf; padding:0em 1em 1em 1em; background-color:#EAF0F8;"> <br />
        <table width="100%"  border="0">
          <tr valign="top">
            <td width="48%"><h4>Application entries</h4><table width="100%" border="0" cellpadding="1" cellspacing="1">
              <tr bgcolor="#5984C3">
            <td style="width:15%;background-color: #598aC3;color:white;font-weight:bold;">Time<//td>
            <td style="width:50%;background-color: #598aC3;color:white;font-weight:bold;">Application</td>
            <td style="width:35%;background-color: #598aC3;color:white;font-weight:bold;">ReactOs</td>
          </tr>';

    $cellcolor1='#E2E2E2';
    $cellcolor2='#EEEEEE';
    $cellcolorcounter=0;

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' ORDER BY comp_id DESC LIMIT 5");
    $stmt->execute();
    while ($entry = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$cellcolorcounter;

      echo '
        <tr bgcolor="'.($cellcolorcounter%2 ? $cellcolor1 : $cellcolor2).'">
          <td style="text-align: center; font-size:1;">'.$entry['comp_date'].'</td>
          <td style="font-size:2;">&nbsp;<strong><a href="'.$RSDB_intern_link_item_comp.$entry['comp_id'].'">'.$entry['comp_name'].'</a></strong></td>
          <td style="font-size:2;">&nbsp;ReactOS '.@show_osversion($entry['comp_osversion']).'</td>
        </tr>';
    } // end while

    echo '
        </table>
      </td>
      <td width="4%">&nbsp;</td>
      <td width="48%">
        <h4>Compatibility Test Reports</h4>
        <table width="100%" border="0" cellpadding="1" cellspacing="1">
          <tr bgcolor="#5984C3">
            <td style="width:15%;background-color: #598aC3;color:white;font-weight:bold;">Time<//td>
            <td style="width:50%;background-color: #598aC3;color:white;font-weight:bold;">Application</td>
            <td style="width:35%;background-color: #598aC3;color:white;font-weight:bold;">Function</td>
          </tr>'; 

    $cellcolor1='#E2E2E2';
    $cellcolor2='#EEEEEE';
    $cellcolorcounter=0;

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_testresults WHERE test_visible = '1' ORDER BY test_id DESC LIMIT 5");
    $stmt->execute();
    while($entry = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$cellcolorcounter;

      //@MERGEME as join with upper query
      $stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_id = :comp_id LIMIT 1");
      $stmt_sub->bindParam('comp_id',$entry['test_comp_id'],PDO::PARAM_STR);
      $stmt_sub->execute();
      $app = $stmt_sub->fetchOnce(PDO::FETCH_ASSOC);

      echo '
        <tr bgcolor="'.($cellcolorcounter%2 ? $cellcolor1 : $cellcolor2).'">
          <td style="text-align:center:font-size:1;">'.$entry['test_user_submit_timestamp'].'</td>
          <td style="font-size:2;">&nbsp;<strong><a href="'.$RSDB_intern_link_item_comp.$app['comp_id'] .'&amp;item2=tests">'.$app['comp_name'].'</a></strong></td>
          <td style="text-align:left;font-size:2;">&nbsp;'.Star::drawSmall($entry['test_result_function'], 1, 5, '').'</td>
        </tr>';
    } // end while

    echo '
          </table>
        </td>
      </tr>
      <tr>
        <td>&nbsp;</td>
        <td><h4>Screenshots</h4>
          <table width="100%" border="0" cellpadding="1" cellspacing="1">
            <tr bgcolor="#5984C3">
              <td style="width:15%;background-color: #598aC3;color:white;font-weight:bold;">Time<//td>
              <td style="width:50%;background-color: #598aC3;color:white;font-weight:bold;">Description</td>
              <td style="width:35%;background-color: #598aC3;color:white;font-weight:bold;">Application</td>
            </tr>';

    $cellcolor1='#E2E2E2';
    $cellcolor2='#EEEEEE';
    $cellcolorcounter=0;

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE media_visible = '1' ORDER BY media_id DESC LIMIT 5");
    $stmt->execute();
    while ($entry = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$cellcolorcounter;

      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_media = :group_id LIMIT 1");
      $stmt->bindParam('group_id',$entry['media_groupid'],PDO::PARAM_STR);
      $stmt->execute();
      $comp = $stmt->fetchOnce(PDO::FETCH_ASSOC);

      echo '
        <tr bgcolor="'.($cellcolorcounter%2 ? $cellcolor1 : $cellcolor2).'">
          <td style="text-align:center:font-size:1;">'.$entry['media_date'].'</td>
          <td style="font-size:2;">&nbsp;<strong><a href="'.$RSDB_intern_link_item_comp.$comp['comp_id'].'&amp;item2=screens&amp;entry='. urlencode($entry['media_id']).'">'.htmlentities($entry['media_description']).'</a></strong></td>
          <td style="font-size:2;">&nbsp;<a href="'.$RSDB_intern_link_item_comp.$comp['comp_id'].'&amp;item2=screens">'.$comp['comp_name'].'</a></td>
        </tr>';
    } // end while

    echo '
              </table>
            </td>
          </tr>
        </table>
      </div>';
  } // end of member function showNew



  private static function showAwards()
  {
    global $RSDB_SET_letter;
    global $RSDB_intern_link_rank_curpos;
    global $RSDB_intern_items_per_page;
    global $RSDB_intern_link_rank2_group;
    global $RSDB_intern_link_vendor_sec;

    if ($RSDB_SET_letter == 'all') {
      $RSDB_SET_letter = '%';
    }

    $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ( SELECT grpentr_id FROM rsdb_groups g JOIN rsdb_item_comp i ON i.comp_groupid = g.grpentr_id AND g.grpentr_visible = '1' AND g.grpentr_comp = '1' GROUP BY grpentr_id) v1");
    $stmt->execute();
    $categories = $stmt->fetchColumn();
    if ($categories>0) {

      echo '<p style="text-align:center;">';
      $j=0;
      for ($i=0; $i < $categories; $i += $RSDB_intern_items_per_page) {
        ++$j;
        if (isset($_GET['curpos']) && $_GET['curpos'] == $i) {
          echo '<strong>'.$j.'</strong> ';
        }
        else {
          echo '<a href="'.$RSDB_intern_link_rank_curpos.$i.'">'.$j.'</a> ';
        }
      }

      echo '
        </p>
        <table width="100%" border="0" cellpadding="1" cellspacing="1">
          <tr bgcolor="#5984C3"> 
            <th style="width:25%; background-color:#5984C3; text-align:center; color:white;">Application</th>
            <th style="width:15%; text-align:center; color:white;" title="Version">Vendor</th>
            <th style="width:14%; text-align:center; color:white;">Award</th>
            <th style="width:19%; text-align:center; color:white;">Function</th>
            <th style="width:19%; text-align:center; color:white;">Install</th>
            <th style="width:8%; text-align:center; color:white;" title="Status">Status</th>
          </tr>';

      if (isset($_GET['rank2']) && $_GET['rank2'] == 'awards') {
        $stmt=CDBConnection::getInstance()->prepare("SELECT v1.grpentr_id, v1.derived_max, v1.grpentr_vendor, v1.grpentr_name FROM (SELECT grpentr_id, MAX(i.comp_award) derived_max, g.grpentr_vendor, g.grpentr_name FROM rsdb_groups g  JOIN rsdb_item_comp i ON i.comp_groupid = g.grpentr_id AND g.grpentr_visible = '1' AND g.grpentr_comp = '1' GROUP BY grpentr_id) v1 ORDER BY v1.derived_max DESC LIMIT ".intval($RSDB_intern_items_per_page)." OFFSET ".intval(@$_GET['curpos'])."");
      }
      elseif (isset($_GET['rank2']) && $_GET['rank2'] == 'ratings') {
        $stmt=CDBConnection::getInstance()->prepare("SELECT v1.grpentr_id, v1.derived_max, v1.grpentr_vendor, v1.grpentr_name FROM (SELECT grpentr_id, MAX(i.comp_award) derived_max, g.grpentr_vendor, g.grpentr_name FROM rsdb_groups g JOIN rsdb_item_comp i ON i.comp_groupid = g.grpentr_id AND g.grpentr_visible = '1' AND g.grpentr_comp = '1' GROUP BY grpentr_id) v1 ORDER BY v1.derived_max DESC LIMIT ".intval($RSDB_intern_items_per_page)." OFFSET ".intval(@$_GET['curpos'])."");
      }
      $stmt->execute();

      $color1='#E2E2E2';
      $color2='#EEEEEE';
      $counter=0;

      while ($entry = $stmt->fetch(PDO::FETCH_ASSOC)) {
        ++$counter;
 
        //@MERGEME as join with upper query
        $stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id");
        $stmt_sub->bindParam('vendor_id',$result_page['grpentr_vendor'],PDO::PARAM_STR);
        $stmt_sub->execute();
        $vendor = $stmt_sub->fetchOnce(PDO::FETCH_ASSOC);

        $counter_stars_install_sum = 0;
        $counter_stars_function_sum = 0;
        $counter_stars_user_sum = 0;
        $counter_items = 0;
        $counter_testentries = 0;
        $counter_forumentries = 0;
        $counter_screenshots = 0;

        $stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_visible = '1' ORDER BY comp_groupid DESC");
        $stmt_sub->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
        $stmt_sub->execute();
        while ($result_page = $stmt_sub->fetch(PDO::FETCH_ASSOC)) { 
          ++$counter_items;

          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) AS user_sum, SUM(test_result_install) AS install_sum, SUM(test_result_function) AS function_sum FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id");
          $stmt_count->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt_count->execute();
          $tmp=$stmt_count->fetch(PDO::FETCH_ASSOC);

          $counter_stars_install_sum += $tmp['install_sum'];
          $counter_stars_function_sum += $tmp['function_sum'];
          $counter_stars_user_sum += $tmp['user_sum'];

          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id");
          $stmt_count->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt_count->execute();
          $result_count_testentries = $stmt_count->fetch(PDO::FETCH_NUM);
          $counter_testentries += $result_count_testentries[0];

          // Forum entries:
          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_forum WHERE fmsg_visible = '1' AND fmsg_comp_id = :comp_id");
          $stmt_count->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt_count->execute();
          $result_count_forumentries = $stmt_count->fetch(PDO::FETCH_NUM);
          $counter_forumentries += $result_count_forumentries[0];
    
          // Screenshots:
          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_object_media WHERE media_visible = '1' AND media_groupid = :group_id");
          $stmt_count->bindParam('group_id',$result_page['comp_media'],PDO::PARAM_STR);
          $stmt_count->execute();
          $result_count_screenshots = $stmt_count->fetch(PDO::FETCH_NUM);
          $counter_screenshots += $result_count_screenshots[0];
        } // end while stmt_sub

        echo '
          <tr bgcolor="'.($counter%2 ? $color1 : $color2).'">
            <td valign="top">
              <div align="left"><font face="Arial, Helvetica, sans-serif">&nbsp;<a href="'.$RSDB_intern_link_rank2_group.$entry['grpentr_id'].'"><strong>'.htmlentities($entry['grpentr_name']).'</strong></a></font></div>
            </td>
            <td valign="top">
              <div align="left">
                <font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<a href="'.$RSDB_intern_link_vendor_sec.$vendor['vendor_id'].'">'.$vendor['vendor_name'].'</a></font>
              </div>
            </td>
            <td valign="top">
              <div align="left">
                <font size="1" face="Arial, Helvetica, sans-serif">&nbsp;<img src="media/icons/awards/'.Award::icon($entry['derived_max']).'.gif" alt="'.Award::name($entry['derived_max']).'" width="16" height="16" /> '.Award::name($result_page['derived_max']).'</font>
              </div>
            </td>
            <td valign="top">
              <div align="left">
                <font face="Arial, Helvetica, sans-serif" size="2">'.Star::drawSmall($counter_stars_function_sum, $counter_stars_user_sum, 5, '').' ('.$counter_stars_user_sum.')</font>
              </div>
            </td>
            <td valign="top">
              <div align="left">
                <font size="2">'.Star::drawSmall($counter_stars_function_sum, $counter_stars_user_sum, 5, '').' ('.$counter_stars_user_sum.')</font>
              </div>
            </td>
            <td valign="top" title="Tests: '.$counter_testentries.', Forum entries: '.$counter_forumentries.', Screenshots: '.$counter_screenshots.'">
              <div align="center">
                <table width="100%" border="0" cellpadding="1" cellspacing="1">
                  <tr>
                    <td width="33%">
                      <div align="center">
                        '.($counter_testentries > 0 ? '<img src="media/icons/info/test.gif" alt="Compatibility Test Report entries" width="13" height="13" />' : '&nbsp;').'
                      </div>
                    </td>
                    <td width="33%">
                      <div align="center">
                        '.($counter_forumentries > 0 ? '<img src="media/icons/info/forum.gif" alt="Forum entries" width="13" height="13" />' : '&nbsp;').'
                      </div>
                    </td>
                    <td width="33%">
                      <div align="center">
                        '.($counter_screenshots > 0 ? '<img src="media/icons/info/screenshot.gif" alt="Screenshots" width="13" height="13" />' : '&nbsp;').'
                      </div>
                    </td>
                  </tr>
                </table>
              </div>
            </td>
          </tr>';
      } // end while entry

      echo '
        </table>
        <p align="center"><strong>'.(@$_GET['curpos']+1).' to '.(((@$_GET['curpos'] + $RSDB_intern_items_per_page) > $categories[0]) ? $categories[0] : (@$_GET['curpos'] + $RSDB_intern_items_per_page)).' of '.$categories.'</strong></p>';
    }
  } // end of member function showAwards



  private static function showVendors() 
  {
    global $RSDB_intern_link_vendor_sec_comp;

    echo '
      <p>Under construction ...</p>
      <table width="100%" border="0" cellpadding="1" cellspacing="1">
        <tr bgcolor="#5984C3">
          <td width="15%">
            <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Vendor</strong></font></div>
          </td>
          <td width="30%">
            <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Fullname</strong></font></div>
          </td>
          <td width="27%">
            <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Website</strong></font></div>
          </td>
          <td width="18%">
            <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Number</strong></font></div>
          </td>
        </tr>';

    $stmt=CDBConnection::getInstance()->prepare("SELECT v1.vendor_id, v1.derived_max, v1.vendor_name, v1.vendor_url, v1.vendor_fullname FROM ( SELECT vendor_id, MAX(i.grpentr_vendor) AS derived_max, g.vendor_name, g.vendor_url, g.vendor_fullname FROM rsdb_item_vendor g JOIN rsdb_groups i ON i.grpentr_vendor = g.vendor_id AND g.vendor_visible = '1' GROUP BY vendor_id ) v1 ORDER BY v1.derived_max DESC");
    $stmt->execute();

    $color1="#E2E2E2";
    $color2="#EEEEEE";
    $counter="0";

    while($result_page = $stmt->fetch(PDO::FETCH_ASSOC)) { // Pages
      ++$counter;

      echo '
        <tr bgcolor="'.($counter%2 ? $color1 : $color2).'">
          <td valign="top">
            <div align="left">
              <font face="Arial, Helvetica, sans-serif">&nbsp;<strong><a href="'.$RSDB_intern_link_vendor_sec_comp.$result_page['vendor_id'].'">'.$result_page['vendor_name'].'</a></strong></font>
            </div>
          </td>
          <td valign="top"><font size="2">'.$result_page['vendor_fullname'].'</font></td>
          <td valign="top"><div align="left"><font size="2"><a href="'.$result_page['vendor_url'].'">'.$result_page['vendor_url'].'</a></font></div></td>
          <td valign="top"><div align="left"><font size="2">'.$result_page['derived_max'].' </font></div></td>
        </tr>';
    } // end while

    echo '
      </table>
      <br />';
  } // end of member function showVendors



  private static function showScreenshots() 
  {
    global $RSDB_intern_items_per_page;
    global $RSDB_intern_link_rank_curpos;
    global $RSDB_intern_link_item_item2;
    global $RSDB_intern_user_id;
    global $RSDB_setting_stars_threshold;
    global $RSDB_intern_link_item_item2_vote;

    echo '<p>Under construction ...</p>';

    $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_object_media WHERE (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5)");
    $stmt->execute();
    $categories = $stmt->fetchColumn();

    if ($categories) {

      echo '<p align="center">';
      $j=0;
      for ($i=0; $i < $categories; $i += $RSDB_intern_items_per_page) {
        $j++;
        if (isset($_GET['curpos']) && $_GET['curpos'] == $i) {
          echo '<strong>'.$j.'</strong> ';
        }
        else {
          echo "<a href='".$RSDB_intern_link_rank_curpos.$i."'>".$j."</a> ";
        }
      }

      echo '
        </p>
        <table width="100%"  border="0" cellpadding="3" cellspacing="1">';

      $roscms_TEMP_counter = 0;

      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5) ORDER BY media_order ASC LIMIT ".intval($RSDB_intern_items_per_page)." OFFSET ".intval(@$_GET['curpos'])."");
      $stmt->execute();
      while($result_screenshots= $stmt->fetch(PDO::FETCH_ASSOC)) {
        $roscms_TEMP_counter++;
        if ($roscms_TEMP_counter == 1) {
          echo '<tr>';
        }
        echo '
          <td width="33%" valign="top">
          <p align="center">
            <br />
            <a href="'.$RSDB_intern_link_item_item2.'screens&amp;entry='.$result_screenshots['media_id'].'">
              <img src="media/files/'.$result_screenshots['media_filetype'].'/'.urlencode($result_screenshots['media_thumbnail']).'" width="250" height="188" border="0" alt="Description: '.htmlentities($result_screenshots['media_description']).'\nUser: '.usrfunc_GetUsername($result_screenshots['media_user_id']).'\nDate: '.$result_screenshots['media_date'].'\n\n'.htmlentities($result_screenshots['media_exif']).'"></a><br /><i>'.htmlentities($result_screenshots['media_description']).'</i><br /><br /><font size="1">';

        $RSDB_TEMP_voting_history = strchr($result_screenshots['media_useful_vote_user_history'],("|".$RSDB_intern_user_id."="));
        if ($RSDB_TEMP_voting_history == false) {
          echo "Rate this screenshot: ";
          if ($result_screenshots['media_useful_vote_user'] > $RSDB_setting_stars_threshold) {
            echo Star::drawVoteable($result_screenshots['media_useful_vote_value'], $result_screenshots['media_useful_vote_user'], 5, "", ($RSDB_intern_link_item_item2_vote.$result_screenshots['media_id']."&amp;vote2="));
          }
          else {
            echo Star::drawVoteable(0, 0, 5, "", ($RSDB_intern_link_item_item2_vote.$result_screenshots['media_id'].'&amp;vote2='));
          }
        }
        else {
          echo 'Rating: '.Star::drawNormal($result_screenshots['media_useful_vote_value'], $result_screenshots['media_useful_vote_user'], 5, '');
        }

        echo '
              </font>
              <br />
              <br />
            </p>
          </td>';
        if ($roscms_TEMP_counter == 3) {
          echo '</tr>';
          $roscms_TEMP_counter = 0;
        }
      } // end while

      if ($roscms_TEMP_counter == 1) {
        echo '
            <td width="33%" valign="top">&nbsp;</td>
            <td width="33%" valign="top">&nbsp;</td>
          </tr>';
      }
      if ($roscms_TEMP_counter == 2) {
        echo '<td width="33%" valign="top">&nbsp;</td></tr>';
      }

      echo '
        </table>
        <p align="center">
          <strong>'.(@$_GET['curpos']+1).' to ';

      if ((@$_GET['curpos'] + $RSDB_intern_items_per_page) > $categories) {
        echo $categories;
      }
      else {
        echo (@$_GET['curpos'] + $RSDB_intern_items_per_page);
      }
      echo ' of '.$categories.'</strong></p>';

    }
  } // end of member function showScreenshots



} // end of Rank
