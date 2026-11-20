/* Test cho [tv4] try_move / hill_climb.
   Chay tu thu muc goc project (can cac file CSV trong input_data/). Xem lenh bien dich o cuoi file. */
#include "database.h"
#include "input.h"
#include "hpwl.h"
#include "drc.h"
#include "placement.h"
#include "report.h"

#ifdef WITH_DRC_PROBE   /* chi bat khi link voi tests/mock_drc.c (xem lenh bien dich cuoi file) */
extern int   mock_drc_probe_on, mock_drc_probe_n, mock_drc_probe_idx[];
extern float mock_drc_probe_x[], mock_drc_probe_y[];
#endif

#define TEST_SEEDS 30   /* so lan chay doc lap tren du lieu that (lon hon NUM_RESTARTS) */

/* database ~114 KB -> dung static de khong tran stack */
static database g_snap;     /* ban chup ngay sau load_all */
static database g_work;     /* database dang chay */
static database g_placed;   /* trang thai ngay sau place_all (truoc hill_climb) */
static database g_small;    /* database nho tu dung tay */
static database g_ref_db;   /* ket qua tham chieu cua seed dau tien */
static database g_ref_placed;
static history  g_hist, g_ref_hist;

static int g_checks = 0, g_fails = 0;

#define CHECK(cond, ...) do {                                   \
        g_checks++;                                             \
        if (!(cond)) {                                          \
            g_fails++;                                          \
            printf("  [FAIL] %s:%d: ", __FILE__, __LINE__);     \
            printf(__VA_ARGS__);                                \
            printf("\n");                                       \
        }                                                       \
    } while (0)

/* ---------- tien ich ---------- */

static int same_positions(const database *a, const database *b)
{
    if (a->num_comps != b->num_comps) return 0;
    for (int i = 0; i < a->num_comps; i++)
        if (a->comps[i].x != b->comps[i].x || a->comps[i].y != b->comps[i].y ||
            a->comps[i].placed != b->comps[i].placed)
            return 0;
    return 1;
}

static int same_history(const history *a, const history *b)
{
    if (a->count != b->count || a->best_index != b->best_index) return 0;
    for (int i = 0; i < a->count; i++)
        if (a->records[i].iteration != b->records[i].iteration ||
            a->records[i].hpwl != b->records[i].hpwl)
            return 0;
    return 1;
}

/* Kiem tra cac tinh chat chung cua 1 history do hill_climb ghi (bat dau tu history_init).
   hpwl0 = HPWL truoc hill_climb, final = gia tri hill_climb tra ve. */
static void check_history(const history *h, float hpwl0, float final, const char *tag)
{
    CHECK(h->count >= 2, "%s: count = %d < 2", tag, h->count);
    if (h->count < 1) return;

    CHECK(h->records[0].iteration == 0, "%s: record dau co iteration %d != 0",
          tag, h->records[0].iteration);
    CHECK(h->records[0].hpwl == hpwl0, "%s: iteration 0 = %.6f, HPWL ban dau = %.6f",
          tag, h->records[0].hpwl, hpwl0);

    for (int i = 0; i < h->count; i++)
        CHECK(h->records[i].iteration == i, "%s: record %d co iteration %d",
              tag, i, h->records[i].iteration);

    /* KHANG DINH: chuoi HPWL khong bao gio tang */
    int monotone = 1;
    for (int i = 1; i < h->count; i++)
        if (h->records[i].hpwl > h->records[i - 1].hpwl) monotone = 0;
    CHECK(monotone, "%s: chuoi HPWL trong history bi TANG", tag);

    CHECK(h->records[h->count - 1].hpwl == final,
          "%s: record cuoi %.6f != gia tri tra ve %.6f", tag,
          h->records[h->count - 1].hpwl, final);
    CHECK(final <= hpwl0, "%s: HPWL cuoi %.6f > ban dau %.6f", tag, final, hpwl0);

    /* best_index: iteration co HPWL nho nhat, hoa lay som nhat (tu tinh doc lap) */
    int best = 0;
    for (int i = 1; i < h->count; i++)
        if (h->records[i].hpwl < h->records[best].hpwl) best = i;
    CHECK(h->best_index == best, "%s: best_index = %d, ky vong %d", tag, h->best_index, best);
    CHECK(report_find_best(h) == best, "%s: report_find_best = %d, ky vong %d",
          tag, report_find_best(h), best);

    /* Dieu kien dung: iteration cuoi khong cai thien (HPWL bang iteration truoc)
       HOAC da du MAX_ITER */
    int stopped_ok = (h->count == MAX_ITER + 1) ||
                     (h->records[h->count - 1].hpwl == h->records[h->count - 2].hpwl);
    CHECK(stopped_ok, "%s: dung khi con dang cai thien (count = %d)", tag, h->count);

    /* Cac iteration truoc iteration cuoi phai cai thien that su (da giu >= 1 nuoc di) */
    int strict = 1;
    for (int i = 1; i + 1 < h->count; i++)
        if (!(h->records[i].hpwl < h->records[i - 1].hpwl)) strict = 0;
    CHECK(strict, "%s: co iteration giua chung khong cai thien nhung van tiep tuc", tag);
}

/* ---------- database nho dung tay ----------
   Board 20 x 10. A (id 0) va B (id 1) kich thuoc 2x2, da dat; C (id 2) chua dat.
   Net 1 noi pad cua A, B, C; pad A dx = 0, pad B dx = pad_dx_b, pad C o (999, 999). */
static void make_small(database *db, float ax, float bx, float by, float pad_dx_b)
{
    memset(db, 0, sizeof *db);
    db->board.x_start = 0; db->board.y_start = 0;
    db->board.x_end = 20;  db->board.y_end = 10;
    db->num_comps = 3;
    strcpy(db->comps[0].ref, "A");
    db->comps[0].x = ax; db->comps[0].y = 5; db->comps[0].width = 2; db->comps[0].height = 2;
    db->comps[0].placed = 1;
    strcpy(db->comps[1].ref, "B");
    db->comps[1].x = bx; db->comps[1].y = by; db->comps[1].width = 2; db->comps[1].height = 2;
    db->comps[1].placed = 1;
    strcpy(db->comps[2].ref, "C");
    db->comps[2].x = 999; db->comps[2].y = 999; db->comps[2].width = 2; db->comps[2].height = 2;
    db->comps[2].placed = 0;
    db->num_pads = 3;
    for (int i = 0; i < 3; i++) {
        db->pads[i].pad = 1; db->pads[i].net_id = 1; db->pads[i].comp_idx = i;
        strcpy(db->pads[i].ref, db->comps[i].ref);
    }
    db->pads[1].dx = pad_dx_b;
}

static void test_try_move(void)
{
    printf("[1] try_move tren database dung tay\n");

    /* a) nuoc di cai thien + hop le -> giu, dich dung (dx, dy) */
    make_small(&g_small, 5, 15, 5, 0);
    CHECK(total_hpwl(&g_small) == 10.0f, "HPWL ban dau = %.3f (ky vong 10)", total_hpwl(&g_small));
    CHECK(try_move(&g_small, 0, 1.0f, 0.0f) == 1, "a) +x cai thien phai tra 1");
    CHECK(g_small.comps[0].x == 6.0f && g_small.comps[0].y == 5.0f, "a) toa do sau khi giu sai");
    CHECK(total_hpwl(&g_small) == 9.0f, "a) HPWL sau = %.3f (ky vong 9)", total_hpwl(&g_small));

    /* b) nuoc di lam HPWL tang -> hoan tac chinh xac (so sanh tung byte) */
    make_small(&g_small, 5, 15, 5, 0);
    {
        static database backup;
        memcpy(&backup, &g_small, sizeof backup);
        CHECK(try_move(&g_small, 0, -1.0f, 0.0f) == 0, "b) -x lam HPWL tang phai tra 0");
        CHECK(memcmp(&backup, &g_small, sizeof g_small) == 0, "b) database bi thay doi sau khi hoan tac");
    }

    /* c) HPWL bang nhau (khong "giam hon 1e-6") -> khong giu */
    make_small(&g_small, 5, 15, 5, 0);
    CHECK(try_move(&g_small, 0, 0.0f, 0.0f) == 0, "c) dich (0,0) phai tra 0");
    CHECK(try_move(&g_small, 0, 0.0f, 1.0f) == 0, "c) +y khong doi HPWL phai tra 0");
    CHECK(g_small.comps[0].x == 5.0f && g_small.comps[0].y == 5.0f, "c) toa do bi doi");

    /* d) huong y: B o y = 8, A dich +y lam HPWL giam 13 -> 12 */
    make_small(&g_small, 5, 15, 8, 0);
    CHECK(total_hpwl(&g_small) == 13.0f, "d) HPWL ban dau = %.3f (ky vong 13)", total_hpwl(&g_small));
    CHECK(try_move(&g_small, 0, 0.0f, 1.0f) == 1, "d) +y phai duoc giu");
    CHECK(g_small.comps[0].y == 6.0f, "d) y sau = %.3f", g_small.comps[0].y);

    /* e) cai thien HPWL nhung chong lan -> hoan tac. A (5) -> 14.5 chong B (15). */
    make_small(&g_small, 5, 15, 5, 0);
    {
        static database backup;
        memcpy(&backup, &g_small, sizeof backup);
        CHECK(try_move(&g_small, 0, 9.5f, 0.0f) == 0, "e) chong lan phai bi tu choi");
        CHECK(memcmp(&backup, &g_small, sizeof g_small) == 0, "e) khong hoan tac chinh xac");
    }
    /* e2) khoang cach vua du CLEARANCE (A -> 12.7, gap 0.3) thi duoc; qua gan (12.9, gap 0.1) thi khong */
    make_small(&g_small, 5, 15, 5, 0);
    CHECK(try_move(&g_small, 0, 7.9f, 0.0f) == 0, "e2) gap 0.1 < CLEARANCE phai bi tu choi");
    CHECK(try_move(&g_small, 0, 7.7f, 0.0f) == 1, "e2) gap 0.3 >= CLEARANCE phai duoc giu");

    /* f) cai thien HPWL nhung ra ngoai board -> hoan tac.
          A (19,5) chi cham bien phai; pad B o x = 5 + 20 = 25 nen A dich +x se giam HPWL 6 -> 5. */
    make_small(&g_small, 19, 5, 5, 20);
    CHECK(total_hpwl(&g_small) == 6.0f, "f) HPWL ban dau = %.3f (ky vong 6)", total_hpwl(&g_small));
    CHECK(try_move(&g_small, 0, 1.0f, 0.0f) == 0, "f) ra ngoai board phai bi tu choi");
    CHECK(g_small.comps[0].x == 19.0f, "f) A.x = %.3f sau hoan tac", g_small.comps[0].x);

    /* g) linh kien placed == 0: khong dich, khong doi placed */
    make_small(&g_small, 5, 15, 5, 0);
    CHECK(total_hpwl(&g_small) == 10.0f, "g) pad cua linh kien chua dat phai bi bo qua");
    CHECK(try_move(&g_small, 2, 1.0f, 0.0f) == 0, "g) linh kien chua dat phai tra 0");
    CHECK(g_small.comps[2].x == 999.0f && g_small.comps[2].y == 999.0f && g_small.comps[2].placed == 0,
          "g) linh kien chua dat bi thay doi");

    /* h) chi so khong hop le */
    CHECK(try_move(&g_small, -1, 1.0f, 0.0f) == 0, "h) chi so -1");
    CHECK(try_move(&g_small, 3, 1.0f, 0.0f) == 0, "h) chi so == num_comps");

    /* i) cac co placed khong bi dong cham sau chuoi thu tren */
    CHECK(g_small.comps[0].placed == 1 && g_small.comps[1].placed == 1 && g_small.comps[2].placed == 0,
          "i) co placed bi thay doi");
}

static void test_trivial_cases(void)
{
    printf("[2] hill_climb tren truong hop tam thuong\n");

    /* Khong co nuoc di nao giu duoc: A, B cach nhau gap = 0.5, moi buoc +-1 lai gan
       lam chong lan (DRC chan) hoac ra xa (HPWL tang), dich y khong doi HPWL */
    make_small(&g_small, 5, 7.5f, 5, 0);
    history_init(&g_hist);
    float hp0 = total_hpwl(&g_small);
    float fin = hill_climb(&g_small, &g_hist);
    CHECK(g_hist.count == 2, "dung ngay iteration 1: count = %d (ky vong 2)", g_hist.count);
    CHECK(fin == hp0, "khong cai thien thi HPWL giu nguyen (%.3f -> %.3f)", hp0, fin);
    check_history(&g_hist, hp0, fin, "trivial");

    /* hist == NULL khong lam hong chuong trinh va van toi uu */
    make_small(&g_small, 5, 15, 5, 0);
    float fin_null = hill_climb(&g_small, NULL);
    make_small(&g_small, 5, 15, 5, 0);
    history_init(&g_hist);
    float fin_hist = hill_climb(&g_small, &g_hist);
    CHECK(fin_null == fin_hist, "hist NULL (%.3f) khac co hist (%.3f)", fin_null, fin_hist);

    /* hill_climb KHONG goi history_init: record co san phai con nguyen */
    make_small(&g_small, 5, 15, 5, 0);
    history_init(&g_hist);
    report_log_iter(&g_hist, -7, 12345.0f);
    hill_climb(&g_small, &g_hist);
    CHECK(g_hist.records[0].iteration == -7 && g_hist.records[0].hpwl == 12345.0f,
          "record co san bi ghi de (hill_climb goi history_init?)");
    CHECK(g_hist.records[1].iteration == 0, "iteration 0 phai ghi ngay sau record co san");

    /* Sau hill_climb tren db nho: DRC van dat, linh kien chua dat khong bi dong cham */
    CHECK(drc_check_all(&g_small) == 1, "DRC sau hill_climb tren db nho");
    CHECK(g_small.comps[2].placed == 0 && g_small.comps[2].x == 999.0f, "linh kien chua dat bi dong cham");
}

static void test_max_iter(void)
{
    printf("[3] hill_climb dung o MAX_ITER (%d)\n", MAX_ITER);

    /* Board 3000 x 10, A o dau trai, B o dau phai; moi iteration A, B tien lai nhau 1 buoc
       moi ben -> can ~1500 iteration, nen bi chan boi MAX_ITER. */
    memset(&g_small, 0, sizeof g_small);
    g_small.board.x_end = 3000; g_small.board.y_end = 10;
    g_small.num_comps = 2;
    strcpy(g_small.comps[0].ref, "A");
    g_small.comps[0].x = 1;    g_small.comps[0].y = 5;
    g_small.comps[0].width = 2; g_small.comps[0].height = 2; g_small.comps[0].placed = 1;
    strcpy(g_small.comps[1].ref, "B");
    g_small.comps[1].x = 2999; g_small.comps[1].y = 5;
    g_small.comps[1].width = 2; g_small.comps[1].height = 2; g_small.comps[1].placed = 1;
    g_small.num_pads = 2;
    for (int i = 0; i < 2; i++) {
        g_small.pads[i].net_id = 1; g_small.pads[i].comp_idx = i; g_small.pads[i].pad = 1;
        strcpy(g_small.pads[i].ref, g_small.comps[i].ref);
    }

    float hp0 = total_hpwl(&g_small);
    history_init(&g_hist);
    float fin = hill_climb(&g_small, &g_hist);

    CHECK(hp0 == 2998.0f, "HPWL ban dau = %.3f (ky vong 2998)", hp0);
    CHECK(g_hist.count == MAX_ITER + 1, "count = %d (ky vong %d)", g_hist.count, MAX_ITER + 1);
    CHECK(g_hist.records[g_hist.count - 1].iteration == MAX_ITER, "iteration cuoi != MAX_ITER");
    CHECK(fin == 2998.0f - 2.0f * MAX_ITER, "HPWL cuoi = %.3f (ky vong %.3f)", fin, 2998.0f - 2.0f * MAX_ITER);
    CHECK(drc_check_all(&g_small) == 1, "DRC sau MAX_ITER vong");
    check_history(&g_hist, hp0, fin, "max_iter");
    printf("    HPWL %.1f -> %.1f sau %d iteration\n", hp0, fin, MAX_ITER);
}

#ifdef WITH_DRC_PROBE
static void test_move_order(void)
{
    printf("[4] thu tu nuoc di: linh kien tang dan chi so, moi linh kien +x, -x, +y, -y\n");

    /* A (5,5), B (7.5,5) cach nhau 0.5: khong nuoc nao duoc giu -> 1 iteration duy nhat,
       moi nuoc deu thu tu vi tri goc. C chua dat phai bi bo qua. */
    make_small(&g_small, 5, 7.5f, 5, 0);
    const int   want_idx[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };
    const float want_x[8]   = { 6, 4, 5, 5,  8.5f, 6.5f, 7.5f, 7.5f };
    const float want_y[8]   = { 5, 5, 6, 4,  5, 5, 6, 4 };

    mock_drc_probe_n = 0;
    mock_drc_probe_on = 1;
    history_init(&g_hist);
    hill_climb(&g_small, &g_hist);
    mock_drc_probe_on = 0;

    CHECK(g_hist.count == 2, "count = %d (ky vong 2)", g_hist.count);
    CHECK(mock_drc_probe_n == 8, "so nuoc di da thu = %d (ky vong 8)", mock_drc_probe_n);
    for (int i = 0; i < 8 && i < mock_drc_probe_n; i++)
        CHECK(mock_drc_probe_idx[i] == want_idx[i] &&
              mock_drc_probe_x[i] == want_x[i] && mock_drc_probe_y[i] == want_y[i],
              "nuoc thu %d: linh kien %d -> (%.2f, %.2f), ky vong linh kien %d -> (%.2f, %.2f)",
              i, mock_drc_probe_idx[i], mock_drc_probe_x[i], mock_drc_probe_y[i],
              want_idx[i], want_x[i], want_y[i]);
}
#endif

static int run_one_seed(unsigned seed, int verbose)
{
    memcpy(&g_work, &g_snap, sizeof g_work);        /* khoi phuc tu ban chup */
    history_init(&g_hist);
    srand(seed);
    if (place_all(&g_work) != STATUS_OK) {
        printf("  seed %u: place_all that bai (bo qua)\n", seed);
        return 0;
    }
    memcpy(&g_placed, &g_work, sizeof g_placed);

    float hp0 = total_hpwl(&g_work);
    CHECK(drc_check_all(&g_work) == 1, "seed %u: diem xuat phat vi pham DRC", seed);

    float fin = hill_climb(&g_work, &g_hist);

    char tag[32];
    snprintf(tag, sizeof tag, "seed %u", seed);
    check_history(&g_hist, hp0, fin, tag);
    CHECK(fin == total_hpwl(&g_work), "%s: gia tri tra ve != total_hpwl(db)", tag);
    CHECK(drc_check_all(&g_work) == 1, "%s: vi pham drc_check_all sau hill_climb", tag);

    for (int i = 0; i < g_work.num_comps; i++) {
        CHECK(g_work.comps[i].placed == g_placed.comps[i].placed, "%s: co placed cua %s bi doi",
              tag, g_work.comps[i].ref);
        CHECK(drc_check_component(&g_work, i) == 1 || g_work.comps[i].placed == 0,
              "%s: %s vi pham DRC", tag, g_work.comps[i].ref);
    }

    /* Chay lai tren trang thai da toi uu: da la cuc tieu cuc bo -> 2 record, HPWL khong doi */
    static history again;
    history_init(&again);
    float fin2 = hill_climb(&g_work, &again);
    CHECK(again.count == 2 && fin2 == fin, "%s: chay lai tren cuc tieu: count=%d, %.3f vs %.3f",
          tag, again.count, fin2, fin);

    if (verbose)
        printf("  seed %2u: dat xong %8.3f -> hill climbing %8.3f  (%d iteration, best_index %d)\n",
               seed, hp0, fin, g_hist.count - 1, g_hist.best_index);
    return 1;
}

static void test_real_data(void)
{
    printf("[5] hill_climb tren du lieu that (%d lan chay doc lap)\n", TEST_SEEDS);

    memset(&g_snap, 0, sizeof g_snap);
    if (load_all(&g_snap) != STATUS_OK) {
        printf("  [FAIL] khong doc duoc input (chay tu thu muc goc project?)\n");
        g_fails++; g_checks++;
        return;
    }
    printf("  %d linh kien, %d pad, %d net\n", g_snap.num_comps, g_snap.num_pads,
           get_num_nets(&g_snap));

    int ran = 0;
    float best = FLT_MAX;
    int   best_seed = -1;
    int   ref_seed = -1;

    for (unsigned seed = 1; seed <= TEST_SEEDS; seed++) {
        if (!run_one_seed(seed, 1)) continue;
        ran++;
        float fin = g_hist.records[g_hist.count - 1].hpwl;
        if (fin < best) { best = fin; best_seed = (int)seed; }

        if (ref_seed < 0) {                 /* luu ket qua seed dau lam tham chieu */
            ref_seed = (int)seed;
            memcpy(&g_ref_db, &g_work, sizeof g_ref_db);
            memcpy(&g_ref_placed, &g_placed, sizeof g_ref_placed);
            memcpy(&g_ref_hist, &g_hist, sizeof g_ref_hist);
        }
    }
    CHECK(ran > 0, "khong co lan chay nao dat duoc");
    printf("  tot nhat: seed %d, HPWL %.3f (%d/%d lan chay hop le)\n", best_seed, best, ran, TEST_SEEDS);

    /* Khong phu thuoc lan goi truoc: sau ~30 lan chay khac, hill_climb lai tu trang thai
       dat cua seed dau voi history_init moi phai cho dung ket qua cu (khong co trang thai ngam) */
    if (ref_seed >= 0) {
        memcpy(&g_work, &g_ref_placed, sizeof g_work);
        history_init(&g_hist);
        float fin = hill_climb(&g_work, &g_hist);
        CHECK(same_history(&g_hist, &g_ref_hist), "chay lai tu trang thai khoi phuc: history khac lan dau");
        CHECK(same_positions(&g_work, &g_ref_db), "chay lai tu trang thai khoi phuc: vi tri khac lan dau");
        CHECK(fin == g_ref_hist.records[g_ref_hist.count - 1].hpwl, "chay lai: HPWL tra ve khac");
        CHECK(g_hist.count >= 2 && drc_check_all(&g_work) == 1, "chay lai: history/DRC khong hop le");
        printf("  chay lai tu trang thai khoi phuc cua seed %d: ket qua giong het lan dau\n", ref_seed);
    }
}

int main(void)
{
    printf("=== TEST hill_climb (tv4) ===\n");
    test_try_move();
    test_trivial_cases();
    test_max_iter();
#ifdef WITH_DRC_PROBE
    test_move_order();
#else
    printf("[4] thu tu nuoc di: BO QUA (can bien dich voi -DWITH_DRC_PROBE + tests/mock_drc.c)\n");
#endif
    test_real_data();

    printf("=== %d kiem tra, %d that bai => %s ===\n", g_checks, g_fails,
           g_fails == 0 ? "PASS" : "FAIL");
    return g_fails == 0 ? 0 : 1;
}

/* BIEN DICH (tu thu muc goc project)
   a) Voi mock (khi hpwl/drc/placement/input/report trong src/ van la stub):
      gcc -Wall -Wextra -std=c99 -Iinclude -DWITH_DRC_PROBE tests/test_hill_climb.c src/hill_climb.c \
          tests/mock_input.c tests/mock_hpwl.c tests/mock_drc.c tests/mock_placement.c \
          tests/mock_report.c -o test_hill_climb -lm && ./test_hill_climb
   b) Khi ca nhom da xong that (khong dung mock): thay cac mock_*.c bang
      src/input.c src/hpwl.c src/drc.c src/placement.c src/report.c (khong lay src/main.c)
      va bo -DWITH_DRC_PROBE (test thu tu nuoc di se bi bo qua vi can probe trong mock_drc.c). */
