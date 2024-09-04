#include <sys/epoll.h>

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


namespace mlibc {

int
sys_epoll_pwait(int epfd, struct epoll_event *ev, int n, int timeout,
    const sigset_t *sigmask, int *raised)
{
	(void)sigmask;
	uintptr_t ret = syscall4(kKrxEPollWait, epfd, (uintptr_t)ev, n, timeout,
	    NULL);
	if (ret < 0)
		return -ret;
	*raised = ret;
	return 0;
}

int
sys_epoll_create(int flags, int *fd)
{
	uintptr_t ret = syscall1(kKrxEPollCreate, flags, NULL);
	if (ret < 0)
		return -ret;
	*fd = ret;
	return 0;
}

int
sys_epoll_ctl(int epfd, int mode, int fd, struct epoll_event *ev)
{
	uintptr_t ret = syscall4(kKrxEPollCtl, epfd, mode, fd, (uintptr_t)ev,
	    NULL);
	return sc_error(ret);
}

int
sys_ppoll(struct pollfd *fds, int nfds, const struct timespec *timeout,
    const sigset_t *sigmask, int *num_events)
{
	int epfd, i, ready;
	struct epoll_event ev, events[nfds];
	int err;

	epfd = epoll_create1(0);
	if (epfd == -1) {
		err = errno;
		perror("sys_ppoll(epoll_create1)");
		return err;
	}

	for (i = 0; i < nfds; i++) {
		ev.events = 0;
		if (fds[i].events & POLLIN)
			ev.events |= EPOLLIN;
		if (fds[i].events & POLLOUT)
			ev.events |= EPOLLOUT;
		if (fds[i].events & POLLPRI)
			ev.events |= EPOLLPRI;
		ev.data.u32 = i;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i].fd, &ev) == -1) {
			err = errno;
			perror("sys_ppoll(epoll_ctl)");
			close(epfd);
			return err;
		}
	}

	int timeout_ms = timeout ?
	    timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000 :
	    -1;

	ready = epoll_pwait(epfd, events, nfds, timeout_ms, sigmask);
	if (ready == -1) {
		err = errno;
		perror("sys_ppoll(epoll_pwait)");
		close(epfd);
		return err;
	}

	for (i = 0; i < ready; i++) {
		int idx = events[i].data.u32;
		fds[idx].revents = 0;
		if (events[i].events & EPOLLIN)
			fds[idx].revents |= POLLIN;
		if (events[i].events & EPOLLOUT)
			fds[idx].revents |= POLLOUT;
		if (events[i].events & EPOLLPRI)
			fds[idx].revents |= POLLPRI;
		if (events[i].events & EPOLLERR)
			fds[idx].revents |= POLLERR;
		if (events[i].events & EPOLLHUP)
			fds[idx].revents |= POLLHUP;
	}

	close(epfd);

	*num_events = ready;

	return 0;
}

int
sys_pselect(int num_fds, fd_set *read_set, fd_set *write_set,
    fd_set *except_set, const struct timespec *timeout, const sigset_t *sigmask,
    int *num_events)
{
	int fd = epoll_create1(0);
	if (fd == -1)
		return -1;

	for (int k = 0; k < FD_SETSIZE; k++) {
		struct epoll_event ev;
		memset(&ev, 0, sizeof(struct epoll_event));

		if (read_set && FD_ISSET(k, read_set))
			ev.events |= EPOLLIN;
		if (write_set && FD_ISSET(k, write_set))
			ev.events |= EPOLLOUT;
		if (except_set && FD_ISSET(k, except_set))
			ev.events |= EPOLLPRI;

		if (!ev.events)
			continue;

		ev.data.u32 = k;
		if (epoll_ctl(fd, EPOLL_CTL_ADD, k, &ev))
			return -1;
	}

	struct epoll_event evnts[16];
	int n = epoll_pwait(fd, evnts, 16,
	    timeout ? (timeout->tv_sec * 1000 + timeout->tv_nsec / 100) : -1,
	    sigmask);

	if (n == -1)
		return -1;

	fd_set res_read_set;
	fd_set res_write_set;
	fd_set res_except_set;
	FD_ZERO(&res_read_set);
	FD_ZERO(&res_write_set);
	FD_ZERO(&res_except_set);

	int m = 0;

	for (int i = 0; i < n; i++) {
		int k = evnts[i].data.u32;

		if (read_set && FD_ISSET(k, read_set) &&
		    evnts[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
			FD_SET(k, &res_read_set);
			m++;
		}

		if (write_set && FD_ISSET(k, write_set) &&
		    evnts[i].events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
			FD_SET(k, &res_write_set);
			m++;
		}

		if (except_set && FD_ISSET(k, except_set) &&
		    evnts[i].events & EPOLLPRI) {
			FD_SET(k, &res_except_set);
			m++;
		}
	}

	if (close(fd))
		__ensure("mlibc::pselect: close() failed on epoll file");

	if (read_set)
		memcpy(read_set, &res_read_set, sizeof(fd_set));

	if (write_set)
		memcpy(write_set, &res_write_set, sizeof(fd_set));

	if (except_set)
		memcpy(except_set, &res_except_set, sizeof(fd_set));

	*num_events = m;
	return 0;
}

} /* namespace mlibc */
