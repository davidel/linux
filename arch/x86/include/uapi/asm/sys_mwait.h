#ifndef _UAPI_ASM_X86_SYS_MWAIT_H
#define _UAPI_ASM_X86_SYS_MWAIT_H

struct mwait_params {
	unsigned char size;
	unsigned char irq_break;
	unsigned char cstate;
	unsigned char sub_cstate;
	unsigned long idle_value;
} __attribute__((packed));

#endif /* _UAPI_ASM_X86_SYS_MWAIT_H */
