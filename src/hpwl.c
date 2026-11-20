/* [tv2] Tinh HPWL (Half-Perimeter Wire Length) */
#include "hpwl.h"

void pad_abs_position(const database *db, int pad_idx, float *abs_x, float *abs_y)
{
    const pad_info *pad = &db->pads[pad_idx];
    const component *comp = &db->comps[pad->comp_idx];

    *abs_x = comp->x + pad->dx;
    *abs_y = comp->y + pad->dy;
}

int get_num_nets(const database *db)
{
    int max_net_id = -1;
    int i;

    /* Tinh ca pad chua dat de giu nguyen mien net_id. */
    for (i = 0; i < db->num_pads; i++) {
        if (db->pads[i].net_id > max_net_id)
            max_net_id = db->pads[i].net_id;
    }
    return max_net_id + 1;
}

float net_hpwl(const database *db, int net_id)
{
    float min_x = FLT_MAX, min_y = FLT_MAX;
    float max_x = -FLT_MAX, max_y = -FLT_MAX;
    int valid_pads = 0;
    int i;

    for (i = 0; i < db->num_pads; i++) {
        const pad_info *pad = &db->pads[i];
        float x, y;

        if (pad->net_id != net_id ||
            db->comps[pad->comp_idx].placed != 1)
            continue;

        pad_abs_position(db, i, &x, &y);
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        valid_pads++;
    }

    /* Chua du hai pad thi chua co do dai day noi. */
    if (valid_pads < 2)
        return 0.0f;
    return (max_x - min_x) + (max_y - min_y);
}

float total_hpwl(const database *db)
{
    int num_nets = get_num_nets(db);
    int net_id;
    float total = 0.0f;

    for (net_id = 0; net_id < num_nets; net_id++)
        total += net_hpwl(db, net_id);
    return total;
}
