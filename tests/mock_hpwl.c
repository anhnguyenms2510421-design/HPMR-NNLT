/* MOCK cua [tv2] hpwl.c - chi dung de test hill_climb, KHONG nop vao src/.
   Cai dat don gian theo "HOP DONG CHUNG": chi tinh pad cua linh kien placed == 1. */
#include "hpwl.h"

void pad_abs_position(const database *db, int pad_idx, float *abs_x, float *abs_y)
{
    const pad_info *p = &db->pads[pad_idx];
    *abs_x = db->comps[p->comp_idx].x + p->dx;
    *abs_y = db->comps[p->comp_idx].y + p->dy;
}

int get_num_nets(const database *db)
{
    int max_id = -1;
    for (int i = 0; i < db->num_pads; i++)
        if (db->pads[i].net_id > max_id)
            max_id = db->pads[i].net_id;
    return max_id + 1;
}

float net_hpwl(const database *db, int net_id)
{
    float min_x = FLT_MAX, max_x = -FLT_MAX;
    float min_y = FLT_MAX, max_y = -FLT_MAX;
    int count = 0;

    for (int i = 0; i < db->num_pads; i++) {
        if (db->pads[i].net_id != net_id) continue;
        if (db->comps[db->pads[i].comp_idx].placed != 1) continue;
        float x, y;
        pad_abs_position(db, i, &x, &y);
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        count++;
    }
    if (count < 2) return 0.0f;
    return (max_x - min_x) + (max_y - min_y);
}

float total_hpwl(const database *db)
{
    float sum = 0.0f;
    int n = get_num_nets(db);
    for (int id = 0; id < n; id++)
        sum += net_hpwl(db, id);
    return sum;
}
