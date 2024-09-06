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

#include "abi-bits/vm-flags.h"
#include "mlibc/tcb.hpp"

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
#if defined(__m68k__)
	syscall1(kKrxTcbSet, (uintptr_t)pointer + 0x7000 + sizeof(Tcb), NULL);
#elif defined(__amd64__)
	syscall1(kKrxTcbSet, (uintptr_t)pointer, NULL);
#elif defined(__aarch64__)
	uintptr_t addr = reinterpret_cast<uintptr_t>(pointer);
	addr += sizeof(Tcb) - 0x10;
	asm volatile("msr tpidr_el0, %0" ::"r"(addr));
#elif defined(__riscv)
	uintptr_t addr = reinterpret_cast<uintptr_t>(pointer);
	addr += sizeof(Tcb);
	asm volatile("mv tp, %0" ::"r"(addr));
#endif
	return 0;
}

[[gnu::weak]] void *sys_tp_get()
{
	return (void*)syscall0(kKrxTcbGet, NULL);
}

extern "C" void *__m68k_read_tp (void)
{
	return sys_tp_get();
}

[[gnu::weak]] int sys_futex_tid()
{
	return syscall0(kKrxGetTid, NULL);
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time)
{
	(void)time;
	return -syscall3(kKrxFutexWait, (uintptr_t)pointer, expected, 0, NULL);
}

int sys_futex_wake(int *pointer)
{
	return -syscall1(kKrxFutexWake, (uintptr_t)pointer, NULL);
}

[[noreturn]] void sys_exit(int status)
{
	(void)status;
	syscall0(kKrxThreadExit, NULL);
	sys_libc_panic();
}

int sys_anon_allocate(size_t size, void **pointer)
{
	int r;
	uintptr_t out;
	r = syscall1(kKrxVmAllocate, size, &out);
	if (r == 0) {
		*pointer = (void*)out;
		return 0;
	}
	return -r;
}

int sys_anon_free(void *pointer, size_t size)
{
	(void)pointer;
	(void)size;
	STUB_ONLY
}

int sys_open(const char *pathname, int flags, mode_t mode, int *fd)
{
	(void)flags;
	(void)mode;
	int r = syscall1(kKrxFileOpen, (uintptr_t)pathname, NULL);
	if (r >= 0) {
		*fd = r;
		return 0;
	}
	return -r;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read)
{
	int r = syscall3(kKrxFileReadCached, fd, (uintptr_t)buf, count, NULL);

	mlibc::infoLogger() << "SYS_READ CALLED ON FD " << fd << "!\n"
			    << frg::endlog;
	if (r >= 0) {
		*bytes_read = r;
		return 0;
	}
	return -r;
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written)
{
	int r = syscall3(kKrxFileWriteCached, fd, (uintptr_t)buf, count, NULL);
	if (r >= 0) {
		*bytes_written = r;
		return 0;
	}
	return -r;
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset)
{
	off_t r = syscall3(kKrxFileSeek, fd, offset, whence, NULL);
	if (r >= 0) {
		*new_offset = r;
		return 0;
	}
	return -r;
}

int
sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags,
    struct stat *statbuf)
{
	uintptr_t r;

	switch (fsfdt) {
	case fsfd_target::path:
		fd = AT_FDCWD;
		break;

	case fsfd_target::fd:
		flags |= AT_EMPTY_PATH;

	case fsfd_target::fd_path:
		break;

	default:
		__ensure(!"stat: Invalid fsfdt");
		__builtin_unreachable();
	}

	r = syscall4(kKrxFileStat, fd, (uintptr_t)path, flags,
	    (uintptr_t)statbuf, NULL);
	return -r;
}

int
sys_isatty(int fd)
{
	struct winsize ws;
	int result;

	if (!sys_ioctl(fd, TIOCGWINSZ, &ws, &result))
		return 0;

	return ENOTTY;
}

int
sys_ioctl(int fd, unsigned long request, void *arg, int *result)
{
	uintptr_t r = syscall3(kKrxFileIoCtl, fd, request, (uintptr_t)arg,
	    NULL);
	if (r < 0)
		return -r;
	*result = r;
	return 0;
}

int sys_close(int fd)
{
	int r = syscall1(kKrxFileClose, fd, NULL);
	return -r;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window)
{
	uintptr_t addr;
	int r = syscall6(kKrxVmMap, (uintptr_t)hint, size, prot,
	    flags, fd, offset, &addr);
	if (r == 0) {
		*window = (void *)addr;
		return 0;
	}
	return -r;
}


int sys_vm_unmap(void *pointer, size_t size)
{
	STUB_ONLY
}

int sys_clock_get(int clock, time_t *secs, long *nanos)
{
	return 0;
}

pid_t
sys_getpid()
{
	return 1;
}

pid_t
sys_getppid()
{
	return 1;
}

uid_t
sys_geteuid()
{
	mlibc::infoLogger() << "mlibc: sys_geteuid is a stub" << frg::endlog;
	return 0;
}

uid_t
sys_getuid()
{
	mlibc::infoLogger() << "mlibc: sys_setuid is a stub" << frg::endlog;
	return 0;
}

gid_t
sys_getgid()
{
	mlibc::infoLogger() << "mlibc: sys_setgid is a stub" << frg::endlog;
	return 0;
}

gid_t
sys_getegid()
{
	mlibc::infoLogger() << "mlibc: sys_setgid is a stub" << frg::endlog;
	return 0;
}

int
sys_getpgid(pid_t, pid_t *)
{
	return 0;
}

int
sys_fork(pid_t *child)
{
	uintptr_t ret = syscall0(kKrxFork, NULL);

	if (int err = sc_error(ret); err) {
		return err;
	}

	*child = ret;
	return 0;
}

} // namespace mlibc
