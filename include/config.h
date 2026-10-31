#ifndef CONFIG_H
#define CONFIG_H

/* ===== Ma tra ve chung cho moi ham (quy uoc ca nhom) ===== */
#define STATUS_OK    0
#define STATUS_ERR  (-1)

/* ===== Gioi han kich thuoc mang ===== */
#define MAX_COMPONENTS  200
#define MAX_PADS        2000
#define MAX_REF_LEN     32      /* do dai toi da cua ref, vd "R1", "U3" */
#define MAX_LINE_LEN    256     /* do dai toi da 1 dong CSV */

/* ===== Duong dan input / output (chay tu thu muc goc cua project) ===== */
#define BOARD_FILE       "input_data/board.csv"
#define COMPONENTS_FILE  "input_data/components.csv"
#define PAD_FILE         "input_data/pad.csv"
#define REPORT_FILE      "output/report.txt"
#define RESULT_FILE      "output/placement_result.csv"

/* ===== Tham so thuat toan ===== */
#define MAX_ITER   1000         /* so vong lap toi da cua hill climbing */
#define STEP       1.0f         /* buoc dich moi lan thu */
#define CLEARANCE  0.2f         /* khoang cach toi thieu giua 2 linh kien */

#endif
