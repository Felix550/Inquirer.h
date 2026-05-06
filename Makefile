ifeq ($(OS),Windows_NT)
    OUT := test.exe
else
    OUT := test
endif

$(OUT): test.c Inquirer.h
	gcc -Wall -Wextra -o $(OUT) test.c
