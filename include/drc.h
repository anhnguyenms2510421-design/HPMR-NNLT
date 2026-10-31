#ifndef DRC_H
#define DRC_H

#include "database.h"

/* [tv4] Design Rule Check. Moi ham tra ve 1 neu DAT, 0 neu VI PHAM.
   comp_idx la chi so trong db->comps[]. Hop bao linh kien = hinh chu nhat
   tam (x, y), kich thuoc width x height. */

/* Linh kien nam tron ven trong board */
int is_inside_board(const database *db, int comp_idx);

/* Hai linh kien chong lan hoac gan nhau hon CLEARANCE */
int is_overlap(const database *db, int idx_a, int idx_b);

/* 1 linh kien: nam trong board VA khong chong lan voi bat ky linh kien
   nao khac da dat (placed == 1) */
int drc_check_component(const database *db, int comp_idx);

/* Tat ca linh kien da dat deu dat DRC */
int drc_check_all(const database *db);

#endif
