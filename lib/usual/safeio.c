/*
 * libusual - Utility library for C
 *
 * Copyright (c) 2007-2009 Marko Kreen, Skype Technologies OÜ
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Wrappers around regular I/O functions (send/recv/read/write)
 * that survive EINTR and also can log problems.
 */

#include <usual/safeio.h>

#include <usual/socket.h>
#include <usual/logging.h>
#include <usual/string.h>
#include <usual/time.h>

#include <linux/io_uring.h>
#include <liburing.h>

#define ENTRIES 32
struct io_uring ring;

void safe_init_uring(void){
	int flags = 0;
	io_uring_queue_init(ENTRIES, &ring, flags);
}

ssize_t safe_uring_recv(int fd, void*buf, size_t len, int flags){
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	ssize_t ret;

recv:
	sqe = io_uring_get_sqe(&ring);
	io_uring_prep_recv(sqe, fd, buf, len, flags | MSG_DONTWAIT);

	// Submit and wait for at least one CQE
    ret = io_uring_submit_and_wait(&ring, 1);
    if (ret == -EINTR) {
		goto recv;
    } else if (ret < 0) {
		return ret;
	}

    // Get the completion queue entry (CQE)
    ret = io_uring_peek_cqe(&ring, &cqe);
    if (ret < 0) {
		return ret;
	}

	ret = cqe->res;

	// Mark the CQE as seen
	io_uring_cqe_seen(&ring, cqe);

    // Check the result of the operation
    if (ret == -EINTR) {
		goto recv;
    }

	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}

ssize_t safe_uring_send(int fd, const void*buf, size_t len, int flags){
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	ssize_t ret;

send:
	sqe = io_uring_get_sqe(&ring);
	io_uring_prep_send(sqe, fd, buf, len, flags);

	// Submit and wait for at least one CQE
    ret = io_uring_submit_and_wait(&ring, 1);
    if (ret == -EINTR) {
		goto send;
    } else if (ret < 0) {
		return ret;
	}

    // Get the completion queue entry (CQE)
    ret = io_uring_peek_cqe(&ring, &cqe);
    if (ret < 0) {
		return ret;
	}

	ret = cqe->res;

	// Mark the CQE as seen
	io_uring_cqe_seen(&ring, cqe);

    // Check the result of the operation
    if (ret == -EINTR) {
		goto send;
    }

	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}

ssize_t safe_uring_read(int fd, void *buf, size_t len) {
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	ssize_t ret;

read:
	sqe = io_uring_get_sqe(&ring);
	io_uring_prep_read(sqe, fd, buf, len, 0);

	// Submit and wait for at least one CQE
    ret = io_uring_submit_and_wait(&ring, 1);
    if (ret == -EINTR) {
		goto read;
    } else if (ret < 0) {
		return ret;
	}

    // Get the completion queue entry (CQE)
    ret = io_uring_peek_cqe(&ring, &cqe);
    if (ret < 0) {
		return ret;
	}

	ret = cqe->res;

	// Mark the CQE as seen
	io_uring_cqe_seen(&ring, cqe);

    // Check the result of the operation
    if (ret == -EINTR) {
		goto read;
    }

	return ret;
}

ssize_t safe_uring_write(int fd, const void *buf, size_t len) {
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	ssize_t ret;

write:
	sqe = io_uring_get_sqe(&ring);
	io_uring_prep_write(sqe, fd, buf, len, 0);

	// Submit and wait for at least one CQE
    ret = io_uring_submit_and_wait(&ring, 1);
    if (ret == -EINTR) {
		goto write;
    } else if(ret < 0) {
		return ret;
	}

    // Get the completion queue entry (CQE)
    ret = io_uring_peek_cqe(&ring, &cqe);
    if (ret < 0) {
		return ret;
	}

	ret = cqe->res;

	// Mark the CQE as seen
	io_uring_cqe_seen(&ring, cqe);

    // Check the result of the operation
    if (ret == -EINTR) {
		goto write;
    }

	return ret;
}

ssize_t safe_uring_recvmsg(int fd, struct msghdr *msg, int flags) {
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	ssize_t ret;

recvmsg:
	sqe = io_uring_get_sqe(&ring);
	io_uring_prep_recvmsg(sqe, fd, msg, flags);

	// Submit and wait for at least one CQE
    ret = io_uring_submit_and_wait(&ring, 1);
    if (ret == -EINTR) {
		goto recvmsg;
    } else if(ret < 0) {
		return ret;
	}

    // Get the completion queue entry (CQE)
    ret = io_uring_peek_cqe(&ring, &cqe);
    if (ret < 0) {
		return ret;
	}

	ret = cqe->res;

	// Mark the CQE as seen
	io_uring_cqe_seen(&ring, cqe);

    // Check the result of the operation
    if (ret == -EINTR) {
		goto recvmsg;
    }

	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}

ssize_t safe_uring_sendmsg(int fd, const struct msghdr *msg, int flags) {
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	ssize_t ret;

sendmsg:
	sqe = io_uring_get_sqe(&ring);
	io_uring_prep_sendmsg(sqe, fd, msg, flags);

	// Submit and wait for at least one CQE
    ret = io_uring_submit_and_wait(&ring, 1);
    if (ret == -EINTR) {
		goto sendmsg;
    } else if(ret < 0) {
		return ret;
	}

    // Get the completion queue entry (CQE)
    ret = io_uring_peek_cqe(&ring, &cqe);
    if (ret < 0) {
		return ret;
	}

	ret = cqe->res;

	// Mark the CQE as seen
	io_uring_cqe_seen(&ring, cqe);

    // Check the result of the operation
    if (ret == -EINTR) {
		goto sendmsg;
    }

	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}

ssize_t safe_read(int fd, void *buf, size_t len)
{
	ssize_t res;
loop:
	res = read(fd, buf, len);
	if (res < 0 && errno == EINTR)
		goto loop;
	return res;
}

ssize_t safe_write(int fd, const void *buf, size_t len)
{
	ssize_t res;
loop:
	res = write(fd, buf, len);
	if (res < 0 && errno == EINTR)
		goto loop;
	return res;
}

ssize_t safe_recv(int fd, void *buf, size_t len, int flags)
{
	ssize_t res;
	char ebuf[128];
loop:
	log_info("going to call receive");

	res = recv(fd, buf, len, flags);

	log_info("%ld received, %d and EINTR: %d", res, errno, EINTR);

	if (res < 0 && errno == EINTR)
		goto loop;
	if (res < 0)
		log_noise("safe_recv(%d, %zu) = %s", fd, len,
			  strerror_r(errno, ebuf, sizeof(ebuf)));
	else if (cf_verbose > 2)
		log_noise("safe_recv(%d, %zu) = %zd", fd, len, res);
	return res;
}

ssize_t safe_send(int fd, const void *buf, size_t len, int flags)
{
	ssize_t res;
	char ebuf[128];
loop:
	res = send(fd, buf, len, flags);
	if (res < 0 && errno == EINTR)
		goto loop;
	if (res < 0)
		log_noise("safe_send(%d, %zu) = %s", fd, len,
			  strerror_r(errno, ebuf, sizeof(ebuf)));
	else if (cf_verbose > 2)
		log_noise("safe_send(%d, %zu) = %zd", fd, len, res);
	return res;
}

int safe_close(int fd)
{
	int res;

#ifndef WIN32
	/*
	 * POSIX says close() can return EINTR but fd state is "undefined"
	 * later.  Seems Linux and BSDs close the fd anyway and EINTR is
	 * simply informative.  Thus retry is dangerous.
	 */
	res = close(fd);
#else
	/*
	 * Seems on windows it can returns proper EINTR but only when
	 * WSACancelBlockingCall() is called.  As we don't do it,
	 * ignore EINTR on win32 too.
	 */
	res = closesocket(fd);
#endif
	if (res < 0) {
		char ebuf[128];
		log_warning("safe_close(%d) = %s", fd,
			    strerror_r(errno, ebuf, sizeof(ebuf)));
	} else if (cf_verbose > 2) {
		log_noise("safe_close(%d) = %d", fd, res);
	}

	/* ignore EINTR */
	if (res < 0 && errno == EINTR)
		return 0;

	return res;
}

ssize_t safe_recvmsg(int fd, struct msghdr *msg, int flags)
{
	ssize_t res;
	char ebuf[128];
loop:
	res = recvmsg(fd, msg, flags);
	if (res < 0 && errno == EINTR)
		goto loop;
	if (res < 0)
		log_warning("safe_recvmsg(%d, msg, %d) = %s", fd, flags,
			    strerror_r(errno, ebuf, sizeof(ebuf)));
	else if (cf_verbose > 2)
		log_noise("safe_recvmsg(%d, msg, %d) = %zd", fd, flags, res);
	return res;
}

ssize_t safe_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	ssize_t res;
	int msgerr_count = 0;
	char ebuf[128];
loop:
	res = sendmsg(fd, msg, flags);
	if (res < 0 && errno == EINTR)
		goto loop;

	if (res < 0) {
		log_warning("safe_sendmsg(%d, msg[%d,%d], %d) = %s", fd,
			    (int)msg->msg_iov[0].iov_len,
			    (int)msg->msg_controllen,
			    flags, strerror_r(errno, ebuf, sizeof(ebuf)));

		/* with ancillary data on blocking socket OSX returns
		 * EMSGSIZE instead of blocking.  try to solve it by waiting */
		if (errno == EMSGSIZE && msgerr_count < 20) {
			struct timeval tv = {1, 0};
			log_warning("trying to sleep a bit");
			select(0, NULL, NULL, NULL, &tv);
			msgerr_count++;
			goto loop;
		}
	} else if (cf_verbose > 2)
		log_noise("safe_sendmsg(%d, msg, %d) = %zd", fd, flags, res);
	return res;
}

int safe_connect(int fd, const struct sockaddr *sa, socklen_t sa_len)
{
	int res;
	char buf[128];
	char ebuf[128];
loop:
	res = connect(fd, sa, sa_len);
	if (res < 0 && errno == EINTR)
		goto loop;
	if (res < 0 && (errno != EINPROGRESS || cf_verbose > 2))
		log_noise("connect(%d, %s) = %s", fd,
			  sa2str(sa, buf, sizeof(buf)),
			  strerror_r(errno, ebuf, sizeof(ebuf)));
	else if (cf_verbose > 2)
		log_noise("connect(%d, %s) = %d", fd, sa2str(sa, buf, sizeof(buf)), res);
	return res;
}

int safe_accept(int fd, struct sockaddr *sa, socklen_t *sa_len_p)
{
	int res;
	char buf[128];
	char ebuf[128];
loop:
	res = accept(fd, sa, sa_len_p);
	if (res < 0 && errno == EINTR)
		goto loop;
	if (res < 0)
		log_noise("safe_accept(%d) = %s", fd,
			  strerror_r(errno, ebuf, sizeof(ebuf)));
	else if (cf_verbose > 2) {
		if (sa->sa_family == AF_UNIX)
			/* sa2str() won't work here since accept() doesn't set sun_path */
			log_noise("safe_accept(%d) = %d (unix)", fd, res);
		else
			log_noise("safe_accept(%d) = %d (%s)", fd, res, sa2str(sa, buf, sizeof(buf)));
	}
	return res;
}
