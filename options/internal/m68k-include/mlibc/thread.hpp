#pragma once

#include <stdint.h>
#include <mlibc/tcb.hpp>
#include <bits/ensure.h>

namespace mlibc {

inline Tcb *get_current_tcb() {
	asm("illegal");
	return NULL;
}

inline uintptr_t get_sp() {
	asm("illegal");
	return NULL;
}

} // namespace mlibc
