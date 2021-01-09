/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

int isnumber(char *num);
void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void Sigfillset(sigset_t *set);
void Sigemptyset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
pid_t Fork(void);
void Setpgid(pid_t pid, pid_t pgid);
void Execve(const char *filename, char *const argv[], char *const envp[]);
pid_t Waitpid(pid_t pid, int *iptr, int options);
void Kill(pid_t pid, int signum);
// ssize_t sio_puts(char s[]); /* Put string */
// ssize_t sio_putl(long v);    /* Put long */
// void sio_error(char s[]);     /* Put error message and exit */

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char *argv[MAXARGS];  /* Argument list execve() */
    char buf[MAXLINE];    /* Holds modified command line */
    int bg;               /* Should the job run in bg of fg? */
    pid_t pid;            /* Process id */

    /* Initialize signal masks. */
    sigset_t mask_all, mask_one, prev_one;
    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  /* Parse command line */

    if (argv[0] == NULL) {
        return;
    }

    /* Not a builtin command, run job in child process */
    if (!builtin_cmd(argv)) {
        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);  /* Block SIGCHLD to avoid Race */

        if ((pid = Fork()) == 0) { /* Child process */
            Setpgid(0, 0);  /* Put the child in new process group whose pgid is identical to child's pid*/

            Sigprocmask(SIG_SETMASK, &prev_one, NULL);  /* Unblock SIGCHLD */

            Execve(argv[0], argv, environ);
        }

        /* Parent process */
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);  /* Mask all signals */

        addjob(jobs, pid, bg+1, cmdline);  /* Add job into joblist, bg+1 indicates the state of job */

        if (bg) {
            // listjobs(jobs);
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }

        Sigprocmask(SIG_SETMASK, &prev_one, NULL); /* Restore previous sigset */

        /* If is foreground job, wait for SIGCHLD to be received */
        if (!bg) {
            // listjobs(jobs);
            waitfg(pid);
        } 
    } 

    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    const char * cmd = argv[0];

    if (!strcmp(cmd, "quit")) {
        exit(0);
        return 1;
    }

    if (!strcmp(cmd, "jobs")) {
        listjobs(jobs);
        fflush(stdout);
        return 1;
    }

    if (!strcmp(cmd, "bg")) {
        do_bgfg(argv);
        return 1;
    }

    if (!strcmp(cmd, "fg")) {
        do_bgfg(argv);
        return 1;
    }

    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    struct job_t *job;    
    int isbg = !strcmp(argv[0], "bg");
    sigset_t mask_sigchld, prev_one;


    /* Some preparations */
    if (argv[1] == NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    int isjid = ('%' == argv[1][0]);

    if (isjid) {
        char *jid = &argv[1][1];
        if (!isnumber(jid)) {
            printf("%s: argument must be a PID or %%jobid\n", argv[0]);
            return;
        }

        job = getjobjid(jobs, atoi(jid));
        if (job == NULL) {
            printf("no such job\n");
            return;
        }
    }
    else {
        if (!isnumber(argv[1])) {
            printf("%s: argument must be a PID or %%jobid\n", argv[0]);
            return;
        }
        
        job = getjobpid(jobs, atoi(argv[1]));
        if (job == NULL) {
            printf("no such process\n");
            return;
        }
    }

    if (isbg && job->state != ST) {
        printf("bg error - Job is not STOPPED.\n");
        return;
    } 
    if (!isbg && !(job->state == ST || job->state == BG)) {
        printf("fg error- Job is not STOPPED or in BACKGROUND.\n");
        return;
    }

    /* Block SIGCHLD, modify job status */
    Sigfillset(&mask_sigchld);
    Sigprocmask(SIG_BLOCK, &mask_sigchld, &prev_one);

    Kill(-(job->pid), SIGCONT);  /* Sen SIGCONT to process */

    if (isbg) {
        listjobs(job);
        job->state = BG;
    } else {
        job->state = FG;
    }

    Sigprocmask(SIG_SETMASK, &prev_one, NULL);  /* Restore mask */

    if (!isbg) {
        waitfg(job->pid);
    }


    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    /* Follow the recommendation in handout, use a busy loop around the sleep function in waitfg,
    and do the jobs of reaping children in signal handler */
    while (1) {
        sleep(1);
        if (fgpid(jobs) != pid)  /* Exit loop when pid is not the foreground job. */
            break;
    }

    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    int child_status;
    pid_t pid;

    Sigfillset(&mask_all);

    /* Reap all children */
    while ((pid = waitpid(-1, &child_status, WNOHANG|WUNTRACED)) > 0) {
        
        /* Normally exit */
        if (WIFEXITED(child_status)) {
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);  /* Block all signals */
            deletejob(jobs, pid);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);  /* Restore */
        }

        /* SIGSTP, do not delete job */
        if (WIFSTOPPED(child_status)) {
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(child_status));
        }

        /* Terminated by signals that wat not caught, for example, SIGINT (ctrl-C) */
        if (WIFSIGNALED(child_status)) {
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(child_status));
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);  /* Block all signals */
            deletejob(jobs, pid);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);  /* Restore */
        }
    }

    /* errno can be 4, interrupted, so comment this out.
    if (errno != ECHILD) {
        printf("waitpid error, errono = %d", errno);
    }
    */

    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int olderrno = errno;

    pid_t fg_pid = fgpid(jobs);  /* Get foreground pid */

    if (fg_pid == 0) return;

    Kill(-fg_pid, SIGINT);  /* Send SIGINT to foreground process, the default action is Terminate */

    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    Sigfillset(&mask_all); 

    pid_t fg_pid = fgpid(jobs);  /* Get foreground pid */

    if (fg_pid == 0) return;

    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    Kill(-fg_pid, SIGTSTP); /* Send SIGTSTP to foreground process group */

    /* Update Job status to stopped in jobs list */
    struct job_t * fg_job= getjobpid(jobs, fg_pid);
    fg_job->state = ST;
    Sigprocmask(SIG_SETMASK, &prev_all, NULL);

    errno = olderrno;
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/* return 1 if isnumber, otherwise return 0 */
int isnumber(char *num) 
{
    int i = 0;
    while (num[i] != '\0') {
        if (!isdigit(num[i])) {
            return 0;
        }
        i++;
    }
    return 1;
}



/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}

/*
 * Sigfillset - wrapper function for sigfillset
 */
void Sigfillset(sigset_t *set) {
    if (sigfillset(set) < 0) unix_error("Sigfillset error");
    return;
}

/* 
 * Sigemptyset - wapper function for sigemptyset
 */
void Sigemptyset(sigset_t *set) {
    if (sigemptyset(set) < 0) unix_error("Sigemptyset error");
    return;
}

/*
 * Sigaddset - wrapper function for sigaddset
 */
void Sigaddset(sigset_t *set, int signum) {
    if (sigaddset(set, signum) < 0) unix_error("Sigaddset error");
    return;
}

/*
 * Sigprocmask - wrapper function for sigprocmask
 */
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    if (sigprocmask(how, set, oldset) < 0) unix_error("Sigprocmask error");
    return;
}

/*
 * Fork - wrapper function for fork
 */
pid_t Fork(void) {
    pid_t pid;

    if ((pid = fork()) < 0) unix_error("Fork error");
    return pid;
}

/*
 * Setpid - wrapper function for setpid
 */
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0) unix_error("Setpgid error");
    return;
}

void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}

pid_t Waitpid(pid_t pid, int *iptr, int options)
{
    pid_t retpid;

    if ((retpid = waitpid(pid, iptr, options)) < 0)
        unix_error("Waitpid error");
    return (retpid);
}

void Kill(pid_t pid, int signum)
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
        unix_error("Kill error");
}

// /* Public Sio functions */
// /* $begin siopublic */

// ssize_t sio_puts(char s[]) /* Put string */
// {
//     return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
// }

// ssize_t sio_putl(long v) /* Put long */
// {
//     char s[128];

//     sio_ltoa(v, s, 10); /* Based on K&R itoa() */ //line:csapp:sioltoa
//     return sio_puts(s);
// }

// void sio_error(char s[]) /* Put error message and exit */
// {
//     sio_puts(s);
//     _exit(1); //line:csapp:sioexit
// }
// /* $end siopublic */