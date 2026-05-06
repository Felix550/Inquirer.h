#ifndef INQUIRER_H_
#define INQUIRER_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <signal.h>

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

int CursorIsOnLastRow(int add)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return 0;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return 0;

    return csbi.dwCursorPosition.Y + add >= csbi.srWindow.Bottom;
}

void GetTerminalSize(int *rows, int *cols)
{
    CONSOLE_SCREEN_BUFFER_INFO c;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &c))
    {
        *cols = c.srWindow.Right - c.srWindow.Left + 1;
        *rows = c.srWindow.Bottom - c.srWindow.Top + 1;
    }
    else
    {
        *rows = 24;
        *cols = 80;
    }
}

#else

/* POSIX (Linux / macOS / BSD) */
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

INQUIRERDEF int _getch(void)
{
    struct termios old_attr, new_attr;
    int ch;

    tcgetattr(STDIN_FILENO, &old_attr);
    new_attr = old_attr;

    new_attr.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &new_attr);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &old_attr);

    return ch;
}

INQUIRERDEF int putch(int c)
{
    int ret = putchar(c);
    fflush(stdout);
    return ret;
}

void GetTerminalSize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    {
        *rows = 0;
        *cols = 0;
        return;
    }

    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

int QueryCursorPositionANSI(int *row, int *col)
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

int CursorIsOnLastRow(int add)
{
    int row = 0, col = 0, rows = 0, cols = 0;

    if (!QueryCursorPositionANSI(&row, &col))
        return 0;

    GetTerminalSize(&rows, &cols);

    return row + add >= rows;
}

static struct termios orig_term;

void init_terminal(void)
{
    tcgetattr(STDIN_FILENO, &orig_term);
}

void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
    printf("\n");
    fflush(stdout);
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
#define DELETE_ROW "\r\x1b[2K"

#define TMP_SPRINTF_SLOTS 8
#define TMP_SPRINTF_SIZE 1024

INQUIRERDEF char *tmp_sprintf(const char *fmt, ...);

// Callback called eachtime user sends submit, to validate an input, should return true or false if it's valid or not
INQUIRERDEF typedef bool (*validation_callback)(const char *input, const char *message, void *data);

// Flags

// Flags for Text, TEXT_ Prefix

// The Text input shold be hidden
#define TEXT_PASSWORD (1 << 0)

// Should not echo back after submit
#define TEXT_HIDE_ECHO (1 << 1)

// Flags for Select, SELECT_ Prefix

// The selection box should have a border
#define SELECT_BORDER (1 << 0)

// Enable multiselect mode
#define SELECT_MULTISELECT (1 << 1)

// Enable Fuzzy Search on Select/Muliselect, You can't put spaces
#define SELECT_FUZZY (1 << 2)

// General Flags

// Field can be empty
#define FIELD_NOT_REQUIRED (1 << 16)

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

    // Message to show when input is required but was submitted empty
    char *required_message;

    // User data for callbacks
    void *data;

    // Flags, they start with TEXT_
    int flags;
} TextParams;

typedef struct
{
    // Mark when Asking
    char *qmark;

    // Mark when Answered
    char *amark;

    // Intructions on how to Answer
    char *instruction;

    // Max options visible
    int visible;

    // Flags, they start with SELECT_
    int flags;

    //----------Multiselect specific----------

    // The text displayer after the ':' when you selected nothing
    char *empty_display;

    // Minimum number of selections required (for multiselect, default 1 if FIELD_NOT_REQUIRED not set)
    int required_count;

    // Message to show when input is required but was submitted empty
    char *required_message;

    // Marks to show when option is selected/unselected (default [X] / [ ])
    char *selected_mark;
    char *unselected_mark;
} SelectParams;

typedef struct
{
    char *display;
    void *value;
} Option;

// Result structure for multiselect
typedef struct
{
    Option *selected; // Array of selected options
    size_t count;     // Count of selected options
} MultiSelectResult;

typedef struct
{
    // Mark when Asking
    char *qmark;

    // Mark when Answered
    char *amark;

    // Intructions on how to Answer, default '(y/N)'
    char *instruction;
} ConfirmParams;

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

// Expanded Select Input (for single select, returns void*; for multiselect, returns MultiSelectResult*)
INQUIRERDEF void *SelectEx(const char *message, Option *options, size_t options_lenght, SelectParams *params);

// expanded Confirm
bool ConfirmEx(const char *message, ConfirmParams *params);

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

// Easy Interface for Select Input
// Call with:
//  Select("What's Your Favourite language:",[options],[options_count],[Params])
//  [Params] = .[param_name]=[value]
//  Example:
//  // We Define the options that can be chosen, can be any vector, static one, dynamic one, etc...
//  // Just has to be a vector off 'Option', a struct with .display as the text displayed, and .value, a void* as the returned value when selected
//  Option options[5] = {0};
//  options[0] = (Option){.display = "Rust", .value = (void *)"rs"};
//  options[1] = (Option){.display = "C", .value = (void *)"c"};
//  options[2] = (Option){.display = "C++", .value = (void *)"cpp"};
//  options[3] = (Option){.display = "Python", .value = (void *)"py"};
//  options[4] = (Option){.display = "Lua", .value = (void *)"how?"};
//
//  // Single select:
//  void *selected = Select("What's Your Favourite language:", options, 5);
//                                                                      ^---- options_count
//  ... use selected (casting it to it's original type) ...
//
//  // Multiselect:
//  MultiSelectResult *result = Select("Select languages:", options, 5, .flags = SELECT_MULTISELECT);
//  for (size_t i = 0; i < result->count; i++)
//    printf("Selected: %s\n", result->selected[i].display);
//  free(result->selected);
//  free(result);
#define Select(message, options, options_lenght, ...) SelectEx(message, options, options_lenght, &(SelectParams){__VA_ARGS__})

#define Confirm(message, ...) ConfirmEx(message, &(ConfirmParams){__VA_ARGS__});

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
#define KEY_ARROW_U 72
#define KEY_ARROW_D 80
#define KEY_ARROW_R 77
#define KEY_ARROW_L 75

#else

#define KEY_ENTER '\n'
#define KEY_BACKSPACE 127
#define KEY_CTRL_C 3
#define KEY_ARROWS 91
#define KEY_ARROW_U 'A'
#define KEY_ARROW_D 'B'
#define KEY_ARROW_R 'C'
#define KEY_ARROW_L 'D'

void handler(int sig)
{
    (void)sig;
    restore_terminal();
    printf(C_RESET DELETE_FROM_CURSOR);
    printf("\x1b[?25h");
    fflush(stdout);
    _exit(0);
}

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
                    if (is_password)
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

char *tmp_sprintf(const char *fmt, ...)
{
    static char buffers[TMP_SPRINTF_SLOTS][TMP_SPRINTF_SIZE];
    static int index = 0;

    char *buf = buffers[index];
    index = (index + 1) % TMP_SPRINTF_SLOTS;

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, TMP_SPRINTF_SIZE, fmt, args);
    va_end(args);

    return buf;
}

void ShowError(const char *message)
{
    printf("\x1b[s");    // save cursor
    printf(BOTTOM_LEFT); // move to bottom left
    printf(DELETE_ROW);  // clean row
    printf(BG_RED C_BWHITE C_BOLD "%s" C_RESET, message);
    printf("\x1b[u"); // restore cursor
    fflush(stdout);
}

void ClearError()
{
    printf("\x1b[s");    // save cursor
    printf(BOTTOM_LEFT); // move to bottom left
    printf(DELETE_ROW);  // clean row
    printf("\x1b[u");    // restore cursor
    fflush(stdout);
}

char *TextEx(const char *message, TextParams *p)
{
#ifndef _WIN32
    init_terminal();
    signal(SIGINT, handler);
#else
    SetConsoleOutputCP(CP_UTF8);
#endif

    TextParams params = {0};
    params.amark = "?";
    params.qmark = "?";
    params.instruction = "";
    params.validation = NULL;
    params.invalid_message = "Invalid";
    params.required_message = "This field is required";
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
        if (p->required_message)
            params.required_message = p->required_message;
        if (p->data)
            params.data = p->data;

        if (p->flags)
            params.flags |= p->flags;
    }

    char *out = malloc(DEFAULT_CAPACITY);

    // Handle overflow: check if cursor is on last row and scroll if needed
    // if (CursorIsOnLastRow(1))
    //    printf("\x1b[A"); // Scroll up one line

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
            bool empty_check = params.flags & FIELD_NOT_REQUIRED || len > 0;

            if ((!params.validation || params.validation(out, message, params.data)) && empty_check)
            {
                break;
            }

            // show message if not valid
            show_invalid = 1;

            if (!empty_check)
                ShowError(params.required_message);
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

static void reserve_block(size_t n, int flow)
{
    for (size_t i = 0; i < n; i++)
        putchar('\n');
    printf("\x1b[%zuA", n + flow);
    fflush(stdout);
}

static int fuzzy_match_ci(const char *text, const char *query)
{
    if (!query || !*query)
        return 1;
    if (!text)
        return 0;

    size_t qi = 0;
    for (size_t ti = 0; text[ti] && query[qi]; ++ti)
    {
        if (tolower((unsigned char)text[ti]) == tolower((unsigned char)query[qi]))
            ++qi;
    }

    return query[qi] == '\0';
}

static size_t build_filtered_indices(Option *options,
                                     size_t options_length,
                                     const char *query,
                                     size_t *filtered)
{
    size_t count = 0;

    for (size_t i = 0; i < options_length; ++i)
    {
        if (fuzzy_match_ci(options[i].display, query))
            filtered[count++] = i;
    }

    return count;
}

static void build_fuzzy_prompt(char *dst, size_t dst_size,
                               const char *qmark,
                               const char *message,
                               const char *query,
                               size_t query_cur,
                               const char *instruction)
{
    size_t pos = 0;
    size_t qlen = strlen(query);

    pos += (size_t)snprintf(dst + pos, dst_size - pos,
                            C_YELLOW "%s " C_RESET "%s ",
                            qmark, message);

    pos += (size_t)snprintf(dst + pos, dst_size - pos, C_BGREEN);

    for (size_t i = 0; i < qlen && pos < dst_size; ++i)
    {
        if (i == query_cur)
            pos += (size_t)snprintf(dst + pos, dst_size - pos, C_BWHITE "|" C_BGREEN);

        pos += (size_t)snprintf(dst + pos, dst_size - pos, "%c", query[i]);
    }

    if (query_cur == qlen && pos < dst_size)
        pos += (size_t)snprintf(dst + pos, dst_size - pos, C_BWHITE "|" C_BGREEN);

    pos += (size_t)snprintf(dst + pos, dst_size - pos, C_RESET);

    if (instruction && instruction[0])
        pos += (size_t)snprintf(dst + pos, dst_size - pos,
                                " " C_BWHITE "%s" C_RESET, instruction);
}

void *SelectEx(const char *message,
               Option *options,
               size_t options_length,
               SelectParams *p)
{
#ifndef _WIN32
    init_terminal();
    signal(SIGINT, handler);
#else
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (!options || options_length == 0)
        return NULL;

    SelectParams params = {0};
    params.amark = "?";
    params.qmark = "?";
    params.instruction = "(Use arrow keys)";
    params.visible = 20;
    params.required_count = 1;
    params.empty_display = "Nothing";
    params.required_message = "This field is required";
    params.selected_mark = "[X]";
    params.unselected_mark = "[ ]";

    if (p)
    {
        if (p->amark)
            params.amark = p->amark;
        if (p->qmark)
            params.qmark = p->qmark;
        if (p->instruction)
            params.instruction = p->instruction;
        if (p->visible)
            params.visible = p->visible;
        if (p->required_count > 0)
            params.required_count = p->required_count;
        if (p->empty_display)
            params.empty_display = p->empty_display;
        if (p->required_message)
            params.required_message = p->required_message;
        if (p->selected_mark)
            params.selected_mark = p->selected_mark;
        if (p->unselected_mark)
            params.unselected_mark = p->unselected_mark;
        if (p->flags)
            params.flags |= p->flags;
    }

    int has_border = (params.flags & SELECT_BORDER) != 0;
    int is_multiselect = (params.flags & SELECT_MULTISELECT) != 0;
    int is_fuzzy = (params.flags & SELECT_FUZZY) != 0;

    printf("\x1b[?25l");
    fflush(stdout);

    size_t buf_size = ((size_t)params.visible + 6) * 512;
    char *buf = (char *)malloc(buf_size);
    if (!buf)
    {
        printf("\x1b[?25h");
        return NULL;
    }

    size_t *filtered = (size_t *)malloc(options_length * sizeof(size_t));
    if (!filtered)
    {
        printf("\x1b[?25h");
        free(buf);
        return NULL;
    }

    bool *selected = NULL;
    if (is_multiselect)
    {
        selected = (bool *)calloc(options_length, sizeof(bool));
        if (!selected)
        {
            printf("\x1b[?25h");
            free(filtered);
            free(buf);
            return NULL;
        }
    }

    char query[256] = {0};
    size_t query_len = 0;
    size_t query_cur = 0;

    int current_pos = 0;      // position in filtered list
    size_t current_index = 0; // original option index
    size_t top = 0;
    size_t last_block_h = 0;
    int first = 1;
    int show_invalid = 0;

    while (1)
    {
        int trows, tcols;
        GetTerminalSize(&trows, &tcols);
        (void)tcols;

        size_t visible = (size_t)params.visible;
        if (visible > options_length)
            visible = options_length;

        size_t overhead = 1 + (has_border ? 2 : 0);
        int available = trows - (int)overhead - (int)has_border;
        size_t max_visible = (size_t)(available >= 1 ? available : 1);

        if (visible > max_visible)
            visible = max_visible;
        if (visible < 1)
            visible = 1;

        size_t block_h = visible + overhead;

        if (first)
        {
            int flow = 0;
            if (CursorIsOnLastRow((int)block_h - 1))
            {
                flow = 1;
                printf("\x1b[S");
            }

            reserve_block(block_h, flow);
            first = 0;
        }
        else if (block_h > last_block_h)
        {
            for (size_t i = 0; i < block_h - last_block_h; i++)
                putchar('\n');
            printf("\x1b[%zuA", block_h);
            fflush(stdout);
        }

        size_t filtered_count = options_length;
        if (is_fuzzy)
            filtered_count = build_filtered_indices(options, options_length, query, filtered);
        else
        {
            for (size_t i = 0; i < options_length; i++)
                filtered[i] = i;
        }

        if (filtered_count == 0)
        {
            current_pos = -1;
        }
        else
        {
            int found = -1;
            for (size_t i = 0; i < filtered_count; i++)
            {
                if (filtered[i] == current_index)
                {
                    found = (int)i;
                    break;
                }
            }

            if (found < 0)
            {
                current_pos = 0;
                current_index = filtered[0];
            }
            else
            {
                current_pos = found;
            }
        }

        if (current_pos >= 0)
        {
            if (current_pos < (int)top)
                top = (size_t)current_pos;
            else if (current_pos >= (int)(top + visible))
                top = (size_t)current_pos - visible + 1;
        }
        else
        {
            top = 0;
        }

        int scroll_up = (top > 0);
        int scroll_down = (top + visible < filtered_count);

        size_t pos = 0;

        pos += (size_t)snprintf(buf + pos, buf_size - pos, DELETE_ROW);

        if (is_fuzzy)
        {
            build_fuzzy_prompt(buf + pos, buf_size - pos,
                               params.qmark, message, query, query_cur, params.instruction);
            pos += strlen(buf + pos);
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "\n");
        }
        else
        {
            if (params.instruction && params.instruction[0])
            {
                pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                        C_YELLOW "%s " C_RESET "%s " C_BWHITE "%s" C_RESET "\n",
                                        params.qmark, message, params.instruction);
            }
            else
            {
                pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                        C_YELLOW "%s " C_RESET "%s\n",
                                        params.qmark, message);
            }
        }

        if (has_border)
        {
            pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                    DELETE_ROW C_BBLACK "┌");
            for (int i = 2; i < tcols; i++)
                pos += (size_t)snprintf(buf + pos, buf_size - pos, "─");
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "┐\n" C_RESET);
        }

        for (size_t i = 0; i < visible; i++)
        {
            size_t idx = top + i;
            pos += (size_t)snprintf(buf + pos, buf_size - pos, DELETE_ROW);

            if (has_border)
                pos += (size_t)snprintf(buf + pos, buf_size - pos, C_BBLACK "│" C_RESET);

            if (filtered_count == 0)
            {
                if (i == 0)
                {
                    pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                            "  " C_BBLACK "%s" C_RESET,
                                            params.empty_display);
                }
            }
            else if (idx < filtered_count)
            {
                size_t original = filtered[idx];

                if ((int)idx == current_pos)
                {
                    if (is_multiselect)
                    {
                        const char *checkbox = selected[original] ? params.selected_mark : params.unselected_mark;
                        pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                                C_CYAN "> " C_BWHITE "%s " C_RESET "%s" C_RESET,
                                                checkbox, options[original].display);
                    }
                    else
                    {
                        pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                                C_CYAN "> " C_BWHITE "%s" C_RESET,
                                                options[original].display);
                    }
                }
                else
                {
                    if (is_multiselect)
                    {
                        const char *checkbox = selected[original] ? params.selected_mark : params.unselected_mark;
                        pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                                "  " C_BBLACK "%s " C_RESET "%s",
                                                checkbox, options[original].display);
                    }
                    else
                    {
                        const char *arrow = "";
                        if (i == 0 && scroll_up)
                            arrow = "  " C_BBLACK "^" C_RESET;
                        if (i == visible - 1 && scroll_down)
                            arrow = "  " C_BBLACK "v" C_RESET;

                        pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                                "  " C_BBLACK "%s" C_RESET "%s",
                                                options[original].display, arrow);
                    }
                }
            }

            if (has_border)
                pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                        "\x1b[%dG" C_BBLACK "│" C_RESET, tcols);

            pos += (size_t)snprintf(buf + pos, buf_size - pos, "\n");
        }

        if (has_border)
        {
            pos += (size_t)snprintf(buf + pos, buf_size - pos,
                                    DELETE_ROW C_BBLACK "└");
            for (int i = 2; i < tcols; i++)
                pos += (size_t)snprintf(buf + pos, buf_size - pos, "─");
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "┘\n" C_RESET);
        }

        if (last_block_h > block_h)
        {
            for (size_t i = 0; i < last_block_h - block_h; i++)
                pos += (size_t)snprintf(buf + pos, buf_size - pos, DELETE_ROW "\n");
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "\x1b[%zuA", last_block_h);
        }
        else
        {
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "\x1b[%zuA", block_h);
        }
        last_block_h = block_h;

        fwrite(buf, 1, pos, stdout);
        fflush(stdout);

        int ch = _getch();

        if (show_invalid)
        {
            ClearError();
            show_invalid = 0;
        }

        if (ch == KEY_CTRL_C)
        {
            printf(DELETE_FROM_CURSOR);
            printf("\x1b[?25h");
            fflush(stdout);
            free(filtered);
            free(buf);
            if (selected)
                free(selected);
            exit(0);
        }

        if (is_fuzzy)
        {
            if (ch == 0 || ch == KEY_ARROWS)
            {
                int ext = _getch();

                if (ext == KEY_ARROW_L)
                {
                    if (query_cur > 0)
                        query_cur--;
                }
                else if (ext == KEY_ARROW_R)
                {
                    if (query_cur < query_len)
                        query_cur++;
                }
                else if (ext == KEY_ARROW_U)
                {
                    if (filtered_count > 0 && current_pos >= 0)
                    {
                        current_pos = (current_pos > 0) ? current_pos - 1 : (int)filtered_count - 1;
                        current_index = filtered[(size_t)current_pos];
                    }
                }
                else if (ext == KEY_ARROW_D)
                {
                    if (filtered_count > 0 && current_pos >= 0)
                    {
                        current_pos = (current_pos < (int)filtered_count - 1) ? current_pos + 1 : 0;
                        current_index = filtered[(size_t)current_pos];
                    }
                }
                continue;
            }

            if (ch == KEY_BACKSPACE)
            {
                if (query_cur > 0)
                {
                    memmove(&query[query_cur - 1],
                            &query[query_cur],
                            query_len - query_cur + 1);
                    query_cur--;
                    query_len--;
                }
                continue;
            }

            if (ch == KEY_ENTER)
            {
                if (is_multiselect)
                {
                    size_t selected_count = 0;
                    for (size_t i = 0; i < options_length; i++)
                    {
                        if (selected[i])
                            selected_count++;
                    }

                    int is_required = !(params.flags & FIELD_NOT_REQUIRED);
                    int min_required = is_required ? params.required_count : 0;

                    if (selected_count < (size_t)min_required)
                    {
                        show_invalid = 1;
                        ShowError(params.required_message);
                        continue;
                    }
                }
                else
                {
                    if (filtered_count == 0)
                        continue;
                }

                break;
            }

            if (is_multiselect && ch == '\t')
            {
                if (current_pos >= 0 && filtered_count > 0)
                {
                    size_t original = filtered[(size_t)current_pos];
                    selected[original] = !selected[original];
                }
                continue;
            }

            if (isprint(ch) && ch != '\t')
            {
                if (query_len < sizeof(query) - 1)
                {
                    memmove(&query[query_cur + 1],
                            &query[query_cur],
                            query_len - query_cur + 1);
                    query[query_cur] = (char)ch;
                    query_cur++;
                    query_len++;
                }
            }

            continue;
        }

        if (ch == 0 || ch == KEY_ARROWS)
        {
            ch = _getch();

            if (filtered_count > 0 && current_pos >= 0)
            {
                if (ch == KEY_ARROW_U)
                    current_pos = (current_pos > 0) ? current_pos - 1 : (int)filtered_count - 1;
                else if (ch == KEY_ARROW_D)
                    current_pos = (current_pos < (int)filtered_count - 1) ? current_pos + 1 : 0;

                current_index = filtered[(size_t)current_pos];
            }
        }
        else if (ch == KEY_ENTER)
        {
            if (is_multiselect)
            {
                size_t selected_count = 0;
                for (size_t i = 0; i < options_length; i++)
                {
                    if (selected[i])
                        selected_count++;
                }

                int is_required = !(params.flags & FIELD_NOT_REQUIRED);
                int min_required = is_required ? params.required_count : 0;

                if (selected_count < (size_t)min_required)
                {
                    show_invalid = 1;
                    ShowError(params.required_message);
                    continue;
                }
            }
            else
            {
                if (filtered_count == 0)
                    continue;
            }

            break;
        }
        else if (ch == '\t' && is_multiselect)
        {
            if (current_pos >= 0 && filtered_count > 0)
            {
                size_t original = filtered[(size_t)current_pos];
                selected[original] = !selected[original];
            }
        }
    }

    printf(DELETE_FROM_CURSOR);

    if (is_multiselect)
    {
        size_t count = 0;
        for (size_t i = 0; i < options_length; i++)
        {
            if (selected[i])
                count++;
        }

        MultiSelectResult *result = (MultiSelectResult *)malloc(sizeof(MultiSelectResult));
        if (result)
        {
            result->count = count;
            result->selected = (Option *)malloc(count * sizeof(Option));

            if (result->selected && result->count > 0)
            {
                size_t idx = 0;
                for (size_t i = 0; i < options_length; i++)
                {
                    if (selected[i])
                        result->selected[idx++] = options[i];
                }

                char *display = (char *)malloc(DEFAULT_CAPACITY);
                if (display)
                {
                    size_t dpos = 0;
                    for (size_t i = 0; i < result->count; i++)
                    {
                        if (i > 0)
                            dpos += snprintf(display + dpos, DEFAULT_CAPACITY - dpos, ", ");
                        dpos += snprintf(display + dpos, DEFAULT_CAPACITY - dpos, "%s",
                                         result->selected[i].display);
                    }

                    printf(DELETE_ROW C_YELLOW "%s " C_RESET "%s " C_BBLUE "%s" C_RESET "\n",
                           params.amark, message, display);
                    free(display);
                }
            }
            else
            {
                if (result->selected)
                    free(result->selected);
                result->selected = NULL;

                printf(DELETE_ROW C_YELLOW "%s " C_RESET "%s " C_BBLUE "%s" C_RESET "\n",
                       params.amark, message, params.empty_display);
            }
        }

        printf("\x1b[?25h");
        fflush(stdout);
        free(filtered);
        free(buf);
        free(selected);

        return (void *)result;
    }
    else
    {
        printf(DELETE_ROW C_YELLOW "%s " C_RESET "%s " C_BBLUE "%s" C_RESET "\n",
               params.amark, message, options[current_index].display);
        printf("\x1b[?25h");
        fflush(stdout);
        free(filtered);
        free(buf);

        return options[current_index].value;
    }
}

bool ConfirmEx(const char *message, ConfirmParams *p)
{
#ifndef _WIN32
    init_terminal();
    signal(SIGINT, handler);
#else
    SetConsoleOutputCP(CP_UTF8);
#endif
    ConfirmParams params = {0};
    params.amark = "?";
    params.qmark = "?";
    params.instruction = "(y/N)";

    if (p)
    {
        if (p->amark)
            params.amark = p->amark;
        if (p->qmark)
            params.qmark = p->qmark;

        if (p->instruction)
            params.instruction = p->instruction;
    }

    // Handle overflow: check if cursor is on last row and scroll if needed
    if (CursorIsOnLastRow(1))
        printf("\x1b[S"); // Scroll up one line

    if (strcmp(params.instruction, "") != 0)
        printf(C_YELLOW "%s " C_RESET "%s " C_BWHITE "%s " C_BGREEN, params.qmark, message, params.instruction);
    else
        printf(C_YELLOW "%s " C_RESET "%s " C_BGREEN, params.qmark, message);

    bool result = false;

    while (1)
    {
        printf(C_BGREEN);

        int ch = _getch();

        if (ch == 'y' || ch == 'Y')
        {
            result = true;
            break;
        }
        else if (ch == 'n' || ch == 'N')
        {
            result = false;
            break;
        }
        else if (ch == KEY_CTRL_C)
        {
            exit(0);
        }
    }

    char display_result[4];

    if (result)
        snprintf(display_result, sizeof(display_result), "Yes");
    else
        snprintf(display_result, sizeof(display_result), "No");

    printf(DELETE_FROM_CURSOR);
    printf(DELETE_ROW C_YELLOW "%s " C_RESET "%s " C_BBLUE "%s" C_RESET "\n",
           params.amark, message, display_result);

    return result;
}

#endif // INQUIRER_IMPL

#endif // INQUIRER_H_