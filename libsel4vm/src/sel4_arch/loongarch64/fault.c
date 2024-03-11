/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <assert.h>
#include <utils/util.h>

#include "fault.h"

// static seL4_Word wzr = 0;
seL4_Word *decode_rt(int reg, seL4_UserContext *c)
{
    switch (reg) {
    case  1:
        return &c->ra;
    case  2:
        return &c->tp;
    case  3:
        return &c->sp;
    case  4:
        return &c->a0;
    case  5:
        return &c->a1;
    case  6:
        return &c->a2;
    case  7:
        return &c->a3;
    case  8:
        return &c->a4;
    case  9:
        return &c->a5;
    case 10:
        return &c->a6;
    case 11:
        return &c->a7;
    case 12:
        return &c->t0;
    case 13:
        return &c->t1;
    case 14:
        return &c->t2;
    case 15:
        return &c->t3;
    case 16:
        return &c->t4;
    case 17:
        return &c->t5;
    case 18:
        return &c->t6;
    case 19:
        return &c->t7;
    case 20:
        return &c->t8;
    case 22:
        return &c->fp;
    case 23:
        return &c->s0;
    case 24:
        return &c->s1;
    case 25:
        return &c->s2;
    case 26:
        return &c->s3;
    case 27:
        return &c->s4;
    case 28:
        return &c->s5;
    case 29:
        return &c->s6;
    case 30:
        return &c->s7;
    case 31:
        return &c->s8;
    default:
        printf("invalid reg %d\n", reg);
        assert(!"Invalid register");
        return NULL;
    }
};

#define PREG(regs, r)    printf(#r ": 0x%lx\n", regs->r)
void print_ctx_regs(seL4_UserContext *regs)
{
    PREG(regs, pc);
    PREG(regs, ra);
    PREG(regs, sp);
    PREG(regs, s0);
    PREG(regs, s1);
    PREG(regs, s2);
    PREG(regs, s3);
    PREG(regs, s4);
    PREG(regs, s5);
    PREG(regs, s6);
    PREG(regs, s7);
    PREG(regs, s8);
    PREG(regs, fp);
    PREG(regs, a0);
    PREG(regs, a1);
    PREG(regs, a2);
    PREG(regs, a3);

    PREG(regs, a4);
    PREG(regs, a5);
    PREG(regs, a6);
    PREG(regs, a7);
    PREG(regs, t0);
    PREG(regs, t1);
    PREG(regs, t2);
    PREG(regs, t3);

    PREG(regs, t4);
    PREG(regs, t5);
    PREG(regs, t6);
    PREG(regs, t7);
    PREG(regs, t8);
    PREG(regs, tp);
}

int decode_vcpu_reg(int rt, fault_t *f)
{
    return seL4_VCPUReg_Num;
}

// void fault_print_data(fault_t *fault)
// {
//     seL4_Word data;
//     data = fault_get_data(fault) & fault_get_data_mask(fault);
//     switch (fault_get_width(fault)) {
//     case WIDTH_WORD:
//         printf("0x%16lx", data);
//         break;
//     case WIDTH_HALFWORD:
//         printf("0x%8lx", data);
//         break;
//     case WIDTH_BYTE:
//         printf("0x%02lx", data);
//         break;
//     default:
//         printf("<Invalid width> 0x%lx", data);
//     }
// }
