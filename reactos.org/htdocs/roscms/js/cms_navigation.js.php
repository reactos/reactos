<?php
  include_once('../config.php'); ?>



/**
 * reloads the page without asking if we want to exit
 */
function refreshPage( )
{
  exitmsg = '';

  window.location.reload();
}



/**
 * load another CMS branch
 *
 * @param branch CMS branch to be loaded
 */
function loadBranch( branch )
{
  exitmsg = '';

  window.location.href = '<?php echo RosCMS::getInstance()->pathRosCMS(); ?>?page=data&branch='+branch;
}
