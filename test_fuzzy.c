#include <stdio.h>
#define INQUIRER_IMPL
#include "Inquirer.h"

#define OPTIONS_LENGHT 10

int main(void)
{
    Option options[OPTIONS_LENGHT] = {0};
    options[0] = (Option){.display = "Rust", .value = (void *)"rs"};
    options[1] = (Option){.display = "C", .value = (void *)"c"};
    options[2] = (Option){.display = "C++", .value = (void *)"cpp"};
    options[3] = (Option){.display = "Python", .value = (void *)"py"};
    options[4] = (Option){.display = "Lua", .value = (void *)"lua"};
    options[5] = (Option){.display = "JavaScript", .value = (void *)"js"};
    options[6] = (Option){.display = "Java", .value = (void *)"java"};
    options[7] = (Option){.display = "Go", .value = (void *)"go"};
    options[8] = (Option){.display = "Ruby", .value = (void *)"rb"};
    options[9] = (Option){.display = "TypeScript", .value = (void *)"ts"};
    
    printf("===== Fuzzy Select Example =====\n\n");
    
    // Single select with fuzzy search
    printf("Single select with FUZZY SEARCH:\n");
    printf("Try typing 'rs', 'p', 'js', 'c++', etc. to filter options\n\n");
    void *selected = Select("Select a language:", options, OPTIONS_LENGHT, 
                           .amark = "!", 
                           .flags = SELECT_BORDER | SELECT_FUZZY,
                           .instruction = "(Type to search, arrows to navigate)");
    printf("You selected: %s\n\n", (char *)selected);
    
    // Multiselect with fuzzy search
    printf("Multiselect with FUZZY SEARCH:\n");
    printf("Try typing to filter, use SPACE to toggle selection, arrows to navigate\n\n");
    MultiSelectResult *multi = Select("Select languages you like:", options, OPTIONS_LENGHT, 
                                     .amark = "!", 
                                     .flags = SELECT_BORDER | SELECT_MULTISELECT | SELECT_FUZZY,
                                     .instruction = "(Type to search, SPACE to select)",
                                     .required_count = 1);
    if (multi)
    {
        printf("You selected: ");
        for (size_t i = 0; i < multi->count; i++)
        {
            if(i > 0)
                printf(", ");
            printf("%s", multi->selected[i].display);
        }
        printf("\n");
        free(multi->selected);
        free(multi);
    }
    
    printf("\nPress Any Key...\n");
    _getch();
    return 0;
}
