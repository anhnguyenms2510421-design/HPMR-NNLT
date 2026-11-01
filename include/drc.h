#ifndef DRC_H
#define DRC_H

#include "database.h"

/* [tv3] Design Rule Check. Cac ham DRC tra ve 1 neu DAT, 0 neu VI PHAM.
   NGOAI LE: is_overlap tra 1 = CHONG/QUA GAN (vi pham), 0 = on.
   comp_idx la chi so trong db->comps[]. Hop bao linh kien = hinh chu nhat
   tam (x, y), kich thuoc width x height. */

/* Linh kien nam tron ven trong board */
int is_inside_board(const database *db, int comp_idx);

/* 1 = hai linh kien chong lan hoac gan nhau hon CLEARANCE (vi pham), 0 = on.
   gap_x = |xa-xb| - (wa+wb)/2, gap_y = |ya-yb| - (ha+hb)/2;
   vi pham khi gap_x < CLEARANCE VA gap_y < CLEARANCE. Khong xet co placed. */
int is_overlap(const database *db, int idx_a, int idx_b);

/* 1 linh kien: nam trong board VA khong is_overlap voi bat ky linh kien
   khac co placed == 1. Kiem tra vi tri HIEN TAI cua comp_idx, bat ke
   placed cua no (de thu vi tri truoc khi dat placed = 1). */
int drc_check_component(const database *db, int comp_idx);

/* Tat ca linh kien da dat deu dat DRC */
int drc_check_all(const database *db);

#endif
