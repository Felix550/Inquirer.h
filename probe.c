#define INQUIRER_IMPL
#include "Inquirer.h"

int main(int argc, char const *argv[])
{
    for(size_t i = 0; i < 10; i++)
        printf("%d Ciao\n",i);

    char read[256];
    printf("Seleziona: ");
    readline(read, sizeof(read), true, false);

    printf("Inserted: %s\n", read);
    return 0;
}
