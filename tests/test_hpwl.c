#include "hpwl.h"

static int check(const char *name, float actual, float expected)
{
    int pass = fabsf(actual - expected) < 1e-6f;
    printf("%s: %s (actual=%.3f, expected=%.3f)\n",
           pass ? "PASS" : "FAIL", name, actual, expected);
    return pass ? 0 : 1;
}

int main(void)
{
    database db = {0};
    float x, y;
    int failures = 0;
    int i;

    db.num_comps = 3;
    db.comps[0] = (component){"A", 10, 20, 2, 2, 1};
    db.comps[1] = (component){"B", 16, 27, 2, 2, 1};
    db.comps[2] = (component){"C", 8, 30, 2, 2, 1};
    db.num_pads = 2;
    db.pads[0] = (pad_info){"A", 1, 1, 1, -2, 0};
    db.pads[1] = (pad_info){"B", 1, 1, -2, 1, 1};

    /* Pad A = (11,18), B = (14,28): HPWL = 3 + 10. */
    pad_abs_position(&db, 0, &x, &y);
    failures += check("pad_abs_position x", x, 11);
    failures += check("pad_abs_position y", y, 18);
    failures += check("1 net 2 pad", net_hpwl(&db, 1), 13);
    failures += check("total 1 net", total_hpwl(&db), 13);
    failures += check("max net_id + 1", get_num_nets(&db), 2);
    failures += check("net khong co pad", net_hpwl(&db, 0), 0);

    /* Pad C = (7,30): HPWL = (14-7) + (30-18) = 19. */
    db.num_pads = 3;
    db.pads[2] = (pad_info){"C", 1, 1, -1, 0, 2};
    failures += check("net 3 pad", net_hpwl(&db, 1), 19);

    db.comps[2].placed = 0;
    db.comps[2].x = db.comps[2].y = 999;
    failures += check("bo qua placed 0", net_hpwl(&db, 1), 13);
    db.comps[1].placed = 0;
    failures += check("chi con 1 pad hop le", net_hpwl(&db, 1), 0);
    db.comps[0].placed = 0;
    failures += check("khong con pad hop le", total_hpwl(&db), 0);
    failures += check("so net khi chua dat", get_num_nets(&db), 2);

    /* Hai net 0 va 4 cung noi A, B: tong = 13 + 13. */
    db.comps[0].placed = db.comps[1].placed = 1;
    db.pads[0].net_id = db.pads[1].net_id = 0;
    db.pads[2] = (pad_info){"A", 2, 4, 1, -2, 0};
    db.pads[3] = (pad_info){"B", 2, 4, -2, 1, 1};
    db.num_pads = 4;
    failures += check("net_id cach quang", get_num_nets(&db), 5);
    failures += check("tong nhieu net gom net 0", total_hpwl(&db), 26);

    /* Dich ca hai linh kien: HPWL khong doi, du toa do am. */
    db.comps[0].x -= 100; db.comps[0].y -= 100;
    db.comps[1].x -= 100; db.comps[1].y -= 100;
    failures += check("toa do am", total_hpwl(&db), 26);

    /* Doi vi tri roi khoi phuc de bat loi luu ket qua cu. */
    for (i = 0; i < 1000; i++) {
        db.comps[1].x += 1;
        if (fabsf(total_hpwl(&db) - 28) >= 1e-6f) break;
        db.comps[1].x -= 1;
        if (fabsf(total_hpwl(&db) - 26) >= 1e-6f) break;
    }
    failures += check("1000 lan doi va khoi phuc", i, 1000);

    db.num_pads = 0;
    failures += check("khong pad: so net", get_num_nets(&db), 0);
    failures += check("khong pad: net HPWL", net_hpwl(&db, 0), 0);
    failures += check("khong pad: total", total_hpwl(&db), 0);

    printf("Tong ket: %d ca FAIL\n", failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
