#ifndef DATABASE_H
#define DATABASE_H

/* ===== Thư viện chuẩn dùng chung cho cả nhóm ===== */
#include <stdio.h>    /* fopen, fgets, fprintf, printf */
#include <stdlib.h>   /* rand, srand, atof, atoi, exit */
#include <string.h>   /* strcmp, strcpy, strtok */
#include <math.h>     /* fabsf, fminf, fmaxf */
#include <float.h>    /* FLT_MAX: khởi tạo min khi tính HPWL */
#include <time.h>     /* time(): seed cho rand() khi chọn linh kiện đầu */

#include "config.h"


typedef struct {
    float x_start, y_start, x_end, y_end;
} board_dimension;


typedef struct {
    char  ref[MAX_REF_LEN];
    float x, y;
    float width, height;
    int   placed;            
} component;


typedef struct {
    char  ref[MAX_REF_LEN];
    int   pad;               /* số thứ tự pad */
    int   net_id;            /* cùng net_id = nối với nhau */
    float dx, dy;
    int   comp_idx;          
} pad;


typedef struct {
    board_dimension board;

    component comps[MAX_COMPONENTS];
    int       num_comps;

    pad  pads[MAX_PADS];
    int       num_pads;
} database;

#endif
