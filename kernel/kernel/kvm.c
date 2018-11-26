#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];
TSS tss;

#define SECTSIZE 512

void waitDisk(void) {
	while((inByte(0x1F7) & 0xC0) != 0x40); 
}

void readSect(void *dst, int offset) {
	int i;
	waitDisk();
	
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i ++) {
		((int *)dst)[i] = inLong(0x1F0);
	}
}

void initSeg() {
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_USER);
	gdt[SEG_UDATA] = SEG(STA_W,         0,       0xffffffff, DPL_USER);
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	setGdt(gdt, sizeof(gdt));

	/*
	 * 初始化TSS
	 */
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/*设置正确的段寄存器*/
	tss.ss0=KSEL(SEG_KDATA);
	tss.esp0=0x200000;
	asm volatile(	"movw %%ax,%%es\n\t"
			"movw %%ax,%%ds\n\t"
			"movw %%ax,%%fs\n\t"
			"movw %%ax,%%ss    "
			:
			 : "a" (KSEL(SEG_KDATA)));
	lLdt(0);
	
}

void enterUserSpace(uint32_t entry) {
	/*
	 * Before enter user space 
	 * you should set the right segment registers here
	 * and use 'iret' to jump to ring3
	 */
        asm volatile("pushl %0":: "r"(USEL(SEG_UDATA)));	// %ss
	asm volatile("pushl %0":: "r"(128 << 20));		// %esp 128MB
	asm volatile("pushfl");					// %eflags
	asm volatile("pushl %0":: "r"(USEL(SEG_UCODE)));	// %cs
	asm volatile("pushl %0":: "r"(entry));			// %eip
	asm volatile("iret");
}

void loadUMain(void) {
        struct ELFHeader *elf;
	struct ProgramHeader *ph;

	unsigned char *buf = (unsigned char *)0x5000000;
	for (int i = 0; i < 100; i ++) {
		readSect((void*)(buf + 512 * i), i + 201);
	}

	elf = (struct ELFHeader *)buf;

	/* Load each program segment */
	ph = (struct ProgramHeader *)(buf + elf->phoff);
	int i;
	for(i = 0; i < elf->phnum; ++i) {

		if (ph->type == 1) {
		/* from the ELF file
		to the memory region [VirtAddr, VirtAddr + FileSiz)*/
			unsigned int p = ph->vaddr, q = ph->off;
			while (p < ph->vaddr + ph->filesz) {
				*(unsigned char*)p = *(unsigned char*)(buf + q);
				q++;
				p++;
			}

			/* zero the memory region [VirtAddr + FileSiz, VirtAddr + MemSiz) */
			while (p < ph->vaddr + ph->memsz) {
				*(unsigned char*)p = 0;
				q++;
				p++;
			}
		}

		ph++;
	}

	enterUserSpace(elf->entry);

}

