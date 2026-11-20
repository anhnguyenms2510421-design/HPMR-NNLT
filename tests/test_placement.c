/* [tv3] Test module placement (place_initial, place_component, place_all).
   Chay tu THU MUC GOC cua project vi load_all doc input_data/ bang duong dan tuong doi:
   gcc -Wall -Wextra -std=c99 -Iinclude tests/test_placement.c src/placement.c src/drc.c \
       src/input.c tests/mock_hpwl.c -lm -o test_placement && ./test_placement
   Khi src/hpwl.c (tv2) xong: thay tests/mock_hpwl.c bang src/hpwl.c (khong link ca hai). */
#include "placement.h"
#include "input.h"
#include "drc.h"
#include "hpwl.h"

#define NUM_SEEDS 20

static database db;        /* ban lam viec (du lieu that) */
static database snapshot;  /* du lieu that ngay sau load_all: khoi phuc truoc moi lan chay */
static database aux;       /* ban phu: replay va brute force */
static database sm;        /* database dung tay cho cac ca nho */
static int fails = 0;

#define CHECK(cond, msg) do { \
    if (cond) printf("  [PASS] %s\n", msg); \
    else { printf("  [FAIL] %s\n", msg); fails++; } \
} while (0)

/* ---------- ham ho tro (viet doc lap voi drc.c / placement.c) ---------- */

static int count_placed(const database *d)
{
    int i, n = 0;
    for (i = 0; i < d->num_comps; i++) if (d->comps[i].placed == 1) n++;
    return n;
}

/* Chi so linh kien placed co tam dung giua board, -1 neu khong co */
static int find_at_center(const database *d)
{
    float cx = (d->board.x_start + d->board.x_end) / 2.0f;
    float cy = (d->board.y_start + d->board.y_end) / 2.0f;
    int i;
    for (i = 0; i < d->num_comps; i++)
        if (d->comps[i].placed == 1 && fabsf(d->comps[i].x - cx) < 1e-6f &&
            fabsf(d->comps[i].y - cy) < 1e-6f) return i;
    return -1;
}

static int shares_net(const database *d, int a, int b)
{
    int p, q;
    for (p = 0; p < d->num_pads; p++) {
        if (d->pads[p].comp_idx != a) continue;
        for (q = 0; q < d->num_pads; q++)
            if (d->pads[q].comp_idx == b && d->pads[q].net_id == d->pads[p].net_id) return 1;
    }
    return 0;
}

/* Kiem tra hinh hoc truc tiep (khong goi drc.c): trong board va moi cap cach >= CLEARANCE */
static int geometry_ok(const database *d)
{
    int i, j;
    for (i = 0; i < d->num_comps; i++) {
        const component *a = &d->comps[i];
        if (a->placed != 1) continue;
        if (a->x - a->width / 2 < d->board.x_start - 1e-3f || a->x + a->width / 2 > d->board.x_end + 1e-3f ||
            a->y - a->height / 2 < d->board.y_start - 1e-3f || a->y + a->height / 2 > d->board.y_end + 1e-3f)
            return 0;
        for (j = i + 1; j < d->num_comps; j++) {
            const component *b = &d->comps[j];
            float gx, gy;
            if (b->placed != 1) continue;
            gx = fabsf(a->x - b->x) - (a->width + b->width) / 2;
            gy = fabsf(a->y - b->y) - (a->height + b->height) / 2;
            if (gx < CLEARANCE - 1e-3f && gy < CLEARANCE - 1e-3f) return 0;
        }
    }
    return 1;
}

/* x, y, placed cua moi linh kien giong het nhau (so sanh tuyet doi) */
static int same_layout(const database *p, const database *q)
{
    int i;
    if (p->num_comps != q->num_comps) return 0;
    for (i = 0; i < p->num_comps; i++)
        if (p->comps[i].x != q->comps[i].x || p->comps[i].y != q->comps[i].y ||
            p->comps[i].placed != q->comps[i].placed) return 0;
    return 1;
}

static float area_of(const database *d, int i) { return d->comps[i].width * d->comps[i].height; }

/* Thu tu dien tich giam dan, hoa thi chi so nho truoc (sap chon, khac cach cai dat) */
static void order_by_area(const database *d, int order[])
{
    int used[MAX_COMPONENTS] = {0};
    int p, i, best;
    for (p = 0; p < d->num_comps; p++) {
        best = -1;
        for (i = 0; i < d->num_comps; i++) {
            if (used[i]) continue;
            if (best < 0 || area_of(d, i) > area_of(d, best)) best = i;   /* ">" chat: hoa giu chi so nho */
        }
        used[best] = 1;
        order[p] = best;
    }
}

/* ---- database dung tay ---- */
static void sm_init(float w, float h)
{
    memset(&sm, 0, sizeof sm);
    sm.board.x_end = w;
    sm.board.y_end = h;
}

static int sm_add(const char *ref, float w, float h)
{
    component *c = &sm.comps[sm.num_comps];
    snprintf(c->ref, MAX_REF_LEN, "%s", ref);
    c->x = 999.0f; c->y = 999.0f; c->width = w; c->height = h; c->placed = 0;
    return sm.num_comps++;
}

static void sm_put(int i, float x, float y)     /* dat san linh kien i */
{
    sm.comps[i].x = x; sm.comps[i].y = y; sm.comps[i].placed = 1;
}

static void sm_pad(int comp_idx, int net_id, float dx, float dy)
{
    pad_info *p = &sm.pads[sm.num_pads];
    snprintf(p->ref, MAX_REF_LEN, "%s", sm.comps[comp_idx].ref);
    p->pad = sm.num_pads + 1; p->net_id = net_id; p->dx = dx; p->dy = dy; p->comp_idx = comp_idx;
    sm.num_pads++;
}

static int at(int i, float x, float y)
{
    return fabsf(sm.comps[i].x - x) < 1e-4f && fabsf(sm.comps[i].y - y) < 1e-4f;
}

/* ---------- 1. place_initial ---------- */
static void test_place_initial(void)
{
    int seed, i, idx, distinct = 0, one = 1, center = 1, rest = 1, drc = 1, okret = 1;
    int seen[MAX_COMPONENTS] = {0};
    float cx = (snapshot.board.x_start + snapshot.board.x_end) / 2.0f;
    float cy = (snapshot.board.y_start + snapshot.board.y_end) / 2.0f;
    int a, b, c;

    printf("== 1. place_initial (du lieu that, 50 seed) ==\n");
    printf("  Tam board = (%g, %g)\n", (double)cx, (double)cy);
    for (seed = 1; seed <= 50; seed++) {
        db = snapshot;
        srand((unsigned)seed);
        if (place_initial(&db) != STATUS_OK) { okret = 0; continue; }
        idx = find_at_center(&db);
        if (count_placed(&db) != 1 || idx < 0) { one = 0; continue; }
        if (fabsf(db.comps[idx].x - 29.0f) > 1e-6f || fabsf(db.comps[idx].y - 15.0f) > 1e-6f) center = 0;
        if (!drc_check_component(&db, idx)) drc = 0;
        for (i = 0; i < db.num_comps; i++)
            if (i != idx && (db.comps[i].placed != 0 || db.comps[i].x != snapshot.comps[i].x ||
                             db.comps[i].y != snapshot.comps[i].y)) rest = 0;
        if (!seen[idx]) { seen[idx] = 1; distinct++; }
        if (seed == 1) printf("  Seed 1 chon %s -> tam (%g, %g)\n", db.comps[idx].ref,
                              (double)db.comps[idx].x, (double)db.comps[idx].y);
    }
    CHECK(okret, "luon tra STATUS_OK");
    CHECK(one, "dung 1 linh kien duoc dat (placed = 1)");
    CHECK(center, "tam linh kien duoc chon dung (29, 15)");
    CHECK(drc, "vi tri giua board qua drc_check_component");
    CHECK(rest, "cac linh kien khac khong bi dong den");
    printf("  50 seed chon %d linh kien khac nhau\n", distinct);
    CHECK(distinct >= 2, "chon ngau nhien (nhieu seed cho nhieu linh kien khac nhau)");

    /* ca nho */
    sm_init(6, 4);
    a = sm_add("BIG", 10, 10);
    CHECK(place_initial(&sm) == STATUS_ERR, "linh kien lon hon board -> STATUS_ERR");
    CHECK(sm.comps[a].placed == 0 && at(a, 999, 999), "  ... placed = 0 va toa do cu duoc hoan tac");

    sm_init(20, 10);
    a = sm_add("A", 2, 2); b = sm_add("B", 2, 2); c = sm_add("C", 2, 2);
    sm_put(a, 3, 3); sm_put(b, 17, 7);
    CHECK(place_initial(&sm) == STATUS_OK && sm.comps[c].placed == 1 && at(c, 10, 5),
          "chi con 1 linh kien chua dat -> duoc chon va dat o (10, 5)");
    CHECK(at(a, 3, 3) && at(b, 17, 7), "  ... linh kien da dat khong bi dong den");

    sm_init(20, 10);
    a = sm_add("A", 4, 4); b = sm_add("B", 2, 2);
    sm_put(a, 10, 5);                           /* A chiem san tam board */
    CHECK(place_initial(&sm) == STATUS_ERR, "tam board bi chiem (vi pham DRC) -> STATUS_ERR");
    CHECK(sm.comps[b].placed == 0 && at(b, 999, 999), "  ... linh kien that bai co placed = 0, toa do cu");
    sm.comps[b].placed = 1;
    CHECK(place_initial(&sm) == STATUS_ERR, "khong con linh kien chua dat -> STATUS_ERR");
}

/* ---------- 2. place_component: ca nho tinh tay ---------- */
static void test_place_component_small(void)
{
    int a, b;

    printf("== 2. place_component (du lieu dung tay, board 20 x 10, A 4x2 o (10, 5)) ==\n");

    /* 2a. khong noi voi linh kien nao -> HPWL = 0 o moi cho -> phai gan tam nhat */
    sm_init(20, 10);
    a = sm_add("A", 4, 2); b = sm_add("B", 2, 2);
    sm_put(a, 10, 5);
    CHECK(place_component(&sm, b) == STATUS_OK && sm.comps[b].placed == 1, "B khong co pad: STATUS_OK, placed = 1");
    printf("  B duoc dat tai (%g, %g), tam board (10, 5)\n", (double)sm.comps[b].x, (double)sm.comps[b].y);
    CHECK(at(b, 10, 2), "hoa HPWL -> chon cho hop le gan tam nhat; hoa tiep lay cho gap dau (10, 2)");
    CHECK(fabsf(sm.comps[b].y - 5) <= 3.0f + 1e-4f && fabsf(sm.comps[b].x - 10) < 1e-4f,
          "khong bi day ra goc board (cach tam <= 3, goc cach ~11.2)");
    CHECK(drc_check_all(&sm) == 1, "ket qua qua drc_check_all");

    /* 2b. noi voi A bang net 1, pad o tam: HPWL = khoang cach Manhattan */
    sm_init(20, 10);
    a = sm_add("A", 4, 2); b = sm_add("B", 2, 2);
    sm_put(a, 10, 5);
    sm_pad(a, 1, 0, 0); sm_pad(b, 1, 0, 0);
    CHECK(place_component(&sm, b) == STATUS_OK && at(b, 10, 2) && fabsf(total_hpwl(&sm) - 3.0f) < 1e-4f,
          "pad o tam: HPWL nho nhat = 3 tai (10, 2) (hoa voi (10, 8), lay cho gap dau)");

    /* 2c. HPWL phai thang khoang cach toi tam: pad cua A lech +2 */
    sm_init(20, 10);
    a = sm_add("A", 4, 2); b = sm_add("B", 2, 2);
    sm_put(a, 10, 5);
    sm_pad(a, 1, 2, 0); sm_pad(b, 1, 0, 0);
    CHECK(place_component(&sm, b) == STATUS_OK && at(b, 14, 5) && fabsf(total_hpwl(&sm) - 2.0f) < 1e-4f,
          "pad A lech +2: HPWL nho nhat = 2 tai (14, 5), du xa tam hon (10, 2)");

    /* 2d. cac ca loi: khong de trang thai do dang */
    sm_init(6, 4);
    a = sm_add("A", 5, 3); b = sm_add("B", 2, 2);
    sm_put(a, 3, 2);
    CHECK(place_component(&sm, b) == STATUS_ERR, "khong con cho trong -> STATUS_ERR");
    CHECK(sm.comps[b].placed == 0 && at(b, 999, 999), "  ... placed = 0 va toa do cu duoc hoan tac");
    CHECK(place_component(&sm, a) == STATUS_ERR && sm.comps[a].placed == 1 && at(a, 3, 2),
          "linh kien da dat (placed = 1) -> STATUS_ERR, khong bi dong den");
    CHECK(place_component(&sm, -1) == STATUS_ERR && place_component(&sm, sm.num_comps) == STATUS_ERR,
          "chi so sai -> STATUS_ERR");
    a = sm_add("BIG", 10, 10);
    CHECK(place_component(&sm, a) == STATUS_ERR && sm.comps[a].placed == 0 && at(a, 999, 999),
          "linh kien lon hon board -> STATUS_ERR, placed = 0");
}

/* ---------- 3. place_component tren du lieu that: so voi brute force ---------- */
static void test_place_component_real(void)
{
    int f, u = -1, v = -1, i, ix, iy, nx, ny, found;
    float cx, cy, x_lo, y_lo, d2, hp, min_d2, min_hp;

    printf("== 3. place_component tren du lieu that (so voi vet can) ==\n");
    db = snapshot;
    srand(1);
    CHECK(place_initial(&db) == STATUS_OK, "place_initial (seed 1) OK");
    f = find_at_center(&db);
    cx = (db.board.x_start + db.board.x_end) / 2.0f;
    cy = (db.board.y_start + db.board.y_end) / 2.0f;
    printf("  Linh kien dau: %s (%gx%g) o giua board\n", db.comps[f].ref,
           (double)db.comps[f].width, (double)db.comps[f].height);
    for (i = 0; i < db.num_comps; i++) {
        if (db.comps[i].placed != 0) continue;
        if (u < 0 && !shares_net(&db, f, i)) u = i;
        if (v < 0 && shares_net(&db, f, i)) v = i;
    }

    /* 3a. linh kien KHONG noi voi linh kien da dat -> phai gan tam board nhat */
    if (u >= 0) {
        aux = db;
        aux.comps[u].placed = 1;
        x_lo = aux.board.x_start + aux.comps[u].width / 2.0f;
        y_lo = aux.board.y_start + aux.comps[u].height / 2.0f;
        nx = (int)floorf((aux.board.x_end - aux.comps[u].width / 2.0f - x_lo + 1e-4f) / STEP) + 1;
        ny = (int)floorf((aux.board.y_end - aux.comps[u].height / 2.0f - y_lo + 1e-4f) / STEP) + 1;
        found = 0; min_d2 = 0.0f;
        for (iy = 0; iy < ny; iy++)
            for (ix = 0; ix < nx; ix++) {
                aux.comps[u].x = x_lo + (float)ix * STEP;
                aux.comps[u].y = y_lo + (float)iy * STEP;
                if (!drc_check_component(&aux, u)) continue;
                d2 = (aux.comps[u].x - cx) * (aux.comps[u].x - cx) + (aux.comps[u].y - cy) * (aux.comps[u].y - cy);
                if (!found || d2 < min_d2) { found = 1; min_d2 = d2; }
            }
        CHECK(place_component(&db, u) == STATUS_OK, "place_component(linh kien khong noi) OK");
        d2 = (db.comps[u].x - cx) * (db.comps[u].x - cx) + (db.comps[u].y - cy) * (db.comps[u].y - cy);
        printf("  %s khong noi voi %s: dat tai (%g, %g), cach tam %.2f; vet can: gan nhat %.2f\n",
               db.comps[u].ref, db.comps[f].ref, (double)db.comps[u].x, (double)db.comps[u].y,
               sqrt((double)d2), sqrt((double)min_d2));
        CHECK(found && fabsf(d2 - min_d2) < 1e-3f, "cach tam bang vi tri hop le gan tam nhat (vet can)");
        CHECK(sqrt((double)d2) < 15.0, "khong bi day ra goc board (goc cach tam ~33)");
        CHECK(geometry_ok(&db), "hinh hoc hop le (kiem doc lap)");
    } else {
        printf("  (bo qua 3a: moi linh kien deu noi voi linh kien dau)\n");
    }

    /* 3b. linh kien CO noi -> HPWL phai bang HPWL nho nhat cua moi cho hop le */
    if (v >= 0) {
        aux = db;
        aux.comps[v].placed = 1;
        x_lo = aux.board.x_start + aux.comps[v].width / 2.0f;
        y_lo = aux.board.y_start + aux.comps[v].height / 2.0f;
        nx = (int)floorf((aux.board.x_end - aux.comps[v].width / 2.0f - x_lo + 1e-4f) / STEP) + 1;
        ny = (int)floorf((aux.board.y_end - aux.comps[v].height / 2.0f - y_lo + 1e-4f) / STEP) + 1;
        found = 0; min_hp = 0.0f;
        for (iy = 0; iy < ny; iy++)
            for (ix = 0; ix < nx; ix++) {
                aux.comps[v].x = x_lo + (float)ix * STEP;
                aux.comps[v].y = y_lo + (float)iy * STEP;
                if (!drc_check_component(&aux, v)) continue;
                hp = total_hpwl(&aux);
                if (!found || hp < min_hp) { found = 1; min_hp = hp; }
            }
        CHECK(place_component(&db, v) == STATUS_OK, "place_component(linh kien co noi) OK");
        hp = total_hpwl(&db);
        printf("  %s noi voi %s: dat tai (%g, %g), total_hpwl = %.3f; vet can nho nhat %.3f\n",
               db.comps[v].ref, db.comps[f].ref, (double)db.comps[v].x, (double)db.comps[v].y,
               (double)hp, (double)min_hp);
        CHECK(found && fabsf(hp - min_hp) < 1e-4f, "total_hpwl bang gia tri nho nhat cua moi cho hop le");
        CHECK(drc_check_all(&db) == 1, "drc_check_all == 1");
    } else {
        printf("  (bo qua 3b: khong co linh kien noi voi linh kien dau)\n");
    }
}

/* ---------- 4. place_all ---------- */
static void build_retry_db(void)    /* board 13 x 2: chi thanh cong khi A (rong 6) la linh kien dau */
{
    sm_init(13, 2);
    sm_add("A", 6, 2);
    sm_add("B", 3, 2);
}

static void test_place_all(void)
{
    int seed, i, n, f, ok_ret = 1, ok_all = 1, ok_drc = 1, ok_geo = 1, distinct = 0;
    int seen[MAX_COMPONENTS] = {0};
    int order[MAX_COMPONENTS];
    float h, hmin = 0.0f, hmax = 0.0f, hsum = 0.0f;
    int replay_ok = 1, det_ok;
    clock_t t0;

    printf("== 4. place_all (du lieu that, %d seed) ==\n", NUM_SEEDS);
    n = snapshot.num_comps;

    /* 4a. mot lan chay: chi tiet */
    db = snapshot;
    srand(12345);
    CHECK(place_all(&db) == STATUS_OK, "place_all (seed 12345) tra STATUS_OK");
    CHECK(count_placed(&db) == n, "moi linh kien placed == 1");
    CHECK(drc_check_all(&db) == 1, "drc_check_all == 1");
    CHECK(geometry_ok(&db), "hinh hoc hop le (kiem doc lap voi drc.c)");
    printf("  total_hpwl sau place_all (seed 12345) = %.3f\n", (double)total_hpwl(&db));

    /* 4b. nhieu lan: khoi phuc roi goi lai */
    t0 = clock();
    for (seed = 1; seed <= NUM_SEEDS; seed++) {
        db = snapshot;
        srand((unsigned)seed);
        if (place_all(&db) != STATUS_OK) { ok_ret = 0; continue; }
        if (count_placed(&db) != n) ok_all = 0;
        if (!drc_check_all(&db)) ok_drc = 0;
        if (!geometry_ok(&db)) ok_geo = 0;
        h = total_hpwl(&db);
        if (seed == 1 || h < hmin) hmin = h;
        if (seed == 1 || h > hmax) hmax = h;
        hsum += h;
        f = find_at_center(&db);
        if (f >= 0 && !seen[f]) { seen[f] = 1; distinct++; }
    }
    CHECK(ok_ret, "moi lan khoi phuc + place_all deu STATUS_OK");
    CHECK(ok_all, "moi lan: moi linh kien placed == 1");
    CHECK(ok_drc, "moi lan: drc_check_all == 1");
    CHECK(ok_geo, "moi lan: hinh hoc hop le (kiem doc lap)");
    printf("  total_hpwl: nho nhat %.3f, trung binh %.3f, lon nhat %.3f; %d linh kien dau khac nhau; %.2f s\n",
           (double)hmin, (double)hsum / NUM_SEEDS, (double)hmax, distinct,
           (double)(clock() - t0) / CLOCKS_PER_SEC);
    CHECK(distinct >= 2, "linh kien dau thay doi giua cac lan (rand)");

    /* 4c. cung seed -> cung ket qua (khong co trang thai an giua cac lan goi) */
    db = snapshot; srand(7); place_all(&db);
    aux = snapshot; srand(7); place_all(&aux);
    det_ok = same_layout(&db, &aux);
    CHECK(det_ok, "cung seed, hai lan goi -> ket qua giong het nhau");

    /* 4d. thu tu: linh kien dau o giua, cac linh kien con lai dat theo dien tich giam dan */
    order_by_area(&snapshot, order);
    for (seed = 1; seed <= 5; seed++) {
        db = snapshot; srand((unsigned)seed); place_all(&db);
        f = find_at_center(&db);
        aux = snapshot;
        if (f < 0) { replay_ok = 0; continue; }
        aux.comps[f].x = (aux.board.x_start + aux.board.x_end) / 2.0f;
        aux.comps[f].y = (aux.board.y_start + aux.board.y_end) / 2.0f;
        aux.comps[f].placed = 1;
        for (i = 0; i < n; i++)
            if (order[i] != f && place_component(&aux, order[i]) != STATUS_OK) replay_ok = 0;
        if (!same_layout(&db, &aux)) replay_ok = 0;
    }
    CHECK(replay_ok, "phat lai thu cong (dau o giua, roi place_component theo dien tich giam dan) trung ket qua place_all");

    /* 4e. that bai: khong de trang thai do dang */
    sm_init(6, 4);
    sm_add("P", 5, 3); sm_add("Q", 5, 3);       /* chi 1 linh kien vua board */
    CHECK(place_all(&sm) == STATUS_ERR, "khong the dat het -> sau 20 lan thu tra STATUS_ERR");
    CHECK(count_placed(&sm) == 0 && at(0, 999, 999) && at(1, 999, 999),
          "  ... moi linh kien placed = 0, x = y = 999 (khong do dang)");
    sm_init(10, 10);
    CHECK(place_all(&sm) == STATUS_ERR, "database khong co linh kien -> STATUS_ERR");

    /* 4f. co che thu lai: lan dau co the chon nham B lam linh kien dau (A khong con cho) */
    ok_ret = 1; ok_drc = 1; ok_geo = 1;
    for (seed = 1; seed <= 30; seed++) {
        build_retry_db();
        srand((unsigned)seed);
        if (place_all(&sm) != STATUS_OK || count_placed(&sm) != 2 || !at(0, 6.5f, 1) || !at(1, 1.5f, 1)) ok_ret = 0;
        if (!drc_check_all(&sm)) ok_drc = 0;
    }
    CHECK(ok_ret, "board chi vua khi A (rong 6) o giua: 30 seed deu thanh cong nho thu lai, A (6.5, 1), B (1.5, 1)");
    CHECK(ok_drc, "  ... va deu qua DRC");
}

/* Tien dieu kien: cac ca HPWL ben duoi chi dung khi total_hpwl tinh that.
   Neu dang link src/hpwl.c con la stub (tra 0) thi dong nay se FAIL va noi ro nguyen nhan. */
static void test_hpwl_precondition(void)
{
    int a, b;

    printf("== 0b. Tien dieu kien: total_hpwl ==\n");
    sm_init(20, 10);
    a = sm_add("A", 2, 2); b = sm_add("B", 2, 2);
    sm_put(a, 5, 5); sm_put(b, 8, 9);
    sm_pad(a, 1, 0, 0); sm_pad(b, 1, 0, 0);
    CHECK(fabsf(total_hpwl(&sm) - 7.0f) < 1e-4f,
          "total_hpwl tinh dung (|dx| + |dy| = 3 + 4 = 7); neu FAIL: hpwl.c con la stub, hay link tests/mock_hpwl.c");
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* in dung thu tu, khong bi tron voi stderr */

    printf("== 0. Nap du lieu that ==\n");
    CHECK(load_all(&snapshot) == STATUS_OK, "load_all OK");
    if (snapshot.num_comps == 0) { printf("Khong co du lieu, dung test.\n"); return 1; }
    printf("  %d linh kien, %d pad, board (%g, %g) - (%g, %g)\n", snapshot.num_comps, snapshot.num_pads,
           (double)snapshot.board.x_start, (double)snapshot.board.y_start,
           (double)snapshot.board.x_end, (double)snapshot.board.y_end);

    test_hpwl_precondition();
    test_place_initial();
    test_place_component_small();
    test_place_component_real();
    test_place_all();

    printf("\nTong ket test_placement: %s (%d ca that bai)\n", fails ? "THAT BAI" : "TAT CA PASS", fails);
    return fails ? 1 : 0;
}
