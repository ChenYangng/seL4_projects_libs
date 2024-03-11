/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4vm/guest_vm.h>
#include <sel4vm/guest_ram.h>
#include <sel4vm/guest_vm_util.h>

#include "fault.h"
#include "arch_fault.h"
#include "sel4/simple_types.h"
#include "sel4utils/arch/util.h"

#include <sel4/sel4.h>
#include <vka/capops.h>

#include <utils/ansi.h>
#include <stdlib.h>
#include <sel4/sel4_arch/constants.h>

// #define DEBUG_FAULTS
// #define DEBUG_ERRATA

#ifdef DEBUG_FAULTS
#define DFAULT(...) printf(__VA_ARGS__)
#else
#define DFAULT(...) do{}while(0)
#endif

#ifdef DEBUG_ERRATA
#define DERRATA(...) printf(__VA_ARGS__)
#else
#define DERRATA(...) do{}while(0)
#endif

#define CONTENT_REGS               BIT(0)
#define CONTENT_DATA               BIT(1)
#define CONTENT_INST               BIT(2)
#define CONTENT_WIDTH              BIT(3)
#define CONTENT_PMODE              BIT(4)

/*************************
 *** Primary functions ***
 *************************/
// static int maybe_fetch_fault_instruction(fault_t *f)
// {
//     if ((f->content & CONTENT_INST) == 0) {
//         seL4_Word inst = 0;
//         /* Fetch the instruction */
//         if (vm_ram_touch(f->vcpu->vm, f->ip, 4, vm_guest_ram_read_callback, &inst)) {
//             return -1;
//         }
//         /* Fixup the instruction */
//         if (fault_is_thumb(f)) {
//             if (thumb_is_32bit_instruction(inst)) {
//                 f->fsr |= HSR_INST32;
//             }
//             if (HSR_IS_INST32(f->fsr)) {
//                 /* Swap half words for a 32 bit instruction */
//                 inst = ((inst & 0xffff)) << 16 | ((inst >> 16) & 0xffff);
//             } else {
//                 /* Mask instruction for 16 bit instruction */
//                 inst &= 0xffff;
//             }
//         } else {
//             /* All ARM instructions are 32 bit so force the HSR flag to be set */
//             f->fsr |= HSR_INST32;
//         }
//         f->instruction = inst;
//         f->content |= CONTENT_INST;
//     }
//     return 0;
// }
//
// static int errata766422_get_rt(fault_t *f, seL4_Word hsr)
// {
//     seL4_Word inst;
//     int err;
//     err = maybe_fetch_fault_instruction(f);
//     inst = f->instruction;
//     if (err) {
//         return err;
//     }
//     if (HSR_IS_INST32(hsr)) {
//         DERRATA("Errata766422 @ 0x%08x (0x%08x)\n", fault_get_ctx(f)->pc, inst);
//         if ((inst & 0xff700000) == 0xf8400000) {
//             return (inst >> 12) & 0xf;
//         } else if ((inst & 0xfff00000) == 0xf8800000) {
//             return (inst >> 12) & 0xf;
//         } else if ((inst & 0xfff00000) == 0xf0000000) {
//             return (inst >> 12) & 0xf;
//         } else if ((inst & 0x0e500000) == 0x06400000) {
//             return (inst >> 12) & 0xf;
//         } else if ((inst & 0xfff00000) == 0xf8000000) {
//             return (inst >> 12) & 0xf;
//         } else {
//             printf("Unable to decode inst %08lx\n", (long) inst);
//             return -1;
//         }
//     } else {
//         DERRATA("Errata766422 @ 0x%08lx (0x%04lx)\n", (long) fault_get_ctx(f)->pc, (long) inst);
//         /* 16 bit insts */
//         if ((inst & 0xf800) == 0x6000) {
//             return (inst >> 0) & 0x7;
//         } else if ((inst & 0xf800) == 0x9000) {
//             return (inst >> 8) & 0x7;
//         } else if ((inst & 0xf800) == 0x5000) {
//             return (inst >> 0) & 0x7;
//         } else if ((inst & 0xfe00) == 0x5400) {
//             return (inst >> 0) & 0x7;
//         } else if ((inst & 0xf800) == 0x7000) {
//             return (inst >> 0) & 0x7;
//         } else if ((inst & 0xf800) == 0x8000) {
//             return (inst >> 0) & 0x7;
//         } else {
//             printf("Unable to decode inst 0x%04lx\n", (long) inst);
//             return -1;
//         }
//     }
// }
//
// static int decode_instruction(fault_t *f)
// {
//     seL4_Word inst;
//     maybe_fetch_fault_instruction(f);
//     inst = f->instruction;
//     /* Single stage by default */
//     f->stage = 1;
//     f->content |= CONTENT_STAGE;
//     /* Decode */
//     if (fault_is_thumb(f)) {
//         if (thumb_is_32bit_instruction(inst)) {
//             f->fsr |= BIT(25); /* 32 bit instruction */
//             /* 32 BIT THUMB insts */
//             if ((inst & 0xff700000) == 0xf8400000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 12) & 0xf;
//             } else if ((inst & 0xfff00000) == 0xf8800000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 12) & 0xf;
//             } else if ((inst & 0xfff00000) == 0xf0000000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 12) & 0xf;
//             } else if ((inst & 0x0e500000) == 0x06400000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 12) & 0xf;
//             } else if ((inst & 0xfff00000) == 0xf8000000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 12) & 0xf;
//             } else if ((inst & 0xfe400000) == 0xe8400000) { /* LDRD/STRD */
//                 int rt;
//                 if ((f->content & CONTENT_STAGE) == 0) {
//                     f->stage = 2;
//                     f->width = WIDTH_DOUBLEWORD;
//                     f->content |= CONTENT_WIDTH | CONTENT_STAGE;
//                 }
//                 f->addr = f->base_addr + ((2 - f->stage) * sizeof(seL4_Word));
//                 rt = ((inst >> 12) & 0xf) + (2 - f->stage);
//                 return rt;
//             } else {
//                 printf("Unable to decode THUMB32 inst 0x%08lx\n", (long) inst);
//                 print_fault(f);
//                 return -1;
//             }
//         } else {
//             /* 16 bit THUMB insts */
//             if ((inst & 0xf800) == 0x6000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 0) & 0x7;
//             } else if ((inst & 0xf800) == 0x9000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 8) & 0x7;
//             } else if ((inst & 0xf800) == 0x5000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 0) & 0x7;
//             } else if ((inst & 0xfe00) == 0x5400) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 0) & 0x7;
//             } else if ((inst & 0xf800) == 0x7000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 0) & 0x7;
//             } else if ((inst & 0xf800) == 0x8000) {
//                 print_fault(f);
//                 assert(!"No data width");
//                 return (inst >> 0) & 0x7;
//             } else {
//                 printf("Unable to decode THUMB16 inst 0x%04lx\n", (long) inst);
//                 print_fault(f);
//                 return -1;
//             }
//         }
//     } else {
//         printf("32 bit ARM insts not decoded\n");
//         print_fault(f);
//         return -1;
//     }
// }

static int get_rt(fault_t *f)
{
    int rt;
    rt = f->instruction & 0x1f;
    assert(rt >= 0);
    return rt;
}

fault_t *fault_init(vm_vcpu_t *vcpu)
{
    fault_t *fault;
    seL4_Error err;
    fault = (fault_t *)calloc(1, sizeof(*fault));
    if (fault != NULL) {
        fault->vcpu = vcpu;
        /* Reserve a slot for saving reply caps */
        err = vka_cspace_alloc_path(vcpu->vm->vka, &fault->reply_cap);
        if (err) {
            free(fault);
            fault = NULL;
        }
    }
    return fault;
}

// int new_vcpu_fault(fault_t *fault, uint32_t hsr)
// {
//     int err;
//     assert(fault_handled(fault));
//     fault->type = VCPU;
//     fault->fsr = hsr;
//     fault->instruction = 0;
//     fault->data = 0;
//     fault->width = -1;
//     fault->content = 0;
//     fault->stage = 1;
//     assert(fault->reply_cap.capPtr);
//     err = vka_cnode_saveCaller(&fault->reply_cap);
//     assert(!err);
//
//     return err;
// }

int new_memory_fault(fault_t *fault)
{
    seL4_Word ip, addr, fsr;
    seL4_Word is_prefetch;
    seL4_Word inst;
    int err;
    vm_t *vm;

    vm = fault->vcpu->vm;
    assert(vm);

    // assert(fault_handled(fault));

    /* First store message registers on the stack to free our message regs */
    is_prefetch = seL4_GetMR(seL4_VMFault_PrefetchFault);
    addr = seL4_GetMR(seL4_VMFault_Addr),
    fsr = seL4_GetMR(seL4_VMFault_FSR);
    ip = seL4_GetMR(seL4_VMFault_IP);
    inst = seL4_GetMR(seL4_VMFault_Inst);
    DFAULT("%s: New fault @ 0x%llx from PC 0x%llx\n", vm->vm_name, addr, ip);
    /* Create the fault object */
    fault->type = is_prefetch ? PREFETCH : DATA;
    fault->ip = ip;
    fault->base_addr = fault->addr = addr;
    fault->fsr = fsr;
    fault->instruction = inst;
    fault->data = 0;
    fault->width = -1;
    if (fault_is_data(fault)) {
        if (fault_is_read(fault)) {
            /* No need to load data */
            fault->content = CONTENT_DATA;
        } else {
            fault->content = 0;
        }
    } else {
        /* No need to load width or data */
        fault->content = CONTENT_DATA | CONTENT_WIDTH;
    }
    fault->is_handled = 0;

    /* Gather additional information */
    assert(fault->reply_cap.capPtr);
    err = vka_cnode_saveCaller(&fault->reply_cap);
    assert(!err);

    return err;
}

int abandon_fault(fault_t *fault)
{
    /* Nothing to do here */
    DFAULT("%s: Release fault @ 0x%lx from PC 0x%lx\n",
           fault->vcpu->vm->vm_name, fault->addr, fault->ip);
    return 0;
}


int restart_fault(fault_t *fault)
{
    /* Send the reply */
    seL4_MessageInfo_t reply;
    reply = seL4_MessageInfo_new(0, 0, 0, 0);
    DFAULT("%s: Restart fault @ 0x%lx from PC 0x%lx\n",
           fault->vcpu->vm->vm_name, fault->addr, fault->ip);
    seL4_Send(fault->reply_cap.capPtr, reply);
    /* Clean up */
    return abandon_fault(fault);
}

int ignore_fault(fault_t *fault)
{
    // printf("ignore fault...\n");
    seL4_UserContext *regs;
    int err;

    regs = fault_get_ctx(fault);
    /* Advance the PC */
    regs->pc += 4;
    // printf("new pc: 0x%llx\n", regs->pc);
    /* Write back CPU registers */
    err = seL4_TCB_WriteRegisters(vm_get_vcpu_tcb(fault->vcpu), false, 0,
                                  sizeof(*regs) / sizeof(regs->pc), regs);
    assert(!err);
    if (err) {
        abandon_fault(fault);
        return err;
    }
    /* Reply to thread */
    return restart_fault(fault);
}

int advance_fault(fault_t *fault)
{
    /* If data was to be read, load it into the user context */
    if (fault_is_data(fault) && fault_is_read(fault)) {
        /* Get register opearand */
        int rt  = get_rt(fault);

        /* Decode whether operand is banked */
        int reg = decode_vcpu_reg(rt, fault);
        if (reg == seL4_VCPUReg_Num) {
            /* register is not banked, use seL4_UserContext */
            seL4_Word *reg_ctx = decode_rt(rt, fault_get_ctx(fault));
            *reg_ctx = fault_emulate(fault, *reg_ctx);
        } else {
            /* register is banked, use vcpu invocations */
            // seL4_ARM_VCPU_ReadRegs_t res = seL4_ARM_VCPU_ReadRegs(fault->vcpu->vcpu.cptr, reg);
            // if (res.error) {
            //     ZF_LOGF("Read registers failed");
            //     return -1;
            // }
            // int error = seL4_ARM_VCPU_WriteRegs(fault->vcpu->vcpu.cptr, reg, fault_emulate(fault, res.value));
            // if (error) {
            //     ZF_LOGF("Write registers failed");
            //     return -1;
            // }
        }
    }
    DFAULT("%s: Emulate fault @ 0x%x from PC 0x%x\n",
           fault->vcpu->vm->vm_name, fault->addr, fault->ip);
    /* If this is the final stage of the fault, return to user */
    assert(fault->is_handled == 0);
    fault->is_handled = 1;

    return ignore_fault(fault);
}

seL4_Word fault_emulate(fault_t *f, seL4_Word o)
{
    seL4_Word n, m, s;
    s = (f->addr & 0x3) * 8;
    m = fault_get_data_mask(f);
    n = fault_get_data(f);
    if (fault_is_read(f)) {
        /* Read data must be shifted to lsb */
        return (o & ~(m >> s)) | ((n & m) >> s);
    } else {
        /* Data to write must be shifted left to compensate for alignment */
        return (o & ~m) | ((n << s) & m);
    }
}

void print_fault(fault_t *fault)
{
    printf("--------\n");
    printf(ANSI_COLOR(RED, BOLD));
    printf("Pagefault from [%s]: %s %s "
           "@ PC: 0x"XFMT" IPA: 0x"XFMT", FSR: 0x"XFMT "\n",
           fault->vcpu->vm->vm_name,
           fault_is_read(fault) ? "read" : "write",
           fault_is_prefetch(fault) ? "prefetch fault" : "fault",
           fault->ip,
           fault->addr,
           fault->fsr);
    printf("Context:\n");
    // print_ctx_regs(fault_get_ctx(fault));
    printf(ANSI_COLOR(RESET));
    printf("--------\n");
}

seL4_Word fault_get_data_mask(fault_t *f)
{
    seL4_Word mask = 0;
    seL4_Word addr = f->addr;
    switch (fault_get_width(f)) {
    case WIDTH_BYTE:
        mask = 0x000000ff;
        assert(!(addr & 0x0));
        break;
    case WIDTH_HALFWORD:
        mask = 0x0000ffff;
        assert(!(addr & 0x1));
        break;
    case WIDTH_WORD:
        mask = 0xffffffff;
        assert(!(addr & 0x3));
        break;
    case WIDTH_DOUBLEWORD:
        mask = ~mask;
        break;
    default:
        /* Should never get here... Keep the compiler happy */
        assert(0);
        return 0;
    }
    mask <<= (addr & 0x3) * 8;
    return mask;
}

/*************************
 *** Getters / Setters ***
 *************************/

seL4_Word fault_get_data(fault_t *f)
{
    if ((f->content & CONTENT_DATA) == 0) {
        /* Get register opearand */
        int rt  = get_rt(f);

        /* Decode whether register is banked */
        int reg = decode_vcpu_reg(rt, f);
        seL4_Word data;
        if (reg == seL4_VCPUReg_Num) {
            /* Not banked, use seL4_UserContext */
            data = sel4utils_reg_get_val(*fault_get_ctx(f), rt);
        } else {
            /* Banked, use VCPU invocations */
            // seL4_ARM_VCPU_ReadRegs_t res = seL4_ARM_VCPU_ReadRegs(f->vcpu->vcpu.cptr, reg);
            // if (res.error) {
            //     ZF_LOGF("Read registers failed");
            // }
            // data = res.value;
        }
        fault_set_data(f, data);
    }
    return f->data;
}

void fault_set_data(fault_t *f, seL4_Word data)
{
    f->data = data;
    f->content |= CONTENT_DATA;
}

seL4_Word fault_get_address(fault_t *f)
{
    return f->addr;
}

seL4_Word fault_get_fsr(fault_t *f)
{
    return f->fsr;
}

seL4_Word fault_get_inst(fault_t *f)
{
    return f->instruction;
}

seL4_UserContext *fault_get_ctx(fault_t *f)
{
    if ((f->content & CONTENT_REGS) == 0) {
        int err;
        err = seL4_TCB_ReadRegisters(vm_get_vcpu_tcb(f->vcpu), false, 0,
                                     sizeof(f->regs) / sizeof(f->regs.pc),
                                     &f->regs);
        assert(!err);
        f->content |= CONTENT_REGS;
    }
    return &f->regs;
}

void fault_set_ctx(fault_t *f, seL4_UserContext *ctx)
{
    f->regs = *ctx;
    f->content |= CONTENT_REGS;
}

int fault_handled(fault_t *f)
{
    return f->is_handled == 1;
}

int fault_is_prefetch(fault_t *f)
{
    return f->type == PREFETCH;
}

// int fault_is_wfi(fault_t *f)
// {
//     return HSR_EXCEPTION_CLASS(f->fsr) == HSR_WFx_EXCEPTION;
// }

int fault_is_vcpu(fault_t *f)
{
    return f->type == VCPU;
}

enum fault_width fault_get_width(fault_t *f)
{
    if ((f->content & CONTENT_WIDTH) == 0) {
        if (fault_is_data(f)) {
            uint32_t inst = f->instruction;

            uint8_t is_opcode_valid = false;

            /* LDPTR, STPTR */
            uint16_t opcode = (inst >> 24) & 0xff;
            if (opcode >= 0x24 && opcode <= 0x27) {
                
                if (opcode & 1) {
                    f->mem_op = ST;
                } else {
                    f->mem_op = LD;
                }

                if (f->mem_op == ST) {
                    if ((opcode & 0x3) == 0x1) {
                        f->width = WIDTH_WORD;
                    } else {
                        f->width = WIDTH_DOUBLEWORD;
                    }
                } else {
                    if ((opcode & 0x3) == 0x0) {
                        f->width = WIDTH_WORD;
                    } else {
                        f->width = WIDTH_DOUBLEWORD;
                    }
                }

                is_opcode_valid = true; 
            }

            /* ld, st, ld.u */
            opcode = (inst >> 22) & 0x3ff;
            if (opcode >= 0xa0 && opcode <= 0xaa) {

                if (opcode >= 0xa0 && opcode <= 0xa3) {
                    f->mem_op = LD;
                } else if (opcode >= 0xa4 && opcode <= 0xa7) {
                    f->mem_op = ST;
                } else {
                    f->mem_op = LD;
                }

                f->width = opcode & 0x3; 

                is_opcode_valid = true;
            }

            if (is_opcode_valid) {
                f->content |= CONTENT_WIDTH;
            }
            
        }
    }

    return f->width;
}

size_t fault_get_width_size(fault_t *fault)
{
    enum fault_width width = fault_get_width(fault);
    switch (width) {
    case WIDTH_DOUBLEWORD:
        return sizeof(long long);
    case WIDTH_WORD:
        return sizeof(int);
    case WIDTH_HALFWORD:
        return sizeof(short);
    case WIDTH_BYTE:
        return sizeof(char);
    default:
        return 0;
    }
    return sizeof(long long);
}
