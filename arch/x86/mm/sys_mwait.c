#include <asm/io.h>
#include <asm/mwait.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/sched.h>

asmlinkage long sys_mwait(void __user *address, unsigned long idle_value,
			  unsigned long flags1, unsigned long flags2,
			  unsigned long flags3)
{
	u64 value;

	if (unlikely(!access_ok(VERIFY_READ, address, sizeof(value))))
		return -EFAULT;
	for (;;) {
		if (unlikely(__copy_from_user(&value, address, sizeof(value))))
			return -EFAULT;
		if (value != idle_value)
			break;
		cond_resched();
		if (unlikely(signal_pending(current)))
			return -ERESTARTSYS;
		__monitor((void *) address, 0, 0);
		if (unlikely(__copy_from_user(&value, address, sizeof(value))))
			return -EFAULT;
		if (value != idle_value)
			break;
		__mwait(0, 0);
	}
	return 0;
}
