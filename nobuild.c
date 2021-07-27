#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"

#define CFLAGS "-DPROF", "-O3", "-Wall", "-Wextra", "-Wswitch-enum", "-std=c11", "-pedantic", "-ggdb"
#define LIBS "-lm", "-lpthread"

const char *cc(void)
{
    const char *result = getenv("CC");
    return result ? result : "cc";
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

    if (argc > 1) {
        if (strcmp(argv[1], "run") == 0) {
            CMD(cc(), CFLAGS, "-o", "wang", "src/main.c", LIBS);
            CMD("./wang");
        } else if (strcmp(argv[1], "gdb") == 0) {
            CMD(cc(), CFLAGS, "-o", "wang", "src/main.c", LIBS);
            CMD("gdb", "./wang");
        } else {
            PANIC("Unknown subcommand %s", argv[1]);
        }
    } else {
        CMD(cc(), CFLAGS, "-o", "wang", "src/main.c", LIBS);
    }
    return 0;
}
