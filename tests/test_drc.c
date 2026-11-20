/* [tv3] Test module DRC. Khong can file CSV, khong phu thuoc hpwl.
   gcc -Wall -Wextra -std=c99 -Iinclude tests/test_drc.c src/drc.c -lm -o test_drc */
#include "drc.h"

static database db;          /* static: database ~114 KB, tranh tran stack */
static int fails = 0;

#define CHECK(cond, msg) do { \
    if (cond) printf("  [PASS] %s\n", msg); \
    else { printf("  [FAIL] %s\n", msg); fails++; } \
} while (0)

/* Xoa db, dat board 0,0,58,30 (giong board that) */
static void reset_db(void)
{
    memset(&db, 0, sizeof db);
    db.board.x_start = 0.0f;  db.board.y_start = 0.0f;
    db.board.x_end   = 58.0f; db.board.y_end   = 30.0f;
}

/* Them linh kien tam (x, y), kich thuoc w x h; tra ve chi so */
static int add(const char *ref, float x, float y, float w, float h, int placed)
{
    component *c = &db.comps[db.num_comps];

    snprintf(c->ref, MAX_REF_LEN, "%s", ref);
    c->x = x; c->y = y; c->width = w; c->height = h; c->placed = placed;
    return db.num_comps++;
}

static void move(int i, float x, float y)
{
    db.comps[i].x = x;
    db.comps[i].y = y;
}

/* Dat linh kien 0 vao (x, y, w, h) roi tra ve is_inside_board */
static int inside_at(float x, float y, float w, float h)
{
    db.comps[0].x = x; db.comps[0].y = y;
    db.comps[0].width = w; db.comps[0].height = h;
    return is_inside_board(&db, 0);
}

/* Linh kien 1 o (xb, yb); linh kien 0 co dinh o (10,10); ca hai 4 x 2 */
static int overlap_at(float xb, float yb)
{
    move(1, xb, yb);
    return is_overlap(&db, 0, 1);
}

int main(void)
{
    int a, b, c, i, sym = 1;
    float sx[] = {12.0f, 10.0f, 14.0f, 14.2f, 14.15f, 14.1f, 30.0f};
    float sy[] = {10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 12.1f, 10.0f};

    setvbuf(stdout, NULL, _IONBF, 0);   /* in dung thu tu, khong bi tron voi stderr */

    /* ---------- 1. is_inside_board ---------- */
    printf("== 1. is_inside_board (board 0,0,58,30; linh kien 10 x 6) ==\n");
    reset_db();
    add("X", 0, 0, 10, 6, 1);
    CHECK(inside_at(29, 15, 10, 6) == 1,   "nam giua board -> 1");
    CHECK(inside_at(5, 15, 10, 6) == 1,    "cham canh trai (x - w/2 = 0) -> 1");
    CHECK(inside_at(53, 15, 10, 6) == 1,   "cham canh phai (x + w/2 = 58) -> 1");
    CHECK(inside_at(29, 3, 10, 6) == 1,    "cham canh duoi (y - h/2 = 0) -> 1");
    CHECK(inside_at(29, 27, 10, 6) == 1,   "cham canh tren (y + h/2 = 30) -> 1");
    CHECK(inside_at(5, 3, 10, 6) == 1,     "cham goc duoi-trai -> 1");
    CHECK(inside_at(29, 15, 58, 30) == 1,  "vua khit ca board -> 1");
    CHECK(inside_at(4.99999f, 15, 10, 6) == 1, "lech 1e-5 ra ngoai canh trai (trong sai so 1e-4) -> 1");
    CHECK(inside_at(4.9f, 15, 10, 6) == 0, "lan ra ngoai canh trai 0.1 -> 0");
    CHECK(inside_at(53.1f, 15, 10, 6) == 0, "lan ra ngoai canh phai 0.1 -> 0");
    CHECK(inside_at(29, 2.9f, 10, 6) == 0, "lan ra ngoai canh duoi 0.1 -> 0");
    CHECK(inside_at(29, 27.1f, 10, 6) == 0, "lan ra ngoai canh tren 0.1 -> 0");
    CHECK(inside_at(999, 999, 10, 6) == 0, "o (999, 999) nhu linh kien chua dat -> 0");
    CHECK(inside_at(29, 15, 60, 6) == 0,   "rong hon board -> 0");
    CHECK(is_inside_board(&db, -1) == 0 && is_inside_board(&db, 1) == 0,
          "chi so sai (-1 va num_comps) -> 0");

    /* ---------- 2. is_overlap (1 = VI PHAM) ---------- */
    printf("== 2. is_overlap (tra 1 = vi pham; CLEARANCE = %.2f) ==\n", (double)CLEARANCE);
    reset_db();
    add("A", 10, 10, 4, 2, 1);
    add("B", 30, 10, 4, 2, 1);
    CHECK(overlap_at(12, 10) == 1,      "chong lan (gap_x = -2, gap_y = -2) -> 1");
    CHECK(overlap_at(10, 10) == 1,      "trung hoan toan -> 1");
    CHECK(overlap_at(14, 10) == 1,      "cham nhau (gap_x = 0 < CLEARANCE) -> 1");
    CHECK(overlap_at(14.15f, 10) == 1,  "cach theo x 0.15 < CLEARANCE -> 1");
    CHECK(overlap_at(14.2f, 10) == 0,   "cach theo x DUNG CLEARANCE -> 0 (so sanh float tho se bao 1)");
    CHECK(overlap_at(10 + 4 + CLEARANCE, 10) == 0, "cach x dung CLEARANCE (tinh bang macro) -> 0");
    CHECK(overlap_at(10, 12.15f) == 1,  "cach theo y 0.15 < CLEARANCE -> 1");
    CHECK(overlap_at(10, 12.2f) == 0,   "cach theo y DUNG CLEARANCE -> 0");
    CHECK(overlap_at(30, 10) == 0,      "xa theo x -> 0");
    CHECK(overlap_at(10, 30) == 0,      "xa theo y -> 0");
    CHECK(overlap_at(14.5f, 10) == 0,   "gap_x = 0.5 >= CLEARANCE du an toan du y chong -> 0");
    CHECK(overlap_at(14.1f, 12.1f) == 1, "goc gan nhau (gap_x = gap_y = 0.1, ca hai < CLEARANCE) -> 1");
    CHECK(overlap_at(14.5f, 12.1f) == 0, "gap_x = 0.5, gap_y = 0.1: chi 1 truc gan -> 0");
    for (i = 0; i < 7; i++) {
        move(1, sx[i], sy[i]);
        if (is_overlap(&db, 0, 1) != is_overlap(&db, 1, 0)) sym = 0;
    }
    CHECK(sym, "doi xung: is_overlap(a, b) == is_overlap(b, a)");

    reset_db();                         /* kich thuoc khac nhau, kieu du lieu that */
    add("R",  20, 10, 12.31f, 3.05f, 1);
    add("C",  20 + 6.155f + 2.775f + CLEARANCE, 10, 5.55f, 5.55f, 1);
    CHECK(is_overlap(&db, 0, 1) == 0,   "kich thuoc khac nhau, cach dung CLEARANCE -> 0");
    move(1, 20 + 6.155f + 2.775f + CLEARANCE - 0.05f, 10);
    CHECK(is_overlap(&db, 0, 1) == 1,   "kich thuoc khac nhau, cach nho hon CLEARANCE 0.05 -> 1");
    db.comps[1].placed = 0;
    CHECK(is_overlap(&db, 0, 1) == 1,   "is_overlap khong xet co placed (placed = 0 van tinh hinh hoc)");
    CHECK(is_overlap(&db, -1, 0) == 1 && is_overlap(&db, 0, 2) == 1, "chi so sai -> 1 (coi la vi pham)");

    /* ---------- 3. drc_check_component (1 = DAT) ---------- */
    printf("== 3. drc_check_component ==\n");
    reset_db();
    a = add("A", 10, 10, 6, 4, 1);
    b = add("B", 20, 10, 6, 4, 1);
    c = add("C", 12, 10, 6, 4, 0);      /* chua dat, hinh hoc chong A */
    CHECK(drc_check_component(&db, a) == 1, "A dat, B xa, C (placed = 0, chong A) bi bo qua -> 1");
    CHECK(drc_check_component(&db, c) == 0, "C o vi tri chong A: kiem tra vi tri hien tai du placed = 0 -> 0");
    move(c, 40, 20);
    CHECK(drc_check_component(&db, c) == 1, "C doi sang cho trong: vi tri hop le du placed = 0 -> 1");
    move(c, 21, 10);
    CHECK(drc_check_component(&db, c) == 0, "C chong B (B placed = 1) -> 0");
    move(c, 999, 999);
    CHECK(drc_check_component(&db, c) == 0, "C o (999, 999) ngoai board -> 0");
    move(b, 16.1f, 10);                 /* gap_x = 6.1 - 6 = 0.1 < CLEARANCE */
    CHECK(drc_check_component(&db, a) == 0 && drc_check_component(&db, b) == 0,
          "A va B cach 0.1 < CLEARANCE -> ca hai 0");
    db.comps[b].placed = 0;
    CHECK(drc_check_component(&db, a) == 1, "B chuyen placed = 0 -> A het bi anh huong -> 1");
    move(a, 2, 10);                     /* canh trai = -1 */
    CHECK(drc_check_component(&db, a) == 0, "A lan ra ngoai board -> 0");
    CHECK(drc_check_component(&db, 99) == 0, "chi so sai -> 0");

    /* ---------- 4. drc_check_all ---------- */
    printf("== 4. drc_check_all ==\n");
    reset_db();
    CHECK(drc_check_all(&db) == 1, "database rong -> 1");
    a = add("A", 10, 10, 6, 4, 1);
    b = add("B", 20, 10, 6, 4, 1);
    c = add("C", 999, 999, 6, 4, 0);    /* linh kien chua dat nhu trong CSV */
    CHECK(drc_check_all(&db) == 1, "A, B hop le, C chua dat o (999, 999) bi bo qua -> 1");
    move(c, 12, 10);
    CHECK(drc_check_all(&db) == 1, "C placed = 0 chong A van bi bo qua -> 1");
    db.comps[c].placed = 1;
    CHECK(drc_check_all(&db) == 0, "C placed = 1 chong A -> 0");
    move(c, 40, 20);
    CHECK(drc_check_all(&db) == 1, "C doi sang cho trong -> 1");
    move(b, 16.1f, 10);
    CHECK(drc_check_all(&db) == 0, "A va B cach < CLEARANCE -> 0");
    move(b, 20, 10);
    move(c, 999, 999);
    CHECK(drc_check_all(&db) == 0, "C placed = 1 nhung o ngoai board -> 0");

    printf("\nTong ket test_drc: %s (%d ca that bai)\n", fails ? "THAT BAI" : "TAT CA PASS", fails);
    return fails ? 1 : 0;
}