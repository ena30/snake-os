/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/star_timer.h>

#define TIMER_COUNTER_VAL	10
#define TIMER_AUTO_RELOAD_VAL	TIMER_COUNTER_VAL

int timer_load_val = TIMER_AUTO_RELOAD_VAL;

DECLARE_GLOBAL_DATA_PTR;

/* macro to read the 16 bit timer */
static inline unsigned long READ_TIMER(void)
{
	return TIMER1_COUNTER_REG;
}

int timer_init(void)
{
	u32 timer_control;
	u32 timer_interrupt_mask;

	HAL_TIMER_DISABLE_TIMER1();
	HAL_TIMER_DISABLE_TIMER2();

	TIMER1_COUNTER_REG = TIMER_COUNTER_VAL;
	TIMER1_AUTO_RELOAD_VALUE_REG = TIMER_AUTO_RELOAD_VAL;
	TIMER1_MATCH_VALUE1_REG = 0;
	TIMER1_MATCH_VALUE2_REG = 0;

	/* mask all the interrupt */
	TIMER1_TIMER2_INTERRUPT_MASK_REG = 0x3f;

	timer_control = TIMER1_TIMER2_CONTROL_REG;

	/* timer1 down counter */
	timer_control |= (1 << TIMER1_UP_DOWN_COUNT_BIT_INDEX);

	/* timer1 disable overflow interrupt */
	timer_control &= ~(1 << TIMER1_OVERFLOW_INTERRUPT_BIT_INDEX);

	/* timer1 seleck 1KHz Clock */
	timer_control |= (1 << TIMER1_CLOCK_SOURCE_BIT_INDEX);

	/* timer1 enable */
	timer_control |= (1 << TIMER1_ENABLE_BIT_INDEX);

	TIMER1_TIMER2_CONTROL_REG = timer_control;

	gd->arch.lastinc = timer_load_val;
	gd->arch.tbl = 0;


	return (0);
}

int arch_interrupt_init() {
}

/*
 * timer without interrupts
 */

void reset_timer(void)
{
	reset_timer_masked();
}

unsigned long get_timer(unsigned long base)
{
	return get_timer_masked() - base;
}


void set_timer(unsigned long t)
{
	gd->arch.tbl = t;
}

void __udelay(unsigned long usec)
{
	unsigned long tmo;
	unsigned long start = get_timer(0);

	tmo = usec / 1000;
	tmo *= (timer_load_val * 100);
	tmo /= 1000;

	while ((unsigned long)(get_timer_masked() - start) < tmo)
		; /* wait */
}

void reset_timer_masked(void)
{
	/* reset time */
	gd->arch.lastinc = READ_TIMER();
	gd->arch.tbl = 0;
}

unsigned long get_timer_masked(void)
{
	unsigned long now = READ_TIMER();

	if (gd->arch.lastinc >= now) {
		/* normal mode */
		gd->arch.tbl += gd->arch.lastinc - now;
	} else {
		/* we have an overflow ... */
		gd->arch.tbl += gd->arch.lastinc + timer_load_val - now;
	}
	gd->arch.lastinc = now;

	return gd->arch.tbl;
}

void udelay_masked(unsigned long usec)
{
	unsigned long tmo;
	unsigned long endtime;
	signed long diff;

	if (usec >= 1000) {
		tmo = usec / 1000;
		tmo *= (timer_load_val * 100);
		tmo /= 1000;
	} else {
		tmo = usec * (timer_load_val * 100);
		tmo /= (1000*1000);
	}

	endtime = get_timer_masked() + tmo;

	do {
		unsigned long now = get_timer_masked();
		diff = endtime - now;
	} while (diff >= 0);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
unsigned long get_tbclk(void)
{
	unsigned long tbclk;

	tbclk = CONFIG_SYS_HZ;

	return tbclk;
}

/*
 * reset the cpu
 */
void reset_cpu(ulong ignored)
{
	__asm__ __volatile__(
		"mov	ip, #0						\n"
		"mcr	p15, 0, ip, c7, c7, 0	@ invalidate cache	\n"
		"mcr	p15, 0, ip, c8, c7, 0	@ flush TLB (v4)	\n"
		"mrc	p15, 0, ip, c1, c0, 0	@ get ctrl register	\n"
		"bic	ip, ip, #0x000f		@ ............wcam	\n"
		"bic	ip, ip, #0x2100		@ ..v....s........	\n"
		"mcr	p15, 0, ip, c1, c0, 0	@ ctrl register		\n"
		"ldr	r0, =0x77000008					\n"
		"mov	r1, #0x1					\n"
		"str	r1, [r0]					\n"
		"mov	r1, #0x0					\n"
		"str	r1, [r0]					\n"
	);

	while (1) ;
}

void do_irq (struct pt_regs *pt_regs)
{
	str8100_cpu_clock_scale_end();
}
