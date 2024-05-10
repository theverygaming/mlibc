#include <asm/ioctls.h>

#include <bits/ensure.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <frg/logging.hpp>
#include <keyronex/syscall.h>
#include <limits.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <stdlib.h>

#define STUB_ONLY                                           \
	{                                                   \
		__ensure(!"STUB_ONLY function was called"); \
		__builtin_unreachable();                    \
	}

namespace mlibc {

void
sys_libc_log(const char *message)
{
	syscall1(kKrxDebugMessage, (uintptr_t)message, NULL);
}

[[noreturn]] void
sys_libc_panic()
{
	sys_libc_log("\nMLIBC PANIC\n");
	for (;;)
		;
}

int sys_tcb_set(void *pointer)
{
	STUB_ONLY
}

[[gnu::weak]] int sys_futex_tid()
{
	return 1;
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time)
{
	STUB_ONLY
}

int sys_futex_wake(int *pointer)
{
	STUB_ONLY
}

[[noreturn]] void sys_exit(int status)
{
	STUB_ONLY
}

int sys_anon_allocate(size_t size, void **pointer)
{
	int r;
	uintptr_t out;
	r = syscall1(kKrxVmAllocate, size, &out);
	if (r == 0)
		*pointer = (void*)out;
	return r;
}

int sys_anon_free(void *pointer, size_t size)
{
	(void)pointer;
	(void)size;
	STUB_ONLY
}

int sys_open(const char *pathname, int flags, mode_t mode, int *fd)
{
	STUB_ONLY
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read)
{
	STUB_ONLY
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset)
{
	STUB_ONLY
}

int sys_close(int fd)
{
	STUB_ONLY
}

int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window)
{
	STUB_ONLY
}


int sys_vm_unmap(void *pointer, size_t size)
{
	STUB_ONLY
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written)
{
	STUB_ONLY
}

int sys_clock_get(int clock, time_t *secs, long *nanos)
{
	STUB_ONLY
}

} // namespace mlibc
