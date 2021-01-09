/**
 * Using sigprocmask to synchronize processes. In this example, the parent ensures that
 * addjob executes before the corresponding deletejob.
 * Output:
init jobs
add job 11945
add job 11946
add job 11947
add job 11948
add job 11949
add job 11950
Wed Jun  3 13:12:41 EDT 2020
Wed Jun  3 13:12:41 EDT 2020
Wed Jun  3 13:12:41 EDT 2020
Wed Jun  3 13:12:41 EDT 2020
Wed Jun  3 13:12:41 EDT 2020
Wed Jun  3 13:12:41 EDT 2020
Delete job 11950
Delete job 11949
Delete job 11948
Delete job 11947
Delete job 11946
Delete job 11945
 */
#include "csapp.h"

void deletejob(pid_t pid)
{
    printf("Delete job %d\n", pid);
}

void initjobs()
{
    printf("init jobs\n");
}

void addjob(pid_t pid)
{
    printf("add job %d\n", pid);
}

void handler(int sig)
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;

    Sigfillset(&mask_all); // Adds every signal to set
    while ((pid = waitpid(-1, NULL, 0)) > 0)
    {                                                 // Reap a zombie child
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // Add the signals in set to blocked
        deletejob(pid);                               // Delete the child from the job list
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);    // Restore set
    }

    if (errno != ECHILD)
        Sio_error("waitpid error");

    errno = olderrno;
}

int main(int argc, char **argv)
{
    int pid, i = 0;
    sigset_t mask_all, mask_one, prev_one;

    Sigfillset(&mask_all);  // Add every signal to mask_all
    Sigemptyset(&mask_one);  // create empty set mask_one
    Sigaddset(&mask_one, SIGCHLD);  // add SIGCHLD to mask_one
    Signal(SIGCHLD, handler); // Install signal handler
    initjobs();               // Initialize the job list

    while (i < 6)
    {
        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);  // Block SIGCHLD
        if ((pid = Fork()) == 0)
        { // Child process
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);  // Unblock SIGCHLD
            Execve("/bin/date", argv, NULL);
        }
        // Parent Process
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);
        addjob(pid);  // Add the child to the job list
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);  // Unblock SIGCHLD
        i++;
    }

    while (1)
        ;

    exit(0);
}
