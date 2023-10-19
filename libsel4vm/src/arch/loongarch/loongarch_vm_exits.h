/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* LOONGARCH VM Exit Reasons */
enum loongarch_vm_exit_reasons {
    VM_GUEST_ABORT_EXIT,
    VM_SYSCALL_EXIT,
    VM_USER_EXCEPTION_EXIT,
    VM_VCPU_EXIT,
    VM_UNKNOWN_EXIT,
    VM_NUM_EXITS
};
