/* [tv1] Doc board.csv, components.csv, pad.csv va luu vao database */
#include "input.h"

/* ---------- Ham phu (static, chi dung trong file nay) ---------- */

/* Chuoi chi gom khoang trang / tab / \r / \n (hoac rong) */
static int only_blank(const char *s)
{
    while (*s) {
        if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') return 0;
        s++;
    }
    return 1;
}

/* Doc dong ke tiep KHONG rong vao buf (da cat \r \n o cuoi).
   Tra ve 1 = co dong, 0 = het file, -1 = dong dai hon MAX_LINE_LEN.
   Dong rong bi bo qua o day de moi ham load_* khong phai tu xu ly. */
static int next_line(FILE *fp, char *buf, int size)
{
    while (fgets(buf, size, fp) != NULL) {
        size_t len = strlen(buf);

        if (len > 0 && buf[len - 1] != '\n' && len == (size_t)size - 1) {
            /* Buffer day ma chua gap '\n': dong qua dai, tru khi day la
               dong cuoi khong co newline (ky tu ke tiep la EOF). */
            if (fgetc(fp) != EOF) return -1;
        }
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';

        if (!only_blank(buf)) return 1;
    }
    return 0;
}

/* Mo file, bo BOM UTF-8 (neu co) va bo dong header.
   Thanh cong: *fp_out la file da doc xong header. Loi: tra STATUS_ERR,
   file da duoc dong. */
static int open_csv(const char *path, FILE **fp_out, char *buf, int size)
{
    FILE *fp;
    int c0, c1, c2, r;

    if (path == NULL) {
        fprintf(stderr, "Loi: duong dan file la NULL\n");
        return STATUS_ERR;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Loi: khong mo duoc file '%s'\n", path);
        return STATUS_ERR;
    }

    /* BOM = EF BB BF. Khong phai BOM thi tua lai dau file de khong mat du lieu. */
    c0 = fgetc(fp); c1 = fgetc(fp); c2 = fgetc(fp);
    if (!(c0 == 0xEF && c1 == 0xBB && c2 == 0xBF))
        rewind(fp);

    r = next_line(fp, buf, size);   /* dong dau tien khong rong = header */
    if (r != 1) {
        fprintf(stderr, r == 0 ? "Loi: file '%s' rong (thieu header)\n"
                               : "Loi: header file '%s' dai qua %d ky tu\n",
                path, MAX_LINE_LEN);
        fclose(fp);
        return STATUS_ERR;
    }
    *fp_out = fp;
    return STATUS_OK;
}

/* ---------- 4 ham theo hop dong trong input.h ---------- */

int load_board(const char *path, database *db)
{
    FILE *fp = NULL;
    char  line[MAX_LINE_LEN];
    float x0, y0, x1, y1;
    int   n = -1, r;

    if (db == NULL) return STATUS_ERR;

    if (open_csv(path, &fp, line, MAX_LINE_LEN) != STATUS_OK) return STATUS_ERR;

    r = next_line(fp, line, MAX_LINE_LEN);
    if (r != 1) {
        fprintf(stderr, r == 0 ? "Loi: '%s' khong co dong du lieu board\n"
                               : "Loi: dong trong '%s' dai qua %d ky tu\n",
                path, MAX_LINE_LEN);
        fclose(fp);
        return STATUS_ERR;
    }
    /* %n ghi vi tri da doc; chi duoc ghi khi du 4 truong -> dung de bat thua cot */
    if (sscanf(line, "%f,%f,%f,%f%n", &x0, &y0, &x1, &y1, &n) != 4
        || n < 0 || !only_blank(line + n)) {
        fprintf(stderr, "Loi: '%s' sai dinh dang board (can x_min,y_min,x_max,y_max): \"%s\"\n",
                path, line);
        fclose(fp);
        return STATUS_ERR;
    }
    if (x1 <= x0 || y1 <= y0) {
        fprintf(stderr, "Loi: '%s' board khong hop le (x_max <= x_min hoac y_max <= y_min)\n", path);
        fclose(fp);
        return STATUS_ERR;
    }
    /* Hop dong: board chi co 1 dong du lieu; thua dong thi coi la file sai */
    if (next_line(fp, line, MAX_LINE_LEN) != 0) {
        fprintf(stderr, "Loi: '%s' co nhieu hon 1 dong du lieu board\n", path);
        fclose(fp);
        return STATUS_ERR;
    }
    fclose(fp);

    db->board.x_start = x0;
    db->board.y_start = y0;
    db->board.x_end   = x1;
    db->board.y_end   = y1;
    return STATUS_OK;
}

int load_components(const char *path, database *db)
{
    FILE *fp = NULL;
    char  line[MAX_LINE_LEN];
    char  fmt[64];
    int   r;

    if (db == NULL) return STATUS_ERR;
    db->num_comps = 0;                      /* khoi tao dau ham theo yeu cau */

    /* Do rong %s lay tu MAX_REF_LEN de khong bao gio tran ref[] */
    snprintf(fmt, sizeof fmt, "%%%d[^,],%%f,%%f,%%f,%%f,%%d%%n", MAX_REF_LEN - 1);

    if (open_csv(path, &fp, line, MAX_LINE_LEN) != STATUS_OK) return STATUS_ERR;

    while ((r = next_line(fp, line, MAX_LINE_LEN)) == 1) {
        component *c;
        int n = -1;

        if (db->num_comps >= MAX_COMPONENTS) {
            fprintf(stderr, "Loi: '%s' vuot MAX_COMPONENTS (%d)\n", path, MAX_COMPONENTS);
            goto fail;
        }
        c = &db->comps[db->num_comps];
        if (sscanf(line, fmt, c->ref, &c->x, &c->y, &c->width, &c->height,
                   &c->placed, &n) != 6 || n < 0 || !only_blank(line + n)) {
            fprintf(stderr, "Loi: '%s' sai dinh dang component (can ref,x,y,width,height,is_placed; "
                            "ref < %d ky tu): \"%s\"\n", path, MAX_REF_LEN, line);
            goto fail;
        }
        db->num_comps++;
    }
    if (r < 0) {
        fprintf(stderr, "Loi: dong trong '%s' dai qua %d ky tu\n", path, MAX_LINE_LEN);
        goto fail;
    }
    if (db->num_comps == 0) {
        fprintf(stderr, "Loi: '%s' khong co linh kien nao\n", path);
        goto fail;
    }
    fclose(fp);
    return STATUS_OK;

fail:
    fclose(fp);
    db->num_comps = 0;                      /* khong de lai du lieu doc do */
    return STATUS_ERR;
}

int load_pads(const char *path, database *db)
{
    FILE *fp = NULL;
    char  line[MAX_LINE_LEN];
    char  fmt[64];
    int   r, i, j;

    if (db == NULL) return STATUS_ERR;
    db->num_pads = 0;                       /* khoi tao dau ham theo yeu cau */

    /* Can comps[] de tim comp_idx -> bat buoc goi sau load_components() */
    if (db->num_comps == 0) {
        fprintf(stderr, "Loi: load_pads can goi sau load_components (num_comps = 0)\n");
        return STATUS_ERR;
    }

    snprintf(fmt, sizeof fmt, "%%%d[^,],%%d,%%d,%%f,%%f%%n", MAX_REF_LEN - 1);

    if (open_csv(path, &fp, line, MAX_LINE_LEN) != STATUS_OK) return STATUS_ERR;

    while ((r = next_line(fp, line, MAX_LINE_LEN)) == 1) {
        pad_info *p;
        int n = -1;

        if (db->num_pads >= MAX_PADS) {
            fprintf(stderr, "Loi: '%s' vuot MAX_PADS (%d)\n", path, MAX_PADS);
            goto fail;
        }
        p = &db->pads[db->num_pads];
        if (sscanf(line, fmt, p->ref, &p->pad, &p->net_id, &p->dx, &p->dy, &n) != 5
            || n < 0 || !only_blank(line + n)) {
            fprintf(stderr, "Loi: '%s' sai dinh dang pad (can ref,pad,net_id,dx,dy; "
                            "ref < %d ky tu): \"%s\"\n", path, MAX_REF_LEN, line);
            goto fail;
        }
        /* Hop dong: net_id >= 0 (HPWL dung net_id lam chi so) */
        if (p->net_id < 0) {
            fprintf(stderr, "Loi: '%s' net_id am: \"%s\"\n", path, line);
            goto fail;
        }
        db->num_pads++;
    }
    if (r < 0) {
        fprintf(stderr, "Loi: dong trong '%s' dai qua %d ky tu\n", path, MAX_LINE_LEN);
        goto fail;
    }
    fclose(fp);
    fp = NULL;

    /* Dien comp_idx 1 lan duy nhat luc load, de HPWL khong phai strcmp lap lai */
    for (i = 0; i < db->num_pads; i++) {
        int found = -1;
        for (j = 0; j < db->num_comps; j++) {
            if (strcmp(db->pads[i].ref, db->comps[j].ref) == 0) { found = j; break; }
        }
        if (found < 0) {
            fprintf(stderr, "Loi: pad %d cua '%s' tham chieu linh kien khong ton tai\n",
                    db->pads[i].pad, db->pads[i].ref);
            db->num_pads = 0;
            return STATUS_ERR;
        }
        db->pads[i].comp_idx = found;
    }
    return STATUS_OK;

fail:
    if (fp != NULL) fclose(fp);
    db->num_pads = 0;
    return STATUS_ERR;
}

int load_all(database *db)
{
    if (db == NULL) return STATUS_ERR;
    db->num_comps = 0;                      /* load_all co the goi lai tren db cu */
    db->num_pads  = 0;

    if (load_board(BOARD_FILE, db) != STATUS_OK)           return STATUS_ERR;
    if (load_components(COMPONENTS_FILE, db) != STATUS_OK) return STATUS_ERR;
    if (load_pads(PAD_FILE, db) != STATUS_OK)              return STATUS_ERR;
    return STATUS_OK;
}