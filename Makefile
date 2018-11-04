all: jobExecutor
jobExecutor: main.c defs.c decs.h
	gcc -o jobExecutor main.c defs.c decs.h -lm -g
clean:
	-rm -f jobExecutor