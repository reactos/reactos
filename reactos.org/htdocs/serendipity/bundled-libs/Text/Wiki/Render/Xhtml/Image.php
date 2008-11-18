<?php
class Text_Wiki_Render_Xhtml_Image extends Text_Wiki_Render {

    var $conf = array(
        'base' => '/',
        'css'  => null,
        'css_link' => null
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
        // note the image source
        $src = $options['src'];
        
        // is the source a local file or URL?
        if (strpos($src, '://') === false) {
            // the source refers to a local file.
            // add the URL base to it.
            $src = $this->getConf('base', '/') . $src;
        }
        
        // stephane@metacites.net
        // is the image clickable? 
        if (isset($options['attr']['link'])) {
            // yes, the image is clickable.
            // are we linked to a URL or a wiki page?
            if (strpos($options['attr']['link'], '://')) {
                // it's a URL
                $href = $options['attr']['link'];
            } else {
                // it's a WikiPage; assume it exists.
                /** @todo This needs to honor sprintf wikilinks (pmjones) */
                /** @todo This needs to honor interwiki (pmjones) */
                /** @todo This needs to honor freelinks (pmjones) */
                $href = $this->wiki->getRenderConf('xhtml', 'wikilink', 'view_url') .
                    $options['attr']['link'];
            }
        } else {
            // image is not clickable.
            $href = null;
        }
        // unset so it won't show up as an attribute
        unset($options['attr']['link']);
        
        // stephane@metacites.net -- 25/07/2004 
        // we make up an align="center" value for the <img> tag.
        if (isset($options['attr']['align']) &&
            $options['attr']['align'] == 'center') {
            
            // unset so it won't show up as an attribute
            unset($options['attr']['align']);
            
            // make sure we have a style attribute
            if (! isset($options['attr']['style'])) {
                // no style, set up a blank one
                $options['attr']['style'] = '';
            } else {
                // style exists, add a space
                $options['attr']['style'] .= ' ';
            }
            
            // add a "center" style to the existing style.
            $options['attr']['style'] .=
                'display: block; margin-left: auto; margin-right: auto;';
        }
        
        // stephane@metacites.net -- 25/07/2004 
        // try to guess width and height
        if (! isset($options['attr']['width']) &&
            ! isset($options['attr']['height'])) {
            
            // does the source refer to a local file or a URL?
            if (strpos($src,'://')) {
                // is a URL link
                $imageFile = $src;
            } else {
                // is a local file
                $imageFile = $_SERVER['DOCUMENT_ROOT'] . $src;
            }
            
            // attempt to get the image size
            $imageSize = @getimagesize($imageFile);
            
            if (is_array($imageSize)) {
                $options['attr']['width'] = $imageSize[0];
                $options['attr']['height'] = $imageSize[1];
            }
            
        }
        
        // start the HTML output
        $output = '<img src="' . htmlspecialchars($src) . '"';
        
        // get the CSS class but don't add it yet
        $css = $this->formatConf(' class="%s"', 'css');
        
        // add the attributes to the output, and be sure to
        // track whether or not we find an "alt" attribute
        $alt = false;
        foreach ($options['attr'] as $key => $val) {
            
            // track the 'alt' attribute
            if (strtolower($key) == 'alt') {
                $alt = true;
            }
            
            // the 'class' attribute overrides the CSS class conf
            if (strtolower($key) == 'class') {
                $css = null;
            }
            
            $key = htmlspecialchars($key);
            $val = htmlspecialchars($val);
            $output .= " $key=\"$val\"";
        }
        
        // always add an "alt" attribute per Stephane Solliec
        if (! $alt) {
            $alt = htmlspecialchars(basename($options['src']));
            $output .= " alt=\"$alt\"";
        }
        
        // end the image tag with the automatic CSS class (if any)
        $output .= "$css />";
        
        // was the image clickable?
        if ($href) {
            // yes, add the href and return
            $href = htmlspecialchars($href);
            $css = $this->formatConf(' class="%s"', 'css_link');
            $output = "<a$css href=\"$href\">$output</a>";
        }
        
        return $output;
    }
}
?>