#ifndef SHELL_H
#define SHELL_H

#define MAXAPPS 8
#define SCREENS 2

#define SHELLSCRN 0

#define INVALIDAPP -1
#define NULL 0

#include "os.h"
#include "shell_api.h"

struct app
{
    char name[16];
    struct quad quad;
    int screen;
	int pid;
    int id;
    FIFO f;
};

struct shell
{
    struct app apps[MAXAPPS];
    struct app* screens[4];
    //struct app* under_shell;
    int focused;
	int toggled;
};

struct cmd
{
    char buf[32];
    int idx;
	char prev_cmds[16][32];
	int prev_idx;
	int prev_count;
};

extern struct shell shell;

void hdl_up(struct cmd* cmd);
void hdl_down(struct cmd* cmd);
void hdl_newline(struct cmd* cmd);
void shell_proc();
void handle_hot_key(char);
int get_empty_idx(struct app*);
int launch_app(void (*process)(void), int, const char*, int, int);
void parse_cmd(char*);
int get_screen_num(char*);
void swap_screen(struct app*, struct app*);
void put_app(struct app* app, int screen);
void ps();
struct app* get_app_from_pid(int pid);
void focus_next();
void add_prev_cmd(struct cmd* cmd);
int get_app_id(char* cmd);
void update_cmd(struct cmd* cmd, unsigned char key);
void init_shell_struct();
void handle_key(struct cmd* cmd, unsigned char key);

#endif
