#ifndef PLACEMENT_H
#define PLACEMENT_H

#include "database.h"
#include "report.h"

/* [tv3] Thuat toan dat linh kien + Hill Climbing.
   Tat ca ham chi dat/di chuyen linh kien sao cho drc_check_component() = 1. */

/* Chon ngau nhien 1 linh kien, dat vao vi tri hop le tren board, placed = 1.
   Goi srand() truoc (main.c lam). STATUS_ERR neu khong co linh kien/khong dat duoc. */
int   place_initial(database *db);

/* Dat linh kien thu n+1 (comp_idx, dang placed = 0) vao vi tri hop le
   co total_hpwl nho nhat. STATUS_ERR neu khong tim duoc cho. */
int   place_component(database *db, int comp_idx);

/* place_initial() roi place_component() cho tung linh kien con lai
   cho den khi het. STATUS_ERR neu 1 linh kien khong dat duoc. */
int   place_all(database *db);

/* Thu dich linh kien (dx, dy). Giu nuoc di neu DRC dat VA HPWL giam;
   nguoc lai hoan tac. Tra ve 1 neu giu, 0 neu hoan tac. */
int   try_move(database *db, int comp_idx, float dx, float dy);

/* Hill Climbing: lap toi da MAX_ITER vong, moi vong thu cac nuoc di (+-STEP
   theo x, y) cho tung linh kien; dung som neu khong cai thien.
   Ghi hist: iteration 0 = HPWL ban dau, roi moi vong 1 record (report_log_iter).
   Tra ve HPWL cuoi cung. */
float hill_climb(database *db, history *hist);

#endif
