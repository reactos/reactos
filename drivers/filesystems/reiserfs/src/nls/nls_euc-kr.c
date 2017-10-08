/*
 * linux/fs/nls_euc-kr.c
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/nls.h>
#include <linux/errno.h>

static struct nls_table *p_nls;

static struct nls_table table = {
	"euc-kr",
	NULL,
	NULL,
	NULL,
	NULL,
	THIS_MODULE,
};

static int __init init_nls_euc_kr(void)
{
	p_nls = load_nls("cp949");

	if (p_nls) {
		table.uni2char = p_nls->uni2char;
		table.char2uni = p_nls->char2uni;
		table.charset2upper = p_nls->charset2upper;
		table.charset2lower = p_nls->charset2lower;
		return register_nls(&table);
	}

	return -EINVAL;
}

static void __exit exit_nls_euc_kr(void)
{
	unregister_nls(&table);
	unload_nls(p_nls);
}

module_init(init_nls_euc_kr)
module_exit(exit_nls_euc_kr)

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 *
---------------------------------------------------------------------------
 * Local variables:
 * c-indent-level: 8
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -8
 * c-argdecl-indent: 8
 * c-label-offset: -8
 * c-continued-statement-offset: 8
 * c-continued-brace-offset: 0
 * End:
 */
