<?php
	function check_lang($lang)
	{
		$lang = substr($lang, 0, 2);
		
		switch($lang)
		{
			case 'ar':
			case 'bg':
			case 'ca':
			case 'cz':
			case 'da':
			case 'de':
			case 'el':
			case 'en':
			case 'es':
			case 'fr':
			case 'he':
			case 'hu':
			case 'id':
			case 'it':
			case 'ja':
			case 'ko':
			case 'lt':
			case 'nl':
			case 'no':
			case 'pl':
			case 'pt':
			case 'ru':
			case 'sv':
			case 'uk':
			case 'zh':
			case 'ro':
			case 'tw':
			case 'sk':
			case 'vi':
				return $lang;
		}
		
		if($lang == "*")
			return "en";
		
		return "";
	}
?>
