#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"

#define CFLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-std=c11", "-pedantic", "-ggdb"
#define LIBS "-lm"

const char *cc(void)
{
    const char *result = getenv("CC");
    return result ? result : "cc";
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);
    CMD(cc(), CFLAGS, "-o", "wang", "main.c", LIBS);

    if (argc > 1) {
        if (strcmp(argv[1], "run") == 0) {
            CMD("./wang");
        } else {
            PANIC("Unknown subcommand %s", argv[1]);
        }
    }
    return 0;
}
