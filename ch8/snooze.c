#include "csapp.h"

void sigint_handler(int sig) {
    sleep(3);
    printf("Terminating after snooze for 3 seconds!\n");
    exit(0);
}

int main() {
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        unix_error("signal error");

    pause();
    return 0;
}