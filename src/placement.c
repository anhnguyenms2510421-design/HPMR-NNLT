/* [tv3] Dat linh kien len board (try_move, hill_climb nam trong hill_climb.c) */
#include "placement.h"
#include "hpwl.h"
#include "drc.h"

/* ===== Hang so cuc bo cua tv3 (khong sua config.h) ===== */
#define PLACE_MAX_RETRY  20       /* place_all thu lai toi da 20 lan (hop dong) */
#define UNPLACED_POS     999.0f   /* toa do "chua dat", giong CSV: nam ngoai board */
#define TIE_EPS          1e-6f    /* HPWL chenh nho hon nay coi la hoa (hop dong) */
#define GRID_EPS         1e-4f    /* dung sai khi dem so o luoi, cung muc voi DRC */

/* ---------- place_initial ---------- */

/* verbose = 1: in loi ra stderr (ham cong khai). verbose = 0: im lang, de
   place_all thu lai nhieu lan ma khong tran man hinh bang cac "loi" binh thuong. */
static int place_initial_impl(database *db, int verbose)
{
    component *c;
    float old_x, old_y;
    int i, n_free = 0, k, idx = -1;

    if (db == NULL) return STATUS_ERR;

    for (i = 0; i < db->num_comps; i++)
        if (db->comps[i].placed == 0) n_free++;
    if (n_free == 0) {
        if (verbose) fprintf(stderr, "Loi: place_initial: khong con linh kien nao chua dat\n");
        return STATUS_ERR;
    }

    /* Chon ngau nhien: lay linh kien chua dat thu k (k = 0 .. n_free-1) */
    k = rand() % n_free;
    for (i = 0; i < db->num_comps; i++) {
        if (db->comps[i].placed != 0) continue;
        if (k == 0) { idx = i; break; }
        k--;
    }
    c = &db->comps[idx];
    old_x = c->x;
    old_y = c->y;

    /* Dat TAM dung giua board; khong thu vi tri nao khac (hop dong) */
    c->x = (db->board.x_start + db->board.x_end) / 2.0f;
    c->y = (db->board.y_start + db->board.y_end) / 2.0f;

    if (!drc_check_component(db, idx)) {
        c->x = old_x;               /* hoan tac, placed van 0: khong de lai trang thai do dang */
        c->y = old_y;
        if (verbose)
            fprintf(stderr, "Loi: place_initial: '%s' khong dat duoc giua board "
                            "(lon hon board hoac vi pham DRC)\n", c->ref);
        return STATUS_ERR;
    }
    c->placed = 1;
    return STATUS_OK;
}

int place_initial(database *db)
{
    return place_initial_impl(db, 1);
}

/* ---------- place_component ---------- */

static int place_component_impl(database *db, int comp_idx, int verbose)
{
    component *c;
    float old_x, old_y, x_lo, x_hi, y_lo, y_hi, cx, cy;
    float best_x = 0.0f, best_y = 0.0f, best_hpwl = 0.0f, best_d2 = 0.0f;
    int nx, ny, ix, iy, found = 0;

    if (db == NULL || comp_idx < 0 || comp_idx >= db->num_comps) {
        if (verbose)
            fprintf(stderr, "Loi: place_component: chi so linh kien %d khong hop le\n", comp_idx);
        return STATUS_ERR;
    }
    c = &db->comps[comp_idx];
    if (c->placed != 0) {           /* chi dat linh kien dang placed = 0 (theo placement.h) */
        if (verbose)
            fprintf(stderr, "Loi: place_component: '%s' da duoc dat roi\n", c->ref);
        return STATUS_ERR;
    }
    if (!(STEP > 0.0f)) {           /* STEP <= 0 lam vong quet khong bao gio ket thuc */
        if (verbose) fprintf(stderr, "Loi: place_component: STEP trong config.h phai > 0\n");
        return STATUS_ERR;
    }

    old_x = c->x;
    old_y = c->y;

    /* Tam linh kien chi duoc nam trong [lo, hi] thi hop bao moi con trong board */
    x_lo = db->board.x_start + c->width  / 2.0f;
    x_hi = db->board.x_end   - c->width  / 2.0f;
    y_lo = db->board.y_start + c->height / 2.0f;
    y_hi = db->board.y_end   - c->height / 2.0f;
    cx   = (db->board.x_start + db->board.x_end) / 2.0f;
    cy   = (db->board.y_start + db->board.y_end) / 2.0f;

    /* So o luoi moi chieu. Dung chi so nguyen (x = lo + ix*STEP) thay vi
       x += STEP de sai so float khong cong don qua hang tram buoc.
       Linh kien lon hon board -> hi < lo -> nx <= 0 -> khong quet gi. */
    nx = (int)floorf((x_hi - x_lo + GRID_EPS) / STEP) + 1;
    ny = (int)floorf((y_hi - y_lo + GRID_EPS) / STEP) + 1;

    c->placed = 1;      /* tam thoi: total_hpwl chi tinh pad cua linh kien placed == 1 */

    for (iy = 0; iy < ny; iy++) {           /* vong ngoai: y (hop dong) */
        for (ix = 0; ix < nx; ix++) {       /* vong trong: x */
            float x = x_lo + (float)ix * STEP;
            float y = y_lo + (float)iy * STEP;
            float hp, d2;

            c->x = x;
            c->y = y;
            if (!drc_check_component(db, comp_idx)) continue;   /* DRC re hon HPWL nen kiem truoc */

            hp = total_hpwl(db);
            d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);     /* binh phuong khoang cach toi tam board */

            /* Lay neu: chua co vi tri nao, hoac HPWL nho hon han, hoac HPWL hoa
               va gan tam hon. d2 so bang "<" nen hoa tiep thi giu vi tri gap dau tien. */
            if (!found || hp < best_hpwl - TIE_EPS ||
                (fabsf(hp - best_hpwl) < TIE_EPS && d2 < best_d2)) {
                found = 1;
                best_x = x; best_y = y; best_hpwl = hp; best_d2 = d2;
            }
        }
    }

    if (!found) {
        c->x = old_x;               /* hoan tac hoan toan: placed = 0, toa do cu */
        c->y = old_y;
        c->placed = 0;
        if (verbose)
            fprintf(stderr, "Loi: place_component: khong tim duoc cho hop le cho '%s'\n", c->ref);
        return STATUS_ERR;
    }
    c->x = best_x;                  /* placed da = 1 tu truoc vong quet */
    c->y = best_y;
    return STATUS_OK;
}

int place_component(database *db, int comp_idx)
{
    return place_component_impl(db, comp_idx, 1);
}

/* ---------- place_all ---------- */

static float area_of(const database *db, int i)
{
    return db->comps[i].width * db->comps[i].height;
}

/* Sap chi so linh kien theo dien tich giam dan (sap chen); hoa thi chi so nho truoc.
   Sap bang mang chi so, khong dao comps[] vi pad.comp_idx dang tro vao comps[]. */
static void sort_by_area_desc(const database *db, int order[], int n)
{
    int i, j, key;

    for (i = 0; i < n; i++) order[i] = i;
    for (i = 1; i < n; i++) {
        key = order[i];
        j = i - 1;
        /* "<" chat: dien tich bang nhau thi KHONG doi cho, giu chi so nho truoc */
        while (j >= 0 && area_of(db, order[j]) < area_of(db, key)) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }
}

/* Dua moi linh kien ve "chua dat" (nhu luc moi doc CSV) de thu lai tu dau */
static void reset_all(database *db)
{
    int i;

    for (i = 0; i < db->num_comps; i++) {
        db->comps[i].placed = 0;
        db->comps[i].x = UNPLACED_POS;
        db->comps[i].y = UNPLACED_POS;
    }
}

int place_all(database *db)
{
    int order[MAX_COMPONENTS];
    int n, i, attempt, ok;

    if (db == NULL || db->num_comps <= 0 || db->num_comps > MAX_COMPONENTS) {
        fprintf(stderr, "Loi: place_all: database khong co linh kien hop le\n");
        return STATUS_ERR;
    }
    n = db->num_comps;
    sort_by_area_desc(db, order, n);        /* dien tich khong doi nen sap 1 lan la du */

    for (attempt = 1; attempt <= PLACE_MAX_RETRY; attempt++) {
        /* 1. Linh kien dau (chon ngau nhien) vao giua board */
        ok = (place_initial_impl(db, 0) == STATUS_OK);

        /* 2. Cac linh kien con lai, to truoc nho sau: linh kien to kho tim cho
              nhat nen can dat khi board con rong */
        for (i = 0; ok && i < n; i++) {
            int idx = order[i];
            if (db->comps[idx].placed == 1) continue;    /* linh kien dau (hoac da dat san) */
            ok = (place_component_impl(db, idx, 0) == STATUS_OK);
        }

        if (ok) return STATUS_OK;
        reset_all(db);      /* that bai: xoa sach roi thu lai tu place_initial (hop dong) */
    }

    /* Het luot: database da duoc reset o tren, khong con linh kien dat do dang */
    fprintf(stderr, "Loi: place_all: khong dat duoc het linh kien sau %d lan thu\n", PLACE_MAX_RETRY);
    return STATUS_ERR;
}
