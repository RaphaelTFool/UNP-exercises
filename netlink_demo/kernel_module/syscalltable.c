#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

struct idt_descriptor
{
	unsigned short off_low;
	unsigned short sel;
	unsigned char none, flags;
	unsigned short off_high;
};

void *get_system_call(void)
{
	unsigned char idtr[6];
	unsigned long base;
	struct idt_descriptor desc;

	asm ("sidt %0" : "=m" (idtr));
	base = *((unsigned long *)&idtr[2]);
	memcpy(&desc, (void *)(base + (0x80 * 8)), sizeof(desc));

	return ((void *)((desc.off_high << 16) + desc.off_low));
}

void *get_sys_call_table(void)
{
	void *system_call = get_system_call();
	unsigned char *p;
	unsigned long s_c_t;
	int count = 0;

	p = (unsigned char *)system_call;
	while (!((*p == 0xff) && (*(p + 1) == 0x14) && (*(p + 2) == 0x85)))
	{
		p++;
		if (count++ > 500)
		{
			count = -1;
			break;
		}
	}

	if (count != -1)
	{
		p += 3;
		s_c_t = *((unsigned long *)p);
	}
	else
		s_c_t = 0;

	return ((void *) s_c_t);
}

unsigned int clear_and_return_cr0(void)
{
	unsigned int cr0 = 0;
	unsigned int ret;

	asm volatile ("movl %%cr0, %%eax" : "=a"(cr0));
	ret = cr0;
	cr0 &= 0xfffeffff;
	asm volatile ("movl %%eax, %%cr0" : : "a"(cr0));

	return ret;
}

void setback_cr0(unsigned int val)
{
	asm volatile ("movl %%eax, %%cr0" : : "a"(val));
}
