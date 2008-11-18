<?php

/* Copyright 2002-2003 Edward Swindelles (ed@readinged.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

if (!defined('ONYX_RSS_VERS'))
{
   define('ONYX_RSS_VERS', '1.0');
   define('ONYX_ERR_NO_PARSER', '<a href="http://www.php.net/manual/en/ref.xml.php">PHP\'s XML Extension</a> is not loaded or available.');
   define('ONYX_ERR_NOT_WRITEABLE', 'The specified cache directory is not writeable.');
   define('ONYX_ERR_INVALID_URI', 'The specified file could not be opened.');
   define('ONYX_ERR_INVALID_ITEM', 'Invalid item index specified.');
   define('ONYX_ERR_NO_STREAM', 'Could not open the specified file.  Check the path, and make sure that you have write permissions to this file.');
   define('ONYX_META', 'meta');
   define('ONYX_ITEMS', 'items');
   define('ONYX_IMAGE', 'image');
   define('ONYX_TEXTINPUT', 'textinput');
   define('ONYX_NAMESPACES', 'namespaces');
   define('ONYX_CACHE_AGE', 'cache_age');
   define('ONYX_FETCH_ASSOC', 1);
   define('ONYX_FETCH_OBJECT', 2);
}

class ONYX_RSS
{
   var $parser;
   var $conf;
   var $rss;
   var $data;
   var $type;
   /* For when PHP v.5 is released
    * http://www.phpvolcano.com/eide/php5.php?page=variables
    * private $parser;
    * private $conf;
    * private $rss;
    * private $data;
    * private $type;
   */

   function ONYX_RSS($charset = 'UTF-8')
   {
      $this->__construct($charset);
   }

   // Forward compatibility with PHP v.5
   // http://www.phpvolcano.com/eide/php5.php?page=start
   function __construct($charset = 'UTF-8')
   {
      $this->conf = array();
      $this->conf['error'] = '<br /><strong>Error on line %s of '.__FILE__.'</strong>: %s<br />';
      $this->conf['cache_path'] = dirname(__FILE__);
      $this->conf['cache_time'] = 180;
      $this->conf['debug_mode'] = true;
      $this->conf['fetch_mode'] = ONYX_FETCH_ASSOC;

      if (!function_exists('xml_parser_create'))
      {
         $this->raiseError((__LINE__-2), ONYX_ERR_NO_PARSER);
         return false;
      }

      if ($charset == 'native') {
         $charset = LANG_CHARSET;
      }
      $this->parser = @xml_parser_create($charset);
      if (!is_resource($this->parser))
      {
         $this->raiseError((__LINE__-3), ONYX_ERR_NO_PARSER);
         return false;
      }
      xml_set_object($this->parser, $this);
      xml_parser_set_option($this->parser, XML_OPTION_CASE_FOLDING, false);
      @xml_parser_set_option($this->parser, XML_OPTION_TARGET_ENCODING, LANG_CHARSET);
      xml_set_element_handler($this->parser, 'tag_open', 'tag_close');
      xml_set_character_data_handler($this->parser, 'cdata');
   }

   function parse($uri, $file=false, $time=false, $local=false)
   {
      $this->rss = array();
      $this->rss['cache_age'] = 0;
      $this->rss['current_tag'] = '';
      $this->rss['index'] = 0;
      $this->rss['output_index'] = -1;
      $this->data = array();

      if ($file)
      {
         if (!is_writeable($this->conf['cache_path']))
         {
            $this->raiseError((__LINE__-2), ONYX_ERR_NOT_WRITEABLE);
            return false;
         }
         $file = str_replace('//', '/', $this->conf['cache_path'].'/'.$file);
         if (!$time)
            $time = $this->conf['cache_time'];
         $this->rss['cache_age'] = file_exists($file) ? ceil((time() - filemtime($file)) / 60) : 0;

         clearstatcache();
         if (!$local && file_exists($file))
            if (($mod = $this->mod_time($uri)) === false)
            {
               $this->raiseError((__LINE__-2), ONYX_ERR_INVALID_URI);
               return false;
            }
            else
               $mod = ($mod !== 0) ? strtotime($mod) : (time()+3600);
         elseif ($local)
            $mod = (file_exists($file) && ($m = filemtime($uri))) ? $m : time()+3600;
      }
      if ( !$file ||
           ($file && !file_exists($file)) ||
           ($file && file_exists($file) && $time <= $this->rss['cache_age'] && $mod >= (time() - ($this->rss['cache_age'] * 60))))
      {
         clearstatcache();
         
         require_once S9Y_PEAR_PATH . 'HTTP/Request.php';
         $req = &new HTTP_Request($uri, array('allowRedirects' => true, 'maxRedirects' => 5));
         $res = $req->sendRequest();
        
         if (PEAR::isError($res) || $req->getResponseCode() != '200')
         {
            $this->raiseError((__LINE__-2), ONYX_ERR_INVALID_URI . ' (#' . $req->getResponseCode() . ')');
            return false;
         }

         $fContent = $req->getResponseBody();
         if (@preg_match('@<?xml[^>]*encoding="([^"]+)"@i', $fContent, $xml_encoding)) {
            $this->rss['encoding'] = strtolower($xml_encoding[1]);
         }

         $parsedOkay = xml_parse($this->parser, $fContent, true);
         if (!$parsedOkay && xml_get_error_code($this->parser) != XML_ERROR_NONE)
         {
            $this->raiseError((__LINE__-3), 'File has an XML error (<em>'.xml_error_string(xml_get_error_code($this->parser)).'</em> at line <em>'.xml_get_current_line_number($this->parser).'</em>).');
            return false;
         }

         clearstatcache();
         if ($file)
         {
            if (!($cache = @fopen($file, 'w')))
            {
               $this->raiseError((__LINE__-2), 'Could not write to cache file (<em>'.$file.'</em>).  The path may be invalid or you may not have write permissions.');
               return false;
            }
            fwrite($cache, serialize($this->data));
            fclose($cache);
            $this->rss['cache_age'] = 0;
         }
      }
      else
      {
         clearstatcache();
         if (!($fp = @fopen($file, 'r')))
         {
            $this->raiseError((__LINE__-2), 'Could not read contents of cache file (<em>'.$cache_file.'</em>).');
            return false;
         }
         $this->data = unserialize(fread($fp, filesize($file)));
         fclose($fp);
      }
      return true;
   }

   function parseLocal($uri, $file=false, $time=false)
   {
      return $this->parse($uri, $file, $time, true);
   }

   //private function tag_open($parser, $tag, $attrs)
   function tag_open($parser, $tag, $attrs)
   {
      $this->rss['current_tag'] = $tag = strtolower($tag);
      switch ($tag)
      {
         case 'channel':
         case 'image':
         case 'textinput':
            $this->type = $tag;
            break;
         case 'item':
            $this->type = $tag;
            $this->rss['index']++;
            break;
         default:
            break;
      }
      if (sizeof($attrs))
         foreach ($attrs as $k => $v)
            if (strpos($k, 'xmlns') !== false)
               $this->data['namespaces'][$k] = $v;
   }

   //private function tag_close($parser, $tag){}
   function tag_close($parser, $tag){}

   //private function cdata($parser, $cdata)
   function cdata($parser, $cdata)
   {
      if (strlen(trim($cdata)) && $cdata != "\n")
         switch ($this->type)
         {
            case 'channel':
            case 'image':
            case 'textinput':
               (!isset($this->data[$this->type][$this->rss['current_tag']]) ||
                !strlen($this->data[$this->type][$this->rss['current_tag']])) ?
                  $this->data[$this->type][$this->rss['current_tag']] = $cdata :
                  $this->data[$this->type][$this->rss['current_tag']].= $cdata;
               break;
            case 'item':
               (!isset($this->data['items'][$this->rss['index']-1][$this->rss['current_tag']]) ||
                !strlen($this->data['items'][$this->rss['index']-1][$this->rss['current_tag']])) ?
                  $this->data['items'][$this->rss['index']-1][$this->rss['current_tag']] = $cdata :
                  $this->data['items'][$this->rss['index']-1][$this->rss['current_tag']].= $cdata;
               break;
         }
   }

   function getData($type)
   {
      if ($type == ONYX_META)
         return $this->conf['fetch_mode'] == 1 ? $this->data['channel'] : (object)$this->data['channel'];
      if ($type == ONYX_IMAGE)
         return $this->conf['fetch_mode'] == 1 ? $this->data['image'] : (object)$this->data['image'];
      if ($type == ONYX_TEXTINPUT)
         return $this->conf['fetch_mode'] == 1 ? $this->data['textinput'] : (object)$this->data['textinput'];
      if ($type == ONYX_ITEMS)
      {
         if ($this->conf['fetch_mode'] == 1)
            return $this->data['items'];

         $temp = array();
         for ($i=0; $i < sizeof($this->data['items']); $i++)
            $temp[] = (object)$this->data['items'][$i];

         return $temp;
      }
      if ($type == ONYX_NAMESPACES)
         return $this->conf['fetch_mode'] == 1 ? $this->data['namespaces'] : (object)$this->data['namespaces'];
      if ($type == ONYX_CACHE_AGE)
         return $this->rss['cache_age'];

      return false;
   }

   function numItems()
   {
      return sizeof($this->data['items']);
   }

   function getNextItem($max=false)
   {
      $type = $this->conf['fetch_mode'];
      $this->rss['output_index']++;
      if (($max && $this->rss['output_index'] > $max) || !isset($this->data['items'][$this->rss['output_index']]))
         return false;

      return ($type == ONYX_FETCH_ASSOC) ? $this->data['items'][$this->rss['output_index']] :
             (($type == ONYX_FETCH_OBJECT) ? (object)$this->data['items'][$this->rss['output_index']] : false);
   }

   function itemAt($num)
   {
      if (!isset($this->data['items'][$num]))
      {
         $this->raiseError((__LINE__-3), ONYX_ERR_INVALID_ITEM);
         return false;
      }

      $type = $this->conf['fetch_mode'];
      return ($type == ONYX_FETCH_ASSOC) ? $this->data['items'][$num] :
             (($type == ONYX_FETCH_OBJECT) ? (object)$this->data['items'][$num] : false);
   }

   function startBuffer($file=false)
   {
      $this->conf['output_file'] = $file;
      ob_start();
   }

   function endBuffer()
   {
      if (!$this->conf['output_file'])
         ob_end_flush();
      else
      {
         if (!($fp = @fopen($this->conf['output_file'], 'w')))
         {
            $this->raiseError((__LINE__-2), ONYX_ERR_NO_STREAM);
            ob_end_flush();
            return;
         }
         fwrite($fp, ob_get_contents());
         fclose($fp);
         ob_end_clean();
      }
   }

   //private function raiseError($line, $err)
   function raiseError($line, $err)
   {
      if ($this->conf['debug_mode'])
         printf($this->conf['error'], $line, $err);
   }

   function setCachePath($path)
   {
      $this->conf['cache_path'] = $path;
   }

   function setExpiryTime($time)
   {
      $this->conf['cache_time'] = $time;
   }

   function setDebugMode($state)
   {
      $this->conf['debug_mode'] = (bool)$state;
   }

   function setFetchMode($mode)
   {
      $this->conf['fetch_mode'] = $mode;
   }

   //private function mod_time($uri)
   function mod_time($uri)
   {
      if (function_exists('version_compare') && version_compare(phpversion(), '4.3.0') >= 0)
      {
         require_once S9Y_PEAR_PATH . 'HTTP/Request.php';
         $req = &new HTTP_Request($uri);

         if (PEAR::isError($req->sendRequest()) || $req->getResponseCode() != '200')
            return false;

         $fHeader = $req->getResponseHeader();
         if (isset($fHeader['last-modified'])) {
            $modtime = $fHeader['last-modified'];
        }
      }
      else
      {
         $parts = parse_url($uri);
         $host  = $parts['host'];
         $path  = $parts['path'];

         if (!($fp = @fsockopen($host, 80)))
            return false;

         $req = "HEAD $path HTTP/1.1\r\nUser-Agent: PHP/".phpversion();
         $req.= "\r\nHost: $host\r\nAccept: */*\r\n\r\n";
         fputs($fp, $req);

         while (!feof($fp))
         {
            $str = fgets($fp, 4096);
            if (strpos(strtolower($str), 'last-modified') !== false)
            {
               $modtime = substr($str, 15);
               break;
            }
         }
         fclose($fp);
      }
      return (isset($modtime)) ? $modtime : 0;
   }
}


?>
