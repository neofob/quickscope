struct QsFd;
struct QsController;

// This fd this sources that use a blocking read on an OS file
// descriptor calling the source read until you empty the OS buffers
// or you stop reading, which multiplexes with something like
// select() and/or epoll().
extern
struct QsController *qsFd_create(int fD, size_t objectSize);
extern
struct QsController *qsFd_createFILE(FILE *file, size_t objectSize);
extern
void qsFd_destroy(struct QsFd *fd);

QS_BASE_DECLARE(qsFd);
