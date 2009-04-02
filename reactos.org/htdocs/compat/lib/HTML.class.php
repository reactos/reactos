<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>
                  2009       Danny Götte <dangerground@web.de>

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
 * class HTML
 * 
 * @package html
 */
abstract class HTML
{

  protected $title; 
  protected $css_files=array(); 
  protected $js_files=array(); 



  /**
   * set page title, start generation time counter and get page output
   *
   * @access public
   */
  public function __construct( $page_title = '' )
  {
    // get page title and register css files
    $this->title = $page_title;
    $this->register_css('style.css');
    $this->register_js('smoothscroll.js');
    $this->register_js('search.js');

    // get output
    $this->build();
  } // end of constructor



  /**
   * output of the whole site
   *
   * @access private
   */
  private function build( )
  {
    $this->header();
    $this->navigation();
    $this->body();
    $this->footer();
  } // end of member function build



  /**
   * includes the page itself without header + footer
   *
   * @access protected
   */
  abstract protected function body( );



  /**
   * output site header
   *
   * @access protected
   */
  protected function header( )
  {
    global $RSDB_langres;
    global $RSDB_intern_path_server;
    global $RSDB_intern_path;
    global $RSDB_intern_user_id;

    echo '
      <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
      <html lang="'.$RSDB_langres['lang_code'].'">
        <head>
          <title>ReactOS '.$this->title.'</title>
          <base href="'.$RSDB_intern_path_server.$RSDB_intern_path.'" />
          <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
          <meta http-equiv="Pragma" content="no-cache" />
          <meta name="Description" content="The ReactOS Compatibility Database has stored a lot of information about application and driver compatibility with ReactOS." />
          <meta name="Keywords" content="ReactOS, Compatibility, application, driver, RSDB, support, database, DB, test, report, rating, Ros32, Ros64, Win32, Win64, W32, W64, Wine, vendor, release, version" />
          <meta name="Copyright" content="ReactOS Foundation" />
          <meta name="Generator" content="CompatDB" />
          <meta name="Content-language" content="'.$RSDB_langres['lang_code'].'" />
          <meta name="Robots" content="index,follow" />
          <link rel="SHORTCUT ICON" href="../favicon.ico" />';

    // link css files (use register_css method)
    foreach($this->css_files as $file) {
      if ($file['condition'] === false) {
        echo '<link href="'.$RSDB_intern_path_server.$RSDB_intern_path.'css/'.$file['name'].'" type="text/css" rel="stylesheet" />';
      }
      else {
        echo '<!--[if '.$file['condition'].']<link href="'.$RSDB_intern_path_server.$RSDB_intern_path.'css/'.$file['name'].'" type="text/css" rel="stylesheet" /><![endif]-->';
      }
    }

    // link js files (use register_js method)
    foreach($this->js_files as $file) {
      echo '<script src="'.$RSDB_intern_path_server.$RSDB_intern_path.'js/'.$file.'" type="text/javascript"></script>';
    }

    echo '
      <script>
      <!--'."
        function clk(url,ct,sg)
        {
          if(document.images) {
            var u='';

            if (url) {
              u='&url='+escape(url).replace(/\+/g,'%2B');
            }

            new Image().src='url.php?t='+escape(ct)+'&u='+u+'&a=".base64_encode($RSDB_intern_user_id)."'+'&i=".base64_encode($_SERVER['REMOTE_ADDR'])."'+'&s'+sg;

          }
          return true;
        }
      ".'-->
      </script>
    </head>
    <body>
      <div id="top">
        <div id="topMenu"> 
          <a href="<?php echo $RSDB_intern_path_server; ?>?page=index">'.$RSDB_langres['Home'].'</a> |
          <a href="<?php echo $RSDB_intern_path_server; ?>?page=about">'.$RSDB_langres['Info'].'</a> |
          <a href="<?php echo $RSDB_intern_path_server; ?>?page=community">'.$RSDB_langres['Community'].'</a> |
          <a href="<?php echo $RSDB_intern_path_server; ?>?page=dev">'.$RSDB_langres['Dev'].'</a> |
          <a href="<?php echo $RSDB_intern_path_server."roscms/"; ?>?page=user">'.$RSDB_langres['myReactOS'].'</a>
        </div>
      </div>';
  } // end of member function header



  /**
   * output site navigation
   *
   * @access protected
   */
  protected function navigation( )
  {
    include('rsdb_setting.php');
    include('rsdb_config.php');

    global $RSDB_SET_page;

    global $roscms_intern_account_level;
    global $roscms_intern_login_check;
    global $roscms_intern_login_check_username;
    global $roscms_intern_account_group;
    global $roscms_intern_usrgrp_sadmin;
    global $roscms_intern_usrgrp_admin;
    global $roscms_intern_usrgrp_dev;
    global $roscms_intern_usrgrp_team;
    global $roscms_intern_usrgrp_trans;
    global $roscms_intern_usrgrp_user;
    global $RSDB_langres;

    echo '
      <table style="border: none; width:100%;" cellpadding="0" cellspacing="0">
        <tr style="vertical-align:top;"> 
          <td style="width:147px" id="leftNav">
            <div class="navTitle">'.$RSDB_langres['Navigation'].'</div>
            <ol>
              <li><a href="'.$RSDB_intern_path_server.'?page=index">'.$RSDB_langres['Home'].'</a></li>
              <li><a href="'.$RSDB_intern_path_server.'?page=about">'.$RSDB_langres['Info'].'</a></li>
              <li><a href="'.$RSDB_intern_path_server.'?page=community">'.$RSDB_langres['Community'].'</a></li>
              <li><a href="'.$RSDB_intern_path_server.'?page=dev">'.$RSDB_langres['Dev'].'</a></li>
              <li><a href="'.$RSDB_intern_path_server.'roscms/?page=user">'.$RSDB_langres['myReactOS'].'</a></li>
            </ol>
            <br />

            <div class="navTitle">Compatibility</div>
            <ol>
              <li><a href="'.$RSDB_intern_link_view_EX.'home'.$RSDB_URI_slash.'">Overview</a></li>
              <li><a href="'.$RSDB_intern_link_category_EX.'0'.$RSDB_URI_slash.'">Browse Database</a></li>';

    if ( $RSDB_SET_sec == 'category' || $RSDB_SET_sec == 'name' || $RSDB_SET_sec == 'vendor' || $RSDB_SET_sec == 'rank') {
      echo '
        <li><a href="'.$RSDB_intern_link_category_EX.'0'.$RSDB_URI_slash.'">&nbsp;- By Category</a></li>
        <li><a href="'.$RSDB_intern_link_name_letter_EX.'all'.$RSDB_URI_slash.'">&nbsp;- By Name</a></li>
        <li><a href="'.$RSDB_intern_link_vendor_letter_EX.'all'.$RSDB_URI_slash.'">&nbsp;- By Vendor</a></li>
        <li><a href="'.$RSDB_intern_link_rank_EX.'rank'.$RSDB_URI_slash.'">&nbsp;- By Rank</a></li>';
    }

    echo '
        <li id="noscriptsearchbar" style="display: block"><a href="'.$RSDB_intern_link_view_EX.'search'.$RSDB_URI_slash.'">Search</a></li>
        <li><a href="'.$RSDB_intern_link_view_EX.'submit'.$RSDB_URI_slash.'">Submit Application</a></li>
        <li><a href="'.$RSDB_intern_link_view_EX.'stats'.$RSDB_URI_slash.'">Database Statistics</a></li>
        <li><a href="'.$RSDB_intern_link_view_EX.'help'.$RSDB_URI_slash.'">Help &amp; FAQ</a></li>
      </ol>
      <br />

      <div class="navTitle">Search</div>
      <ol>
        <li>
          <div id="ajaxsearchbar" align="center" style="display: none">
          <div align="center">
            <label for="searchbar" accesskey="s"></label>
            <input name="searchbar" type="text" id="searchbar" tabindex="0" onkeyup="'."loadItemList(this.value,'bar','comp','ajaxloadbar','sresultbar');".'" size="17" maxlength="50" style="font-family: Verdana; font-size: x-small; font-style: normal;" />
          </div>
          <div id="sresultbar" style="display: none" align="left"></div>
          <div style="text-align: center;">
            <img id="ajaxloadbar" src="images/ajax_loading.gif" style="display: none" />
          </div>
        </li>
      </ol>
      <br />

      <script type="text/javascript">
      <!--'."
        document.getElementById('ajaxsearchbar').style.display = 'block';
        document.getElementById('noscriptsearchbar').style.display = 'none';
      ".'-->
      </script>

      <div class="navTitle">Language</div>   
      <ol>
        <li> 
          <div align="center">';

    // get selected language
    if (empty($_GET['lang']) && isset($_COOKIE['roscms_usrset_lang'])) {
      $lang = $_COOKIE['roscms_usrset_lang'];
      if (substr($lang, -1) == '/') {
        $lang = substr($lang, -1);
      }
    }

    // check if language exists, use fallback if needed
    $lang = CLanguage::validate($lang);

    // get name of currently used language
    $stmt=CDBConnection::getInstance()->prepare("SELECT lang_name FROM rsdb_languages WHERE lang_id = :lang_id");
    $stmt->bindParam('lang_id',$lang,PDO::PARAM_STR);
    $stmt->execute();
    $language_name = $stmt->fetchColumn();

    echo '
      <select id="select" size="1" name="select" class="selectbox" style="width:140px" onchange="'."window.open(this.options[this.selectedIndex].value,'_main');".'">
        <optgroup label="current language">
          <option value="#">'.$language_name.'</option>
        </optgroup>
        <optgroup label="most popular">';
    $stmt=CDBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM rsdb_languages WHERE lang_id != :lang_id");
    $stmt->bindParam('lang_id',$lang,PDO::PARAM_STR);
    $stmt->execute();
    while ($language=$stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '
        <option value="'.$RSDB_intern_link_language.$language['lang_id'].'">'.$language['lang_name'].'</option>';
    }

    echo '
              </optgroup>
            </select>
          </li>
        </ol>
      </td>
      <td id="content">';
  } // end of member function navigation



  /**
   * output site footer
   *
   * @access protected
   */
  protected function footer( )
  {
    // footer information
    echo '
              </td>
            </tr>
          </table>


          <hr size="1" />
          <address style="text-align:center">
            <a href="http://www.reactos.org" onmousedown="'."return clk(this.href,'res','');".'">ReactOS</a> is a registered trademark or a trademark of <a href="http://www.reactos.org/?page=foundation">ReactOS Foundation</a> in the United States and other countries.<br />
            Copyright  &copy; Klemens Friedl 2005-'.date('Y').', All rights reserved.
          </address>

        </body>
      </html>';
  } // end of member function footer



  /**
   * register a css file to be included in the header
   *
   * @param string name path & filename to a css file
   * @param bool|string condition conditional statement (used by IE) to e.g. include sheets only for specific IE versions
   * @access protected
   */
  protected function register_css( $name, $condition = false )
  {
    $this->css_files[] = array('name'=>$name, 'condition'=>$condition);
  } // end of member function register_css



  /**
   * register a javascript file to be included in the header
   *
   * @param string name path & filename to a javascript file
   * @access protected
   */
  protected function register_js( $name )
  {
    $this->js_files[] = $name;
  } // end of member function register_js



} // end of HTML
?>
