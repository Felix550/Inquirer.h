ifeq ($(OS),Windows_NT)
    OUT := main.exe
else
    OUT := main
endif

$(OUT): main.c inquirer.h
	gcc -Wall -Wextra -o $(OUT) main.c