<?php
class Text_Wiki_Render_Latex_Image extends Text_Wiki_Render {

    var $conf = array(
        'base' => '/'
    );
    
    
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
    
    function token($options)
    {
        return 'Image: NI';
        
        $src = '"' .
            $this->getConf('base', '/') .
            $options['src'] . '"';
        
        if (isset($options['attr']['link'])) {
        
            // this image has a link
            if (strpos($options['attr']['link'], '://')) {
                // it's a URL
                $href = $options['attr']['link'];
            } else {
                $href = $this->wiki->getRenderConf('xhtml', 'wikilink', 'view_url') .
                    $options['attr']['link'];
            }
            
        } else {
            // image is not linked
            $href = null;
        }
        
        // unset these so they don't show up as attributes
        unset($options['attr']['link']);
        
        $attr = '';
        $alt = false;
        foreach ($options['attr'] as $key => $val) {
            if (strtolower($key) == 'alt') {
                $alt = true;
            }
            $attr .= " $key=\"$val\"";
        }
        
        // always add an "alt" attribute per Stephane Solliec
        if (! $alt) {
            $attr .= ' alt="' . basename($options['src']) . '"';
        }
        
        if ($href) {
            return "<a href=\"$href\"><img src=$src$attr/></a>";
        } else {
            return "<img src=$src$attr/>";
        }
    }
}
?>