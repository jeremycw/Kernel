#include "os.h"
#include "vga.h"
#include "shell.h"
#include "refresher.h"

void refresher()
{
	char empty_quad[QUAD_HEIGHT][QUAD_WIDTH];

	int i, j;
	
	for(i = 0; i < 80; i++)
	{
		for(j = 0; j < 25; j++)
		{
			setchar('\0', 0, i, j);
		}
	}
	
	for(i = 0; i < QUAD_HEIGHT; i++)
	{
		for(j = 0; j < QUAD_WIDTH; j++)
		{
			empty_quad[i][j] = '\0';
		}
	}

    while(1)
    {
		draw_win_frames();
        update_screen(empty_quad);
        //highlight_active_frame();
        OS_Yield();
    }
}

//------------------------------------------------------------------------
// Draws the four window frames to the char buffer
//------------------------------------------------------------------------
void draw_win_frames()
{
	
	int focused = shell.apps[shell.focused].screen;
	
	draw_frame(focused == 0 ? 0x55:0x77, 0,0); //screen 0
	draw_frame(focused == 1 ? 0x55:0x77, 40,0);// screen 1
	/*draw_frame(focused == 2 ? 0x12c7:0xFFFF, 1,122);// screen 2
	draw_frame(focused == 3 ? 0x12c7:0xFFFF, 162,122);// screen 3*/

    //TODO place corners
}

//------------------------------------------------------------------------
// Copy the quad buffers into the frame buffer
//------------------------------------------------------------------------
void update_screen(char empty_quad[][QUAD_WIDTH])
{
    if(shell.toggled)
		draw_quad(shell.apps[0].quad.quad_buf, 1, 1);
	else if(shell.screens[0])
		draw_quad(shell.screens[0]->quad.quad_buf, 1, 1);
	else
		draw_quad(empty_quad, 1, 1);
	if(shell.screens[1])
		draw_quad(shell.screens[1]->quad.quad_buf, 41, 1);
	else
		draw_quad(empty_quad, 41, 1);
	/*if(shell.toggled)
		draw_quad(shell.apps[0].quad.quad_buf, 1, 31);
	else if(shell.screens[2])
		draw_quad(shell.screens[2]->quad.quad_buf, 1, 31);
	else
		draw_quad(empty_quad, 1, 31);
	if(shell.screens[3])
		draw_quad(shell.screens[3]->quad.quad_buf, 41, 31);
	else
		draw_quad(empty_quad, 41, 31);*/
}

//------------------------------------------------------------------------
// Copy a quad buffer into the frame buffer with a given offset
//------------------------------------------------------------------------
void draw_quad(char buf[][QUAD_WIDTH], int x, int y)
{
    int i,j;
    for(i = 0; i < QUAD_HEIGHT; i++)
    {
        for(j = 0; j < QUAD_WIDTH; j++)
        {
            setchar(buf[i][j], 0x07, j+x, i+y);
        }
    }
}

void draw_frame(char rgb, int x, int y)
{
	int i;
	for(i = 0; i < 40; i++)
	{
		setchar('-', rgb, x+i, y);
		setchar('-', rgb, x+i, y+24);if(shell.toggled)
		draw_quad(shell.apps[0].quad.quad_buf, 1, 31);
	}
	
	for(i = 0; i < 24; i++)
	{
		setchar('|', rgb, x, y+i);
		setchar('|', rgb, x+39, y+i);
	}
}


