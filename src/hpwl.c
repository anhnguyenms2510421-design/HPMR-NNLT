/* [tv2] Tinh HPWL (Half-Perimeter Wire Length) */
#include "hpwl.h"

void pad_abs_position(const database *db, int pad_idx, float *abs_x, float *abs_y)
{
    /* TODO(tv2): abs = comps[pads[pad_idx].comp_idx].(x,y) + pads[pad_idx].(dx,dy) */
    (void)db; (void)pad_idx;
    *abs_x = 0.0f;
    *abs_y = 0.0f;
}

int get_num_nets(const database *db)
{
    /* TODO(tv2): net_id lon nhat + 1 */
    (void)db;
    return 0;
}

float net_hpwl(const database *db, int net_id)
{
    /* TODO(tv2): min/max x,y cua cac pad (linh kien placed) thuoc net_id */
    (void)db; (void)net_id;
    return 0.0f;
}

float total_hpwl(const database *db)
{
    /* TODO(tv2): cong net_hpwl() cho net_id = 0 .. get_num_nets()-1 */
    (void)db;
    return 0.0f;
}
