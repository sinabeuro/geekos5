/*
 * Command History
 * Copyright (c) 2016, Beom-jin Kim <riddler117@gmail.com>
 * $Revision: 0.1 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_SIZE 10
#define BUFSIZE 79

#define UP 1
#define DOWN -1

typedef struct History history_t;

typedef struct History {
	char list[HISTORY_SIZE][BUFSIZE];
	int front;
	int rear;
	int cursor;
	void (*Print_History)(history_t* self);
	int (*Add_History_Item)(history_t* self, char* command);
	char* (*Get_History_Item)(history_t* self, int direction);
}history_t;

extern history_t history;

int Init_History(history_t* self);
void Print_History(history_t* self);
int Add_History_Item(history_t* self, char* command);
char* Get_History_Item(history_t* self, int direction);

#endif


