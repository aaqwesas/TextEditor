#include "kilo.h"  
//Control characters are nonprintable characters that we don’t want to print to the screen (ASCII codes 0–31,127)
//https://viewsourcecode.org/snaptoken/kilo/index.html

void abAppend(struct abuf *ab, const char *s, int len){

  //realloc is used to change the size of the previously allocated memory size
  char *new = realloc(ab->b,ab->len + len);

  //memcpy is used to copy len bytes from the memory location pointed by s to the memory location pointed by b
  //this function is used to append a string to the buffer
  if(new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

//destructor that deallocates the dynamic memory used by an abuf
void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** terminal ****/

/* a function used to handle error */
void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}


//this function job is to keep reading the input and then return that char
int editorReadKey() {
  int nread;
  char c;
  //read() is used to read a single character from the standard input (stdin)
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    // handle error when reading from stdin
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  // \x1b is the escape character, \x1b[H is a sequence to move the cursor to the top left corner of the screen
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      //handling pg up and pg down key
      //seq[1] is the first digit of the sequence, seq[2] is the second digit, and seq[3] is the third digit.
      //if the third digit is '~', it’s a special sequence for page up and page down.
      //we only care about the first two digits for this case.
      //if seq[1] is between '0' and '9', it’s a number, and we read the third digit to determine the direction.
      //if seq[1] is not between '0' and '9', it’s an arrow key, and we use a switch statement to determine the direction.
      //we use the arrow keys to navigate through the text editor.
      //the arrow keys are represented by the ASCII codes for the arrow keys.
      //we use the arrow key codes to determine the direction.
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          //home key and end key have multiple escape sequences, so we need to handle them separately
          switch (seq[1]) {
            case '1': return HOME_KEY; //here
            case '3': return DEL_KEY; //handle delete key <esc>[3~,.
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY; //here
            case '8': return END_KEY;
          }
        }
      } else {
        //use the arrow keys to navigate through the text editor
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    }else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
      
    }
    return '\x1b';
  } else {
    return c;
  }
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}
/*
c_iflag (input flags), c_oflag (output flags), and c_cflag (control flags)
ICANON mode: reading input line by line, disable it will let it read bit by bit
ECHO: causes each key you type to be printed to the terminal, so you can see what you’re typing.
IXON: Ctrl-S stops data from being transmitted to the terminal until you press Ctrl-Q
IEXTEN: disable the Ctrl-V flag
INPCK: enables parity checking, which doesn’t seem to apply to modern terminal emulators.
ISTRIP: causes the 8th bit of each input byte to be stripped, meaning it will set it to 0. This is probably already turned off.
ISIG: disabling the signal interrupt for Ctrl + c (send sigint to terminate the process) and Ctrl + z (Ctrl + z send a signal interrupt to suspend the process)
CS8 is not a flag, it is a bit mask with multiple bits, which we set using the bitwise-OR (|) operator unlike all the flags we are turning off
mode when you use typing password in sudo mode, it got the text from the terminal but not print it out to show you
*/
void enableRawMode() {
  // Get the current terminal attributes
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");

  // Register disableRawMode to be called automatically when the program exits
  atexit(disableRawMode);

  // Create a copy of the original terminal settings
  struct termios raw = E.orig_termios;

  // Modify input flags (c_iflag)
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  // BRKINT: Disable break condition generating a SIGINT
  // ICRNL: Disable translating carriage return to newline
  // INPCK: Disable parity checking
  // ISTRIP: Disable stripping the 8th bit of each input byte
  // IXON: Disable software flow control (Ctrl-S and Ctrl-Q)

  // Modify output flags (c_oflag)
  raw.c_oflag &= ~(OPOST);
  // OPOST: Disable all output processing features

  // Modify control flags (c_cflag)
  raw.c_cflag |= (CS8);
  // CS8: Set character size to 8 bits per byte

  // Modify local flags (c_lflag)
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  // ECHO: Disable echoing input characters
  // ICANON: Disable canonical mode (line-by-line input)
  // ISIG: Disable sending signals on special characters (like Ctrl-C)
  // IEXTEN: Disable implementation-defined input processing

  // Modify control characters (c_cc)
  raw.c_cc[VMIN] = 0;  // Minimum number of bytes before read() can return
  raw.c_cc[VTIME] = 1; // Maximum time to wait before read() returns

  // Apply the modified attributes to the terminal
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}


void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename);
  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
        editorInsertRow(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  E.dirty = 0;
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  //getting the height and width of the terminal
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1; // %d;%d, which tells it to parse two integers separated by a ;, and put the values into the rows and cols variables.

  return 0;
}

int getWindowSize(int *rows, int *cols){
  struct winsize ws;
  // ioctl() will place the number of columns wide and the number of rows high the terminal is into the given winsize struct, -1 if failed
  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    /* 
    If ioctl fails or the terminal size is zero, use the escape sequence method
    this method will move the cursor to the bottom-right corner of the terminal window
    */
    if(write(STDOUT_FILENO,"\x1b[999C\x1b[999B",12) != 12) return -1;//if not equal to 12, produce an error
    return getCursorPosition(rows, cols);
  } else {
    // If ioctl succeeds and provides a non-zero column width, use these values
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}


/*** input ***/
void editorScroll(){//This function is used to scroll the text in the editor.
  E.rx = 0;
  if (E.cy < E.numrows) {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }
  //check if the cursor move above the visible area
  if(E.cy < E.rowoff){
    E.rowoff = E.cy;
  }
  //check if the cursor move below the visible area
  if(E.cy >= E.rowoff + E.screenrows){
    E.rowoff = E.cy - E.screenrows + 1;
  }
  //check if the cursor move to the left of the visible area
  if (E.cx < E.coloff) {
    E.coloff = E.rx;
  }
  //check if the cursor move to the right of the visible area
  if (E.cx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

//controlling the movement of the curs  or, this allow to use keyboard to move the cursor
// Function to handle cursor movement based on arrow key input
void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  switch (key) {
    case ARROW_LEFT:
      // Move cursor left if it's not at the leftmost position
      if (E.cx != 0){
        E.cx--;
      }else if (E.cy > 0) {
        // Move to the end of the previous line if we're not on the first line
        E.cy--;
        // Set the cursor to the end of the previous line
        E.cx = E.row[E.cy].size;
      }
      break;
      //allow the user to scrool pass the right edge of the screen
    case ARROW_RIGHT:
      if (row && E.cx < row->size) {
        E.cx++;
      } else if (row && E.cx == row->size) {
        // Move to the beginning of the next line if we're on the last line
        E.cy++;
        E.cx = 0;
      }
      break;
    case ARROW_UP:
      // Move cursor up if it's not at the topmost row
      if (E.cy != 0){
        E.cy--;
      }
      break;
    case ARROW_DOWN:
      // Move cursor down if it's not at the bottom of the file
      if (E.cy != E.numrows){
        E.cy++;
      }
      break;
  }
  // Get a pointer to the current row, or NULL if we're past the end of the file
  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  // Determine the length of the current row
  // If we're on a valid row, use its size; otherwise, use 0
  int rowlen = row ? row->size : 0;

  // Ensure the cursor doesn't go beyond the end of the current line
  if (E.cx > rowlen) {
      // If the cursor is beyond the line's end, move it to the end of the line
      E.cx = rowlen;
  }
}

//this function call editorReadKey(), then it will handle that key for differernt key input
void editorProcessKeyPress() {
  static int quit_times = KILO_QUIT_TIMES;
  int c = editorReadKey();

  switch(c){ 
    case '\r':
      editorInsertNewline();
      break;
    case CTRL_KEY('q'):// default operation for the text editor, use ctrl-q to quit
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("WARNING!!! File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    
    //handle the save key to save the file
    case CTRL_KEY('s'):
      editorSave();
      break;

    // handle home key to go to the start of the line
    case HOME_KEY:
      E.cx = 0;
      break;
    
    //go to the end of the line
    case END_KEY:
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      break;
    case CTRL_KEY('f'):
      editorFind();
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      editorDelChar();
      break;

    //handle page up and down
    case PAGE_UP:
    case PAGE_DOWN:
    {
      if (c == PAGE_UP) {
        E.cy = E.rowoff;
      }else if (c == PAGE_DOWN) {
        E.cy = E.rowoff + E.screenrows - 1;
        if(E.cy > E.numrows) E.cy = E.numrows;
      }
      
      int tiems = E.screenrows;
      while (tiems--) {
        //move the cursor to the top or bottom of the screen
        editorMoveCursor(c == PAGE_UP? ARROW_UP : ARROW_DOWN);
      } 
      break;
    }
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
    case CTRL_KEY('l'):
    case '\x1b':
      break;
    default:
      editorInsertChar(c);
      break;
  }
  quit_times = KILO_QUIT_TIMES;
  
}
void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

/*** output ***/
//https://vt100.net/docs/vt100-ug/chapter3.html#CUP

void editorRowInsertChar(erow *row, int at, int c) {
  if(at < 0 || at > row->size) return; // Check for invalid insertion position
  row->chars = realloc(row->chars, row->size + 2); // Reallocate memory for new character
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1); // Shift characters to make space
  row->size++; // Increase the row size
  row->chars[at] = c; // Insert the new character
  editorUpdateRow(row); // Update the rendered version of the row
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorDrawMessageBar(struct abuf *ab) {
  // Clear the message bar with escape sequence
  abAppend(ab, "\x1b[K", 3);
  // Get the length of the current status message
  int msglen = strlen(E.statusmsg);
  // Truncate message if it's longer than the screen width
  if (msglen > E.screencols) msglen = E.screencols;
  // Display the message if it's not empty and less than 5 seconds old
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmsg, msglen);
}


void editorRefreshScreen(){
  // Handle scrolling if the cursor has moved out of the visible area
  editorScroll();

  // Initialize an append buffer to store the screen update commands
  struct abuf ab = ABUF_INIT; // init an append buffer

  // Hide the cursor while updating the screen
  abAppend(&ab,"\x1b[?25l",6);

  // Move the cursor to the top-left corner of the screen
  // Note: "\x1b[2J" (clear entire screen) is commented out to avoid flickering
  abAppend(&ab, "\x1b[H", 3);

  // Draw the rows of the editor (text content or welcome message)
  editorDrawRows(&ab);
  editorDrawStatusBar(&ab); 
  editorDrawMessageBar(&ab);

  // Move the cursor to its current position in the editor
  char buf[32];
  // Calculate the actual screen position, accounting for scrolling offsets
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  // Show the cursor again
  abAppend(&ab, "\x1b[?25h", 6); 

  // Write the entire buffer to the terminal in one go
  write(STDOUT_FILENO, ab.b, ab.len);

  // Free the memory used by the append buffer
  abFree(&ab);
}

//varidaric function can take variable number of arguments
void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
}

// Function to draw the status bar at the bottom of the editor
void editorDrawStatusBar(struct abuf *ab) {
  // Set the terminal to inverted colors (background becomes foreground and vice versa)
  abAppend(ab, "\x1b[7m", 4);
  // Declare buffers for the left and right sides of the status bar
  char status[80], rstatus[80];
  // Format the left side of the status bar with filename and number of lines
  // Limit filename to 20 characters, use "[No Name]" if no filename is set
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    E.filename ? E.filename : "[No Name]", E.numrows,
    E.dirty ? "(modified)" : "");
  // Format the right side of the status bar with current line/total lines
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
    E.cy + 1, E.numrows);
  // Truncate the left status if it's longer than the screen width
  if (len > E.screencols) len = E.screencols;
  // Append the left status to the buffer
  abAppend(ab, status, len);
  // Fill the remaining space with either spaces or the right status
  while (len < E.screencols) {
    // If there's exactly enough space left for the right status
    if (E.screencols - len == rlen) {
      // Append the right status
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      // Otherwise, append a space and increment the length
      abAppend(ab, " ", 1);
      len++;
    }
  }

  // Reset the terminal colors back to normal
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

// Function to convert cursor x position (cx) to render x position (rx)
// This accounts for the presence of tab characters in the row
int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;  // Initialize render x position
  // Iterate through each character up to the cursor position
  for(int j = 0; j < cx; j++) {
    if(row->chars[j] == '\t') {
      // If the character is a tab, adjust rx to the next tab stop
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    } else {
      // For non-tab characters, simply increment rx
      rx++;
    }
  }
  // Return the final render x position
  return rx;
}


// Function to update the rendered version of a row
void editorUpdateRow(erow *row){
  int tabs = 0;
  for(int j = 0; j < row->size; j++) {
    if(row->chars[j] == '\t') {
      tabs++; 
    }
  }
  // Free the previously allocated memory for the rendered row
  free(row->render);
  // Allocate new memory for the rendered row
  // +1 for the null terminator
  row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);
  // Initialize index for the rendered row
  int idx = 0;
  // Loop through each character in the original row
  for(int j = 0; j < row->size; j++) {
    // handling special characters tabs
    if (row->chars[j] == '\t') {
      row->render[idx++] =' ';
      while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
    }else{
    // Copy the character to the rendered row
    row->render[idx++] = row->chars[j];
    }
  }
  // Null-terminate the rendered string
  row->render[idx] = '\0';
  // Update the size of the rendered row
  row->size = idx;
}
// Function to free the memory allocated for a single row
void editorFreeRow(erow *row) {
  // Free the memory allocated for the rendered version of the row
  free(row->render);
  // Free the memory allocated for the actual characters in the row
  free(row->chars);
}

void editorFindCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;
  if (key == '\r' || key == '\x1b') {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }
  if (last_match == -1) direction = 1;
  int current = last_match;
  int i;
  for (i = 0; i < E.numrows; i++) {
    current += direction;
    if (current == -1) current = E.numrows - 1;
    else if (current == E.numrows) current = 0;
    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      last_match = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;
      break;
    }
  }
}

void editorFind() {
  int saved_cx = E.cx; // save the cursor position and scroll position
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;
  char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",editorFindCallback);
  if (query) {
    free(query);
  }else{
    E.cx = saved_cx; //restore those values after the search is cancelled.
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}
// Function to delete a row from the editor at a specified index
void editorDelRow(int at) {
  // Check if the given index is valid
  if (at < 0 || at >= E.numrows) return;

  // Free the memory allocated for the row to be deleted
  editorFreeRow(&E.row[at]);

  // Move all subsequent rows up by one position
  // This effectively overwrites the deleted row with the next row
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));

  // Decrease the total number of rows in the editor
  E.numrows--;

  // Mark the file as modified
  E.dirty++;
}

// Function to draw the rows of the editor
void editorDrawRows(struct abuf *ab) {
  int y;
  // Loop through each row of the screen
  for (y = 0; y < E.screenrows; y++) {
    // Calculate the actual file row, accounting for vertical scroll offset
    int filerow = y + E.rowoff;
    // If we're past the end of the file
    if (filerow >= E.numrows) {
      // If the file is empty and we're at 1/3 of the screen height
      if (E.numrows == 0 && y == E.screenrows / 3) {
        // Display a welcome message
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Kilo editor -- version %s", KILO_VERSION);
        // Truncate welcome message if it's too long
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        // Calculate padding to center the welcome message
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          // Add a tilde at the start of the line
          abAppend(ab, "~", 1);
          padding--;
        }
        // Add spaces for padding
        while (padding--) abAppend(ab, " ", 1);
        // Append the welcome message
        abAppend(ab, welcome, welcomelen);
      } else {
        // For empty lines, just add a tilde
        abAppend(ab, "~", 1);
      }
    } else {
      // We're drawing a row with file content
      // Calculate the length of the row to display, accounting for horizontal scroll
      int len = E.row[filerow].size - E.coloff;
      if (len < 0) len = 0;
      // Truncate if it's longer than the screen width
      if (len > E.screencols) len = E.screencols;
      // Append the row content
      abAppend(ab, &E.row[filerow].chars[E.coloff], len);
    }

    // Clear the rest of the line
    abAppend(ab, "\x1b[K", 3);
    // Add a newline, except for the last row
    abAppend(ab, "\r\n", 2);
  }
}


// this function concatenates all rows into a single string
char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  // Calculate total length of all rows plus newline characters
  for (j = 0; j < E.numrows; j++)
    totlen += E.row[j].size + 1;
  // Store total length in the provided pointer
  *buflen = totlen;
  // Allocate memory for the entire text content
  char *buf = malloc(totlen);
  char *p = buf;
  // Copy each row's content into the buffer
  for (j = 0; j < E.numrows; j++) {
    // Copy the row's content
    memcpy(p, E.row[j].chars, E.row[j].size);
    // Move the pointer to the end of the copied content
    p += E.row[j].size;
    // Add a newline character
    *p = '\n';
    // Move the pointer past the newline
    p++;
  }
  // Return the buffer containing all rows
  return buf;
}

int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
  // Initialize buffer with a starting size of 128 bytes
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';  // Ensure the buffer starts as an empty string
  while (1) {
    // Display the prompt and current input in the status bar
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();

    // Read a key from the user
    int c = editorReadKey();

    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      // Handle backspace: remove the last character if buffer is not empty
      if (buflen != 0) buf[--buflen] = '\0';
    } else if(c == '\x1b'){
      // Handle escape: cancel the prompt
      editorSetStatusMessage("");
      if (callback) callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      // Handle return: if buffer is not empty, return the input
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback) callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      // Handle regular character input
      if (buflen == bufsize - 1) {
        // If buffer is full, double its size
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      // Append the character to the buffer
      buf[buflen++] = c;
      buf[buflen] = '\0';  // Ensure the buffer remains null-terminated
    }
    if (callback) callback(buf, c);
  }
}

void editorSave() {
  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as:%s (ESC to cancel) ", editorFindCallback); 
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
  }
  int len;
  char *buf = editorRowsToString(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        E.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }
  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}




/* init */
void initEditor() { // & refer to pass by reference, this way the data actually changed
  //init the screen size for the text editor (horizontal and vertical)
  E.cy = 0;
  E.cx = 0;
  E.rx = 0;
  E.numrows = 0;
  E.rowoff = 0;
  E.coloff = 0; // initialize column offsets
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  E.screenrows -= 2;
}
// Function to append a new row to the editor's content
// Parameters:
//   s: Pointer to the string content of the new row
//   len: Length of the string to be appended
void editorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.numrows) return; // check if the given index is valid
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1)); // reallocate memory for the new row
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at)); // move all subsequent rows down by one position

    // Set the size of the new row
    E.row[at].size = len;

    // Allocate memory for the new row's content
    // +1 for the null terminator
    E.row[at].chars = malloc(len + 1);

    // Copy the content from the input string to the new row
    memcpy(E.row[at].chars, s, len);

    // Null-terminate the new row's content
    E.row[at].chars[len] = '\0';

    // Initialize render-related fields
    // These are likely used for handling special characters or formatting
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);

    // Increment the total number of rows in the editor
    E.numrows++;
    E.dirty++; // Update dirty to indicate that the file has been modified
}
void editorInsertChar(int c){
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], E.cx,c);
  E.cx++;
}


// This function deletes a character from a specific row at a given position
void editorRowDelChar(erow *row, int at) {
    // Check if the deletion position is valid
    if (at < 0 || at >= row->size) return;
    
    // Move the characters after 'at' one position to the left, effectively deleting the character at 'at'
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    
    // Decrease the size of the row
    row->size--;
    
    // Update the rendered version of the row
    editorUpdateRow(row);
    
    // Mark the file as modified
    E.dirty++;
}

// This function handles the deletion of a character in the editor
void editorDelChar() {
    // If the cursor is beyond the last row, there's nothing to delete
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;
    
    // Get a pointer to the current row
    erow *row = &E.row[E.cy];
    
    // If the cursor is not at the beginning of the line
    if (E.cx > 0) {
        // Delete the character before the cursor
        editorRowDelChar(row, E.cx - 1);
        // Move the cursor one position to the left
        E.cx--;
    } else {
      E.cx = E.row[E.cy - 1].size;
      editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
      editorDelRow(E.cy);
      E.cy--;
    }
    // Note: This function doesn't handle backspace at the beginning of a line yet
}

int main(int argc , char *argv[]) {
  // Clear the entire screen and scrollback buffer
  write(STDOUT_FILENO, "\x1b[2J", 4); // Clear the entire screen
  write(STDOUT_FILENO, "\x1b[3J", 4); // Clear the scrollback buffer
  write(STDOUT_FILENO, "\x1b[H", 3); // Move the cursor to the top-left corner
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]); 
  }
  // asking read() to read 1 char byte for the standard input and put it into the variable c
  
  editorSetStatusMessage(
  "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");
  
  while(1){
    editorRefreshScreen();
    editorProcessKeyPress();
    }
  // run echo $? to get the return value
  return 0;
}
