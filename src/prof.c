// TODO: document how to use the profiler module @doc
#include <assert.h>
#include <time.h>
#include <stddef.h>

#ifdef PROF
typedef struct {
    const char *label;
    double elapsed;
    size_t size;
} Entry;

typedef struct {
    struct timespec begin;
    Entry *entry;
} Clock;

// TODO: several active contexts

#define CLOCK_STACK_CAP 256
Clock clock_stack[CLOCK_STACK_CAP];
size_t clock_stack_count = 0;

#define SUMMARY_CAP 1024
Entry summary[SUMMARY_CAP];
size_t summary_count = 0;

// TODO: Profiler: several passes over the same path and compute the average elapsed time
// It would be interesting to explore how to do that. If you do several passes over the
// same path without cleaning the summary you will just pile more entries to the summary.
// We need a way to "rewind" the summary_count so the pass hits the same entries again and
// computes the average time on their elapsed values.
//
// This would be also super useful when we integrate our renderer with a display window. This
// will enable us to display the average renderer performance in real time.

void begin_clock(const char *label)
{
    assert(clock_stack_count < CLOCK_STACK_CAP);
    assert(summary_count < SUMMARY_CAP);

    Entry *e = &summary[summary_count++];
    e->label = label;
    e->size = 1;
    e->elapsed = 0.0;

    Clock *c = &clock_stack[clock_stack_count++];

    if (clock_gettime(CLOCK_MONOTONIC, &c->begin) < 0) {
        fprintf(stderr, "ERROR: could not get current monotonic time: %s\n",
                strerror(errno));
        exit(1);
    }

    c->entry = e;
}

void end_clock(void)
{
    assert(clock_stack_count > 0);

    Clock *c = &clock_stack[--clock_stack_count];
    struct timespec end;

    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        fprintf(stderr, "ERROR: could not get current monotonic time: %s\n",
                strerror(errno));
        exit(1);
    }

    c->entry->elapsed = (end.tv_sec - c->begin.tv_sec) + (end.tv_nsec - c->begin.tv_nsec) * 1e-9;

    if (clock_stack_count > 0) {
        Clock *pc = &clock_stack[clock_stack_count - 1];
        pc->entry->size += c->entry->size;
    }
}

void render_entry(FILE *stream, ptrdiff_t root, size_t level, size_t line_width)
{
    fprintf(stream, "%*s%-*s%.9lf secs\n",
            (int) level * 2, "",
            (int) line_width - (int) level * 2, summary[root].label,
            summary[root].elapsed);

    size_t size = summary[root].size - 1;
    ptrdiff_t child = root + 1;

    while (size > 0) {
        render_entry(stream, child, level + 1, line_width);
        size -= summary[child].size;
        child += summary[child].size;
    }
}

void render_summary(FILE *stream, size_t line_width)
{
    ptrdiff_t root = 0;

    while ((size_t) root < summary_count) {
        render_entry(stream, root, 0, line_width);
        root += summary[root].size;
    }
}

size_t estimate_entry_line_width(ptrdiff_t root, size_t level)
{
    size_t line_width = 2 * level + strlen(summary[root].label);

    size_t size = summary[root].size - 1;
    ptrdiff_t child = root + 1;

    while (size > 0) {
        size_t entry_line_width = estimate_entry_line_width( child, level + 1);
        if (entry_line_width > line_width) {
            line_width = entry_line_width;
        }
        size -= summary[child].size;
        child += summary[child].size;
    }

    return line_width;
}

size_t estimate_line_width(void)
{
    size_t line_width = 0;
    ptrdiff_t root = 0;
    while ((size_t) root < summary_count) {
        size_t entry_line_width = estimate_entry_line_width(root, 0);
        if (entry_line_width > line_width) {
            line_width = entry_line_width;
        }
        root += summary[root].size;
    }

    return line_width;
}

void clear_summary()
{
    clock_stack_count = 0;
    summary_count = 0;
}

void dump_summary(FILE *stream)
{
    size_t line_width = estimate_line_width();
    render_summary(stream, line_width + 2);
    clear_summary();
}

#else
#define begin_clock(...)
#define end_clock(...)
#define dump_summary(...)
#define clear_summary(...)
#endif
