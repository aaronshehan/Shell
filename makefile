OBJS	= bash2.o main.o myHistory.o
SOURCE	= bash2.c main.c myHistory.c
HEADER	= bash2.h myHistory.h
OUT	= a.out
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

bash2.o: bash2.c
	$(CC) $(FLAGS) bash2.c 

main.o: main.c
	$(CC) $(FLAGS) main.c 

myHistory.o: myHistory.c
	$(CC) $(FLAGS) myHistory.c 


clean:
	rm -f $(OBJS) $(OUT)