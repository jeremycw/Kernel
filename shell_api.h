#ifndef SHELL_API_H
#define SHELL_API_H

#define QUAD_WIDTH 38
#define QUAD_HEIGHT 23

struct quad
{
    char quad_buf[QUAD_HEIGHT][QUAD_WIDTH];
    int idx_col;
    int idx_row;
};

char sh_getchar();
struct quad* sh_getquad();
void sh_cls();

#endif
