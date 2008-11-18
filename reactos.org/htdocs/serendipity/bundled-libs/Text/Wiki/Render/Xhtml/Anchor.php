<?php

/**
* 
* This class renders an anchor target name in XHTML.
*
* @author Manuel Holtgrewe <purestorm at ggnore dot net>
*
* @author Paul M. Jones <pmjones at ciaweb dot net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Render_Xhtml_Anchor extends Text_Wiki_Render {
    
    var $conf = array(
        'css' => null
    );
    
    function token($options)
    {
        extract($options); // $type, $name
        
        if ($type == 'start') {
            $css = $this->formatConf(' class="%s"', 'css');
            $format = "<a$css id=\"%s\">";
            return sprintf($format ,$name);
        }
        
        if ($type == 'end') {
            return '</a>';
        }
    }
}

?>