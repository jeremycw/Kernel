#include "shell.h"
#include "out.h"
#include "os.h"
#include "refresher.h"
#include "keyboard.h"
#include "apps.h"

struct shell shell;

void shell_proc()
{
    init_shell_struct();

    struct cmd cmd;
    unsigned int key;
    struct quad* quad = &shell.apps[0].quad;

    cmd.idx = 0;
	cmd.prev_idx = -1;
	cmd.prev_count = 0;

    int i,j;
	for(i = 0; i < 16; i++)
		for(j = 0; j < 32; j++)
			cmd.prev_cmds[i][j] = '\0';
	
    //init shell quad buf
    printf("badash $ ");
    quad->quad_buf[quad->idx_row][quad->idx_col] = '_';

    //spawn screen drawing process
    OS_Create(&refresher, NULL, DEVICE, 10);

	while(1)
    {
        key = getchar();
        if(shell.focused > 0) //shell not focused
        {
            if(!is_alt()) //not hotkey
                OS_Write(shell.apps[shell.focused].f, key); //write char to focused process fifo
            else
                handle_hot_key(key);
        }
        else //shell is focused
        {
            if(!is_alt()) //not hotkey
                handle_key(&cmd, key);
            else
                handle_hot_key(key);
        }
    }
}

//------------------------------------------------------------------------
// Init the shell struct
//------------------------------------------------------------------------
void init_shell_struct()
{
    //init shell struct
    shell.focused = 0;
	shell.toggled = 1;
    shell.screens[0] = NULL;
	shell.screens[1] = NULL;
	shell.screens[2] = NULL;
	shell.screens[3] = NULL;

    int i;
    for(i = 0; i < MAXAPPS; i++) //init apps
    {
		shell.apps[i].pid = INVALIDPID;
        shell.apps[i].id = INVALIDAPP;
        shell.apps[i].f = INVALIDFIFO;
        shell.apps[i].quad.idx_col = 0;
        shell.apps[i].quad.idx_row = 0;

        //zero out quad buffers
        int row, col;
        for(row = 0; row < QUAD_HEIGHT; row++)
        {
            for(col = 0; col < QUAD_WIDTH; col++)
            {
                shell.apps[i].quad.quad_buf[row][col] = 0;
            }
        }
    }

	shell.apps[0].pid = OS_GetPid();
	shell.apps[0].id = 0;
    shell.apps[0].f = -1;
	shell.apps[0].screen = SHELLSCRN;
}

//------------------------------------------------------------------------
// handles the entering of a command
//------------------------------------------------------------------------
void hdl_newline(struct cmd* cmd)
{
    struct quad* quad = &shell.apps[0].quad;
    if(cmd->prev_idx < 0)
	{
	    parse_cmd(cmd->buf);
		add_prev_cmd(cmd);	
	}
	else
	{
		parse_cmd(cmd->prev_cmds[cmd->prev_idx]);
		add_prev_cmd(cmd);
		cmd->prev_idx = -1;
	}

    printf("badash $ ");
    quad->quad_buf[quad->idx_row][quad->idx_col] = '_';

    int i;
    for(i = 0; i < 32; i++)
        cmd->buf[i] = '\0';
	cmd->idx = 0;
}

//------------------------------------------------------------------------
// Handles selecting a previous command further back in the history
//------------------------------------------------------------------------
void hdl_up(struct cmd* cmd)
{
    struct quad* quad = &shell.apps[0].quad;
	if((cmd->prev_idx + 1) < cmd->prev_count)
	{
		cmd->prev_idx++;
		quad->idx_col = 9;
		int i;
		for(i = 9; i < QUAD_WIDTH; i++)
			quad->quad_buf[quad->idx_row][i] = '\0';
		printf("%s", cmd->prev_cmds[cmd->prev_idx]);
		for (i = 0; i < 32; i++)
        {
            if (cmd->prev_cmds[cmd->prev_idx][i] == '\0')
            {
                cmd->idx = i;
                break;
            }
        }
	}
}

//------------------------------------------------------------------------
// Handles selecting a previous command forward in the history
//------------------------------------------------------------------------
void hdl_down(struct cmd* cmd)
{
    struct quad* quad = &shell.apps[0].quad;
	if(cmd->prev_idx >= 0) // sd - changed > to >= because we need to get prev_idx 0 as well.
	{
		cmd->prev_idx--;
		quad->idx_col = 9;
		int i;
		for(i = 9; i < QUAD_WIDTH; i++)
			quad->quad_buf[quad->idx_row][i] = '\0';
		if(cmd->prev_idx != -1)
		{
			printf("%s", cmd->prev_cmds[cmd->prev_idx]);
			for (i = 0; i < 32; i++)
            {
                if (cmd->prev_cmds[cmd->prev_idx][i] == '\0')
                {
                   cmd->idx = i;
                   break;
                }
            }
		}
		else
		{
			printf("%s", cmd->buf);
			for (i = 0; i < 32; i++)
            {
                if (cmd->buf[i] == '\0')
                {
                    cmd->idx = i;
                    break;
                }
            }
		}
	}
}

//------------------------------------------------------------------------
// Handles non-hotkeys
//------------------------------------------------------------------------
void handle_key(struct cmd* cmd, unsigned char key)
{
    struct quad* quad = &shell.apps[0].quad;
    //print the character to the screen
    if(!(key == '\b' && quad->idx_col == 9) && //guard against deleting "badash $ "
       !((key == 129) || (key == 130))) // don't print up and down keys
    {
        if(key == '\b' || key == '\n')
            quad->quad_buf[quad->idx_row][quad->idx_col] = '\0';
		putchar(key);
        quad->quad_buf[quad->idx_row][quad->idx_col] = '_';
    }

    update_cmd(cmd, key);

    if(key == '\n') //enter triggers command
    {
		hdl_newline(cmd);
    }
	else if(key == 129)
	{
        hdl_up(cmd);
	}
	else if(key == 130)
	{
        hdl_down(cmd);
	}
}

//------------------------------------------------------------------------
// Handles the hotkey actions
//------------------------------------------------------------------------
void update_cmd(struct cmd* cmd, unsigned char key)
{
    if(!(key == '\b' || key == '\n' || key == 129 || key == 130))
	{
	    //sd - added this if to allow for editing of prev_cmds commands when using
	    //     backspace
	    if(cmd->prev_idx >= 0)
	        cmd->prev_cmds[cmd->prev_idx][cmd->idx++] = key;
	    else
	        cmd->buf[cmd->idx++] = key;
	}
	else if(key == '\b' && cmd->idx > 0) //backspace & there is stuff in the command
	{
	    if(cmd->prev_idx >= 0)
	        cmd->prev_cmds[cmd->prev_idx][--cmd->idx] = '\0';
	    else
	        cmd->buf[--cmd->idx] = '\0';
	}
}

//------------------------------------------------------------------------
// Handles the hotkey actions
//------------------------------------------------------------------------
void handle_hot_key(char key)
{
    switch(key)
    {
    case '\t':
		focus_next();
        break;
    case '`':
		if(shell.toggled)
		{
			if(shell.focused != 0)
			{
				shell.focused = 0;
			}
			else
			{
				shell.toggled = 0;
				if(!shell.screens[SHELLSCRN])
					focus_next();
				else
					shell.focused = shell.screens[SHELLSCRN]->id;
			}
		}
		else
		{
			shell.toggled = 1;
			shell.focused = 0;
		}
        break;
   }
}

//------------------------------------------------------------------------
// switches focus to the next screen
//------------------------------------------------------------------------
void focus_next()
{
	int i = 1;
	while (!shell.screens[(shell.apps[shell.focused].screen+i)%SCREENS] && !(((shell.apps[shell.focused].screen+i)%SCREENS == SHELLSCRN) && shell.toggled))
		i++;
	if (((shell.apps[shell.focused].screen+i)%SCREENS == SHELLSCRN) && shell.toggled)
		shell.focused = 0;
	else
		shell.focused = shell.screens[(shell.apps[shell.focused].screen+i)%SCREENS]->id;
}

//------------------------------------------------------------------------
// Finds an empty app slot
//------------------------------------------------------------------------
int get_empty_idx(struct app* apps)
{
    int i;
    for(i = 0; i < MAXAPPS; i++)
    {
        if(apps[i].id == INVALIDAPP)
            return i;
    }
    return -1;
}

//------------------------------------------------------------------------
// Launch app command
//------------------------------------------------------------------------
int launch_app(void (*process)(void), int screen, const char* name, int level, int n)
{
    int i = get_empty_idx(shell.apps);
    if(i < 0)
        return -1;
    shell.apps[i].id = i;
    shell.apps[i].f = OS_InitFiFo();
    shell.apps[i].pid = OS_Create(process, shell.apps[i].f, level, n);
    //OS_Write(shell.apps[i].f, (int)&shell.apps[i].quad);
    shell.focused = i;
	shell.apps[i].screen = -1;
	if(screen == SHELLSCRN)
		shell.toggled = 0;
	if(shell.screens[screen])
		swap_screen(shell.screens[screen], shell.apps+i);
	else
		shell.screens[screen] = shell.apps+i;

    int j = 0;
    while(name[j] && j < 16)
    {
        shell.apps[i].name[j] = name[j];
        j++;
    }
    shell.apps[i].name[j] = '\0';
	shell.apps[i].screen = screen;
	
    return 0;	
}

//------------------------------------------------------------------------
// Parse the command string and take the appropriate action
//------------------------------------------------------------------------
void parse_cmd(char* cmd)
{
    int screen = get_screen_num(cmd);
	int app_id;
    switch(cmd[0])
    {
    case 'h': //hangman
		if(screen == -1)
			printf("malformed command.\n");
		else
			launch_app(&hangman, screen, "hangman", PERIODIC, 1);
        break;
    case 's': //semapp
		if(screen == -1)
			printf("malformed command.\n");
		else
			launch_app(&semapp, screen, "semapp", SPORADIC, NULL);
        break;
    case 't':
        switch(cmd[1])
        {
        case 'i': //time
			if(screen == -1)
				printf("malformed command.\n");
			else
				launch_app(&time, screen, "time", DEVICE, 1000);
            break;
        case 'e': //text
            if(screen == -1)
				printf("malformed command.\n");
			else
				launch_app(&text, screen, "text", PERIODIC, 3);
            break;
        default:
            printf("command doesn't exist\n");
        }
        break;
    case 'c': //calc
        launch_app(&calc, screen, "calc", PERIODIC, 2);
        break;
    case 'm':
        switch(cmd[1])
        {
        case 'o': //move
            app_id = get_app_id(cmd);
			if(app_id == -1 || screen == -1)
				printf("malformed command.\n");
			else if (app_id == 0)
				printf("cannot move shell.\n");
			else
			{
				if(!shell.screens[screen])
					put_app(shell.apps+app_id, screen);
				else
					swap_screen(shell.screens[screen], shell.apps+app_id);
			}
            break;
        /*case: 'a': //mandelbrot
            //launch_gfx_app(); //TODO
            break;*/
        default:
            printf("command doesn't exist\n");
        }
        break;
    case 'p': //ps
        ps();
        break;
    /*case 'k': //kill
        //TODO implement kill
        break;*/
    default:
        printf("command doesn't exist\n");
    }
}

//------------------------------------------------------------------------
// parse the command string and return the specified screen number
//------------------------------------------------------------------------
int get_screen_num(char* cmd)
{
    int i = 0;
    while(cmd[i] != '>' && i < 32)
        i++;
    
    if(i == 32)
        return -1;

    switch(cmd[i+2])
    {
    case '1':
        return 0;
    case '2':
        return 1;
    /*case '3':
        return 2;
    case '4':
        return 3;*/
    default:
        return -1;
    }
}

//------------------------------------------------------------------------
// parse the command string and return the specified app id
//------------------------------------------------------------------------
int get_app_id(char* cmd)
{
    int i = 0;
    while(cmd[i] != '>' && i < 32)
        i++;
    
    if(i == 32)
        return -1;

    switch(cmd[i-2])
    {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    default:
        return -1;
    }
}

//------------------------------------------------------------------------
// Swap the screens that the apps are displayed in
//------------------------------------------------------------------------
void swap_screen(struct app* app1, struct app* app2)
{
	int temp = app1->screen;
	
	app1->screen = app2->screen;
	app2->screen = temp;
	if(app1->screen >= 0)
		shell.screens[app1->screen] = app1;
	if(app2->screen >= 0)
		shell.screens[app2->screen] = app2;
}

void put_app(struct app* app, int screen)
{
	if(app->screen != -1)
		shell.screens[app->screen] = NULL;
	shell.screens[screen] = app;
	app->screen = screen;
}

//------------------------------------------------------------------------
// Displays active apps
//------------------------------------------------------------------------
void ps()
{
    printf("APP\t\t\t\tID\t\t\tSCREEN\n\n");

    int i;
    for(i = 0; i < MAXAPPS; i++)
    {
        if(shell.apps[i].id > 0) //all valid non-shell apps
            printf("%s\t\t\t%d\t\t\t%d\n",shell.apps[i].name,shell.apps[i].id,shell.apps[i].screen+1 > 0 ? shell.apps[i].screen+1 : -1);
    }
	putchar('\n');
}

//------------------------------------------------------------------------
// get app from pid
//------------------------------------------------------------------------
struct app* get_app_from_pid(int pid)
{
	int i;
	for(i = 0; i < MAXAPPS; i++)
    {
        if(shell.apps[i].pid == pid)
            return shell.apps+i;
    }
    return NULL;
}

//------------------------------------------------------------------------
// add the command to the history
//------------------------------------------------------------------------
void add_prev_cmd(struct cmd* cmd)
{
	int i, j;
	
	char temp[32];
	for(i = 0; i < 32; i++)
		temp[i] = cmd->buf[i];
	
	for(i = 15; i > 0; i--)
		for(j = 0; j < 32; j++)
			cmd->prev_cmds[i][j] = cmd->prev_cmds[i-1][j];
	for(i = 0; i < 32; i++)
		cmd->prev_cmds[0][i] = temp[i];
	if(cmd->prev_count < 16)
		cmd->prev_count++;
}

