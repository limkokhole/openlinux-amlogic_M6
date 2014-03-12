#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/sched_clock.h>
#include <plat/regops.h>
#include <mach/map.h>
#include <mach/hardware.h>
#include <mach/reg_addr.h>




/***********************************************************************
 * System timer
 **********************************************************************/
#define TIMER_E_INPUT_BIT         8
#define TIMER_D_INPUT_BIT         6
#define TIMER_C_INPUT_BIT         4
#define TIMER_B_INPUT_BIT         2
#define TIMER_A_INPUT_BIT         0
#define TIMER_E_INPUT_MASK       (7UL << TIMER_E_INPUT_BIT)
#define TIMER_D_INPUT_MASK       (3UL << TIMER_D_INPUT_BIT)
#define TIMER_C_INPUT_MASK       (3UL << TIMER_C_INPUT_BIT)
#define TIMER_B_INPUT_MASK       (3UL << TIMER_B_INPUT_BIT)
#define TIMER_A_INPUT_MASK       (3UL << TIMER_A_INPUT_BIT)
#define TIMER_UNIT_1us            0
#define TIMER_UNIT_10us           1
#define TIMER_UNIT_100us          2
#define TIMER_UNIT_1ms            3
#define TIMERE_UNIT_SYS           0
#define TIMERE_UNIT_1us           1
#define TIMERE_UNIT_10us          2
#define TIMERE_UNIT_100us         3
#define TIMERE_UNIT_1ms           4
#define TIMER_A_ENABLE_BIT        16
#define TIMER_E_ENABLE_BIT        20
#define TIMER_A_PERIODIC_BIT      12
/********** Clock Source Device, Timer-E *********/

static cycle_t cycle_read_timerE(struct clocksource *cs)
{
    return (cycles_t) aml_read_reg32(P_ISA_TIMERE);
}

static struct clocksource clocksource_timer_e = {
    .name   = "Timer-E",
    .rating = 300,
    .read   = cycle_read_timerE,
    .mask   = CLOCKSOURCE_MASK(32),
    .flags  = CLOCK_SOURCE_IS_CONTINUOUS,
};

static struct clocksource clocksource_timer_f = {
    .name   = "Timer-F",
    .rating = 300,
    .read   = cycle_read_timerE,
    .mask   = CLOCKSOURCE_MASK(32),
    .flags  = CLOCK_SOURCE_IS_CONTINUOUS,
};

#if CONFIG_HAVE_SCHED_CLOCK

static DEFINE_CLOCK_DATA(cd);
/*
 * sched_clock()
 */
unsigned long long notrace sched_clock(void)
{
    struct clocksource *cs = &clocksource_timer_e;
    u32 cyc = cycle_read_timerE(NULL);
    return cyc_to_fixed_sched_clock(&cd, cyc, (u32)~0, cs->mult, cs->shift);
}

static void notrace meson6_update_sched_clock(void)
{
    u32 cyc = cycle_read_timerE(NULL);
    update_sched_clock(&cd, cyc, (u32)~0);
}

static void __init meson_clocksource_init(void)
{
    aml_clr_reg32_mask(P_ISA_TIMER_MUX, TIMER_E_INPUT_MASK);
    aml_set_reg32_mask(P_ISA_TIMER_MUX, TIMERE_UNIT_1us << TIMER_E_INPUT_BIT);
///    aml_write_reg32(P_ISA_TIMERE, 0);

    /**
     * (counter*mult)>>shift=xxx ns
     */
    /**
     * Constants generated by clocks_calc_mult_shift(m, s, 1MHz, NSEC_PER_SEC, 0).
     * This gives a resolution of about 1us and a wrap period of about 1h11min.
     */
    clocksource_timer_e.shift = 22;
    clocksource_timer_e.mult = 4194304000u;
    clocksource_register(&clocksource_timer_e);

    clocksource_timer_f.shift = 22;
    clocksource_timer_f.mult = 3711016000u;
    clocksource_register(&clocksource_timer_f);

    init_fixed_sched_clock(&cd, meson6_update_sched_clock, 32,
                           USEC_PER_SEC,
                           clocksource_timer_e.mult,
                           clocksource_timer_e.shift);
}
#else
static void __init meson_clocksource_init(void)
 {
     aml_clr_reg32_mask(P_ISA_TIMER_MUX, TIMER_E_INPUT_MASK);
     aml_set_reg32_mask(P_ISA_TIMER_MUX, TIMERE_UNIT_1us << TIMER_E_INPUT_BIT);
 ///    aml_write_reg32(P_ISA_TIMERE, 0);

       /**
        * (counter*mult)>>shift=xxx ns
        */
    clocksource_timer_e.shift = 0;
    clocksource_timer_e.mult = 1000;

     clocksource_register(&clocksource_timer_e);
}

/*
 * sched_clock()
 */
unsigned long long sched_clock(void)
{
    static unsigned long last_timeE=0;
    static cycle_t cyc=0;
    struct clocksource *cs = &clocksource_timer_e;
    unsigned long cur;
    cur=cycle_read_timerE(NULL);
    cyc += cur - last_timeE;
    last_timeE=cur;
    return clocksource_cyc2ns(cyc, cs->mult, cs->shift);

 }

#endif
/********** Clock Event Device, Timer-ABCD *********/
#define MESON_TIMERA 0
#define MESON_TIMERB 1
#define MESON_TIMERC 2
#define MESON_TIMERD 3
struct meson_clock {
	struct clock_event_device   clockevent;
    struct irqaction            irq;
    unsigned id;
    unsigned mux_reg;
    unsigned reg;
};

static struct meson_clock *clockevent_to_clock(struct clock_event_device *evt)
{
	return container_of(evt, struct meson_clock, clockevent);
}
static void meson_clkevt_set_mode(enum clock_event_mode mode,
                                  struct clock_event_device *dev)
{

    struct meson_clock * clk=clockevent_to_clock(dev);
    switch (mode) {
    case CLOCK_EVT_MODE_RESUME:
        printk("Resume timer%c\n",'A'+clk->id);
        aml_set_reg32_mask(clk->mux_reg, 0x1<<(TIMER_A_ENABLE_BIT+clk->id));
        break;

    case CLOCK_EVT_MODE_PERIODIC:
    	aml_set_reg32_mask(clk->mux_reg, 0x1<<(TIMER_A_PERIODIC_BIT+clk->id));
         aml_set_reg32_mask(clk->mux_reg, 0x1<<(TIMER_A_ENABLE_BIT+clk->id));

        break;

    case CLOCK_EVT_MODE_ONESHOT:
        aml_clr_reg32_mask(clk->mux_reg, 0x1<<(TIMER_A_PERIODIC_BIT+clk->id));
        aml_set_reg32_mask(clk->mux_reg, 0x1<<(TIMER_A_ENABLE_BIT+clk->id));
		break;
    case CLOCK_EVT_MODE_SHUTDOWN:
    case CLOCK_EVT_MODE_UNUSED:
        printk("Disable timer%c\n",'A'+clk->id);
        aml_clr_reg32_mask(clk->mux_reg, 0x1<<(TIMER_A_ENABLE_BIT+clk->id));
        break;
    }
}
static int meson_set_next_event(unsigned long evt,
                                struct clock_event_device *dev)
{
    struct meson_clock * clk=clockevent_to_clock(dev);
    /* use a big number to clear previous trigger cleanly */
    aml_set_reg32_mask(clk->reg, evt & 0xffff);

    /* then set next event */
    aml_set_reg32_bits(clk->reg, evt, 0, 16);
    return 0;
}
#if (defined CONFIG_SMP)&& !(defined CONFIG_HAVE_ARM_TWD)
#if (defined CONFIG_MESON_TIMERB ) || (defined CONFIG_MESON_TIMERD )
#error "under smp build, if you deselect HAVE_ARM_TWD , you should not enable TIMERB and TIMERD"
#endif


static void meson_tick_set_mode(enum clock_event_mode mode,
                                  struct clock_event_device *dev);
static int meson_tick_set_next_event(unsigned long evt,
                                struct clock_event_device *dev);
#define meson_tick_rating 450
//{
//    int timer =2*smp_processor_id()+1;
//    meson_clkevt_set_mode(mode,&)
//}
#else
#define meson_tick_set_mode meson_clkevt_set_mode
#define meson_tick_set_next_event meson_set_next_event
#define meson_tick_rating 300
#endif

/* Clock event timer interrupt handler */
static irqreturn_t meson_timer_interrupt(int irq, void *dev_id);
#if 0
static struct clock_event_device clockevent_meson_1mhz = {
    .name           = "TIMER-AC",
    .rating         = 300, /* Reasonably fast and accurate clock event */

    .features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
    .shift          = 20,
    .set_next_event = meson_set_next_event,
    .set_mode       = meson_clkevt_set_mode,
};

static struct irqaction meson_timer_irq = {
    .name           = "Meson Timer Tick",
    .flags          = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
    .handler        = meson_timer_interrupt,
};
#endif
static struct meson_clock meson_timers[]={
    [MESON_TIMERA]={
        .clockevent={
            .name           = "MESON TIMER-A",
            .rating         = 400, /* Reasonably fast and accurate clock event */

            .features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
            .shift          = 20,
            .set_next_event = meson_set_next_event,
            .set_mode       = meson_clkevt_set_mode,
        },
        .irq={
            .name           = "MESON TIMER-A",
            .flags          = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
            .handler        = meson_timer_interrupt,
            .irq            =INT_TIMER_A,
        },
        .id=0,
        .mux_reg=P_ISA_TIMER_MUX,
        .reg=P_ISA_TIMERA
    },
    [MESON_TIMERB]=
    {
        .clockevent={
            .name           = "MESON TIMER-B",
            .rating         = meson_tick_rating, /* Reasonably fast and accurate clock event */

            .features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
            .shift          = 20,
            .set_next_event = meson_tick_set_next_event,
            .set_mode       = meson_tick_set_mode,
        },
        .irq={
            .name           = "MESON TIMER-B",
            .flags          = IRQF_TIMER | IRQF_NOBALANCING,
            .handler        = meson_timer_interrupt,
            .irq            =INT_TIMER_B,
        },
        .id=1,
        .mux_reg=P_ISA_TIMER_MUX,
        .reg=P_ISA_TIMERB
    },
    [MESON_TIMERC]=

    {
        .clockevent={
            .name           = "MESON TIMER-C",
            .rating         = meson_tick_rating, /* Reasonably fast and accurate clock event */

            .features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
            .shift          = 20,
            .set_next_event = meson_set_next_event,
            .set_mode       = meson_clkevt_set_mode,
        },
        .irq={
            .name           = "MESON TIMER-C",
            .flags          = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
            .handler        = meson_timer_interrupt,
            .irq            =INT_TIMER_C,
        },
        .id=2,
        .mux_reg=P_ISA_TIMER_MUX,
        .reg=P_ISA_TIMERC
    },
    [MESON_TIMERD]=
    {
        .clockevent={
            .name           = "MESON TIMER-D",
            .rating         = 400, /* Reasonably fast and accurate clock event */
            .features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
            .shift          = 20,
            .set_next_event = meson_tick_set_next_event,
            .set_mode       = meson_tick_set_mode,
        },
        .irq={
            .name           = "MESON TIMER-D",
            .flags          = IRQF_TIMER | IRQF_NOBALANCING,
            .handler        = meson_timer_interrupt,
            .irq            =INT_TIMER_D,
        },
        .id=3,
        .mux_reg=P_ISA_TIMER_MUX,
        .reg=P_ISA_TIMERD
    }

};
static irqreturn_t meson_timer_interrupt(int irq, void *dev_id)
{
    struct clock_event_device *evt = dev_id;
    if(evt==NULL||evt->event_handler==NULL)
    {
        WARN_ONCE(evt==NULL||evt->event_handler==NULL,"%p %s %p %d",evt,evt?evt->name:NULL,evt?evt->event_handler:NULL,irq);
        return IRQ_HANDLED;
    }
    evt->event_handler(evt);
	return IRQ_HANDLED;

}
static void __cpuinit meson_timer_init_device(struct clock_event_device *evt)
{
    printk("%s %p\n",evt->name,evt);
    evt->mult=div_sc(1000000, NSEC_PER_SEC, 20);
    evt->max_delta_ns =
            clockevent_delta2ns(0xfffe, evt);
    evt->min_delta_ns=clockevent_delta2ns(1, evt);
    evt->cpumask = cpumask_of(smp_processor_id());
}
static void __init meson_timer_setup(struct clock_event_device *evt, unsigned i )
{
    struct meson_clock * clk;
    clk=&meson_timers[i];
    /**
     * Enable Timer and setting the time base;
     */
    aml_set_reg32_mask(clk->mux_reg,
                (1<<(TIMER_A_ENABLE_BIT+clk->id))
                |(1<<(TIMER_A_PERIODIC_BIT+clk->id))
                |(TIMER_UNIT_1us << (TIMER_A_INPUT_BIT+clk->id*2))
            );
    aml_write_reg32(clk->reg, 9999);
    meson_timer_init_device(&clk->clockevent);

    clk->irq.dev_id=&clk->clockevent;
    clockevents_register_device(&(clk->clockevent));

    /* Set up the IRQ handler */
    setup_irq(clk->irq.irq, &clk->irq);
}
static void __init meson_clockevent_init(void)
{

    /***
     * Disable Timer A~D, and clean the time base
     * Now all of the timer is 1us base
     */
    aml_clr_reg32_mask(P_ISA_TIMER_MUX,~(TIMER_E_INPUT_MASK));
#ifdef CONFIG_MESON_TIMERA
    meson_timer_setup(NULL,MESON_TIMERA);
#endif
#ifdef CONFIG_MESON_TIMERB
    meson_timer_setup(NULL,MESON_TIMERB);
#endif
#ifdef CONFIG_MESON_TIMERC
    meson_timer_setup(NULL,MESON_TIMERC);
#endif
#ifdef CONFIG_MESON_TIMERD
    meson_timer_setup(NULL,MESON_TIMERD);
#endif

}
#ifdef CONFIG_SMP
/***********************************************************************
 * ARM TWD System timer
 **********************************************************************/


#include <asm/localtimer.h>
#ifdef CONFIG_HAVE_ARM_TWD
#include <asm/smp_twd.h>
int __cpuinit local_timer_setup(struct clock_event_device *evt)
{

    evt->irq = 29;
    twd_timer_setup(evt);

    return 0;
}
static void meson_twd_private_timer_init(void)
{
    twd_base=(void __iomem *)((IO_PERIPH_BASE+0x600));
}
#else///CONFIG_HAVE_ARM_TWD
#if (defined CONFIG_MESON_TIMERB ) || (defined CONFIG_MESON_TIMERD )
#error "under smp build, if you deselect HAVE_ARM_TWD , you should not enable TIMERB and TIMERD"
#endif

static void meson_tick_set_mode(enum clock_event_mode mode,
                                  struct clock_event_device *dev)
{
    int timer =(2*smp_processor_id())+1;
    meson_clkevt_set_mode(mode,&meson_timers[timer].clockevent);
}
static int meson_tick_set_next_event(unsigned long evt,
                                struct clock_event_device *dev)
{
    int timer =2*smp_processor_id()+1;
     return   meson_set_next_event(evt,&meson_timers[timer].clockevent);
}
int __cpuinit local_timer_setup(struct clock_event_device *evt)
{
    int cpu=smp_processor_id();
    int timer=2*cpu+1;

    struct meson_clock * clk;
    clk=&meson_timers[timer];
        aml_set_reg32_mask(clk->mux_reg,
                    (1<<(TIMER_A_ENABLE_BIT+clk->id))
                    |(1<<(TIMER_A_PERIODIC_BIT+clk->id))
                    |(TIMER_UNIT_1us << (TIMER_A_INPUT_BIT+clk->id*2))
                );
    aml_write_reg32(clk->reg, 9999);
    meson_timer_init_device(&(clk->clockevent));
    *evt=clk->clockevent;
    clk->irq.dev_id=evt;

    clockevents_register_device(evt);

    if(cpu)
    {
        irq_set_affinity(clk->irq.irq, cpumask_of(cpu));
    }
    /* Set up the IRQ handler */
    setup_irq(clk->irq.irq, &clk->irq);

    /**
     * Enable Timer and setting the time base;
     */




    return 0;
}
inline int local_timer_ack(void)
{
    return 1;
}


#endif///CONFIG_HAVE_ARM_TWD
#endif///CONFIG_SMP


/*
 * This sets up the system timers, clock source and clock event.
 */
static void __init meson_timer_init(void)
{
    meson_clocksource_init();
    meson_clockevent_init();
#ifdef CONFIG_SMP
#ifdef CONFIG_HAVE_ARM_TWD
    meson_twd_private_timer_init();
#endif
#endif
}

struct sys_timer meson_sys_timer = {
    .init   = meson_timer_init,
};
