/**
 *  gingko.h
 *  gingko
 *
 *  Created by Auxten on 11-4-9.
 *  Copyright 2011 Baidu. All rights reserved.
 *
 **/
#ifndef                     _GNU_SOURCE
#define                     _GNU_SOURCE
#endif                      /** _GNU_SOURCE **/

//#define UNITTEST  /* tmp for unittest code write */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <dirent.h>
#include <limits.h>
#include <ftw.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <pthread.h>

#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <sys/sendfile.h>
#endif /** __APPLE__ **/

#include "event.h"


#ifndef GINGKO_H_
#define GINGKO_H_

using namespace std;

#define                 NO_SO_CALLED_FNV_OPTIMIZE
#define                 ROT_XOR_HASH
#define                 _FILE_OFFSET_BITS       64
/// only 8 or 4 here
#define                 HASH_BYTE_NUM_ONCE      8

typedef unsigned long long      GKO_UINT64;
typedef long long               GKO_INT64;
typedef const char * const      GKO_CONST_STR;

/// block size in bytes
static const int        BLOCK_SIZE =            (2 * 1024 * 1024);
/// just like ZERO :p
static const int        SERV_PORT =             2120;
/// tcp buffer size
static const int        TCP_BUF_SZ =            262144;
/// request queue length
static const int        REQ_QUE_LEN =           50;
/// host name length in max
static const int        MAX_HOST_NAME =         255;
/// MAXPATHLEN in linux is 4096!!!,is it too long?
static const u_int      MAX_PATH_LEN =          1024;
/// Lowest port number
static const int        MIN_PORT =              1025;
/// Highest port number
static const int        MAX_PORT =              65535;
/// Maximum listen port
static const int        MAX_LIS_PORT =          60000;
/// Maximum packet length to be received
static const int        MAX_PACKET_LEN =        65536;
/// for global locks
static const int        MAX_JOBS =              1024;
/// req at max MAX_REQ_SERV_BLOCKS from serv
static const int        MAX_REQ_SERV_BLOCKS =   20;
/// max length of a uri
static const int        MAX_URI =               MAX_PATH_LEN;
/// nftw depth this sames have no effect....whatever 100 is enough
static const int        MAX_SCAN_DIR_FD =       5;
///((1LL<<sizeof(GKO_INT64)*8-1)-1);
static const GKO_INT64  MAX_INT64 =             9223372036854775807LL;
/// retry 3 times then fail
static const int        MAX_RETRY =             3;
/// min value of ulimit -n
static const GKO_INT64  MIN_NOFILE =            5000;
/// bind port failed return
static const int        BIND_FAIL =             -13;
/// connect failed return
static const int        HOST_DOWN_FAIL =        -13;
/// bind port try interval, in microseconds
static const int        BIND_INTERVAL =         10000;
/// readall retry interval, in microseconds
static const int        READ_INTERVAL =         20000;
/// gko.sig_flag check interval, in microseconds
static const int        CK_SIG_INTERVAL =       200000;
/// GKO_INT64 int char
static const int        MAX_LONG_INT =          19;
/// default at MacOS is 512k
static const int        MYSTACKSIZE =           (10 * 1024 * 1024);
/// when download is over seed time
static const int        SEED_TIME =             60;
/// default client conn limit
static const int        CLNT_POOL_SIZE =        6;
/// default client thread num
static const int        CLNT_ASYNC_THREAD_NUM = 20;
/// default server conn limit
static const int        SERV_POOL_SIZE =        2000;
/// default client thread num
static const int        SERV_ASYNC_THREAD_NUM = 200;
/// default xor hash thread num
static const int        XOR_HASH_TNUM =         2;
/// host count got from hash ring
static const int        FIRST_HOST_COUNT =      5;
/// host count got from speed test
static const int        SECOND_HOST_COUNT =     5;
/// in seconds
static const int        RCV_TIMEOUT =           10;
/// in seconds
static const int        SND_TIMEOUT =           5;
/// sleep time before free the job related mem
static const int        ERASE_JOB_MEM_WAIT =    (SND_TIMEOUT + 5);
///bytes per second
static const int        CLNT_LIMIT_UP_RATE =    (20 * 1024 * 1024);
///bytes per second
static const int        CLNT_LIMIT_DOWN_RATE =  (20 * 1024 * 1024);
///bytes per second
static const int        SERV_LIMIT_UP_RATE =    (110 * 1024 * 1024);
///bytes per second
static const int        LIMIT_MK_SEED_RATE =    (200 * 1024 * 1024);
/// every physical node have VNODE_NUM vnodes
static const int        VNODE_NUM =             3;
/// to sizeof an extern array we must define it
static const int        CMD_COUNT =             9;
/// IP char length
static const int        IP_LEN =                16;
/// -rw-rw-rw-
static const mode_t     SNAP_FILE_MODE =        0666;

static const int        SNAP_OPEN_FLAG =        (O_RDONLY | O_CREAT | O_NOCTTY);
static const int        SNAP_REOPEN_FLAG =      (O_WRONLY | O_TRUNC | O_CREAT | O_NOCTTY);
static const int        CREATE_OPEN_FLAG =      (O_WRONLY | O_CREAT | O_NOCTTY);
static const int        READ_OPEN_FLAG =        (O_RDONLY | O_NOCTTY);
static const int        WRITE_OPEN_FLAG =       (O_WRONLY | O_NOCTTY);

static const int        MSG_LEN =               (MAX_URI + IP_LEN + 32 + MAX_LONG_INT * 2);
static const int        CLNT_READ_BUFFER_SIZE = (MSG_LEN);
///"NEWW 255.255.255.255 65535"
static const int        SHORT_MSG_LEN =         (4 + 1 + IP_LEN + 1 + 5 + 1);
/// 10s
static const int        SERV_TRANS_TIME_PLUS =  10000000;
static const int        JOIN_ERR =              1;
static const int        MK_DIR_SYMLINK_ERR =    2;
static const int        DOWNLOAD_ERR =          3;


static GKO_CONST_STR     SERVER_LOG =            "/dev/stdout";
static GKO_CONST_STR     CLIENT_LOG =            "/dev/stdout";
/// file for continue interrupted job
static GKO_CONST_STR     GKO_SNAP_FILE =         "._gk_snapshot_";
/// for debug usage
static GKO_CONST_STR     SERVER_IP =             "127.0.0.1";
static GKO_CONST_STR     TIME_FORMAT =           "[%Y-%m-%d %H:%M:%S] ";

/// host_hash usage
static const u_char  DEL_HOST =                 0x01;
static const u_char  ADD_HOST =                 0x02;

/// job state
static const u_char  INITING =                  0x01;    ///initializing
static const u_char  INITED =                   0x02;    ///initialized

/// lock state
static const u_char  LK_USING =                 0x01;
static const u_char  LK_FREE =                  0x00;

/// get_blocks_c flag
static const u_char  W_DISK =                   0x01;


/// calculate the offset of a member in struct
#define OFFSETOF(s, i) ((unsigned) &((s *)0)->i)

/// Macros for min/max.
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif /** MIN **/
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif  /** MAX **/

/// calculate block count by size
#define BLOCK_COUNT(a) \
    (((a) % (BLOCK_SIZE)) ? ((a)/(BLOCK_SIZE) + 1) : ((a)/(BLOCK_SIZE)))

/// reload > for s_host_t
#ifndef GINGKO_OVERLOAD_S_HOST_LT
#define GINGKO_OVERLOAD_S_HOST_LT \
static bool operator <(const s_host_t& lhost, const s_host_t& rhost) {\
    return (((((GKO_UINT64) ntohl(inet_addr(lhost.addr))) << 16)\
            + (GKO_UINT64) lhost.port) < ((((GKO_UINT64) ntohl(\
            inet_addr(rhost.addr))) << 16) + (GKO_UINT64) rhost.port));\
}
#endif /** GINGKO_OVERLOAD_S_HOST_LT **/

/// reload == for s_host_t
#ifndef GINGKO_OVERLOAD_S_HOST_EQ
#define GINGKO_OVERLOAD_S_HOST_EQ \
static bool operator ==(const s_host_t& lhost, const s_host_t& rhost) {\
    return ( strncmp(lhost.addr, rhost.addr, sizeof(lhost.addr)) == 0 &&\
        lhost.port == rhost.port);\
}
#endif /** GINGKO_OVERLOAD_S_HOST_EQ **/

/// Macro judging errno
#define ERR_RW_RETRIABLE(e) \
    ((e) == EINTR || (e) == EAGAIN || (e) == EWOULDBLOCK)
#define CONNECT_DEST_DOWN(e) \
    ((e) == ECONNREFUSED || (e) == ENETUNREACH || (e) == ETIMEDOUT)

/// Evaluates to the same boolean value as 'p', and hints to the compiler that
/// we expect this value to be false.
#ifdef __GNUC__
#define LIKELY(p) __builtin_expect(!!(p), 1)
#define UNLIKELY(p) __builtin_expect(!!(p), 0)
#else
#define LIKELY(p) (p)
#define UNLIKELY(p) (p)
#endif
#define SUCC_CHECK(cond) LIKELY(cond)
#define FAIL_CHECK(cond) UNLIKELY(cond)

/// Macro for unittest
#ifndef UNITTEST
#define GKO_STATIC_FUNC static
#else
#define GKO_STATIC_FUNC
#endif

/// Replacement for assert() that log to file on failure.
#define ASSERT(cond)                     \
    do {                                \
        if (UNLIKELY(!(cond))) {             \
            gko_log(FATAL, "Assertion: (%s) failed, %s() in %s:%d :",     \
                 #cond,__func__,__FILE__,__LINE__);      \
            /** In case a user-supplied handler tries to **/  \
            /** return control to us, log and abort here. **/ \
            (void)fprintf(stderr,               \
                "Assertion: (%s) failed, %s() in %s:%d \n",     \
                #cond,__func__,__FILE__,__LINE__);      \
            exit(-11);                    \
        }                           \
    } while (0)

#define LOG_IF_NOT(cond)                     \
    do {                                \
        if (UNLIKELY(!(cond))) {             \
            gko_log(WARNING, "Assertion: (%s) failed, %s() in %s:%d : ",     \
                 #cond,__func__,__FILE__,__LINE__);      \
            /** In case a user-supplied handler tries to **/  \
            /** return control to us, log and abort here. **/ \
            (void)fprintf(stderr,               \
                "Assertion: (%s) failed, %s() in %s:%d ",     \
                #cond,__func__,__FILE__,__LINE__);                  \
        }                           \
    } while (0)


/// some struct define
#pragma pack (4)
typedef void * (*func_t)(void *, int);

/// file structure
typedef struct _s_file_t
{
    struct stat f_stat;
    mode_t mode;
    GKO_INT64 size; /// -1 for dir, -2 for symbol link
    char sympath[MAX_PATH_LEN];
    char name[MAX_PATH_LEN];
    u_char md5[16];
} s_file_t;

/// host structure
typedef struct _s_host_t
{
    char addr[IP_LEN];
    int port;
} s_host_t;

/// block structure
typedef struct _s_block_t
{
    GKO_INT64 size;
    GKO_INT64 start_off;
    GKO_INT64 start_f;
    u_char done;
    unsigned int digest;
    set<s_host_t> * host_set; ///only used by client, lock here
} s_block_t;

/// job structure
typedef struct _s_job_t
{
    int job_state;    /// pass job state to client
    int host_num;
    int lock_id;
    char uri[MAX_URI];
    char path[MAX_PATH_LEN]; ///localpath, serv part and clnt part are diff
    s_file_t * files;
    GKO_INT64 files_size;
    s_block_t * blocks;
    GKO_INT64 blocks_size;
    set<s_host_t> * host_set; ///lock here
    GKO_INT64 file_count;
    GKO_INT64 block_count;
    GKO_INT64 total_size;
    GKO_INT64 hash_progress[XOR_HASH_TNUM]; /// the hash progress in byte
    /** hash worker threads **/
    pthread_t hash_worker[XOR_HASH_TNUM];
    u_char * hash_buf[XOR_HASH_TNUM];
} s_job_t;

/// store host hash result
typedef struct _s_host_hash_result_t
{
    GKO_INT64 v_node[VNODE_NUM];
    GKO_INT64 length[VNODE_NUM];
} s_host_hash_result_t;

/// for seed TLS usage
typedef struct _s_seed_t
{
    GKO_INT64 file_count;
    GKO_INT64 init_s_file_t_iter;
    s_file_t * files;
    s_block_t * blocks;
    GKO_INT64 init_s_block_t_iter;
    GKO_INT64 total_size;
    GKO_INT64 tmp_size;
    GKO_INT64 last_init_block;
} s_seed_t;

/// job specified lock
typedef struct _s_lock_t
{
    char state;
    pthread_mutex_t lock;
} s_lock_t;

/// for passing write arg in libevent callback
typedef struct _s_write_arg_t
{
    int sent;
    int send_counter;
    int flag;
    int retry;
    int sz;
    char * p;
    struct event_base *ev_base;
    struct event ev_write;
} s_write_arg_t;

/// for passing sendfile arg in libevent callback
typedef struct _s_gsendfile_arg_t
{
    GKO_INT64 sent;
    GKO_UINT64 send_counter;
    int in_fd;
    int retry;
    off_t offset;
    GKO_UINT64 count;
    struct event_base *ev_base;
    struct event ev_write;
} s_gsendfile_arg_t;

/// vnode download params
typedef struct _s_vnode_download_arg_t
{
    s_job_t * jo;
    GKO_INT64 blk_idx;
    GKO_INT64 blk_count;
} s_vnode_download_arg_t;

/// async server params
typedef struct _s_async_server_arg_t
{
    s_host_t * s_host_p;
} s_async_server_arg_t;

/// for args processing
typedef struct _s_option_t
{
    char need_help;
    char to_continue;
    int port;
    int worker_thread;
    int connlimit;
    int max_dir_depth;
    int seed_time;
    /// up band width limit rate
    int limit_up_rate;
    /// down band width limit rate
    int limit_down_rate;
    /// make seed speed limit rate
    int limit_mk_seed_rate;
    in_addr_t bind_ip; /// this is usually 32bit
    char logpath[MAX_PATH_LEN];
    char listen_ip[MAX_HOST_NAME];
} s_option_t;

/// gingko global stuff
typedef struct _s_gingko_global_t
{
    FILE * log_fp;

    /// FUNC DICT
    char (* cmd_list_p)[8];
    ///server func list
    func_t * func_list_p;

    /// options switch
    s_option_t opt;

    ///  the host and server
    s_host_t the_clnt;
    s_host_t the_serv;

    /// ready_to_serv flog, when the upload thread
    volatile char ready_to_serv;

    /// save the NEWWed, DELEed host when server is not ready
    vector<s_host_t> hosts_new_noready;
    vector<s_host_t> hosts_del_noready;

    /// snap file path
    char snap_fpath[MAX_PATH_LEN];
    int snap_fd;

    /// signal flag
    volatile u_char sig_flag;
} s_gingko_global_t;

/// for snap file read and write
typedef struct _s_snap_t
{
    unsigned int digest;
    u_char done;
} s_snap_t;
#pragma pack ()


/************** FUNC DICT **************/
/// send HELO handler
int helo_serv_c(void *, int);
/// send JOIN handler
void * join_job_c(void *, int);
/// send QUIT handler
void * quit_job_c(void *, int);
/// send GETT handler
GKO_INT64 get_blocks_c(s_job_t * jo, s_host_t * dhost, GKO_INT64 num, GKO_INT64 count,
        u_char flag, char * buf);

/************** FUNC DECL **************/
/// send JOIN handler
void abort_handler(const int sig);
/// send JOIN handler
void set_sig(void(*int_handler)(int));
/// start a thread to watch the gko.sig_flag and take action
int sig_watcher(void * (*worker)(void *));
/// send a mem to fd(usually socket)
int sendall(int, const void *, int sz, int flag);
/// send a mem to fd(usually socket)
int sep_arg(char * inputstring, char * arg_array[], int max);
/// parse the request return the proper func handle num
int parse_req(char *req);
/// parse the request return the proper func handle num
struct hostent * gethostname_my(const char *host, struct hostent *hostbuf);
/// get host in_addr_t, this only works on IPv4. that is enough
int getaddr_my(const char *host, in_addr_t * addr_p);
/// check ulimit -n
int check_ulimit();
///// erase job related stuff
//int erase_job(string &uri_string);
/// hash the host to the data ring
s_host_hash_result_t * host_hash(s_job_t * jo, const s_host_t * new_host,
        s_host_hash_result_t * result, const u_char usage);
/// send blocks to the out_fd(usually socket)
int sendblocks(int out_fd, s_job_t * jo, GKO_INT64 start, GKO_INT64 num);
/// write block to disk
int writeblock(s_job_t * jo, const u_char * buf, s_block_t * blk);
/// Before send data all req to server will filtered by conn_send_data
void conn_send_data(int fd, void *str, unsigned int len);
/// send cmd msg to host, not read response, on succ return 0
int sendcmd(s_host_t *h, const char * cmd, int recv_sec, int send_sec);
/// try best to read specificed bytes from a file to buf
int readfileall(int fd, off_t offset, off_t count, char ** buf);
/// try best to read specificed bytes from a file to buf
int readall(int socket, void* data, int data_len, int flags);


/************** INLINE FUNC DECL **************/
/**
 * @brief calculate the sum of an given length array, return the sum
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
static inline GKO_INT64 array_sum(GKO_INT64 * a, GKO_INT64 count)
{
    GKO_INT64 sum = 0;
    while (count--)
    {
        sum += *(a + count);
    }
    return sum;
}

/**
 * @brief return index num of the next file in the s_file_t array
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
static inline GKO_INT64 next_f(s_job_t * jo, GKO_INT64 file)
{
    do
    {
        file = (1 + file) % (jo->file_count);
    } while (((jo->files) + file)->size <= 0);
    return file;
}

/**
 * @brief return index num of the next block in the s_block_t array
 *        max + 1 = 0
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
static inline GKO_INT64 next_b(s_job_t * jo, GKO_INT64 block)
{
    return (++block) % (jo->block_count);
}

/**
 * @brief return index num of the previous block in the s_block_t array
 *        0 - 1 = max
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
static inline GKO_INT64 prev_b(s_job_t * jo, GKO_INT64 block)
{
    return (block ? block - 1 : jo->block_count - 1);
}

/**
 * @brief calculate the s_host_t ip distance
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
static inline GKO_UINT64 host_distance(const s_host_t * h1, const s_host_t * h2)
{
    return ((((GKO_UINT64) ntohl(inet_addr(h1->addr))) << 16)
            + (GKO_UINT64) h1->port)
            ^ ((((GKO_UINT64) ntohl(inet_addr(h2->addr))) << 16)
                    + (GKO_UINT64) h2->port);
}

/**
 * @brief sendfile wrapper
 *
 * @see
 * @note
 * @author auxten <wangpengcheng01@baidu.com> <auxtenwpc@gmail.com>
 * @date 2011-8-1
 **/
static inline int gsendfile(int out_fd, int in_fd, off_t *offset,
        GKO_UINT64 *count)
{
#if defined (__APPLE__)
    int ret = sendfile(in_fd, out_fd, *offset, (off_t *) count, NULL, 0);
    if (ret == -1 && !ERR_RW_RETRIABLE(errno))
        return (-1);

    return (*count);
#elif defined (__FreeBSD__)
    int ret = sendfile(in_fd, out_fd, *offset, *count, NULL, (off_t *) count, 0);
    if (ret == -1 && !ERR_RW_RETRIABLE(errno))
    return (-1);

    return (*count);
#elif defined(__linux__)
    int ret = sendfile(out_fd, in_fd, offset, *count);
    if (ret == -1 && ERR_RW_RETRIABLE(errno))
    {
        /** if this is EAGAIN or EINTR return 0; otherwise, -1 **/
        return (0);
    }
    return (ret);
#endif
}

#endif /** GINGKO_H_ **/
