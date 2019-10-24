all:
	gcc  simulator.c uart.c -lSDL2 -lpthread -o simulator

clean:
	rm -rf simulator

