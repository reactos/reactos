<?php
// $Id: Toc.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to find all heading tokens and
* build a table of contents.  The [[toc]] tag gets replaced with a list
* of all the level-2 through level-6 headings.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/


class Text_Wiki_Parse_Toc extends Text_Wiki_Parse {
    
    
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
    
    var $regex = "/\n\[\[toc( .*)?\]\]\n/m";
    
    
    /**
    * 
    * Generates a replacement for the matched text.
    *  
    * Token options are:
    * 
    * 'type' => ['list_start'|'list_end'|'item_start'|'item_end'|'target']
    *
    * 'level' => The heading level (1-6).
    *
    * 'count' => Which entry number this is in the list.
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
        $count = 0;
        
        if (isset($matches[1])) {
            $attr = $this->getAttrs(trim($matches[1]));
        } else {
            $attr = array();
        }
        
        $output = $this->wiki->addToken(
            $this->rule,
            array(
                'type' => 'list_start',
                'level' => 0,
                'attr' => $attr
            )
        );
        
        foreach ($this->wiki->getTokens('Heading') as $key => $val) {
            
            if ($val[1]['type'] != 'start') {
                continue;
            }
            
            $options = array(
                'type'  => 'item_start',
                'id'    => $val[1]['id'],
                'level' => $val[1]['level'],
                'count' => $count ++
            );
            
            $output .= $this->wiki->addToken($this->rule, $options);
            
            $output .= $val[1]['text'];
            
            $output .= $this->wiki->addToken(
                $this->rule,
                array(
                    'type' => 'item_end',
                    'level' => $val[1]['level']
                )
            );
        }
        
        $output .= $this->wiki->addToken(
            $this->rule, array(
                'type' => 'list_end',
                'level' => 0
            )
        );
        
        return $output;
    }
}
?>