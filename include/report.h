#ifndef REPORT_H
#define REPORT_H

#include "database.h"

/* ===== Lich su HPWL qua tung iteration ===== */
typedef struct {
    int   iteration;         /* 0 = HPWL ban dau truoc khi toi uu */
    float hpwl;              /* tong HPWL tai iteration do */
} iter_record;

typedef struct {
    iter_record records[MAX_ITER + 1];
    int         count;       /* so record da ghi */
    int         best_index;  /* chi so record co HPWL nho nhat, -1 neu rong */
} history;

/* [tv5] */

/* Khoi tao history rong (count = 0, best_index = -1) */
void history_init(history *h);

/* Ghi them 1 record; tu cap nhat best_index. Het cho -> bo qua. */
void report_log_iter(history *h, int iteration, float hpwl);

/* Chi so record tot nhat (HPWL nho nhat), -1 neu rong */
int  report_find_best(const history *h);

/* Ghi output/report.txt: HPWL tung iteration + best case + % giam so voi ban dau */
int  report_write(const char *path, const history *h);

/* Ghi output/placement_result.csv: ref, x, y, width, height cua tung linh kien */
int  report_write_placement(const char *path, const database *db);

#endif
