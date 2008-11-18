<?php



	// To prevent hacking activity:
	if ( !defined('ROST') )
	{
		die(" ");
	}

	if($ROST_intern_user_id != 0) {

?><div class="navTitle"><?php echo $ROST_langres['Account']; ?></div>
	<ol>
		<li>
			&nbsp;Nick:&nbsp;<?php echo substr($ROST_USER_name, 0, 9);
			 ?>
		</li>
		<li><a href="<?php echo $ROST_intern_link_roscms_page; ?>logout"><?php echo $ROST_langres['Logout']; ?></a></li>
	</ol>
</div>
<p></p><?php
	}
	else {
		?><form action="?page=login" method="post"><div class="navTitle">Login</div>   
		  <ol>
		<li><a href="<?php echo $ROST_intern_link_roscms_page; ?>login"><?php echo $ROST_langres['Global_Login_System']; ?></a></li>
		<li><a href="<?php echo $ROST_intern_link_roscms_page; ?>register"><?php echo $ROST_langres['Register_Account']; ?></a></li>
		  </ol></div></form>
		  <p></p><?php
	}
?>
