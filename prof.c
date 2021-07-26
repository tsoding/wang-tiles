#include <time.h>

#ifdef PROF
struct timespec tp_begin;
struct timespec tp_end;
const char *tp_label;

void begin_clock(const char *label)
{
    if (clock_gettime(CLOCK_MONOTONIC, &tp_begin) < 0) {
        fprintf(stderr, "ERROR: could not get current monotonic time: %s\n",
                strerror(errno));
        exit(1);
    }

    tp_label = label;
}

void end_clock(void)
{
    if (clock_gettime(CLOCK_MONOTONIC, &tp_end) < 0) {
        fprintf(stderr, "ERROR: could not get current monotonic time: %s\n",
                strerror(errno));
        exit(1);
    }

    double elapsed = (tp_end.tv_sec - tp_begin.tv_sec) + (tp_end.tv_nsec - tp_begin.tv_nsec) * 1e-9;
    fprintf(stderr, "Clock elapsed time %.9lf secs on %s\n", elapsed, tp_label);
}
#else
#define begin_clock(...)
#define end_clock(...)
#endif
