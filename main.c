#include <stdio.h>
#define INQUIRER_IMPL
#include "Inquirer.h"

bool validate_password(const char* input, const char* message, void* data) {
    (void)message;
    (void)data;
    return strlen(input) >= 8;
}

int main(void)
{
    char* name = Text("What's Your Name:", NULL);
    char* pswd = Text("What's Your Password:", .instruction = "Keep it a secret!", .validation = validate_password, .invalid_message = "Inserisci Una Password Valida", .flags = TEXT_PASSWORD);
    char* job = Text("What's Your Job:", .instruction = "Let's not make other see that...", .flags = TEXT_HIDE_ECHO);

    printf("Welcome %s with password: %s, job: %s\n", name, pswd, job);

    free(name);
    free(pswd);
    free(job);

    char read[256];
    printf("Inserisci Qualcosa: ");
    readline(read, sizeof(read), true, false);

    printf("inserito: %s\n", read);

    printf("Press Any Key...\n");
    _getch(); // Wait for a Key Press
    return 0;
}
