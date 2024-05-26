CC=gcc
CFLAGS=-g
LIBS=-lglut -lGL -lGLU -fno-stack-protector -lpthread -lm 
# x="this is paragraph one : ,1000! ; 55 % 33 , 18"

all: main 

main: main.c
	${CC} ${CFLAGS} main.c -o main $(LIBS)

run : main
	./main

clean:
	rm -f main


# Here's how you can use this Makefile:

# Save the Makefile to a file named Makefile.
# Open a terminal and navigate to the directory containing the Makefile.
# Type make to build the executables parent, child, and game.
# Type make clean to remove the executables.
# Note that the clean target is provided to remove the executables after they have been built. This is useful to avoid cluttering the directory with unnecessary files.
