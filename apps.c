#include "apps.h"
#include "os.h"
#include "out.h"
#include "shell_api.h"

void hangman()
{
	char c;
	struct quad* quad = sh_getquad();
	char* words[11] = {"computer",
				     "kangaroo",
				     "aardvark",
				     "zebra",
				     "keyboard",
				     "sailboat",
				     "pirate",
				     "fuck",
				     "binary",
				     "bicycle",
				     "automobile"};
					 
	int word_idx = 0;
	while(1)
	{
		sh_cls();
		quad->quad_buf[0][0] = '_';
		quad->quad_buf[0][1] = '_';
		quad->quad_buf[0][2] = '_';
		quad->quad_buf[0][3] = '_';
	
		quad->quad_buf[1][0] = '|';
		quad->quad_buf[2][0] = '|';
		quad->quad_buf[3][0] = '|';
		quad->quad_buf[4][0] = '|';
	
		quad->quad_buf[1][3] = '|';
		
		word_idx++;
		if(word_idx == 10)
			word_idx = 0;
			
		char* word = words[word_idx];
		int i = 0;
		int wrong = 0;
		int correct = 0;
		char guesses[26];
		int guesses_idx = 0;
		int still_playing = 1;
		int word_len = 0;
		int valid_guess = 0;
			
		i = 0;
		while(word[i] && i < QUAD_WIDTH)
		{
			quad->quad_buf[6][i] = '_';
			i++;
		}
		word_len = i;
		
		while(guesses_idx > -1)
			guesses[guesses_idx--] = '\0';
		
		while(still_playing)
		{
			do
			{
				do
				{
					c = sh_getchar();
				}
				while(c < 97 || c > 122);
				
				valid_guess = 1;
				for(i = 0; i < guesses_idx+1; i++)
				{
					if(guesses[i] == c)
						valid_guess = 0;
				}
			}
			while(!valid_guess);
			
			guesses[guesses_idx++] = c;
			
			i = 0;
			int found = 0;
			while(word[i])
			{
				if(word[i] == c && i < QUAD_WIDTH)
				{
					quad->quad_buf[6][i] = c;
					correct++;
					found = 1;
				}
				i++;
			}
			if(!found)
			{
				quad->quad_buf[8][wrong] = c;
				switch(wrong)
				{
				case 0:
					quad->quad_buf[2][3] = 'O';
					break;
				case 1:
					quad->quad_buf[3][3] = '|';
					break;
				case 2:
					quad->quad_buf[2][2] = '_';
					break;
				case 3:
					quad->quad_buf[2][4] = '_';
					break;
				case 4:
					quad->quad_buf[4][2] = '/';
					break;
				case 5:
					quad->quad_buf[4][4] = '\\';
					quad->idx_row = 10;
					quad->idx_col = 0;
					printf("YOU LOSE! The word was: %s\n\nPress enter to play again.", word);
					still_playing = 0;
					while(sh_getchar() != '\n');
					break;
				}
				wrong++;
			}
			if(correct == word_len)
			{
				quad->idx_row = 10;
				quad->idx_col = 0;
				printf("YOU WIN!\n\nPress enter to play again.");
				still_playing = 0;
				while(sh_getchar() != '\n');
			}
		}
	}
}

void time()
{
	int seconds = 0;
	int minutes = 0;
	int hours = 0;
	while(1)
	{
		printf("time: %d:%d:%d\n", hours, minutes, seconds);
		if(++seconds == 60)
		{
			minutes++;
			seconds = 0;
		}
		if(minutes == 60)
		{
			hours++;
			minutes = 0;
		}
		OS_Yield();
	}
}

void semapp()
{
	int val;
	volatile BOOL sem = 1;
	volatile FIFO producer_fifo = OS_InitFiFo();
	FIFO shell_fifo = OS_GetParam();
	
	OS_Create(&producer, (int)&sem, SPORADIC, 0);
	OS_Create(&producer, (int)&sem, SPORADIC, 0);
	OS_Create(&producer, (int)&sem, SPORADIC, 0);
	
	while(1)
	{
        if(OS_Read(shell_fifo, &val) && val == 's')
            sem = sem ? 0 : 1;
		while(OS_Read(producer_fifo, &val))
		    putchar(val);
		OS_Yield();
	}
}

void producer()
{	
		int* sem_flag = (int*)OS_GetParam();
		FIFO f = *(sem_flag-1);
		OS_InitSem(1, 1);
		int num;
		static int count = 0;
		num = count++;
		char val;
		
		switch (num)
		{
			case 0:
				val = 'A';
				break;
			case 1:
				val = '1';
				break;
			case 2:
				val = '!';
				break;
		}
		
		while(1)
		{
			if(*sem_flag)
				OS_Wait(1);
			OS_Write(f, val);
			OS_Yield();
			OS_Write(f, val);
			OS_Yield();
			OS_Write(f, val);
			OS_Yield();
			OS_Signal(1);
		}
}

void text()
{
	while(1)
	{
		char c = sh_getchar();
		putchar(c);
	}
}

void calc()
{
	struct quad* quad = sh_getquad();
	
    unsigned int key;
    
    char cmd[5];
    int cmd_idx = 0;
    
    char prev_cmds[16][5];
	int prev_idx = -1;
	int prev_count = 0;
    
    printf("Enter an equation.\nMust be in form 'operand' 'operator' 'operand'.\nValid operators are +, -, / and *.\nThe operands must be single digit\nnumbers (0-9).\n\n");
    
    while(1) //FIXME backspace not working, altering cmd but not screen, also up/down for prevcmds appears to be putting the actual char to screen.
    {
        key = sh_getchar();
		if(!(key == 8) && !((key == 129) || (key == 130)))
			putchar(key);
		if(!(key == 8 || key == '\n' || key == 129 || key == 130))
		{
			if(prev_idx >= 0)
				prev_cmds[prev_idx][cmd_idx++] = key;
			else
				cmd[cmd_idx++] = key;
		}
		if(key == '\n') //enter triggers command
		{
			putchar('\n');
			if(prev_idx < 0)
			{
				calc_parse_cmd(cmd);
				calc_add_prev_cmd(cmd, prev_cmds, &prev_count);	
			}
			else
			{
				calc_parse_cmd(prev_cmds[prev_idx]);
				calc_add_prev_cmd(prev_cmds[prev_idx], prev_cmds, &prev_count);
				prev_idx = -1;
			}
			int i;
			for(i = 0; i < 5; i++)
				cmd[i] = '\0';
			cmd_idx = 0;
		}
		else if(key == 8 && cmd_idx > 0)
		{
			if(prev_idx >= 0)
				prev_cmds[prev_idx][--cmd_idx] = '\0';
			else
				cmd[--cmd_idx] = '\0';
		}
		else if(key == 129)
		{
			if((prev_idx + 1) < prev_count)
			{
				prev_idx++;
				quad->idx_col = 0;
				int i;
				for(i = 0; i < 38; i++)
					quad->quad_buf[quad->idx_row][i] = '\0';
				printf("%s", prev_cmds[prev_idx]);
				for (i = 0; i < 5; i++)
				{
					if (prev_cmds[prev_idx][i] == '\0')
					{
						cmd_idx = i;
						break;
					}
				}
			}
		}
		else if(key == 130)
		{
			if(prev_idx >= 0)
			{
				prev_idx--;
				quad->idx_col = 0;
				int i;
				for(i = 0; i < 38; i++)
					quad->quad_buf[quad->idx_row][i] = '\0';
				if(prev_idx != -1)
				{
					printf("%s", prev_cmds[prev_idx]);
					for (i = 0; i < 5; i++)
					{
						if (prev_cmds[prev_idx][i] == '\0')
						{
						   cmd_idx = i;
						   break;
						}
					}
				}
				else
				{
					printf("%s", cmd);
					for (i = 0; i < 5; i++)
					{
						if (cmd[i] == '\0')
						{
							cmd_idx = i;
							break;
						}
					}
				}
			}
		}
    }
}

void calc_parse_cmd(char* cmd)
{
    int op1, op2, res;
    char operand;
    
	if(cmd[0] == '\0')
	{
		printf("Empty equation.\n\n");
		return;
	}
    if((cmd[0] < 48 || cmd[0] > 57) || (cmd[4] < 48 || cmd[4] > 57))
	{
		printf("Invalid operand.\n\n");
		return;
	}
	if(cmd[1] != ' ' || cmd[3] != ' ')
	{
		printf("Invalid  equation format.\n\n");
		return;
	}
	if(!(cmd[2] == '+' || cmd[2] == '-' || cmd[2] == '/' || cmd[2] == '*'))
	{
		printf("Invalid  operator.\n\n");
		return;
	}
	op1 = cmd[0] - 48;
	op2 = cmd[4] - 48;
	
	operand = cmd[2];
	
	switch (operand)
	{
		case '+':
			res = op1 + op2;
			break;
		case '-':
			res = op1 - op2;
			break;
		case '/':
			res = op1 / op2;
			break;
		case '*':
			res = op1 * op2;
			break;
	}
	
	printf("=%d\n\n", res);
}

void calc_add_prev_cmd(char* cmd, char prev[][5], int* prev_count)
{
	int i, j;
	
	char temp[5];
	for(i = 0; i < 5; i++)
		temp[i] = cmd[i];
	
	for(i = 15; i > 0; i--)
		for(j = 0; j < 5; j++)
			prev[i][j] = prev[i-1][j];
	for(i = 0; i < 5; i++)
		prev[0][i] = temp[i];
    if((*prev_count) < 16)
        (*prev_count)++;
}
