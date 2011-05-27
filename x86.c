#include "x86.h"

void excep();
void irq();
void schedule();
void kb();

struct gdt_entry gdt[3];
struct gdt_ptr gp;
struct idt_entry idt[256];
struct idt_ptr idtp;

//------------------------------------------------------------------------
// memset
//------------------------------------------------------------------------
void * memset(void *dest, int c, int count)
{
    unsigned char* p=dest;
    while(count--)
        *p++ = (unsigned char)c;
    return dest;
}

//------------------------------------------------------------------------
// IO 
//------------------------------------------------------------------------
void outportb (unsigned short _port, unsigned char _data)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

//------------------------------------------------------------------------
// IO 
//------------------------------------------------------------------------
unsigned char inportb (unsigned short _port)
{
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

//------------------------------------------------------------------------
// Setup a descriptor in the Global Descriptor Table 
//------------------------------------------------------------------------
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran)
{
    /* Setup the descriptor base address */
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* Setup the descriptor limits */
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    /* Finally, set up the granularity and access flags */
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

//------------------------------------------------------------------------
// set up GDT
//------------------------------------------------------------------------
void gdt_install()
{
    /* Setup the GDT pointer and limit */
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (int)&gdt;

    /* Our NULL descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* The second entry is our Code Segment. The base address
    *  is 0, the limit is 4GBytes, it uses 4KByte granularity,
    *  uses 32-bit opcodes, and is a Code Segment descriptor.
    *  Please check the table above in the tutorial in order
    *  to see exactly what each value means */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* The third entry is our Data Segment. It's EXACTLY the
    *  same as our code segment, but the descriptor type in
    *  this entry's access byte says it's a Data Segment */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* Flush out the old GDT and install the new changes! */
    //gdt_flush();
    asm("lgdt gp");        // Load the GDT with our 'gp' which is a special pointer
    asm("mov $0x10, %ax"); // 0x10 is the offset in the GDT to our data segment
    asm("mov %ax, %ds");
    asm("mov %ax, %es");
    asm("mov %ax, %fs");
    asm("mov %ax, %gs");
    asm("mov %ax, %ss");

}

//------------------------------------------------------------------------
// register an ISR
//------------------------------------------------------------------------
void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags)
{
    /* The interrupt routine's base address */
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;

    /* The segment or 'selector' that this IDT entry will use
    *  is set here, along with any access flags */
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

//------------------------------------------------------------------------
// remap IRQs that conflict with exceptions
//------------------------------------------------------------------------
void irq_remap(void)
{
    outportb(0x20, 0x11);
    outportb(0xA0, 0x11);
    outportb(0x21, 0x20);
    outportb(0xA1, 0x28);
    outportb(0x21, 0x04);
    outportb(0xA1, 0x02);
    outportb(0x21, 0x01);
    outportb(0xA1, 0x01);
    outportb(0x21, 0x0);
    outportb(0xA1, 0x0);
}

//------------------------------------------------------------------------
// register IRQs
//------------------------------------------------------------------------
void irq_install()
{
    irq_remap();

    idt_set_gate(32, (unsigned)schedule, 0x08, 0x8E);
    idt_set_gate(33, (unsigned)kb, 0x08, 0x8E);

    int i;
    for(i = 34; i < 48; i++)
        idt_set_gate(i, (unsigned)irq, 0x08, 0x8E);
}


//------------------------------------------------------------------------
// Installs the IDT 
//------------------------------------------------------------------------
void idt_install()
{
    /* Sets the special IDT pointer up, just like in 'gdt.c' */
    idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
    idtp.base = (int)&idt;

    /* Clear out the entire IDT, initializing it to zeros */
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    /* Add any new ISRs to the IDT here using idt_set_gate */
    int i;
    for(i = 0; i < 32; i++)
        idt_set_gate(i, (unsigned)excep, 0x08, 0x8E);

    idt_set_gate(48, (unsigned)schedule, 0x08, 0x8E);


    /* Points the processor's internal register to the new IDT */
    asm("lidt idtp");
}

//------------------------------------------------------------------------
// exception ISR 
//------------------------------------------------------------------------
asm("excep:");
asm("pushal");
asm("call handle_exception");
asm("popal");
asm("iret");

//------------------------------------------------------------------------
// irq ISR 
//------------------------------------------------------------------------
asm("irq:");
asm("pushal");
asm("call handle_irq");
asm("popal");
asm("iret");

//------------------------------------------------------------------------
// keyboard ISR 
//------------------------------------------------------------------------
asm("kb:");
asm("pushal");
asm("call keyboard_handler");
asm("popal");
asm("iret");

//------------------------------------------------------------------------
// schedule ISR 
//------------------------------------------------------------------------
asm("schedule:");
asm("push %eax");
asm("mov running_proc, %eax");
asm("push %ebp");
asm("push %ebx");
asm("push %ecx");
asm("push %esi");
asm("push %edi");
asm("mov %esp, (%eax)");
asm("call scheduler");
asm("mov running_proc, %eax");
asm("mov (%eax), %esp");
asm("pop %edi");
asm("pop %esi");
asm("pop %ecx");
asm("pop %ebx");
asm("pop %ebp");
asm("pop %eax");
asm("iret");

