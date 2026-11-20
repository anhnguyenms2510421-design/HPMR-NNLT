/* MOCK cua [tv5] report.c - chi cac ham history, du cho test hill_climb.
   Khong cai report_write*: test khong goi toi. KHONG nop vao src/. */
#include "report.h"

void history_init(history *h)
{
    h->count = 0;
    h->best_index = -1;
}

void report_log_iter(history *h, int iteration, float hpwl)
{
    if (h->count > MAX_ITER) return;            /* het cho -> bo qua */
    h->records[h->count].iteration = iteration;
    h->records[h->count].hpwl = hpwl;
    /* hpwl < best (nghiem ngat) -> hoa giu iteration som hon */
    if (h->best_index < 0 || hpwl < h->records[h->best_index].hpwl)
        h->best_index = h->count;
    h->count++;
}

int report_find_best(const history *h)
{
    return h->best_index;
}
