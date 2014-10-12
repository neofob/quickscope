/* Quickscope - a software oscilloscope
 * Copyright (C) 2012-2014  Lance Arsenault
 * GNU General Public License version 3
 */
struct QsFd;
struct QsController;

extern
struct QsController *qsFd_create(int fD, size_t objectSize);
extern
struct QsController *qsFd_createFILE(FILE *file, size_t objectSize);
extern
void qsFd_destroy(struct QsFd *fD);

QS_BASE_DECLARE(qsFd);
