#include "os.h"
#include "shell_api.h"
#include "shell.h"

//------------------------------------------------------------------------
// getchar for apps
//------------------------------------------------------------------------
char sh_getchar()
{
	FIFO f = OS_GetParam();
	int val;
	while(!OS_Read(f, &val)) 
		OS_Yield();
	return (char)val;
}

struct quad* sh_getquad()
{
    PID pid = OS_GetPid();
    struct app* app = get_app_from_pid(pid);
    
    return &app->quad;
}

void sh_cls()
{
	struct quad* quad = sh_getquad();
	int i,j;
	for(i = 0; i < QUAD_HEIGHT; i++)
	{
		for(j = 0; j < QUAD_WIDTH; j++)
		{
			quad->quad_buf[i][j] = '\0';
		}
	}
}

