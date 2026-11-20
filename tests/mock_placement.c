/* MOCK cua [tv3] placement.c (place_initial, place_component, place_all).
   Chi dung de tao diem xuat phat cho test hill_climb. KHONG nop vao src/.
   try_move / hill_climb KHONG co o day (nam trong src/hill_climb.c). */
#include "placement.h"
#include "hpwl.h"
#include "drc.h"

int place_initial(database *db)
{
    int n_free = 0;
    for (int i = 0; i < db->num_comps; i++)
        if (db->comps[i].placed == 0) n_free++;
    if (n_free == 0) return STATUS_ERR;

    int pick = rand() % n_free;
    int idx = -1;
    for (int i = 0; i < db->num_comps; i++) {
        if (db->comps[i].placed != 0) continue;
        if (pick == 0) { idx = i; break; }
        pick--;
    }
    db->comps[idx].x = (db->board.x_start + db->board.x_end) / 2;
    db->comps[idx].y = (db->board.y_start + db->board.y_end) / 2;
    if (!drc_check_component(db, idx)) return STATUS_ERR;
    db->comps[idx].placed = 1;
    return STATUS_OK;
}

int place_component(database *db, int comp_idx)
{
    component *c = &db->comps[comp_idx];
    float cx = (db->board.x_start + db->board.x_end) / 2;
    float cy = (db->board.y_start + db->board.y_end) / 2;
    int   found = 0;
    float best_h = 0, best_d2 = 0, best_x = 0, best_y = 0;

    c->placed = 1;      /* tam thoi = 1 de total_hpwl tinh pad cua chinh no */
    for (float y = db->board.y_start + c->height / 2;
         y <= db->board.y_end - c->height / 2 + 1e-4f; y += STEP) {
        for (float x = db->board.x_start + c->width / 2;
             x <= db->board.x_end - c->width / 2 + 1e-4f; x += STEP) {
            c->x = x; c->y = y;
            if (!drc_check_component(db, comp_idx)) continue;
            float h  = total_hpwl(db);
            float d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            if (!found || h < best_h - 1e-6f ||
                (fabsf(h - best_h) < 1e-6f && d2 < best_d2)) {
                found = 1; best_h = h; best_d2 = d2; best_x = x; best_y = y;
            }
        }
    }
    if (!found) { c->placed = 0; c->x = 999; c->y = 999; return STATUS_ERR; }
    c->x = best_x; c->y = best_y;
    return STATUS_OK;
}

int place_all(database *db)
{
    for (int attempt = 0; attempt < 20; attempt++) {
        int ok = (place_initial(db) == STATUS_OK);
        while (ok) {
            /* chon linh kien chua dat co dien tich lon nhat (hoa: chi so nho) */
            int pick = -1;
            for (int i = 0; i < db->num_comps; i++) {
                if (db->comps[i].placed != 0) continue;
                if (pick < 0 || db->comps[i].width * db->comps[i].height >
                                db->comps[pick].width * db->comps[pick].height)
                    pick = i;
            }
            if (pick < 0) return STATUS_OK;                /* da dat het */
            ok = (place_component(db, pick) == STATUS_OK);
        }
        for (int i = 0; i < db->num_comps; i++) {          /* that bai -> lam lai */
            db->comps[i].placed = 0;
            db->comps[i].x = 999; db->comps[i].y = 999;
        }
    }
    return STATUS_ERR;
}
