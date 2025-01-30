#ifndef KILO_H_
#define KILO_H_

#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 8
#define ABUF_INIT {NULL, 0}
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

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
void editorAppendRow(char *s, size_t len);
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


#endif /* KILO_H_ */