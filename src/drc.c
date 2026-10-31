/* [tv4] Design Rule Check */
#include "drc.h"

int is_inside_board(const database *db, int comp_idx)
{
    /* TODO(tv4): x +- width/2, y +- height/2 nam trong [x_start,x_end] x [y_start,y_end] */
    (void)db; (void)comp_idx;
    return 1;
}

int is_overlap(const database *db, int idx_a, int idx_b)
{
    /* TODO(tv4): 2 hop bao chong lan hoac cach nhau < CLEARANCE */
    (void)db; (void)idx_a; (void)idx_b;
    return 0;
}

int drc_check_component(const database *db, int comp_idx)
{
    /* TODO(tv4): is_inside_board() && khong is_overlap() voi linh kien placed khac */
    (void)db; (void)comp_idx;
    return 1;
}

int drc_check_all(const database *db)
{
    /* TODO(tv4): drc_check_component() cho moi linh kien placed */
    (void)db;
    return 1;
}
