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
// $Id: colortext.php,v 1.2 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked for
* coloring.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_colortext extends Text_Wiki_Rule {
    
    var $colors = array(
        'aqua',
        'black',
        'blue',
        'fuchsia',
        'gray',
        'green',
        'lime',
        'maroon',
        'navy',
        'olive',
        'purple',
        'red',
        'silver',
        'teal',
        'white',
        'yellow'
    );
    
    
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
    
    var $regex = "/\#\#(.+?)\|(.+?)\#\#/";
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' => ['start'|'end'] The starting or ending point of the
    * emphasized text.  The text itself is left in the source.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return string A pair of delimited tokens to be used as a
    * placeholder in the source text surrounding the text to be
    * emphasized.
    *
    */
    
    function process(&$matches)
    {
        $start = $this->addToken(
        	array(
        		'type' => 'start',
        		'color' => $matches[1]
        	)
        );
        
        $end = $this->addToken(
        	array(
        		'type' => 'end',
        		'color' => $matches[1]
        	)
        );
        
        return $start . $matches[2] . $end;
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
        $type = $options['type'];
        $color = $options['color'];
        
        if (! in_array($color, $this->colors)) {
            $color = '#' . $color;
        }
        
        if ($type == 'start') {
            return "<span style=\"color: $color;\">";
        }
        
        if ($options['type'] == 'end') {
            return '</span>';
        }
    }
}
?>
