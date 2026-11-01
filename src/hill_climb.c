/* [tv4] Hill Climbing: thu dich linh kien, giu nuoc di neu HPWL giam va DRC dat */
#include "placement.h"
#include "hpwl.h"
#include "drc.h"

/* Muc giam toi thieu de coi la "HPWL giam" (theo hop dong chung).
   Tranh giu nuoc di chi vi sai so lam tron cua float. */
#define HPWL_EPS 1e-6

int try_move(database *db, int comp_idx, float dx, float dy)
{
    if (db == NULL || comp_idx < 0 || comp_idx >= db->num_comps)
        return 0;

    component *c = &db->comps[comp_idx];
    if (c->placed != 1)         /* HPWL/DRC bo qua linh kien chua dat -> khong dich */
        return 0;

    float old_hpwl = total_hpwl(db);
    /* Luu toa do cu de hoan tac CHINH XAC: gan lai gia tri da luu,
       khong tru dx, dy (cong roi tru voi float co the lech vai ulp). */
    float old_x = c->x;
    float old_y = c->y;

    c->x = old_x + dx;
    c->y = old_y + dy;

    /* DRC kiem tra truoc (re hon); && dam bao total_hpwl chi tinh khi DRC dat */
    if (drc_check_component(db, comp_idx) == 1 &&
        total_hpwl(db) < old_hpwl - HPWL_EPS)
        return 1;

    c->x = old_x;
    c->y = old_y;
    return 0;
}

float hill_climb(database *db, history *hist)
{
    /* Thu tu 4 huong theo hop dong: +x, -x, +y, -y */
    const float step_x[4] = { STEP, -STEP, 0.0f, 0.0f };
    const float step_y[4] = { 0.0f, 0.0f, STEP, -STEP };

    float cur = total_hpwl(db);
    if (hist != NULL)
        report_log_iter(hist, 0, cur);      /* iteration 0 = HPWL ban dau */

    for (int k = 1; k <= MAX_ITER; k++) {
        int kept = 0;                        /* so nuoc di duoc giu trong iteration nay */

        for (int i = 0; i < db->num_comps; i++) {
            if (db->comps[i].placed != 1)    /* bo qua som, do goi total_hpwl vo ich */
                continue;
            for (int d = 0; d < 4; d++)
                kept += try_move(db, i, step_x[d], step_y[d]);
        }

        /* Tinh lai tu dau thay vi cong don: moi nuoc di giu deu giam HPWL,
           nen day cung la gia tri nho nhat tu truoc toi gio (khong bao gio tang). */
        cur = total_hpwl(db);
        if (hist != NULL)
            report_log_iter(hist, k, cur);

        if (kept == 0)                       /* khong cai thien -> cuc tieu cuc bo, dung */
            break;
    }
    return cur;
}