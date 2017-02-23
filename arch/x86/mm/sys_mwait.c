#include <asm/io.h>
#include <asm/processor.h>
#include <asm/mwait.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <uapi/asm/sys_mwait.h>

struct mwait_info {
	unsigned int min_size;
	unsigned int max_size;
	unsigned char enum_support;
	unsigned char irq_break;
	unsigned char cstate[8];
};
static struct mwait_info mwinfo __read_mostly;

static int parse_params(const struct mwait_params __user* params,
			unsigned long *weax, unsigned long *wecx,
			unsigned long *mecx, unsigned long *medx,
			unsigned long *idle_value)
{
	struct mwait_params mwp;

	if (unlikely(copy_from_user(&mwp, params, sizeof(mwp))))
		return -EFAULT;
	/* Handle multiple structure versions here, if necessary. */
	if (unlikely(mwp.size != sizeof(mwp)))
		return -EINVAL;
	*wecx = 0;
	if (mwp.irq_break) {
		if (unlikely(!mwinfo.irq_break))
			return -EINVAL;
		*wecx |= 1;
	}
	if (unlikely(mwp.cstate + 1 >= sizeof(mwinfo.cstate)))
		return -EINVAL;
	if (unlikely(mwp.sub_cstate >= mwinfo.cstate[mwp.cstate + 1]))
		return -EINVAL;
	*weax = ((unsigned long) mwp.cstate << 4) | mwp.sub_cstate;
	*mecx = 0;
	*medx = 0;
	*idle_value = mwp.idle_value;
	return 0;
}

asmlinkage long sys_mwait(void __user *address,
			  const struct mwait_params __user* params)
{
	int error;
	unsigned long weax, wecx, mecx, medx, idle_value, value;

	error = parse_params(params, &weax, &wecx, &mecx, &medx, &idle_value);
	if (unlikely(error))
		return error;
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
		__monitor((void *) address, mecx, medx);
		if (unlikely(__copy_from_user(&value, address, sizeof(value))))
			return -EFAULT;
		if (value != idle_value)
			break;
		__mwait(weax, wecx);
	}
	return 0;
}

static int __init mwait_init(void)
{
	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;

	cpuid(5, &eax, &ebx, &ecx, &edx);

	mwinfo.min_size = eax & 0xffff;
	mwinfo.max_size = ebx & 0xffff;
	mwinfo.enum_support = ecx & 1;
	mwinfo.irq_break = (ecx >> 1) & 1;
	mwinfo.cstate[0] = edx & 0x0f;
	mwinfo.cstate[1] = (edx >> 4) & 0x0f;
	mwinfo.cstate[2] = (edx >> 8) & 0x0f;
	mwinfo.cstate[3] = (edx >> 12) & 0x0f;
	mwinfo.cstate[4] = (edx >> 16) & 0x0f;
	
	return 0;
}
arch_initcall(mwait_init);
