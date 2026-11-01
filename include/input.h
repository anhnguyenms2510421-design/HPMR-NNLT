#ifndef INPUT_H
#define INPUT_H

#include "database.h"

/* [tv1] Doc 3 file CSV vao database.
   Tat ca ham tra ve STATUS_OK neu thanh cong, STATUS_ERR neu loi
   (khong mo duoc file, sai dinh dang, vuot MAX_*). */

/* Doc board.csv (x_min,y_min,x_max,y_max) -> db->board */
int load_board(const char *path, database *db);

/* Doc components.csv (ref,x,y,width,height,is_placed) -> db->comps[], db->num_comps.
   Cot is_placed -> truong placed (giu nguyen gia tri doc duoc). */
int load_components(const char *path, database *db);

/* Doc pads.csv (ref,pad,net_id,dx,dy) -> db->pads[], db->num_pads.
   Phai goi SAU load_components(): dien pad.comp_idx bang cach tim
   linh kien co ref trung; ref khong ton tai -> STATUS_ERR. */
int load_pads(const char *path, database *db);

/* Tien ich: goi lan luot 3 ham tren voi BOARD_FILE, COMPONENTS_FILE, PAD_FILE */
int load_all(database *db);

#endif
