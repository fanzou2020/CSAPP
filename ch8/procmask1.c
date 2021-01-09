/**
 * A shell program with a subtle synchronization error. If the child terminates before
 * the parent is able to run, then addjob and deletejob will be called in the wrong order
 * WARNING: This code is buggy! 
 * Output:
init jobs
add job 11991
add job 11992
add job 11993
add job 11994
add job 11995
add job 11996
Wed Jun  3 13:13:19 EDT 2020
Wed Jun  3 13:13:19 EDT 2020
Delete job 11992
Delete job 11991
Wed Jun  3 13:13:19 EDT 2020
Delete job 11993
Wed Jun  3 13:13:19 EDT 2020
Wed Jun  3 13:13:19 EDT 2020
Delete job 11994
Delete job 11995
Wed Jun  3 13:13:19 EDT 2020
Delete job 11996
 */
#include "csapp.h"

void deletejob(pid_t pid) {
    printf("Delete job %d\n",pid);
}

void initjobs() {
    printf("init jobs\n");
}

void addjob(pid_t pid) {
    printf("add job %d\n", pid);
}

void handler(int sig) {
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;

    Sigfillset(&mask_all);  // Adds every signal to set
    while ((pid = waitpid(-1, NULL, 0)) > 0) {  // Reap a zombie child
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);  // Add the signals in set to blocked
        deletejob(pid);  // Delete the child from the job list
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);  // Restore set
    }

    if (errno != ECHILD)
        Sio_error("waitpid error");
    
    errno = olderrno;
}

int main(int argc, char **argv) {
    int pid, i = 0;
    sigset_t mask_all, prev_all;

    Sigfillset(&mask_all);
    Signal(SIGCHLD, handler);  // Install signal handler
    initjobs();  // Initialize the job list

    while (i < 6) {
        if ((pid = Fork()) == 0) {  // Child process
            Execve("/bin/date", argv, NULL);
        }
        // Parent Process
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all); 
        addjob(pid);
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
        i++;
    }

    while (1);
    exit(0);
}
