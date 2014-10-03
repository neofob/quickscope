/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */

struct QsAdjusterList;


struct QsWidget;

/* Base Class for parameter changing thingy */
struct QsAdjuster
{
  char *description;
  void (*changeValueCallback)(void *data); // user callback
  void *data;

  // inheriting objects set these methods or not
  void (*destroy)(void *obj);
  void (*getTextRender)(void *obj, char *str, size_t maxLen, size_t *len);
  gboolean (*shiftLeft)(void *obj);
  gboolean (*shiftRight)(void *obj);
  gboolean (*inc)(void *obj, struct QsWidget *w);
  gboolean (*dec)(void *obj, struct QsWidget *w);
  
  // For making adjuster groups
  // moves QsWidget to different next in the QsAdjusterList
  GList *(*next)(void *obj, struct QsAdjusterList *);
  // moves QsWidget to different prev in the QsAdjusterList
  GList *(*prev)(void *obj, struct QsAdjusterList *);

  // reset the object after setting parameter value not using this object
  gboolean (*reset)(void *obj);
  // icon is a callback to return pre description text, so it may change
  // between calls.  Returns the length of the string.
  size_t (*icon)(char *buf, size_t len, void *iconData);
  void *iconData;

  // Keeps a list of QsAdjusterList(s) that it is in
  GSList *adjs;
  //struct QsAdjusterList *adjs;
  
#ifdef QS_DEBUG
  size_t objSize;
#endif
};

/* widget that can display/feed any adjuster in the QsAdjusterList */
struct QsWidget
{
  // The adjusterList that is managing this
  struct QsAdjusterList *adjs;
  /* current adjuster that this widget is displaying/feeding */
  GList *current;
  int adjCount; // how far into the adjusterList starting at 1

  void (*displayCallback)(struct QsWidget *w,
        const struct QsAdjuster *adj, const char *text, void *data);
  void *displayCallbackData;
#ifdef QS_DEBUG
  size_t objSize;
#endif
};


#define INACTIVE_BG_COLOR  "bgcolor=\"#B0B9F9\""
#define ACTIVE_BG_COLOR    "bgcolor=\"#3CC3D4\""



extern
void *_qsAdjusterList_create(size_t objSize);
extern // frees adjuster(s) and widget
void _qsAdjusterList_destroy(struct QsAdjusterList *adjs);
extern // display all widgets
void _qsAdjusterList_display(struct QsAdjusterList *adjs);

extern
void *_qsWidget_create(struct QsAdjusterList *adjs,
    void (*displayCallback)(struct QsWidget *w,
      const struct QsAdjuster *adj, const char *text, void *data),
    void *displayCallbackData, size_t objSize);
extern // push the display callback to be called
       // with the current adjuster
void _qsWidget_display(struct QsWidget *w);
extern
void _qsWidget_destroy(struct QsWidget *w);
extern
void _qsWidget_next(struct QsWidget *w);
extern
void _qsWidget_prev(struct QsWidget *w);
extern
void _qsWidget_shiftRight(struct QsWidget *w);
extern
void _qsWidget_shiftLeft(struct QsWidget *w);
extern
void _qsWidget_inc(struct QsWidget *w);
extern
void _qsWidget_dec(struct QsWidget *w);
extern
void *_qsAdjuster_create(struct QsAdjusterList *adjs,
    const char *description,
    void (*callback)(void *data), void *data,
    size_t objSize);
extern
void _qsAdjuster_destroy(struct QsAdjuster *adj);
extern
void _qsAdjusterList_append(struct QsAdjusterList *adjL,
    struct QsAdjusterList *appendL);
extern
void _qsAdjusterList_prepend(struct QsAdjusterList *adjL,
    struct QsAdjusterList *prependL);
extern
void _qsAdjusterList_remove(struct QsAdjusterList *adjL,
    struct QsAdjusterList *removedL);

