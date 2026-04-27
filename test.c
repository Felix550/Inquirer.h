#include <stdio.h>
#define INQUIRER_IMPL
#include "Inquirer.h"

#define OPTIONS_LENGHT 50

bool validate_password(const char *input, const char *message, void *data)
{
    (void)message;
    (void)data;
    return strlen(input) >= 8;
}

int main(void)
{
    Option options[5] = {0};
    options[0] = (Option){.display = "Rust", .value = (void *)"rs"};
    options[1] = (Option){.display = "C", .value = (void *)"c"};
    options[2] = (Option){.display = "C++", .value = (void *)"cpp"};
    options[3] = (Option){.display = "Python", .value = (void *)"py"};
    options[4] = (Option){.display = "Lua", .value = (void *)"how?"};

    void *selected = Select("What's Your Favourite language:", options, 5, .amark = "!", .flags = SELECT_BORDER);
    printf("Input: %s\n", (char *)selected);

    char *name = Text("What's Your Name:", NULL);
    char *pswd = Text("What's Your Password:", .instruction = "Keep it a secret!", .validation = validate_password, .invalid_message = "Inserisci Una Password Valida", .flags = TEXT_PASSWORD);
    char *job = Text("What's Your Job:", .instruction = "Not everyone has one...", .flags = TEXT_HIDE_ECHO | TEXT_NOT_REQUIRED);

    printf("Welcome %s with password: %s, job: %s\n", name, pswd, job);

    free(name);
    free(pswd);
    free(job);

    char read[256];
    printf("Insert Something: ");
    readline(read, sizeof(read), true, false);

    printf("Inserted: %s\n", read);

    printf("Press Any Key...\n");
    _getch();
    return 0;
}
