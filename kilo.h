#ifndef KILO_H_
#define KILO_H_

#define CTRL_KEY(k) ((k) & 0x1f) // convert to ASCII code for control keys
#define KILO_VERSION "0.0.2"
#define KILO_QUIT_TIMES 2 // number of times to allow unsaved changes before quitting
#define KILO_TAB_STOP 8 // number of spaces per tab stop
#define ABUF_INIT {NULL, 0} // initialize an empty buffer
#define _DEFAULT_SOURCE // needed for getline
#define _BSD_SOURCE // needed for strdup
#define _GNU_SOURCE // needed for strdup

#include <termios.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h> //input output control library
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>




/*** Data ***/
//E.cx is the horizontal coordinate of the cursor (the column) and E.cy is the vertical coordinate (the row).


//erow stand for "Editor Row"
typedef struct erow{
  int size; //row size in bytes
  int rsize; //row size in characters
  char *chars; //pointets for characters
  char *render; //rendered row with highlights
} erow;



struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  int dirty;
  erow *row;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct termios orig_termios;
};




/*** append buffer ***/
/*
An append buffer consists of a pointer to our buffer in memory, and a length. We define an ABUF_INIT constant which represents an empty buffer
as using a bunch of small write() is not good for performance, we use a append buffer to make a big write which write the whole screen at once
*/ 
struct abuf {
  char *b;
  int len;
};

enum editorKeyP{
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
  DEL_KEY,
};

struct editorConfig E;


// Function prototypes
void editorRefreshScreen();
void die(const char *s);
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);
int editorReadKey();
void disableRawMode();
void enableRawMode();
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
void editorMoveCursor(int key);
void editorProcessKeyPress();
void editorDrawRows(struct abuf *ab);
void initEditor();
void editorOpen();
void editorInsertRow(int at, char *s, size_t len);
void editorUpdateRow(erow *row);
int editorRowCxToRx(erow *row, int cx);
void editorDrawStatusBar(struct abuf *ab);
void editorSetStatusMessage(const char *fmt, ...);
void editorDrawMessageBar(struct abuf *ab);
void editorRowInsertChar(erow *row, int at, int c);
void editorInsertChar(int c);
void editorScroll();
char *editorRowsToString(int *buflen);
void editorSave();
void editorRowDelChar(erow *row, int at);
void editorDelChar();
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorInsertNewline();
char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorFind();
void editorFindCallback(char *query, int key);
int editorRowRxToCx(erow *row, int rx);



#endif /* KILO_H_ */