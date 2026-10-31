/* [tv5] Ghi lich su HPWL, xac dinh best case, xuat report.txt / placement_result.csv */
#include "report.h"

void history_init(history *h)
{
    /* TODO(tv5): count = 0, best_index = -1 */
    h->count = 0;
    h->best_index = -1;
}

void report_log_iter(history *h, int iteration, float hpwl)
{
    /* TODO(tv5): them record, cap nhat best_index neu hpwl nho nhat */
    (void)h; (void)iteration; (void)hpwl;
}

int report_find_best(const history *h)
{
    /* TODO(tv5): duyet records tim HPWL nho nhat */
    return h->best_index;
}

int report_write(const char *path, const history *h)
{
    /* TODO(tv5): ghi HPWL tung iteration + best case ra file */
    (void)h;
    fprintf(stderr, "report_write(%s): chua cai dat\n", path);
    return STATUS_ERR;
}

int report_write_placement(const char *path, const database *db)
{
    /* TODO(tv5): ghi ref,x,y,width,height cua tung linh kien */
    (void)db;
    fprintf(stderr, "report_write_placement(%s): chua cai dat\n", path);
    return STATUS_ERR;
}
