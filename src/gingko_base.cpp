/**
 *  gingko_base.cpp
 *  gingko
 *
 *  Created by Auxten on 11-4-10.
 *  Copyright 2011 Baidu. All rights reserved.
 *
 **/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <sys/sendfile.h>
#endif /** __APPLE__ **/
#ifdef __APPLE__
#include <poll.h>
#else
#include <sys/poll.h>
#endif /** __APPLE__ **/

#include "event.h"
#include "config.h"

#include "hash/xor_hash.h"
#include "gingko.h"
#include "path.h"
#include "log.h"
#include "socket.h"
#include "limit.h"

///mutex for gethostbyname limit
static pthread_mutex_t g_netdb_mutex = PTHREAD_MUTEX_INITIALIZER;


/// global struct gko
extern s_gingko_global_t gko;

/**
 * @brief Set signal handler
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
void set_sig(void(*int_handler)(int))
{
    struct sigaction sa;

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);
    signal(SIGQUIT, int_handler);
    signal(SIGHUP, SIG_IGN);

    /** Ignore terminal signals **/
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    /** Ignore SIGPIPE & SIGCLD **/
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    if(sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1)
    {
        gko_log(FATAL, "ignore SIGPIPE failed");
    }
    return;
}

/**
 * @brief start a thread to watch the gko.sig_flag and take action
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int sig_watcher(void * (*worker)(void *))
{
    pthread_attr_t attr;
    pthread_t sig_watcher_thread;

    /** initialize the pthread attrib for every worker **/
    pthread_attr_init(&attr);
    /** set attr state for join() in the mother **/
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, MYSTACKSIZE);

    pthread_create(&sig_watcher_thread, &attr, worker, NULL);

    return 0;
}

/**
 * @brief separate the element from a req string
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int sep_arg(char * inputstring, char * arg_array[], int max)
{
    char **ap;
    int i = 0;
    max++;
    for (ap = arg_array; (*ap = strsep(&inputstring, "\t")) != NULL;)
    {
        if (**ap != '\0')
        {
            if (i++, ++ap >= &arg_array[max])
            {
                break;
            }
        }
    }

    return i;
}

/**
 * @brief parse the request return the proper func handle num
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int parse_req(char *req)
{
    u_int i;
    if (UNLIKELY(!req))
    {
        return CMD_COUNT - 1;
    }
    for (i = 0; i < CMD_COUNT - 1; i++)
    {
        if (gko.cmd_list_p[i][0] == req[0] &&
            gko.cmd_list_p[i][1] == req[1] &&
            gko.cmd_list_p[i][2] == req[2] &&
            gko.cmd_list_p[i][3] == req[3])
        {
            break;
        }
    }
    return i;
}

/**
 * @brief thread safe gethostbyname
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
struct hostent * gethostname_my(const char *host, struct hostent * ret)
{
    struct hostent * tmp;
    if (!ret)
    {
        gko_log(FATAL, "Null buf passed to gethostname_my error");
        return (struct hostent *) NULL;
    }

    pthread_mutex_lock(&g_netdb_mutex);
    tmp = gethostbyname(host);
    if (tmp)
    {
        memcpy(ret, tmp, sizeof(struct hostent));
    }
    else
    {
        gko_log(WARNING, "resolve %s failed", host);
        ret = NULL;
    }
    pthread_mutex_unlock(&g_netdb_mutex);

    return ret;
}

/**
 * @brief get host in_addr_t, this only works on IPv4. that is enough
 *        return h_length, on error return 0
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-25
 **/
int getaddr_my(const char *host, in_addr_t * addr_p)
{
    struct hostent * tmp;
    int ret = 0;
    if (!addr_p)
    {
        gko_log(FATAL, "Null buf passed to getaddr_my error");
        return 0;
    }

    pthread_mutex_lock(&g_netdb_mutex);
    tmp = gethostbyname(host);
    if (tmp)
    {
        *addr_p = *(in_addr_t *)tmp->h_addr;
        ret = tmp->h_length;
    }
    else
    {
        gko_log(WARNING, "resolve %s failed", host);
        ret = 0;
    }
    pthread_mutex_unlock(&g_netdb_mutex);

    return ret;
}

/**
 * @brief check the ulimit -n
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-24
 **/
int check_ulimit()
{
    struct rlimit lmt;
    if(getrlimit(RLIMIT_NOFILE, &lmt) != 0)
    {
        gko_log(FATAL, "getrlimit(RLIMIT_NOFILE, &lmt) error");
        return -1;
    }
    if(lmt.rlim_max < MIN_NOFILE)
    {
        fprintf(stderr, "The max open files limit is %lld, you had better make it >= %lld\n", lmt.rlim_max, MIN_NOFILE);
        gko_log(FATAL, "The max open files limit is %lld, you had better make it >= %lld\n", lmt.rlim_max, MIN_NOFILE);
        return -1;
    }
    return 0;
}

/**
 * @brief event handle of write
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
void ev_fn_gsend(int fd, short ev, void *arg)
{
    s_write_arg_t * a = (s_write_arg_t *) arg;
    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 1;
    if (((a->sent = send(fd, a->p + a->send_counter, a->sz - a->send_counter,
            a->flag)) >= 0) || ERR_RW_RETRIABLE(errno))
    {
        ///gko_log(NOTICE, "res: %d",res);
        ///gko_log(NOTICE, "%s",client->read_buffer+read_counter);
        ///gko_log(NOTICE, "sent: %d", a->sent);
        if ((a->send_counter = MAX(a->sent, 0) + a->send_counter) == a->sz)
        {
            event_del(&(a->ev_write));
        }
        a->retry = 0;
    }
    else
    {
        gko_log(WARNING, "gsend error");
        if ((a->retry)++ > 3)
        {
            event_del(&(a->ev_write));
            event_base_loopexit(a->ev_base, &t);
        }
    }
    return;
}


/**
 * @brief send a mem to fd(usually socket)
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
//int sendall(int fd, const void * void_p, int sz, int flag)
//{
//    s_write_arg_t arg;
//    if (!sz)
//    {
//        return 0;
//    }
//    if (!void_p)
//    {
//        gko_log(WARNING, "Null Pointer");
//        return -1;
//    }
//    arg.sent = 0;
//    arg.send_counter = 0;
//    arg.sz = sz;
//    arg.p = (char *) void_p;
//    arg.flag = flag;
//    arg.retry = 0;
//    /// FIXME event_init() and event_base_free() waste open pipe
//    arg.ev_base = event_init();
//
//    event_set(&(arg.ev_write), fd, EV_WRITE | EV_PERSIST, ev_fn_gsend,
//            (void *) (&arg));
//    event_base_set(arg.ev_base, &(arg.ev_write));
//    if (-1 == event_add(&(arg.ev_write), 0))
//    {
//        gko_log(WARNING, "Cannot handle write data event");
//    }
//    event_base_loop(arg.ev_base, 0);
//    event_del(&(arg.ev_write));
//    event_base_free(arg.ev_base);
//    ///gko_log(NOTICE, "sent: %d", arg.sent);
//    if (arg.sent < 0)
//    {
//        gko_log(WARNING, "ev_fn_write error");
//        return -1;
//    }
//    return 0;
//}

/**
 * @brief send a mem to fd(usually socket)
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int sendall(int fd, const void * void_p, int sz, int timeout)
{
    int sent = 0;
    int send_counter = 0;
    int retry = 0;
    int select_ret;
    char * p;
    if (!sz)
    {
        return 0;
    }
    if (!void_p)
    {
        gko_log(WARNING, "Null Pointer");
        return -1;
    }
    p =(char *)void_p;

    while (send_counter < sz)
    {
#if HAVE_POLL
        {
            struct pollfd pollfd;

            pollfd.fd = fd;
            pollfd.events = POLLOUT;

            /* send_sec is in seconds, timeout in ms */
            select_ret = poll(&pollfd, 1, (int)(timeout * 1000 + 1));
        }
#else
        {
            fd_set wset;
            FD_ZERO(&wset);
            FD_SET(fd, &wset);
            select_ret = select(fd + 1, 0, &wset, 0, &send_timeout);
        }
#endif /* HAVE_POLL */
        if (select_ret < 0)
        {
            if (ERR_RW_RETRIABLE(errno))
            {
                if (retry++ > 3)
                {
                    gko_log(WARNING, "select error over 3 times");
                    return -1;
                }
                continue;
            }
            gko_log(WARNING, "select/poll error on sendall");
            return -1;
        }
        else if (!select_ret)
        {
            gko_log(NOTICE, "select/poll timeout on sendall");
            return -1;
        }
        else
        { /** select/poll returned a fd **/
            if (((sent = send(fd, p + send_counter, sz - send_counter,
                    0)) >= 0) || ERR_RW_RETRIABLE(errno))
            {
                ///gko_log(NOTICE, "res: %d",res);
                ///gko_log(NOTICE, "%s",client->read_buffer+read_counter);
                ///gko_log(NOTICE, "sent: %d", a->sent);
                retry = 0;
                send_counter += MAX(sent, 0);
            }
            else
            {
                if (retry++ > 3)
                {
//                    gko_log(WARNING, "send error over 3 times");
                    return -1;
                }
            }

        }
    }

    return send_counter;
}

/**
 * @brief event handle of sendfile
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
void ev_fn_gsendfile(int fd, short ev, void *arg)
{
    s_gsendfile_arg_t * a = (s_gsendfile_arg_t *) arg;
    off_t tmp_off = a->offset + a->send_counter;
    GKO_UINT64 tmp_counter = a->count - a->send_counter;
    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 1;
    if (((a->sent = gsendfile(fd, a->in_fd, &tmp_off, &tmp_counter)) >= 0))
    {
        ///gko_log(NOTICE, "res: %d",res);
        ///gko_log(NOTICE, "%s",client->read_buffer+read_counter);
        bw_up_limit(a->sent, gko.opt.limit_up_rate);
        ///gko_log(NOTICE, "gsentfile: %d", a->sent);
        if ((a->send_counter += a->sent) == a->count)
        {
            event_del(&(a->ev_write));
        }
        a->retry = 0;
    }
    else
    {
        if (a->retry++ > 3)
        {
            event_del(&(a->ev_write));
            event_base_loopexit(a->ev_base, &t);
        }
    }
    return;
}

/**
 * @brief send conut bytes file  from in_fd to out_fd at offset
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int sendfileall(int out_fd, int in_fd, off_t *offset, GKO_UINT64 *count)
{
    s_gsendfile_arg_t arg;
    if (!*count)
    {
        return 0;
    }
    arg.sent = 0;
    arg.send_counter = 0;
    arg.in_fd = in_fd;
    arg.offset = *offset;
    arg.count = *count;
    arg.retry = 0;
    /// FIXME event_init() and event_base_free() waste open pipe
    arg.ev_base = (event_base*)event_init();

    event_set(&(arg.ev_write), out_fd, EV_WRITE | EV_PERSIST, ev_fn_gsendfile,
            (void *) (&arg));
    event_base_set(arg.ev_base, &(arg.ev_write));
    if (-1 == event_add(&(arg.ev_write), 0))
    {
        gko_log(WARNING, "Cannot handle write data event");
    }
    event_base_loop(arg.ev_base, 0);
    event_del(&(arg.ev_write));
    event_base_free(arg.ev_base);
    if (arg.sent < 0)
    {
//        gko_log(WARNING, "ev_fn_gsendfile error");
        return -1;
    }
    return 0;
}


/**
 * @brief try best to read specificed bytes from a file to buf
 * @brief so the file should not be very large...
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int readfileall(int fd, off_t offset, off_t count, char ** buf)
{
    int res;
    off_t read_counter = 0;
    if (!count)
    {
        return 0;
    }

    /// Initialize buffer
    *buf = new char[count];

    while (read_counter < count && (res = pread(fd, *buf + read_counter,
            count - read_counter, offset + read_counter)) > 0)
    {
        ///gko_log(NOTICE, "res: %d",res);
        ///gko_log(NOTICE, "%s",client->read_buffer+read_counter);
        read_counter += res;
    }
    if (res < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            ///#define EAGAIN      35      /** Resource temporarily unavailable **/
            ///#define EWOULDBLOCK EAGAIN      /** Operation would block **/
            ///#define EINPROGRESS 36      /** Operation now in progress **/
            ///#define EALREADY    37      /** Operation already in progress **/
            gko_log(FATAL, "File read error");
            delete [] (*buf);
            return -1;
        }
    }
    return 0;
}


/**
 * @brief read all from a socket by best effort
 *
 * @see
 * @note
 * receive all the data or returns error (handles EINTR etc.)
 * params: socket
 *         data     - buffer for the results
 *         data_len -
 *         flags    - recv flags for the first recv (see recv(2)), only
 *                    0, MSG_WAITALL and MSG_DONTWAIT make sense
 * if flags is set to MSG_DONWAIT (or to 0 and the socket fd is non-blocking),
 * and if no data is queued on the fd, recv_all will not wait (it will
 * return error and set errno to EAGAIN/EWOULDBLOCK). However if even 1 byte
 *  is queued, the call will block until the whole data_len was read or an
 *  error or eof occured ("semi-nonblocking" behaviour,  some tcp code
 *   counts on it).
 * if flags is set to MSG_WAITALL it will block even if no byte is available.
 *
 * returns: bytes read or error (<0)
 * can return < data_len if EOF
 *
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
//int readall(int socket, void* data, int data_len, int flags)
//{
//    int b_read = 0;
//    int n;
//    int timer = RCV_TIMEOUT * 1000000;/// useconds
//    while (b_read < data_len)
//    {
//        n = recv(socket, (char*) data + b_read, data_len - b_read, flags);
//        if (UNLIKELY((n < 0 && !ERR_RW_RETRIABLE(errno)) || !n))
//        {
//            gko_log(WARNING, "readall error");
//            return -1;
//        }
//        else
//        {
//            b_read += (n < 0 ? 0 : n);
//            usleep(READ_INTERVAL);
//            timer -= READ_INTERVAL;
//            if (timer <= 0)
//            {
//                gko_log(WARNING, "readall timeout");
//                return -1;
//            }
//        }
//    }
//    return b_read;
//}

/**
 * @brief read all from a socket by best effort
 *
 * @see
 * @note
 * receive all the data or returns error (handles EINTR etc.)
 * params: socket
 *         data     - buffer for the results
 *         data_len -
 *         flags    - recv flags for the first recv (see recv(2)), only
 *                    0, MSG_WAITALL and MSG_DONTWAIT make sense
 * if flags is set to MSG_DONWAIT (or to 0 and the socket fd is non-blocking),
 * and if no data is queued on the fd, recv_all will not wait (it will
 * return error and set errno to EAGAIN/EWOULDBLOCK). However if even 1 byte
 *  is queued, the call will block until the whole data_len was read or an
 *  error or eof occured ("semi-nonblocking" behaviour,  some tcp code
 *   counts on it).
 * if flags is set to MSG_WAITALL it will block even if no byte is available.
 *
 * returns: bytes read or error (<0)
 * can return < data_len if EOF
 *
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int readall(int fd, void* data, int data_len, int timeout)
{
    int b_read = 0;
    int n = 0;
    int retry = 0;
    int select_ret = 0;

    while (b_read < data_len)
    {
#if HAVE_POLL
        {
            struct pollfd pollfd;

            pollfd.fd = fd;
            pollfd.events = POLLIN;

            /* send_sec is in seconds, timeout in ms */
            select_ret = poll(&pollfd, 1, (int)(timeout * 1000 + 1));
        }
#else
        {
            fd_set wset;
            FD_ZERO(&wset);
            FD_SET(fd, &wset);
            select_ret = select(fd + 1, &wset, 0, 0, &send_timeout);
        }
#endif /* HAVE_POLL */
        if (select_ret < 0)
        {
            if (ERR_RW_RETRIABLE(errno))
            {
                if (retry++ > 3)
                {
                    gko_log(WARNING, "select error over 3 times");
                    return -1;
                }
                continue;
            }
            gko_log(WARNING, "select/poll error on readall");
            return -1;
        }
        else if (!select_ret)
        {
            gko_log(NOTICE, "select/poll timeout on readall");
            return -1;
        }
        else
        { /** select/poll returned a fd **/
            n = recv(fd, (char*) data + b_read, data_len - b_read, 0);
            if (UNLIKELY((n < 0 && !ERR_RW_RETRIABLE(errno)) || !n))
            {
                gko_log(WARNING, "readall error");
                return -1;
            }
            else
            {
                if (n < 0)
                {
                    if (retry++ > 3)
                    {
                        gko_log(WARNING, "readall timeout");
                        return -1;
                    }
                }
                else
                {
                    b_read += n;
                }
            }

        }
    }

    return b_read;
}


/**
 * @brief send blocks to the out_fd(usually socket)
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int sendblocks(int out_fd, s_job_t * jo, GKO_INT64 start, GKO_INT64 num)
{
    if (num <= 0)
    {
        return 0;
    }
    /**
     * if include the last block, the total size
     *    may be smaller than BLOCK_SIZE * num
     **/
    GKO_INT64 sent = 0;
    GKO_INT64 b = start;
    GKO_INT64 file_size;
    GKO_UINT64 file_left;
    int fd = -1;

    GKO_INT64 total = BLOCK_SIZE * (num - 1)
            + (start + num >= jo->block_count ? (jo->blocks + jo->block_count
                    - 1)->size : BLOCK_SIZE);
    GKO_INT64 f = (jo->blocks + start)->start_f;
    GKO_UINT64 block_left = (jo->blocks + b)->size;
    off_t offset = (jo->blocks + start)->start_off;
    while (total > 0)
    {
        if (fd == -1)
        {
            fd = open((jo->files + f)->name, READ_OPEN_FLAG);
        }
        if (fd < 0)
        {
            gko_log(WARNING, "sendblocks open error");
            return -2;
        }
        file_size = (jo->files + f)->size;
        file_left = file_size - offset;

        if (block_left > file_left)
        {
            if (sendfileall(out_fd, fd, &offset, &file_left) != 0)
            {
//                gko_log(NOTICE, "sendfileall error");
                close(fd);
                fd = -1;
                return -1;
            }
            close(fd);
            fd = -1;
            block_left -= file_left;
            total -= file_left;
            sent += file_left;
            offset = 0;
            f = next_f(jo, f);
        }
        else if (block_left == file_left)
        {
            if (sendfileall(out_fd, fd, &offset, &file_left) != 0)
            {
//                gko_log(NOTICE, "sendfileall error");
                close(fd);
                fd = -1;
                return -1;
            }
            close(fd);
            fd = -1;
            offset = 0;
            total -= file_left;
            sent += file_left;
            f = next_f(jo, f);
            b = next_b(jo, b);
            block_left = (jo->blocks + b)->size;
        }
        else
        { /**block_left < file_left**/
            if (sendfileall(out_fd, fd, &offset, &block_left) != 0)
            {
//                gko_log(NOTICE, "sendfileall error");
                close(fd);
                fd = -1;
                return -1;
            }
            b = next_b(jo, b);
            total -= block_left;
            offset += block_left;
            sent += block_left;
            block_left = (jo->blocks + b)->size;
        }
        if (fd != -1)
        {
            close(fd);
            fd = -1;
        }
    }
    return 0;
}

/**
 * @brief write block to disk
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int writeblock(s_job_t * jo, const u_char * buf, s_block_t * blk)
{
    GKO_INT64 f = blk->start_f;
    off_t offset = blk->start_off;
    GKO_INT64 total = blk->size;
    GKO_INT64 counter = 0;
    GKO_INT64 file_size;

    while (total > 0)
    {
        GKO_INT64 wrote;
        s_file_t * file = jo->files + f;
        file_size = file->size;
        int fd = open(file->name, WRITE_OPEN_FLAG);
        if (fd < 0)
        {
            gko_log(WARNING, "open file '%s' for write error", file->name);
            return -2;
        }
        wrote = pwrite(fd, buf + counter, MIN(file_size - offset, total),
                offset);
        if (wrote < 0)
        {
            gko_log(WARNING, "write file error");
            return -1;
        }
        total -= wrote;
        counter += wrote;
        fsync(fd);
        close(fd);
        f = next_f(jo, f);
        offset = 0;
    }
    return counter;
}

/**
 * @brief send cmd msg to host, not read response, on succ return 0
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
int sendcmd(s_host_t *h, const char * cmd, int recv_sec, int send_sec)
{
    int sock;
    int result;

    sock = connect_host(h, recv_sec, send_sec);
    if (sock < 0)
    {
//        gko_log(NOTICE, "sendcmd() connect_host error");
        return -1;
    }
    result = sendall(sock, cmd, strlen(cmd), send_sec);
    close_socket(sock);
    return result;
}
