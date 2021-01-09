#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "csapp.h"

int main() {
    int a = 9;

    if (Fork() == 0) {
        printf("p1: a=%d\n", a--);
        exit(0);
    }
    printf("p2: a=%d\n", a++);
    exit(0);
}