#ifndef FS_QUEUE_H
#define FS_QUEUE_H

#define SFS_QUEUE_KEY "simplefs_queue"

#define SFSQ_OK                 0
#define SFSQ_MSGGET_ERROR       -1
#define SFSQ_MSGRCV_ERROR       -2
#define SFSQ_MSGSND_ERROR       -3
#define SFSQ_MSGCREAT_ERROR     -4

/*
 * Message type for queue messages.
 */
struct sfs_msg {
    long fd;
};

/*
 * Blocks the execution of the process until mutex for a file with
 * with file descriptor fd is unlocked.
 * Returns SFSQ_OK if no errors occurred.
 */
int mutex_lock(int fd);

/*
 * Unblocks mutex access to the given file with file descriptor fd.
 * Returns SFSQ_OK if no errors occurred.
 */
int mutex_unlock(int fd);

/*
 * Initialises a message queue for simple fs management.
 * Returns SFSQ_OK if no errors occurred.
 */
int queue_init();

#endif //FS_QUEUE_H
