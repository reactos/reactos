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

class Text_Wiki_Render_Plain_Anchor extends Text_Wiki_Render {
    
    function token($options)
    {
        return $options['name'];
    }
}

?>
