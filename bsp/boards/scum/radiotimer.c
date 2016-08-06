/**
\brief SCuM-specific definition of the "radiotimer" bsp module.

\author Tengfei Chang <tengfei.chang@inria.fr>, Auguest 2016.
*/

#include "memory_map.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "radiotimer.h"
#include "board.h"

//=========================== variables =======================================

typedef struct {
    radiotimer_compare_cbt    overflow_cb;
    radiotimer_compare_cbt    compare_cb;
    PORT_RADIOTIMER_WIDTH     interrupt_flag;
} radiotimer_vars_t;

radiotimer_vars_t radiotimer_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

//===== admin

void radiotimer_init() {
    // clear local variables
    memset(&radiotimer_vars,0,sizeof(radiotimer_vars_t));
}

void radiotimer_setOverflowCb(radiotimer_compare_cbt cb) {
    radiotimer_vars.overflow_cb    = cb;
}

void radiotimer_setCompareCb(radiotimer_compare_cbt cb) {
    radiotimer_vars.compare_cb     = cb;
}

void radiotimer_setStartFrameCb(radiotimer_capture_cbt cb) {
    while(1);
}

void radiotimer_setEndFrameCb(radiotimer_capture_cbt cb) {
    while(1);
}

void radiotimer_start(PORT_RADIOTIMER_WIDTH period) {
    // set period of radiotimer
    RFTIMER_REG__MAX_COUNT          = period;
    RFTIMER_REG__COUNTER            = 1;
    // enable timer and interrupt
    RFTIMER_REG__CONTROL            = 0x07;
    
    // set compare timer counter 0 to perform an overflow interrupt
    RFTIMER_REG__COMPARE0           = 0;
    // enable compare0 module and interrup
    RFTIMER_REG__COMPARE0_CONTROL   = 0x03;
}

//===== direct access

PORT_RADIOTIMER_WIDTH radiotimer_getValue() {
    return RFTIMER_REG__COUNTER;
}

void radiotimer_setPeriod(PORT_RADIOTIMER_WIDTH period) {
    RFTIMER_REG__MAX_COUNT           = period;
}

PORT_RADIOTIMER_WIDTH radiotimer_getPeriod() {
    return RFTIMER_REG__COUNTER;
}

//===== compare

void radiotimer_schedule(PORT_RADIOTIMER_WIDTH offset) {
    // offset when to fire
    RFTIMER_REG__COMPARE1            = offset;
   
    // enable compare interrupt (this also cancels any pending interrupts)
    RFTIMER_REG__COMPARE1_CONTROL    = 0x03;
}

void radiotimer_cancel() {
    // disable compare interrupt
    RFTIMER_REG__COMPARE1_CONTROL = 0x00;
}

//===== capture

PORT_RADIOTIMER_WIDTH radiotimer_getCapturedTime() {
    return RFTIMER_REG__COUNTER;
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================

kick_scheduler_t radiotimer_isr() {
    unsigned int interrupt_flag = RFTIMER_REG__INT;
    switch (interrupt_flag & 0xffff) {
        case 0x0002:
            if (radiotimer_vars.compare_cb!=NULL) {
                // call the callback
                radiotimer_vars.compare_cb();
                RFTIMER_REG__INT_CLEAR = interrupt_flag;
                // kick the OS
                return KICK_SCHEDULER;
            }
            break;
        case 0x0001: // timer overflows
            if (radiotimer_vars.overflow_cb!=NULL) {
                // call the callback
                radiotimer_vars.overflow_cb();
                RFTIMER_REG__INT_CLEAR = interrupt_flag;
                // kick the OS
                return KICK_SCHEDULER;
            }
            break;
        default:
            while(1);   // this should not happen
    }
    return DO_NOT_KICK_SCHEDULER;
}