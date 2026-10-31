/* [tv1] Doc board.csv, components.csv, pad.csv va luu vao database */
#include "input.h"

int load_board(const char *path, database *db)
{
    /* TODO(tv1): mo file, doc x_start,y_start,x_end,y_end -> db->board */
    (void)db;
    fprintf(stderr, "load_board(%s): chua cai dat\n", path);
    return STATUS_ERR;
}

int load_components(const char *path, database *db)
{
    /* TODO(tv1): doc ref,x,y,width,height -> db->comps[], db->num_comps; placed = 0 */
    (void)db;
    fprintf(stderr, "load_components(%s): chua cai dat\n", path);
    return STATUS_ERR;
}

int load_pads(const char *path, database *db)
{
    /* TODO(tv1): doc ref,pad,net_id,dx,dy -> db->pads[], db->num_pads; dien comp_idx */
    (void)db;
    fprintf(stderr, "load_pads(%s): chua cai dat\n", path);
    return STATUS_ERR;
}

int load_all(database *db)
{
    if (load_board(BOARD_FILE, db) != STATUS_OK)           return STATUS_ERR;
    if (load_components(COMPONENTS_FILE, db) != STATUS_OK) return STATUS_ERR;
    if (load_pads(PAD_FILE, db) != STATUS_OK)              return STATUS_ERR;
    return STATUS_OK;
}
