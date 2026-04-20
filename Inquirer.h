#ifndef INQUIRER_H_
#define INQUIRER_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#ifndef INQUIRERDEF
#ifdef INQUIRER_IMPL
#define INQUIRERDEF
#else
#define INQUIRERDEF extern
#endif
#endif // INQUIRERDEF

// Custom _getch() for cross-platform
#ifdef _WIN32

/* Windows */
#include <conio.h>
#include <windows.h>

int CursorIsOnLastRow(void)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return 0;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return 0;

    int cursorRow = csbi.dwCursorPosition.Y + 1; // 1-based
    int lastRow = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    return cursorRow == lastRow;
}

#else

/* POSIX (Linux / macOS / BSD) */
#include <termios.h>
#include <unistd.h>

static int getch_impl(void)
{
    struct termios old_attr, new_attr;
    int ch;

    tcgetattr(STDIN_FILENO, &old_attr);
    new_attr = old_attr;

    new_attr.c_lflag &= ~(ICANON | ECHO | ECHOE);

    tcsetattr(STDIN_FILENO, TCSANOW, &new_attr);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &old_attr);

    return ch;
}

INQUIRERDEF int _getch(void)
{
    return getch_impl();
}

INQUIRERDEF int putch(int c)
{
    int ret = putchar(c);
    fflush(stdout);
    return ret;
}

static int GetTerminalSize(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        return 0;

    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 1;
}

static int QueryCursorPositionANSI(int *row, int *col)
{
    struct termios oldt, raw;

    if (tcgetattr(STDIN_FILENO, &oldt) == -1)
        return 0;

    raw = oldt;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 10; // 1 second max wait

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
        return 0;

    printf("\x1b[6n");
    fflush(stdout);

    char buf[32];
    int i = 0;

    while (i < (int)sizeof(buf) - 1)
    {
        char ch;
        int n = (int)read(STDIN_FILENO, &ch, 1);
        if (n != 1)
            break;

        buf[i++] = ch;
        if (ch == 'R')
            break;
    }

    buf[i] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    int r = 0, c = 0;
    if (sscanf(buf, "\x1b[%d;%dR", &r, &c) != 2)
        return 0;

    *row = r;
    *col = c;
    return 1;
}

int CursorIsOnLastRow(void)
{
    int row = 0, col = 0, rows = 0, cols = 0;

    if (!QueryCursorPositionANSI(&row, &col))
        return 0;

    if (!GetTerminalSize(&rows, &cols))
        return 0;

    return row == rows;
}

#endif /* _WIN32 */

#ifndef DEFAULT_CAPACITY
#define DEFAULT_CAPACITY 1024 * 14
#endif

#define C_RESET "\033[0m"
#define C_BOLD "\x1b[1m"

#define C_BLACK "\033[30m"
#define C_RED "\033[31m"
#define C_GREEN "\033[32m"
#define C_YELLOW "\033[33m"
#define C_BLUE "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN "\033[36m"
#define C_WHITE "\033[37m"

#define C_BBLACK "\033[90m"
#define C_BRED "\033[91m"
#define C_BGREEN "\033[92m"
#define C_BYELLOW "\033[93m"
#define C_BBLUE "\033[94m"
#define C_BMAGENTA "\033[95m"
#define C_BCYAN "\033[96m"
#define C_BWHITE "\033[97m"

#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"

#define BG_BBLACK "\033[100m"
#define BG_BRED "\033[101m"
#define BG_BGREEN "\033[102m"
#define BG_BYELLOW "\033[103m"
#define BG_BBLUE "\033[104m"
#define BG_BMAGENTA "\033[105m"
#define BG_BCYAN "\033[106m"
#define BG_BWHITE "\033[107m"

#define DELETE_PREVIOUS_ROW "\033[1A\r\033[2K"
#define BOTTOM_LEFT "\x1b[999;1H"
#define DELETE_FROM_CURSOR "\x1b[0J"

// Callback called eachtime user sends submit, to validate an input, should return true or false if it's valid or not
INQUIRERDEF typedef bool (*validation_callback)(const char *input, const char *message, void *data);

// The Text input shold be hidden
#define TEXT_PASSWORD (1 << 0)

// Should not echo back after submit
#define TEXT_HIDE_ECHO (1 << 1)

// Field can be empty
#define TEXT_NOT_REQUIRED (1 << 2)

typedef struct
{
    // Mark when Asking
    char *qmark;

    // Mark when Answered
    char *amark;

    // Intructions on how to Answer
    char *instruction;

    // Callback on each submit, should return 0 if OK or NON 0 if not OK
    validation_callback validation;

    // Message to show when input is invalid
    char *invalid_message;

    // User data for callbacks
    void *data;

    // Flags, they start with TEXT_
    int flags;
} TextParams;

// reads a line from the stdin and puts result into out
//  put_newline: if should put a new line on submit
//  is_password: if should hide the input
// Example:
//  char out[DEFAULT_CAPACITY];
//  readline(out, sizeof(out), true, false);
//  ... use out ...
INQUIRERDEF void readline(char *out, size_t out_size, bool put_newline, bool is_password);

// Expanded Text Input, Must Call free(...) on the out char*
INQUIRERDEF char *TextEx(const char *message, TextParams *params);

// Show an error in the bottom left of the console
INQUIRERDEF void ShowError(const char *message);

// Clear the error
INQUIRERDEF void ClearError();

// Easy Interface for Text Input, Must Call free(...) on the out char*
// Call with:
//  Text("What's Your Name:",[Params])
//  [Params] = .[param_name]=[value]
//  Example:
//  char *name = Text("What's Your Name:", .qmark="?", .amark="!", .flags=TEXT_HIDE_ECHO | TEXT_PASSWORD);
//  ... use name ...
//  free(name);
#define Text(message, ...) TextEx(message, &(TextParams){__VA_ARGS__})

#ifdef INQUIRER_IMPL

void readline_old(char *out, size_t out_size)
{
    if (fgets(out, out_size, stdin))
    {
        out[strcspn(out, "\n")] = '\0';
    }
}

#ifdef _WIN32

#define KEY_ENTER '\r'
#define KEY_BACKSPACE '\b'
#define KEY_CTRL_C 3
#define KEY_ARROWS 224
#define KEY_ARROW_L 75
#define KEY_ARROW_R 77

#else

#define KEY_ENTER '\n'
#define KEY_BACKSPACE 127
#define KEY_CTRL_C 3
#define KEY_ARROWS 91
#define KEY_ARROW_L 'D'
#define KEY_ARROW_R 'C'

#endif // _WIN32

void readline(char *out, size_t out_size, bool put_newline, bool is_password)
{
    size_t len = 0; // string lenght
    size_t cur = 0; // cursor position

    while (1)
    {
        int ch = _getch();

        if (ch == 0 || ch == KEY_ARROWS)
        {
            int ext = _getch();

            if (ext == KEY_ARROW_L)
            {
                if (cur > 0)
                {
                    cur--;
                    putch('\b');
                }
            }
            else if (ext == KEY_ARROW_R)
            {
                if (cur < len)
                {
                    if(is_password)
                        putch('*');
                    else
                        putch(out[cur]);
                    cur++;
                }
            }
        }
        else if (ch == KEY_CTRL_C)
        {
            exit(0);
        }
        else if (ch == KEY_ENTER)
        {
            if (put_newline)
                putch('\n');
            break;
        }
        else if (ch == KEY_BACKSPACE)
        {
            if (cur > 0)
            {
                cur--;
                len--;
                memmove(&out[cur], &out[cur + 1], len - cur);

                putch('\b');
                for (size_t j = cur; j < len; j++)
                {
                    if (is_password)
                        putch('*'); // Print Hidden Char
                    else
                        putch(out[j]);
                }
                putch(' '); // delete last char
                for (size_t j = cur; j < len + 1; j++)
                    putch('\b');
            }
        }
        else if (isprint(ch))
        {
            if (len < out_size - 1)
            {
                memmove(&out[cur + 1], &out[cur], len - cur);
                out[cur] = (char)ch;
                len++;

                for (size_t j = cur; j < len; j++)
                {
                    if (is_password)
                        putch('*'); // Print Hidden Char
                    else
                        putch(out[j]);
                }
                cur++;
                for (size_t j = cur; j < len; j++)
                    putch('\b');
            }
        }
    }

    out[len] = '\0';
}

char *make_str(char c, size_t len)
{
    char *s = malloc(len + 1);
    if (!s)
        return NULL;

    memset(s, c, len);
    s[len] = '\0';

    return s;
}

void ShowError(const char *message)
{
    bool will_overwrite = CursorIsOnLastRow();

    printf("\x1b[s"); // save cursor

    if (will_overwrite)
        printf("\x1b[S");

    printf(BOTTOM_LEFT); // go bottom left

    printf("\x1b[2K"); // clean row
    printf(BG_RED C_BWHITE C_BOLD "%s" C_RESET, message);

    printf("\x1b[u"); // restore cursor

    if (will_overwrite)
        printf("\x1b[A");
}

void ClearError()
{
    printf("\x1b[s");           // save cursor

    printf("\r");               // got the the start of the line
    printf("\x1b[B");           // go down a line

    printf(DELETE_FROM_CURSOR); // clean up from cursor

    printf("\x1b[A");           // go up a line
    printf("\x1b[u");           // reset cursor
}

char *TextEx(const char *message, TextParams *p)
{
    TextParams params = {0};
    params.amark = "?";
    params.qmark = "?";
    params.instruction = "";
    params.validation = NULL;
    params.invalid_message = "Invalid";
    params.data = NULL;

    if (p)
    {
        if (p->amark)
            params.amark = p->amark;
        if (p->qmark)
            params.qmark = p->qmark;
        if (p->instruction)
            params.instruction = p->instruction;
        if (p->validation)
            params.validation = p->validation;
        if (p->invalid_message)
            params.invalid_message = p->invalid_message;
        if (p->data)
            params.data = p->data;

        if (p->flags)
            params.flags |= p->flags;
    }

    char *out = malloc(DEFAULT_CAPACITY);

    if (strcmp(params.instruction, "") != 0)
        printf(C_YELLOW "%s " C_RESET "%s " C_BWHITE "%s " C_BGREEN, params.qmark, message, params.instruction);
    else
        printf(C_YELLOW "%s " C_RESET "%s " C_BGREEN, params.qmark, message);

    size_t len = 0;
    size_t cur = 0;
    int show_invalid = 0;

    while (1)
    {
        printf(C_BGREEN);

        int ch = _getch();

        // clear invalid message
        if (show_invalid)
        {
            ClearError();
            show_invalid = 0;
        }

        if (ch == 0 || ch == KEY_ARROWS)
        {
            int ext = _getch();
            if (ext == KEY_ARROW_L)
            {
                if (cur > 0)
                {
                    cur--;
                    putch('\b');
                }
            }
            else if (ext == KEY_ARROW_R)
            {
                if (cur < len)
                {
                    if (params.flags & TEXT_PASSWORD)
                        putch('*'); // Print Hidden Char
                    else
                        putch(out[cur]);
                    cur++;
                }
            }
        }
        else if (ch == KEY_CTRL_C)
        {
            printf(C_RESET);
            exit(0);
        }
        else if (ch == KEY_ENTER)
        {

            out[len] = '\0';

            // true if it is not a requirement, false if it is a requirement and empty, true if it is a requirement and full
            bool empty_check = params.flags & TEXT_NOT_REQUIRED || strlen(out) > 0;

            if ((!params.validation || params.validation(out, message, params.data)) && empty_check)
            {
                break;
            }

            // show message if not valid
            show_invalid = 1;

            if (!empty_check)
                ShowError("This field is required");
            else
                ShowError(params.invalid_message);
        }
        else if (ch == KEY_BACKSPACE)
        {
            if (cur > 0)
            {
                cur--;
                len--;
                memmove(&out[cur], &out[cur + 1], len - cur);
                putch('\b');
                for (size_t j = cur; j < len; j++)
                {
                    if (params.flags & TEXT_PASSWORD)
                        putch('*'); // Print Hidden Char
                    else
                        putch(out[j]);
                }
                putch(' ');
                for (size_t j = cur; j < len + 1; j++)
                    putch('\b');
            }
        }
        else if (isprint(ch))
        {
            if (len < DEFAULT_CAPACITY - 1)
            {
                memmove(&out[cur + 1], &out[cur], len - cur);
                out[cur] = (char)ch;
                len++;
                for (size_t j = cur; j < len; j++)
                {
                    if (params.flags & TEXT_PASSWORD)
                        putch('*'); // Print Hidden Char
                    else
                        putch(out[j]);
                }
                cur++;
                for (size_t j = cur; j < len; j++)
                    putch('\b');
            }
        }
    }

    printf("\n");
    printf(DELETE_FROM_CURSOR);
    if (!(params.flags & TEXT_HIDE_ECHO))
    {
        if (params.flags & TEXT_PASSWORD)
        {
            char *hidden = make_str('*', strlen(out));
            printf(DELETE_PREVIOUS_ROW C_YELLOW "%s " C_RESET "%s " C_BBLUE "%s\n" C_RESET,
                   params.amark, message, hidden);
            free(hidden);
        }
        else
        {
            printf(DELETE_PREVIOUS_ROW C_YELLOW "%s " C_RESET "%s " C_BBLUE "%s\n" C_RESET,
                   params.amark, message, out);
        }
    }
    else
    {
        printf(DELETE_PREVIOUS_ROW C_RESET);
    }

    fflush(stdout);

    return out;
}

#endif // INQUIRER_IMPL

#endif // INQUIRER_H_