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
 * class HTML_CMS_Welcome
 * 
 */
class HTML_CMS_Welcome extends HTML_CMS
{


  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '', $page_css = 'roscms' )
  {
    $this->branch = 'welcome';

    parent::__construct( $page_title, $page_css);
  }


  /**
   *
   *
   * @access protected
   */
  protected function body( )
  {
    global $roscms_standard_language;

    echo_strip('
      <p>&nbsp;</p>
      <h2>Welcome</h2>
      <p style="font-weight: bold;">Welcome to RosCMS v3</p>
      <br />
      <h3>Content</h3>
      <ul style="font-weight: bold;">
        <li><a href="#web_news">RosCMS Website News</a></li>
        <li><a href="#web_news_langgroup">Translator Information</a></li>
      </ul>
      <br />
      <a name="web_news"></a><h3>');echo Data::getContent('web_news', 'system', 'en', 'title', 'stext');echo_strip('</h3>
      <p style="font-weight: bold;">');echo Data::getContent('web_news', 'system', 'en', 'heading', 'stext').'</p>'.
      Data::getContent('web_news', 'system', 'en', 'content', 'text').'<br />';

      if (ThisUser::getInstance()->isMemberOfGroup('translator', 'transmaint')) {
    
        $stmt=DBConnection::getInstance()->prepare("SELECT user_language FROM users WHERE user_id = :user_id LIMIT 1");
        $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
        $stmt->execute();
        $user_lang = $stmt->fetchColumn();

        if ($user_lang != '') {
          echo_strip('
            <a name="web_news_langgroup"></a>
            <h3>Translator Information</h3>');

          // try to get content in local language, otherwise use standard language
          $content = Data::getContent('web_news_langgroup', 'system', $user_lang, 'content', 'text');
          if ($content == '') {
            $content = Data::getContent('web_news_langgroup', 'system', $roscms_standard_language, 'content', 'text');
          }
          echo $content;
        }
        else {
          echo_strip('
            <h2>Please set your favorite language in the myReactOS settings.</h2>
            <p>This language will also be the default language to that you can translate content.</p>');
        }
      }
    echo '<br />';
  }


} // end of HTML_CMS_Welcome
?>
