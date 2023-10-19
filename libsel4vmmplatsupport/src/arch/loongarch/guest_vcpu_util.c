/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4vm/guest_vm.h>
#include <sel4vm/boot.h>

#include <sel4vmmplatsupport/guest_vcpu_util.h>
#include <sel4vmmplatsupport/arch/guest_vcpu_fault.h>


vm_vcpu_t *create_vmm_plat_vcpu(vm_t *vm, int priority)
{
    int err;
    vm_vcpu_t *vm_vcpu = vm_create_vcpu(vm, priority);
    if (vm_vcpu == NULL) {
        ZF_LOGE("Failed to create platform vcpu");
        return NULL;
    }

    return vm_vcpu;
}
