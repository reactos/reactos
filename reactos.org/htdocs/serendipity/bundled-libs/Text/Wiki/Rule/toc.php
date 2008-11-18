<?php
/* vim: set expandtab tabstop=4 shiftwidth=4: */
// +----------------------------------------------------------------------+
// | PHP version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2003 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at                              |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Paul M. Jones <pmjones@ciaweb.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: toc.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find all heading tokens and
* build a table of contents.  The [[toc]] tag gets replaced with a list
* of all the level-2 through level-6 headings.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/


class Text_Wiki_Rule_toc extends Text_Wiki_Rule {
    
    
    /**
    * 
    * The regular expression used to parse the source text and find
    * matches conforming to this rule.  Used by the parse() method.
    * 
    * @access public
    * 
    * @var string
    * 
    * @see parse()
    * 
    */
    
    var $regex = "/\[\[toc\]\]/m";
    
    
    /**
    * 
    * The collection of headings (text and levels).
    * 
    * @access public
    * 
    * @var array
    * 
    * @see _getEntries()
    * 
    */
    
    var $entry = array();
    
    
    /**
    * 
    * Custom parsing (have to process heading entries first).
    * 
    * @access public
    * 
    * @see Text_Wiki::parse()
    * 
    */
    
    function parse()
    {
        $this->_getEntries();
        parent::parse();
    }
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' => ['list_start'|'list_end'|'item_end'|'item_end'|'target']
    *
    * 'level' => The heading level (1-6).
    *
    * 'count' => Which heading this is in the list.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return string A token indicating the TOC collection point.
    *
    */
    
    function process(&$matches)
    {
        $output = $this->addToken(array('type' => 'list_start'));
        
        foreach ($this->entry as $key => $val) {
        
            $options = array(
                'type' => 'item_start',
                'count' => $val['count'],
                'level' => $val['level']
            );
            
            $output .= $this->addToken($options);
            
            $output .= $val['text'];
            
            $output .= $this->addToken(array('type' => 'item_end'));
        }
        
        $output .= $this->addToken(array('type' => 'list_end'));
        return $output;
    }
    
    
    /**
    * 
    * Finds all headings in the text and saves them in $this->entry.
    * 
    * @access private
    *
    * @return void
    * 
    */
    
    function _getEntries()
    {
        // the wiki delimiter
        $delim = $this->_wiki->delim;
        
        // list of all TOC entries (h2, h3, etc)
        $this->entry = array();
        
        // the new source text with TOC entry tokens
        $newsrc = '';
        
        // when passing through the parsed source text, keep track of when
        // we are in a delimited section
        $in_delim = false;
        
        // when in a delimited section, capture the token key number
        $key = '';
        
        // TOC entry count
        $count = 0;
        
        // pass through the parsed source text character by character
        $k = strlen($this->_wiki->_source);
        for ($i = 0; $i < $k; $i++) {
            
            // the current character
            $char = $this->_wiki->_source{$i};
            
            // are alredy in a delimited section?
            if ($in_delim) {
            
                // yes; are we ending the section?
                if ($char == $delim) {
                    
                    // yes, get the replacement text for the delimited
                    // token number and unset the flag.
                    $key = (int)$key;
                    $rule = $this->_wiki->_tokens[$key][0];
                    $opts = $this->_wiki->_tokens[$key][1];
                    $in_delim = false;
                    
                    // is the key a start heading token
                    // of level 2 or deeper?
                    if ($rule == 'heading' &&
                        $opts['type'] == 'start' &&
                        $opts['level'] > 1) {
                        
                        // yes, add a TOC target link to the
                        // tokens array...
                        $token = $this->addToken(
                            array(
                                'type' => 'target',
                                'count' => $count,
                                'level' => $opts['level']
                            )
                        );
                        
                        // ... and to the new source, before the
                        // heading-start token.
                        $newsrc .= $token . $delim . $key . $delim;
                        
                        // retain the toc item
                        $this->entry[] = array (
                            'count' => $count,
                            'level' => $opts['level'],
                            'text' => $opts['text']
                        );
                        
                        // increase the count for the next entry
                        $count++;
                        
                    } else {
                        // not a heading-start of 2 or deeper.
                        // re-add the delimited token number
                        // as it was in the original source.
                        $newsrc .= $delim . $key . $delim;
                    }
                    
                } else {
                
                    // no, add to the delimited token key number
                    $key .= $char;
                    
                }
                
            } else {
                
                // not currently in a delimited section.
                // are we starting into a delimited section?
                if ($char == $delim) {
                    // yes, reset the previous key and
                    // set the flag.
                    $key = '';
                    $in_delim = true;
                } else {
                    // no, add to the output as-is
                    $newsrc .= $char;
                }
            }
        }
        
        // replace with changed source text
        $this->_wiki->_source = $newsrc;
        
        
        /*
        // PRIOR VERSION
        // has problems mistaking marked-up numbers for delimited tokens
        
        // creates target tokens, retrieves heading level and text
        $this->entry = array();
        $count = 0;
        
        // loop through all tokens and get headings
        foreach ($this->_wiki->_tokens as $key => $val) {
            
            // only get heading starts of level 2 or deeper
            if ($val[0] == 'heading' &&
                $val[1]['type'] == 'start' &&
                $val[1]['level'] > 1) {
                
                // the level of this header
                $level = $val[1]['level'];
                
                // the text of this header
                $text = $val[1]['text'];
                
                // add a toc-target link to the tokens array
                $token = $this->addToken(
                    array(
                        'type' => 'target',
                        'count' => $count,
                        'level' => $level
                    )
                );
                
                // put the toc target token in front of the
                // heading-start token
                $start = $delim . $key . $delim;
                $this->_wiki->_source = str_replace($start, $token.$start,
                    $this->_wiki->_source);
                
                // retain the toc item
                $this->entry[] = array (
                    'count' => $count,
                    'level' => $level,
                    'text' => $text
                );
                
                // increase the count for the next item
                $count++;
            }
        }
        */
    }

    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function renderXhtml($options)
    {
        // type, count, level
        extract($options);
        
        // the prefix used for anchor names
        $prefix = 'toc';
        
        if ($type == 'target') {
            // ... generate an anchor.
            return "<a id=\"$prefix$count\"></a>";
        }
        
        if ($type == 'list_start') {
            return "<p>\n";
        }
        
        if ($type == 'list_end') {
            return "</p>\n";
        }
        
        if ($type == 'item_start') {
            
            // build some indenting spaces for the text
            $indent = ($level - 2) * 4;
            $pad = str_pad('', $indent);
            $pad = str_replace(' ', '&nbsp;', $pad);
            
            // add an extra linebreak in front of heading-2
            // entries (makes for nice section separations)
            if ($level <= 2 && $count > 0) {
                $pad = "<br />\n$pad";
            }
            
            // create the entry line as a link to the toc anchor
            return "$pad<a href=\"#$prefix$count\">";
        }
        
        if ($type == 'item_end') {
            return "</a><br />\n";
        }
    }

}
?>
