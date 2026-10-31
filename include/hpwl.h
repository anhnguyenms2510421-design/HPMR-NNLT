#ifndef HPWL_H
#define HPWL_H

#include "database.h"

/* [tv2] Tinh HPWL = (max_x - min_x) + (max_y - min_y) tren cac pad cua moi net.
   Chi tinh pad cua linh kien da dat (placed == 1).
   Net co < 2 pad hop le -> HPWL = 0. */

/* Toa do tuyet doi tren board cua pad thu pad_idx (chi so trong db->pads[]):
   abs = tam linh kien + (dx, dy) */
void  pad_abs_position(const database *db, int pad_idx, float *abs_x, float *abs_y);

/* So net = (net_id lon nhat) + 1; net_id hop le nam trong [0, so net) */
int   get_num_nets(const database *db);

/* HPWL cua 1 net */
float net_hpwl(const database *db, int net_id);

/* Tong HPWL cua tat ca net */
float total_hpwl(const database *db);

#endif
