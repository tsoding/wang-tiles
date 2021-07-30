#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"

// TODO: port to Windows
// Should use WinAPI instead of pthread and X11
// Maybe we should first factor out the platform layer
// so it's easier to add more platforms.

// TODO: port to MacOS
// I have no idea what MacOS uses for threading and simple graphics

#define CFLAGS "-DPROF", "-O0", "-Wall", "-Wextra", "-Wswitch-enum", "-std=c11", "-pedantic", "-ggdb"
#define LIBS "-lm", "-lpthread", "-lX11"

const char *cc(void)
{
    const char *result = getenv("CC");
    return result ? result : "cc";
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

    // TODO: nobuild subcommand to regenerate ./imgs/*
    // Requires CLI params
    // - [ ] -tile <width>x<height>
    // - [ ] -grid <width>x<height>
    // - [ ] -shader <blobs|digits>

    if (argc > 1) {
        if (strcmp(argv[1], "run") == 0) {
            CMD(cc(), CFLAGS, "-o", "wang", "src/main.c", LIBS);

            Cstr_Array line = cstr_array_make("./wang", NULL);
            for (int i = 2; i < argc; ++i) {
                line = cstr_array_append(line, argv[i]);
            }
            Cmd cmd = {.line = line};
            INFO("CMD: %s", cmd_show(cmd));
            cmd_run_sync(cmd);
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
