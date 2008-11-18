<?php
// $Id: Revise.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to find source text marked for
* revision.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Revise extends Text_Wiki_Parse {
    
    
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
    
    var $regex = "/\@\@({*?.*}*?)\@\@/U";
    
    
    /**
    * 
    * Config options.
    * 
    * @access public
    * 
    * @var array
    *
    */
    
    var $conf = array(
        'delmark' => '---',
        'insmark' => '+++'
    );
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' => ['start'|'end'] The starting or ending point of the
    * inserted text.  The text itself is left in the source.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return string A pair of delimited tokens to be used as a
    * placeholder in the source text surrounding the teletype text.
    *
    */
    
    function process(&$matches)
    {
        $output = '';
        $src = $matches[1];
        $delmark = $this->getConf('delmark'); // ---
        $insmark = $this->getConf('insmark'); // +++
        
        // '---' must be before '+++' (if they both appear)
        $del = strpos($src, $delmark);
        $ins = strpos($src, $insmark);
        
        // if neither is found, return right away
        if ($del === false && $ins === false) {
            return $matches[0];
        }
        
        // handle text to be deleted
        if ($del !== false) {
            
            // move forward to the end of the deletion mark
            $del += strlen($delmark);
            
            if ($ins === false) {
                // there is no insertion text following
                $text = substr($src, $del);
            } else {
                // there is insertion text following,
                // mitigate the length
                $text = substr($src, $del, $ins - $del);
            }
            
            $output .= $this->wiki->addToken(
                $this->rule, array('type' => 'del_start')
            );
            
            $output .= $text;
            
            $output .= $this->wiki->addToken(
                $this->rule, array('type' => 'del_end')
            );
        }
        
        // handle text to be inserted
        if ($ins !== false) {
            
            // move forward to the end of the insert mark
            $ins += strlen($insmark);
            $text = substr($src, $ins);
            
            $output .= $this->wiki->addToken(
                $this->rule, array('type' => 'ins_start')
            );
            
            $output .= $text;
            
            $output .= $this->wiki->addToken(
                $this->rule, array('type' => 'ins_end')
            );
        }
        
        return $output;
    }
}
?>