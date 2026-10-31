#ifndef DATABASE_H
#define DATABASE_H

/* ===== Thu vien chuan dung chung cho ca nhom ===== */
#include <stdio.h>    /* fopen, fgets, fprintf, printf */
#include <stdlib.h>   /* rand, srand, atof, atoi, exit */
#include <string.h>   /* strcmp, strcpy, strtok */
#include <math.h>     /* fabsf, fminf, fmaxf */
#include <float.h>    /* FLT_MAX: khoi tao min khi tinh HPWL */
#include <time.h>     /* time(): seed cho rand() khi chon linh kien dau */

#include "config.h"

/* ===== board.csv: 1 dong ===== */
typedef struct {
    float x_start, y_start, x_end, y_end;
} board_dimension;

/* ===== components.csv: 1 dong = 1 linh kien =====
   (x, y) la TAM linh kien. */
typedef struct {
    char  ref[MAX_REF_LEN];
    float x, y;
    float width, height;
    int   placed;            /* 0 = chua dat (dang ngoai board), 1 = da dat */
} component;

/* ===== pad.csv: 1 dong = 1 pad =====
   (dx, dy) la offset cua pad so voi tam linh kien.
   Vi tri pad tren board = (comp.x + dx, comp.y + dy). */
typedef struct {
    char  ref[MAX_REF_LEN];
    int   pad;               /* so thu tu pad */
    int   net_id;            /* cung net_id = noi voi nhau (>= 0) */
    float dx, dy;
    int   comp_idx;          /* chi so trong comps[]; input.c dien sau khi doc
                                (tranh strcmp(ref) lap lai trong vong lap HPWL) */
} pad_info;

/* ===== Toan bo du lieu cua he thong ===== */
typedef struct {
    board_dimension board;

    component comps[MAX_COMPONENTS];
    int       num_comps;

    pad_info  pads[MAX_PADS];
    int       num_pads;
} database;

#endif
