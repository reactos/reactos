	function pagerefresh() {
		exitmsg = '';
		window.location.href = '<?php echo $roscms_intern_page_link."data&branch=".urldecode(@$_GET['branch']); ?>';
	}

	function roscms_mainmenu(temp_page) {
		exitmsg = '';
		
		temp_page2 = '';
		
		switch (temp_page) {
			case 'welcome':
				temp_page2 = 'data&branch=welcome';
				break;
			case 'website':
				temp_page2 = 'data&branch=website';
				break;
			case 'rost':
				temp_page2 = 'data&branch=reactos';
				break;
			case 'user':
				temp_page2 = 'data&branch=user';
				break;
			case 'maintain':
				temp_page2 = 'data&branch=maintain';
				break;
			case 'stats':
				temp_page2 = 'data&branch=stats';
				break;
		}
		
		window.location.href = '<?php echo $roscms_intern_page_link; ?>'+temp_page2;
	}