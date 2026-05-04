#include <stdio.h>
#define INQUIRER_IMPL
#include "Inquirer.h"

#define OPTIONS_LENGHT 5

bool validate_password(const char *input, const char *message, void *data)
{
    (void)message;
    (void)data;
    return strlen(input) >= 8;
}

int main(void)
{
    Option options[OPTIONS_LENGHT] = {0};
    options[0] = (Option){.display = "Rust", .value = (void *)"rs"};
    options[1] = (Option){.display = "C", .value = (void *)"how?"};
    options[2] = (Option){.display = "C++", .value = (void *)"cpp"};
    options[3] = (Option){.display = "Python", .value = (void *)"py"};
    options[4] = (Option){.display = "Lua", .value = (void *)"lua"};

    void *selected = Select("What's Your Favourite language:", options, OPTIONS_LENGHT, .amark = "!", .flags = SELECT_BORDER);
    printf("Input: %s\n", (char *)selected);

    // Multiselect example (will infer it's Type)
    MultiSelectResult *multi = Select("What Languages do you Hate:", options, OPTIONS_LENGHT, .amark = "!", .flags = SELECT_BORDER | SELECT_MULTISELECT, .required_count = 2);
    if (multi)
    {
        printf("You Hate: ");
        for (size_t i = 0; i < multi->count; i++)
        {
            if(i > 0)
                printf(" | %s (%s)", multi->selected[i].display, (char *)multi->selected[i].value);
            else
                printf("%s (%s)", multi->selected[i].display, (char *)multi->selected[i].value);
        }
        printf("\n");
        free(multi->selected);
        free(multi);
    }

    char *name = Text("What's Your Name:", NULL);
    char *pswd = Text("What's Your Password:", .instruction = "Keep it a secret!", .validation = validate_password, .invalid_message = "Inssert a valid password, > 8", .flags = TEXT_PASSWORD);
    char *job = Text("What's Your Job:", .instruction = "Not everyone has one...", .flags = TEXT_HIDE_ECHO | FIELD_NOT_REQUIRED);

    bool confirm = Confirm("Did you answer truthfully?",.amark = "!");
    printf("Welcome %s with password: %s, job: %s\n", name, pswd, job);

    if(confirm)
        printf("You aswered all thequestion truthfully, maybe\n");
    else
        printf("You are a LIAR!\n");

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
