#ifndef CONFIG_H
#define CONFIG_H

/* ===== Giới hạn kích thước mảng ===== */
#define MAX_COMPONENTS  200
#define MAX_PADS        2000
#define MAX_REF_LEN     32      /* độ dài tối đa của ref, vd "R1", "U3" */
#define MAX_LINE_LEN    256     /* độ dài tối đa 1 dòng CSV */

/* ===== Đường dẫn input / output ===== */
#define BOARD_FILE       "input_data/board.csv"
#define COMPONENTS_FILE  "input_data/components.csv"
#define PAD_FILE         "input_data/pad.csv"
#define REPORT_FILE      "output/report.txt"
#define RESULT_FILE      "output/placement_result.csv"

/* ===== Tham số thuật toán ===== */
#define MAX_ITER   1000
#define STEP       1.0f         /* bước dịch mỗi lần thử */
#define CLEARANCE  0.2f         /* khoảng cách tối thiểu giữa 2 linh kiện */

#endif
