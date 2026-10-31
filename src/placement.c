/* [tv3] Dat linh kien + thuat toan Hill Climbing */
#include "placement.h"
#include "hpwl.h"
#include "drc.h"

int place_initial(database *db)
{
    /* TODO(tv3): rand() chon 1 linh kien, thu vi tri tren board den khi drc dat */
    (void)db;
    return STATUS_ERR;
}

int place_component(database *db, int comp_idx)
{
    /* TODO(tv3): quet cac vi tri hop le (drc), giu vi tri co total_hpwl nho nhat */
    (void)db; (void)comp_idx;
    return STATUS_ERR;
}

int place_all(database *db)
{
    /* TODO(tv3): place_initial(); roi place_component() cho cac linh kien placed == 0 */
    (void)db;
    return STATUS_ERR;
}

int try_move(database *db, int comp_idx, float dx, float dy)
{
    /* TODO(tv3): dich -> drc_check_component() va total_hpwl() giam ? giu : hoan tac */
    (void)db; (void)comp_idx; (void)dx; (void)dy;
    return 0;
}

float hill_climb(database *db, history *hist)
{
    /* TODO(tv3): report_log_iter(hist, 0, hpwl ban dau); lap try_move() den khi
       het cai thien hoac du MAX_ITER, moi vong goi report_log_iter() */
    (void)hist;
    return total_hpwl(db);
}
