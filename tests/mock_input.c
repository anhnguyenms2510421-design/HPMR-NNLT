/* MOCK cua [tv1] input.c - doc 3 file CSV that de test hill_climb tren du lieu that.
   Xu ly BOM UTF-8, CRLF, dong trong. KHONG nop vao src/. */
#include "input.h"

/* Doc 1 dong "du lieu" (bo qua dong trong, bo BOM o dau file, bo CRLF).
   Tra ve 1 neu doc duoc, 0 neu het file. */
static int read_data_line(FILE *f, char *buf, int size)
{
    while (fgets(buf, size, f) != NULL) {
        unsigned char *u = (unsigned char *)buf;
        if (u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF)
            memmove(buf, buf + 3, strlen(buf + 3) + 1);
        size_t n = strlen(buf);
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
            buf[--n] = '\0';
        if (n > 0) return 1;
    }
    return 0;
}

int load_board(const char *path, database *db)
{
    FILE *f = fopen(path, "r");
    char line[MAX_LINE_LEN];
    if (f == NULL) { fprintf(stderr, "Loi: khong mo duoc %s\n", path); return STATUS_ERR; }
    int ok = read_data_line(f, line, sizeof line) &&      /* header */
             read_data_line(f, line, sizeof line) &&
             sscanf(line, "%f,%f,%f,%f", &db->board.x_start, &db->board.y_start,
                    &db->board.x_end, &db->board.y_end) == 4;
    fclose(f);
    return ok ? STATUS_OK : STATUS_ERR;
}

int load_components(const char *path, database *db)
{
    FILE *f = fopen(path, "r");
    char line[MAX_LINE_LEN];
    if (f == NULL) { fprintf(stderr, "Loi: khong mo duoc %s\n", path); return STATUS_ERR; }
    db->num_comps = 0;
    read_data_line(f, line, sizeof line);                 /* bo header */
    while (read_data_line(f, line, sizeof line)) {
        if (db->num_comps >= MAX_COMPONENTS) { fclose(f); return STATUS_ERR; }
        component *c = &db->comps[db->num_comps];
        if (sscanf(line, "%31[^,],%f,%f,%f,%f,%d", c->ref, &c->x, &c->y,
                   &c->width, &c->height, &c->placed) != 6) { fclose(f); return STATUS_ERR; }
        db->num_comps++;
    }
    fclose(f);
    return STATUS_OK;
}

int load_pads(const char *path, database *db)
{
    FILE *f = fopen(path, "r");
    char line[MAX_LINE_LEN];
    if (f == NULL) { fprintf(stderr, "Loi: khong mo duoc %s\n", path); return STATUS_ERR; }
    db->num_pads = 0;
    read_data_line(f, line, sizeof line);                 /* bo header */
    while (read_data_line(f, line, sizeof line)) {
        if (db->num_pads >= MAX_PADS) { fclose(f); return STATUS_ERR; }
        pad_info *p = &db->pads[db->num_pads];
        if (sscanf(line, "%31[^,],%d,%d,%f,%f", p->ref, &p->pad, &p->net_id,
                   &p->dx, &p->dy) != 5) { fclose(f); return STATUS_ERR; }
        p->comp_idx = -1;
        for (int i = 0; i < db->num_comps; i++)
            if (strcmp(db->comps[i].ref, p->ref) == 0) { p->comp_idx = i; break; }
        if (p->comp_idx < 0) { fclose(f); return STATUS_ERR; }
        db->num_pads++;
    }
    fclose(f);
    return STATUS_OK;
}

int load_all(database *db)
{
    if (load_board(BOARD_FILE, db) != STATUS_OK)           return STATUS_ERR;
    if (load_components(COMPONENTS_FILE, db) != STATUS_OK) return STATUS_ERR;
    if (load_pads(PAD_FILE, db) != STATUS_OK)              return STATUS_ERR;
    return STATUS_OK;
}
