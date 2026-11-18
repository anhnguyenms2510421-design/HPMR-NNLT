/* [tv3] Design Rule Check */
#include "drc.h"

/* Sai so khi so sanh float. Float chi chinh xac ~1e-6 o toa do ~50, nen
   "cach dung CLEARANCE" (vd a o x=10, b o x=14.2, rong 4) co the ra
   gap = 0.1999998 < 0.2 va bi bao vi pham oan. Dung chung 1e-4 cho:
   cham bien board (hop dong) va cach dung CLEARANCE. */
#define DRC_EPS  1e-4f

int is_inside_board(const database *db, int comp_idx)
{
    const component *c;
    float hw, hh;

    /* Chi so sai -> coi la khong hop le (an toan hon la tra 1) */
    if (db == NULL || comp_idx < 0 || comp_idx >= db->num_comps) return 0;

    c  = &db->comps[comp_idx];
    hw = c->width  / 2.0f;      /* (x, y) la TAM nen moi canh = tam +- nua kich thuoc */
    hh = c->height / 2.0f;

    /* Viet dang "ca 4 canh deu dung" de gia tri NaN cung ra 0, khong lot qua */
    return (c->x - hw >= db->board.x_start - DRC_EPS &&
            c->x + hw <= db->board.x_end   + DRC_EPS &&
            c->y - hh >= db->board.y_start - DRC_EPS &&
            c->y + hh <= db->board.y_end   + DRC_EPS) ? 1 : 0;
}

/* LUU Y: is_overlap tra 1 = VI PHAM (chong lan / qua gan), 0 = on.
   NGUOC voi cac ham DRC khac (tra 1 = DAT). Khong xet co placed. */
int is_overlap(const database *db, int idx_a, int idx_b)
{
    const component *a, *b;
    float gap_x, gap_y;

    /* Chi so sai -> coi la vi pham (an toan hon la bao "on") */
    if (db == NULL ||
        idx_a < 0 || idx_a >= db->num_comps ||
        idx_b < 0 || idx_b >= db->num_comps) return 1;

    a = &db->comps[idx_a];
    b = &db->comps[idx_b];

    /* gap > 0: hai hop bao cach nhau tren truc do; gap < 0: chong lan */
    gap_x = fabsf(a->x - b->x) - (a->width  + b->width)  / 2.0f;
    gap_y = fabsf(a->y - b->y) - (a->height + b->height) / 2.0f;

    /* Chi vi pham khi CA HAI truc deu qua gan; chi can 1 truc cach >= CLEARANCE la du xa */
    return (gap_x < CLEARANCE - DRC_EPS && gap_y < CLEARANCE - DRC_EPS) ? 1 : 0;
}

int drc_check_component(const database *db, int comp_idx)
{
    int j;

    /* Kiem tra vi tri HIEN TAI, bat ke placed cua comp_idx: place_component()
       can thu vi tri truoc khi dat placed = 1. Gom ca truong hop chi so sai. */
    if (!is_inside_board(db, comp_idx)) return 0;

    for (j = 0; j < db->num_comps; j++) {
        if (j == comp_idx) continue;               /* khong so voi chinh no */
        if (db->comps[j].placed != 1) continue;    /* linh kien chua dat thi bo qua */
        if (is_overlap(db, comp_idx, j)) return 0;
    }
    return 1;
}

int drc_check_all(const database *db)
{
    int i;

    if (db == NULL) return 0;
    for (i = 0; i < db->num_comps; i++) {
        if (db->comps[i].placed != 1) continue;    /* chi xet linh kien da dat */
        if (!drc_check_component(db, i)) return 0;
    }
    return 1;
}
