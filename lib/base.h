// Butt ugly macros used to define and declare functions
// to "QS base class" that may be inherited.
// 

#define QS_BASE_DECLARE(prefix) \
/* To be called in inheriting _destroy() function */ \
extern \
void prefix ## _checkBaseDestroy(void *x); \
/* To be called in inheriting _create() function */ \
/* type of second arg is like void (*destroy)(void *) */ \
/* void *destroy just makes code shorter. */ \
extern \
void prefix ## _addSubDestroy(void *x, void *destroy)


#define QS_BASE_DEFINE_FULL(prefix, type, dprefix) \
/* To be called at end of inheriting _destroy() function */ \
void prefix ## _checkBaseDestroy(void *x) \
{ \
  type *s; \
  s = x; \
  QS_ASSERT(s); \
  if(s->destroy) \
  { \
    /* We are not in dprefix_destroy() */ \
    s->destroy = NULL; \
    dprefix ## _destroy(s); \
  } \
  /* printf("end of %s()\n", __func__); */ \
} \
 \
/* To be called in inheriting _create() function */ \
/* type of second arg is like void (*destroy)(void *) */ \
/* void *destroy just makes code shorter. */ \
void prefix ## _addSubDestroy(void *x, void *destroy) \
{ \
  type *s; \
  s = x; \
  QS_ASSERT(s); \
  s->destroy = destroy; \
}


// The case where the destroy function is public
// so it has a destroy function like prefix_destroy
// and a create function like prefix_create()
#define QS_BASE_DEFINE(prefix, type) QS_BASE_DEFINE_FULL(prefix, type, prefix)



#define QS_BASE_CHECKSUBDESTROY(s) \
/* Call at the start of base _destroy() */ \
do { \
  QS_ASSERT((s)); \
  /* printf("start of %s()\n", __func__); */ \
 \
  if((s)->destroy) \
  { \
    void (*destroy)(void *); \
    destroy = (s)->destroy; \
    (s)->destroy = NULL; \
    /* destroy inheriting (sub) object first */ \
    destroy((s)); \
  } \
} while(0)
