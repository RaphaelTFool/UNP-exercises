#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/unistd.h>

void netlink_release(void);
void netlink_init(void);
int audit_open(const char *pathname, int flags, int ret);

void *get_sys_call_table(void);
unsigned int clear_and_return_cr0(void);
void setback_cr0(unsigned int val);

void **sys_call_table;
asmlinkage long (*orig_open)(const char *pathname, int flags, mode_t mode);

asmlinkage long hacked_open(const char *pathname, int flags, mode_t mode)
{
	long ret;

	if (!pathname)
		return -1;
	ret = orig_open(pathname, flags, mode);
	audit_open(pathname, flags, ret);

	return ret;
}

static int __init init(void)
{
	unsigned int orig_cr0 = clear_and_return_cr0();
	sys_call_table = get_sys_call_table();

	printk(KERN_INFO "sys_call_table found at %lx\n", (unsigned long)sys_call_table);
	printk(KERN_INFO "audit_mod init\n");

	orig_open = sys_call_table[__NR_open]; /* backup system open */
	sys_call_table[__NR_open] = hacked_open; /* set new system open */
	setback_cr0(orig_cr0);
	netlink_init();

	return 0;
}

static void __exit exit(void)
{
	unsigned int orig_cr0 = clear_and_return_cr0();
	sys_call_table[__NR_open] = orig_open;
	setback_cr0(orig_cr0);
	netlink_release();
	printk(KERN_INFO "audit_mod exit\n");
}

module_init(init);
module_exit(exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("RAL");
MODULE_DESCRIPTION("system open audit");
MODULE_VERSION("0.0.0");
MODULE_ALIAS("Audit open via netlink");
