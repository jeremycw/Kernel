#ifndef APPS_H
#define APPS_H

void hangman();
void time();
void semapp();
void producer();
void text();
void calc();
void calc_parse_cmd(char* cmd);
void calc_add_prev_cmd(char* cmd, char prev[][5], int* prev_count);

#endif
