<?php
/**
 * @defgroup Templates Templates
 * @file
 * @ingroup Templates
 */
if( !defined( 'MEDIAWIKI' ) ) die( -1 );

/**
 * HTML template for Special:Userlogin form
 * @ingroup Templates
 */
class UserloginTemplate extends QuickTemplate {
	function execute() {
		if( $this->data['message'] ) {
?>
	<div class="<?php $this->text('messagetype') ?>box">
		<?php if ( $this->data['messagetype'] == 'error' ) { ?>
			<h2><?php $this->msg('loginerror') ?>:</h2>
		<?php } ?>
		<?php $this->html('message') ?>
	</div>
	<div class="visualClear"></div>
<?php } ?>

<div id="loginstart"><?php $this->msgWiki( 'loginstart' ); ?></div>
<div id="userloginForm">
<form name="userlogin" method="post" action="<?php $this->text('action') ?>">
	<h2><?php $this->msg('login') ?></h2>
	<p id="userloginlink"><?php $this->html('link') ?></p>
	<?php $this->html('header'); /* pre-table point for form plugins... */ ?>
	<div id="userloginprompt"><?php  $this->msgWiki('loginprompt') ?></div>
	<?php if( @$this->haveData( 'languages' ) ) { ?><div id="languagelinks"><p><?php $this->html( 'languages' ); ?></p></div><?php } ?>
	<table>
		<tr>
			<td class="mw-label"><label for='wpName1'><?php $this->msg('yourname') ?></label></td>
			<td class="mw-input">
				<input type='text' class='loginText' name="wpName" id="wpName1"
					tabindex="1"
					value="<?php $this->text('name') ?>" size='20' />
			</td>
		</tr>
		<tr>
			<td class="mw-label"><label for='wpPassword1'><?php $this->msg('yourpassword') ?></label></td>
			<td class="mw-input">
				<input type='password' class='loginPassword' name="wpPassword" id="wpPassword1"
					tabindex="2"
					value="" size='20' />
			</td>
		</tr>
	<?php if( $this->data['usedomain'] ) {
		$doms = "";
		foreach( $this->data['domainnames'] as $dom ) {
			$doms .= "<option>" . htmlspecialchars( $dom ) . "</option>";
		}
	?>
		<tr>
			<td class="mw-label"><?php $this->msg( 'yourdomainname' ) ?></td>
			<td class="mw-input">
				<select name="wpDomain" value="<?php $this->text( 'domain' ) ?>"
					tabindex="3">
					<?php echo $doms ?>
				</select>
			</td>
		</tr>
	<?php } ?>
		<tr>
			<td></td>
			<td class="mw-input">
				<input type='checkbox' name="wpRemember"
					tabindex="4"
					value="1" id="wpRemember"
					<?php if( $this->data['remember'] ) { ?>checked="checked"<?php } ?>
					/> <label for="wpRemember"><?php $this->msg('remembermypassword') ?></label>
			</td>
		</tr>
		<tr>
			<td></td>
			<td class="mw-submit">
				<input type='submit' name="wpLoginattempt" id="wpLoginattempt" tabindex="5" value="<?php $this->msg('login') ?>" />&nbsp;<?php if( $this->data['useemail'] && $this->data['canreset']) { ?><input type='submit' name="wpMailmypassword" id="wpMailmypassword"
					tabindex="6"
									value="<?php $this->msg('mailmypassword') ?>" />
				<?php } ?>
			</td>
		</tr>
	</table>
<?php if( @$this->haveData( 'uselang' ) ) { ?><input type="hidden" name="uselang" value="<?php $this->text( 'uselang' ); ?>" /><?php } ?>
</form>
</div>
<div id="loginend"><?php $this->msgWiki( 'loginend' ); ?></div>
<?php

	}
}

/**
 * @ingroup Templates
 */
class UsercreateTemplate extends QuickTemplate {
	function addInputItem( $name, $value, $type, $msg ) {
		$this->data['extraInput'][] = array(
			'name' => $name,
			'value' => $value,
			'type' => $type,
			'msg' => $msg,
		);
	}
	
	function execute() {
		if( $this->data['message'] ) {
?>
	<div class="<?php $this->text('messagetype') ?>box">
		<?php if ( $this->data['messagetype'] == 'error' ) { ?>
			<h2><?php $this->msg('loginerror') ?>:</h2>
		<?php } ?>
		<?php $this->html('message') ?>
	</div>
	<div class="visualClear"></div>
<?php } ?>
<div id="userlogin">

<form name="userlogin2" id="userlogin2" method="post" action="<?php $this->text('action') ?>">
	<h2><?php $this->msg('createaccount') ?></h2>
	<p id="userloginlink"><?php $this->html('link') ?></p>
	<?php $this->html('header'); /* pre-table point for form plugins... */ ?>
	<?php if( @$this->haveData( 'languages' ) ) { ?><div id="languagelinks"><p><?php $this->html( 'languages' ); ?></p></div><?php } ?>
	<table>
		<tr>
			<td class="mw-label"><label for='wpName2'><?php $this->msg('yourname') ?></label></td>
			<td class="mw-input">
				<input type='text' class='loginText' name="wpName" id="wpName2"
					tabindex="1"
					value="<?php $this->text('name') ?>" size='20' />
			</td>
		</tr>
		<tr>
			<td class="mw-label"><label for='wpPassword2'><?php $this->msg('yourpassword') ?></label></td>
			<td class="mw-input">
				<input type='password' class='loginPassword' name="wpPassword" id="wpPassword2"
					tabindex="2"
					value="" size='20' />
			</td>
		</tr>
	<?php if( $this->data['usedomain'] ) {
		$doms = "";
		foreach( $this->data['domainnames'] as $dom ) {
			$doms .= "<option>" . htmlspecialchars( $dom ) . "</option>";
		}
	?>
		<tr>
			<td class="mw-label"><?php $this->msg( 'yourdomainname' ) ?></td>
			<td class="mw-input">
				<select name="wpDomain" value="<?php $this->text( 'domain' ) ?>"
					tabindex="3">
					<?php echo $doms ?>
				</select>
			</td>
		</tr>
	<?php } ?>
		<tr>
			<td class="mw-label"><label for='wpRetype'><?php $this->msg('yourpasswordagain') ?></label></td>
			<td class="mw-input">
				<input type='password' class='loginPassword' name="wpRetype" id="wpRetype"
					tabindex="4"
					value=""
					size='20' />
			</td>
		</tr>
		<tr>
			<?php if( $this->data['useemail'] ) { ?>
				<td class="mw-label"><label for='wpEmail'><?php $this->msg('youremail') ?></label></td>
				<td class="mw-input">
					<input type='text' class='loginText' name="wpEmail" id="wpEmail"
						tabindex="5"
						value="<?php $this->text('email') ?>" size='20' />
					<div class="prefsectiontip">
						<?php if( $this->data['emailrequired'] ) {
									$this->msgWiki('prefs-help-email-required');
						      } else {
									$this->msgWiki('prefs-help-email');
						      } ?>
					</div>
				</td>
			<?php } ?>
			<?php if( $this->data['userealname'] ) { ?>
				</tr>
				<tr>
					<td class="mw-label"><label for='wpRealName'><?php $this->msg('yourrealname') ?></label></td>
					<td class="mw-input">
						<input type='text' class='loginText' name="wpRealName" id="wpRealName"
							tabindex="6"
							value="<?php $this->text('realname') ?>" size='20' />
						<div class="prefsectiontip">
							<?php $this->msgWiki('prefs-help-realname'); ?>
						</div>
					</td>
			<?php } ?>
		</tr>
		<tr>
			<td></td>
			<td class="mw-input">
				<input type='checkbox' name="wpRemember"
					tabindex="7"
					value="1" id="wpRemember"
					<?php if( $this->data['remember'] ) { ?>checked="checked"<?php } ?>
					/> <label for="wpRemember"><?php $this->msg('remembermypassword') ?></label>
			</td>
		</tr>
<?php
		$tabIndex = 8;
		if ( isset( $this->data['extraInput'] ) && is_array( $this->data['extraInput'] ) ) {
			foreach ( $this->data['extraInput'] as $inputItem ) { ?>
		<tr>
			<?php 
				if ( !empty( $inputItem['msg'] ) && $inputItem['type'] != 'checkbox' ) {
					?><td class="mw-label"><label for="<?php 
					echo htmlspecialchars( $inputItem['name'] ); ?>"><?php
					$this->msgWiki( $inputItem['msg'] ) ?></label><?php
				} else {
					?><td><?php
				}
			?></td>
			<td class="mw-input">
				<input type="<?php echo htmlspecialchars( $inputItem['type'] ) ?>" name="<?php
				echo htmlspecialchars( $inputItem['name'] ); ?>"
					tabindex="<?php echo $tabIndex++; ?>"
					value="<?php 
				if ( $inputItem['type'] != 'checkbox' ) {
					echo htmlspecialchars( $inputItem['value'] );
				} else {
					echo '1';
				}					
					?>" id="<?php echo htmlspecialchars( $inputItem['name'] ); ?>"
					<?php 
				if ( $inputItem['type'] == 'checkbox' && !empty( $inputItem['value'] ) )
					echo 'checked="checked"'; 
					?> /> <?php 
					if ( $inputItem['type'] == 'checkbox' && !empty( $inputItem['msg'] ) ) {
						?>
				<label for="<?php echo htmlspecialchars( $inputItem['name'] ); ?>"><?php
					$this->msg( $inputItem['msg'] ) ?></label><?php
					}
				?>
			</td>
		</tr>
<?php				
				
			}
		}
?>
		<tr>
			<td></td>
			<td class="mw-submit">
				<input type='submit' name="wpCreateaccount" id="wpCreateaccount"
					tabindex="<?php echo $tabIndex++; ?>"
					value="<?php $this->msg('createaccount') ?>" />
				<?php if( $this->data['createemail'] ) { ?>
				<input type='submit' name="wpCreateaccountMail" id="wpCreateaccountMail"
					tabindex="<?php echo $tabIndex++; ?>"
					value="<?php $this->msg('createaccountmail') ?>" />
				<?php } ?>
			</td>
		</tr>
	</table>
<?php if( @$this->haveData( 'uselang' ) ) { ?><input type="hidden" name="uselang" value="<?php $this->text( 'uselang' ); ?>" /><?php } ?>
</form>
</div>
<div id="signupend"><?php $this->msgWiki( 'signupend' ); ?></div>
<?php

	}
}
