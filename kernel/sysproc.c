#include "types.h"
#include "defs.h"
#include "proc.h"
#include "x86.h"


int
sys_fork(void)
{
	return fork();
}

int
sys_exit(void)
{
	exit();
	return 0;  // not reached
}

int
sys_wait(void)
{
	return wait();
}

int
sys_kill(void)
{
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

int
sys_getpid(void)
{
	return myproc()->pid;
}

int
sys_sbrk(void)
{
	int addr;
	int n;

	if(argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if(growproc(n) < 0)
		return -1;
	return addr;
}

int
sys_sleep(void)
{
	int n;
	uint ticks0;

	if(argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while(ticks - ticks0 < n){
		if(myproc()->killed){
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

// 0x3D4 je CRT_INDEX
// 0x3D5 je CRT_DATA
int
sys_get_cursor_pos(void)
{
	int pos;
	outb(0x3D4, 14);
	pos = inb(0x3D5) << 8;
	outb(0x3D4, 15);
	pos |= inb(0x3D5);
	return pos;
}

static void
sys_set_cursor_pos(int pos)
{
	if(pos < 0 || pos >=25*80) return;
	outb(0X3D4, 14);
	outb(0X3D5, pos >> 8);
	outb(0X3D4, 15);
	outb(0X3D5, pos);
}

int sys_get_cursor(void){
	return sys_get_cursor_pos();
}

int sys_set_cursor(void){
	int pos;
	if(argint(0, &pos) < 0) return -1;
	sys_set_cursor_pos(pos);
	return 0;
}
