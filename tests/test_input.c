/* [tv1] Test module input: chay tu thu muc goc cua project.
   gcc -Wall -Wextra -std=c99 -Iinclude tests/test_input.c src/input.c -lm -o test_input */
#include "input.h"

static database db;          /* static: database ~114 KB, tranh tran stack */
static int fails = 0;

#define CHECK(cond, msg) do { \
    if (cond) printf("  [PASS] %s\n", msg); \
    else { printf("  [FAIL] %s\n", msg); fails++; } \
} while (0)

#define TMP_FILE "tmp_test_input.csv"

/* Ghi noi dung bat ky ra file tam (de thu cac file CSV hong) */
static void write_tmp(const char *content)
{
    FILE *f = fopen(TMP_FILE, "wb");
    if (f) { fputs(content, f); fclose(f); }
}

int main(void)
{
    int i, ok;

    /* ---------- 1. Du lieu that ---------- */
    printf("== 1. load_all tren input_data that ==\n");
    memset(&db, 0xAB, sizeof db);          /* rac: de bat loi quen khoi tao */
    CHECK(load_all(&db) == STATUS_OK, "load_all tra STATUS_OK");
    printf("  So component: %d (ky vong 23)\n", db.num_comps);
    printf("  So pad      : %d (ky vong 49)\n", db.num_pads);
    CHECK(db.num_comps == 23, "num_comps == 23");
    CHECK(db.num_pads == 49, "num_pads == 49");
    printf("  Board: x_start=%g y_start=%g x_end=%g y_end=%g\n",
           db.board.x_start, db.board.y_start, db.board.x_end, db.board.y_end);
    CHECK(db.board.x_start == 0 && db.board.y_start == 0 &&
          db.board.x_end == 58 && db.board.y_end == 30, "board = 0,0,58,30");

    printf("  3 component dau:\n");
    for (i = 0; i < 3 && i < db.num_comps; i++)
        printf("    [%d] ref=%s x=%g y=%g w=%g h=%g placed=%d\n", i, db.comps[i].ref,
               db.comps[i].x, db.comps[i].y, db.comps[i].width, db.comps[i].height,
               db.comps[i].placed);
    printf("  3 pad dau:\n");
    for (i = 0; i < 3 && i < db.num_pads; i++)
        printf("    [%d] ref=%s pad=%d net=%d dx=%g dy=%g comp_idx=%d\n", i, db.pads[i].ref,
               db.pads[i].pad, db.pads[i].net_id, db.pads[i].dx, db.pads[i].dy,
               db.pads[i].comp_idx);

    ok = 1;
    for (i = 0; i < db.num_pads; i++) {
        int k = db.pads[i].comp_idx;
        if (k < 0 || k >= db.num_comps || strcmp(db.pads[i].ref, db.comps[k].ref) != 0 ||
            db.pads[i].net_id < 0) { ok = 0; printf("    pad %d sai comp_idx=%d\n", i, k); }
    }
    CHECK(ok, "moi pad: comp_idx hop le, ref khop comps[comp_idx], net_id >= 0");

    ok = 1;
    for (i = 0; i < db.num_comps; i++)
        if (db.comps[i].placed != 0 || db.comps[i].x != 999 || db.comps[i].y != 999) ok = 0;
    CHECK(ok, "moi component: placed = 0, x = y = 999 (giu nguyen gia tri CSV)");
    CHECK(strcmp(db.comps[22].ref, "Thermistor_1") == 0, "ref dai (Thermistor_1) doc dung, khong dinh ky tu \\r");

    /* ---------- 2. Duong dan sai ---------- */
    printf("== 2. Duong dan sai -> STATUS_ERR ==\n");
    CHECK(load_board("khong_ton_tai/board.csv", &db) == STATUS_ERR, "load_board duong dan sai");
    CHECK(load_components("khong_ton_tai/c.csv", &db) == STATUS_ERR, "load_components duong dan sai");
    CHECK(db.num_comps == 0, "load_components loi -> num_comps = 0");
    CHECK(load_pads("khong_ton_tai/p.csv", &db) == STATUS_ERR, "load_pads (num_comps = 0) -> ERR");
    load_components(COMPONENTS_FILE, &db);
    CHECK(load_pads("khong_ton_tai/p.csv", &db) == STATUS_ERR, "load_pads duong dan sai");
    CHECK(db.num_pads == 0, "load_pads loi -> num_pads = 0");

    /* ---------- 3. File hong (tao file tam) ---------- */
    printf("== 3. File CSV hong / dinh dang bien ==\n");
    load_components(COMPONENTS_FILE, &db);

    write_tmp("ref,pad,net_id,dx,dy\nXX,1,1,0,0\n");
    CHECK(load_pads(TMP_FILE, &db) == STATUS_ERR, "pad co ref khong ton tai -> ERR");

    write_tmp("ref,pad,net_id,dx,dy\nR1,1,1,0\n");
    CHECK(load_pads(TMP_FILE, &db) == STATUS_ERR, "pad thieu cot -> ERR");

    write_tmp("ref,pad,net_id,dx,dy\nR1,1,1,0,0,9\n");
    CHECK(load_pads(TMP_FILE, &db) == STATUS_ERR, "pad thua cot -> ERR");

    write_tmp("ref,pad,net_id,dx,dy\nR1,1,-1,0,0\n");
    CHECK(load_pads(TMP_FILE, &db) == STATUS_ERR, "net_id am -> ERR");

    write_tmp("ref,x,y,width,height,is_placed\nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA,0,0,1,1,0\n");
    CHECK(load_components(TMP_FILE, &db) == STATUS_ERR, "ref dai >= MAX_REF_LEN -> ERR (khong tran)");

    write_tmp("ref,x,y,width,height,is_placed\nR1,0,0,1,1\n");
    CHECK(load_components(TMP_FILE, &db) == STATUS_ERR, "component thieu is_placed -> ERR");

    write_tmp("ref,x,y,width,height,is_placed\n");
    CHECK(load_components(TMP_FILE, &db) == STATUS_ERR, "component chi co header -> ERR");

    write_tmp("x_min,y_min,x_max,y_max\n0,0,58\n");
    CHECK(load_board(TMP_FILE, &db) == STATUS_ERR, "board thieu cot -> ERR");

    write_tmp("x_min,y_min,x_max,y_max\n0,0,58,30\n1,1,2,2\n");
    CHECK(load_board(TMP_FILE, &db) == STATUS_ERR, "board nhieu hon 1 dong du lieu -> ERR");

    /* File "dep" cua Linux: khong BOM, LF, dong trong, dong cuoi khong newline,
       is_placed = 1 phai giu nguyen */
    write_tmp("ref,x,y,width,height,is_placed\n\nR1,1.5,2.5,3,4,1\n   \nC1,999,999,2,2,0");
    CHECK(load_components(TMP_FILE, &db) == STATUS_OK && db.num_comps == 2, "khong BOM/LF/dong trong -> doc 2 component");
    CHECK(db.comps[0].placed == 1 && db.comps[0].x == 1.5f && db.comps[1].placed == 0, "is_placed giu nguyen gia tri doc");

    write_tmp("\xEF\xBB\xBF" "x_min,y_min,x_max,y_max\r\n\r\n0,0,10,5\r\n\r\n");
    CHECK(load_board(TMP_FILE, &db) == STATUS_OK && db.board.x_end == 10 && db.board.y_end == 5, "BOM + CRLF + dong trong -> board dung");

    /* Vuot MAX_COMPONENTS */
    {
        FILE *f = fopen(TMP_FILE, "wb");
        if (f) {
            fputs("ref,x,y,width,height,is_placed\n", f);
            for (i = 0; i < MAX_COMPONENTS + 1; i++) fprintf(f, "C%d,0,0,1,1,0\n", i);
            fclose(f);
        }
        CHECK(load_components(TMP_FILE, &db) == STATUS_ERR, "vuot MAX_COMPONENTS -> ERR");
    }

    remove(TMP_FILE);
    printf("\n%s (%d loi)\n", fails == 0 ? "TAT CA TEST DAT" : "CO TEST THAT BAI", fails);
    return fails == 0 ? 0 : 1;
}