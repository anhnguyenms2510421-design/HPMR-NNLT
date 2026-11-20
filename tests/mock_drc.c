/* MOCK cua [tv3] drc.c - chi dung de test hill_climb, KHONG nop vao src/. */
#include "drc.h"

/* PROBE (chi co o mock): khi mock_drc_probe_on = 1, ghi lai (chi so, x, y) cua moi lan
   drc_check_component duoc goi. try_move luon goi DRC dung 1 lan cho moi nuoc di,
   nen day chinh la chuoi nuoc di hill_climb da thu -> test kiem tra duoc thu tu 4 huong. */
#define PROBE_MAX 64
int   mock_drc_probe_on = 0;
int   mock_drc_probe_n = 0;
int   mock_drc_probe_idx[PROBE_MAX];
float mock_drc_probe_x[PROBE_MAX];
float mock_drc_probe_y[PROBE_MAX];

int is_inside_board(const database *db, int comp_idx)
{
    const component *c = &db->comps[comp_idx];
    const float eps = 1e-4f;
    return (c->x - c->width  / 2 >= db->board.x_start - eps) &&
           (c->x + c->width  / 2 <= db->board.x_end   + eps) &&
           (c->y - c->height / 2 >= db->board.y_start - eps) &&
           (c->y + c->height / 2 <= db->board.y_end   + eps);
}

int is_overlap(const database *db, int idx_a, int idx_b)
{
    const component *a = &db->comps[idx_a];
    const component *b = &db->comps[idx_b];
    float gap_x = fabsf(a->x - b->x) - (a->width  + b->width)  / 2;
    float gap_y = fabsf(a->y - b->y) - (a->height + b->height) / 2;
    return (gap_x < CLEARANCE && gap_y < CLEARANCE);
}

int drc_check_component(const database *db, int comp_idx)
{
    if (mock_drc_probe_on && mock_drc_probe_n < PROBE_MAX) {
        mock_drc_probe_idx[mock_drc_probe_n] = comp_idx;
        mock_drc_probe_x[mock_drc_probe_n]   = db->comps[comp_idx].x;
        mock_drc_probe_y[mock_drc_probe_n]   = db->comps[comp_idx].y;
        mock_drc_probe_n++;
    }
    if (!is_inside_board(db, comp_idx)) return 0;
    for (int j = 0; j < db->num_comps; j++) {
        if (j == comp_idx || db->comps[j].placed != 1) continue;
        if (is_overlap(db, comp_idx, j)) return 0;
    }
    return 1;
}

int drc_check_all(const database *db)
{
    for (int i = 0; i < db->num_comps; i++)
        if (db->comps[i].placed == 1 && !drc_check_component(db, i))
            return 0;
    return 1;
}
