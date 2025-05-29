#include <linux/export.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/version.h> /* LINUX_VERSION_CODE, KERNEL_VERSION macros */
#include <linux/workqueue.h>
#include <linux/init.h>

#include "allowlist.h"
#include "core_hook.h"
#include "klog.h" // IWYU pragma: keep
#include "ksu.h"
#include "throne_tracker.h"

#ifdef CONFIG_KSU_SUSFS
#include <linux/susfs.h>
#endif

unsigned int enable_ksu = 1;

static int __init read_kernelsu_state(char *s)
{
	if (s)
		enable_ksu = simple_strtoul(s, NULL, 0);

	return 1;
}
__setup("ksu.enabled=", read_kernelsu_state);

unsigned int get_ksu_state(void)
{
	return enable_ksu;
}

static struct workqueue_struct *ksu_workqueue;

bool ksu_queue_work(struct work_struct *work)
{
	return queue_work(ksu_workqueue, work);
}

#ifdef KSU_USE_STRUCT_FILENAME
extern int ksu_handle_execveat_sucompat(int *fd, struct filename **filename_ptr,
					void *argv, void *envp, int *flags);

extern int ksu_handle_execveat_ksud(int *fd, struct filename **filename_ptr,
				    void *argv, void *envp, int *flags);

int ksu_handle_execveat(int *fd, struct filename **filename_ptr, void *argv,
			void *envp, int *flags)
{
	ksu_handle_execveat_ksud(fd, filename_ptr, argv, envp, flags);
	return ksu_handle_execveat_sucompat(fd, filename_ptr, argv, envp,
					    flags);
}
#endif // KSU_USE_STRUCT_FILENAME

int __init ksu_kernelsu_init(void)
{
	if (enable_ksu < 1) {
		pr_info_once(" is disabled");
		return 0;
	}
#ifdef CONFIG_KSU_DEBUG
	pr_alert("*************************************************************");
	pr_alert("**     NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE    **");
	pr_alert("**                                                         **");
	pr_alert("**         You are running KernelSU in DEBUG mode          **");
	pr_alert("**                                                         **");
	pr_alert("**     NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE    **");
	pr_alert("*************************************************************");
#endif

#ifdef CONFIG_KSU_SUSFS
	susfs_init();
#endif

	ksu_core_init();

	ksu_workqueue = alloc_ordered_workqueue("kernelsu_work_queue", 0);

	ksu_allowlist_init();

	ksu_throne_tracker_init();

	return 0;
}

void ksu_kernelsu_exit(void)
{
	if (enable_ksu < 1)
		return;
	
	ksu_allowlist_exit();

	ksu_throne_tracker_exit();

	destroy_workqueue(ksu_workqueue);

}

module_init(ksu_kernelsu_init);
module_exit(ksu_kernelsu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("weishu");
MODULE_DESCRIPTION("Android KernelSU");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
