struct QsController;
struct QsSource;

extern
void qsController_destroy(struct QsController *c);
extern
void qsController_appendSource(struct QsController *c, struct QsSource *s);
extern
void qsController_prependSource(struct QsController *c, struct QsSource *s);
extern
void qsController_removeSource(struct QsController *c, struct QsSource *s);

struct QsController;

extern
struct QsController *qsInterval_create(float period /* seconds */);
// Destroy with qsController_destroy();
struct QsWin;

extern
struct QsController *qsDrawSync_create(struct QsWin *win);

extern
struct QsController *qsIdle_create(void);

// This fd this sources that use a blocking read on an OS file
// descriptor calling the source read until you empty the OS buffers
// or you stop reading, which multiplexes with something like
// select() and/or epoll().
extern
struct QsController *qsFd_create(int fD, size_t objectSize);
extern
struct QsController *qsFd_createFILE(FILE *file, size_t objectSize);

QS_BASE_DECLARE(qsFd);
