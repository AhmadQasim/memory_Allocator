TARGET  = libmem.so
SRC     = mem.c
OBJ = mem.o
OPTS1 = -Wall -c -fPIC
OPTS2 = -shared -o

all: $(TARGET)
$(TARGET): $(SRC)
	gcc $(OPTS1) $(SRC)
	gcc $(OPTS2) $(TARGET) $(OBJ)	
clean:
	rm -f $(TARGET)
