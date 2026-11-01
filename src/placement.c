/* [tv3] Dat linh kien len board (try_move, hill_climb nam trong hill_climb.c) */
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
