/*
 * Command History
 * Copyright (c) 2016, Beom-jin Kim <riddler117@gmail.com>
 * $Revision: 0.1 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

 #include <history.h>
 #include <String.h>
 #include <Conio.h>

 int Init_History(history_t* self)
 {
	 int i;
	 for(i = 0; i < HISTORY_SIZE; i++)
		  memset(self->list[i], '\0', BUFSIZE);
	 self->front = 0;
	 self->rear = 0;
	 self->cursor = 0;
	 self->Print_History = Print_History;
	 self->Add_History_Item = Add_History_Item;
	 self->Get_History_Item = Get_History_Item;
	 return 0;
 }
 
 char* Get_History_Item(history_t* self, int direction)
 {
	/* Need refactoring */
	char* ret;
	int mod;
	/* Consider case : is list empty? */
	if( (self->front == self->cursor && direction == UP) ||
		(self->rear == self->cursor && direction == DOWN))
		ret = self->list[self->cursor];
	else
	{
		mod = (self->cursor - direction)%HISTORY_SIZE;  
		self->cursor = (mod >= 0)? mod : mod + HISTORY_SIZE;
		ret = self->list[(self->cursor)];
	}
	return ret;
 }
 
 int Add_History_Item(history_t* self, char* command)
 {	 
	if((self->rear+1)%HISTORY_SIZE == self->front)
		self->front = (self->front+1)%HISTORY_SIZE ;
	strcpy(self->list[self->rear], command);
	self->rear = (self->rear+1)%HISTORY_SIZE;
	strcpy(self->list[self->rear], "\0");
	self->cursor = self->rear;
	return 0;
 }
  
 void Print_History(history_t* self)
 {
	 int i;
	 for(i = self->front;
	 		i != (self->rear)%HISTORY_SIZE; i = (i+1)%HISTORY_SIZE) 
		 Print(" %d  %s\n", i, self->list[i]);
 }

