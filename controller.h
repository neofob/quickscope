struct QsController;
struct QsSource;

extern
void qsController_destroy(struct QsController *c);
extern
void qsController_appendSource(struct QsController *c,
    struct QsSource *s, void *callbackData);
extern
void qsController_prependSource(struct QsController *c,
    struct QsSource *s, void *sourceCallbackData);
extern
void qsController_removeSource(struct QsController *c, struct QsSource *s);

