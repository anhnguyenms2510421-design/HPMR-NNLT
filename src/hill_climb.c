/* [tv4] Hill Climbing: thu dich linh kien, giu nuoc di neu HPWL giam va DRC dat */
#include "placement.h"
#include "hpwl.h"
#include "drc.h"

int try_move(database *db, int comp_idx, float dx, float dy)
{
    /* TODO(tv4): dich -> drc_check_component() va total_hpwl() giam ? giu : hoan tac */
    (void)db; (void)comp_idx; (void)dx; (void)dy;
    return 0;
}

float hill_climb(database *db, history *hist)
{
    /* TODO(tv4): report_log_iter(hist, 0, hpwl ban dau); lap try_move() den khi
       het cai thien hoac du MAX_ITER, moi vong goi report_log_iter() */
    (void)hist;
    return total_hpwl(db);
}
