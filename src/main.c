/* Dieu phoi: input -> dat ban dau -> hill climbing -> report */
#include "database.h"
#include "input.h"
#include "hpwl.h"
#include "drc.h"
#include "placement.h"
#include "report.h"

static database db;     /* static: database ~114 KB, tranh tran stack */
static history  hist;

int main(void)
{
    srand((unsigned)time(NULL));
    history_init(&hist);

    if (load_all(&db) != STATUS_OK) {
        fprintf(stderr, "Loi: khong doc duoc du lieu input\n");
        return 1;
    }

    if (place_all(&db) != STATUS_OK) {
        fprintf(stderr, "Loi: khong dat duoc het linh kien len board\n");
        return 1;
    }
    printf("HPWL sau khi dat ban dau: %.3f\n", total_hpwl(&db));

    float final_hpwl = hill_climb(&db, &hist);
    printf("HPWL sau hill climbing:  %.3f\n", final_hpwl);

    if (!drc_check_all(&db))
        fprintf(stderr, "Canh bao: ket qua vi pham DRC\n");

    if (report_write(REPORT_FILE, &hist) != STATUS_OK ||
        report_write_placement(RESULT_FILE, &db) != STATUS_OK) {
        fprintf(stderr, "Loi: khong ghi duoc output\n");
        return 1;
    }
    return 0;
}
