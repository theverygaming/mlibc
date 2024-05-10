#pragma once

#include <stdint.h>
#include <mlibc/internal-sysdeps.hpp>
#include <mlibc/tcb.hpp>
#include <bits/ensure.h>

namespace mlibc {

inline Tcb *get_current_tcb() {
	void *ptr = sys_tp_get();
	return reinterpret_cast<Tcb *>((uintptr_t)ptr - 0x7000 - sizeof(Tcb));
}

inline uintptr_t get_sp() {
	asm("illegal");
	return NULL;
}

} // namespace mlibc
