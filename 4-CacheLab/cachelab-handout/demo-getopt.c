/*
 * demo-getopt.c - learning the usage of getopt
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

/* type "./demo-getopt -y -x 1" in CLI */
int main(int argc, char** argv) {
    // printf("argc = %d\n", argc);
    // for (int i = 0; i < argc; i++) {
    //     printf("%s\n", argv[i]);
    // }

    /* "vf::o:" means argumentless v, an optional-argument f, and a mandatory-argument o */
    int opt, x, y;
    while ((opt = getopt(argc, argv, "yx:")) != -1) {
        switch (opt) {
            case 'x':
                x = atoi(optarg);
                printf("x = %d\n", x);
                break;
            case 'y':
                printf("y = %s\n", y);
                break;
            default:
                printf("wrong argument\n");
                break;
        }
    }
}