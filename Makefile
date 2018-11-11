IDIR = .
CC=g++
CFLAGS=-I$(IDIR) -g -O0 -Wall

ODIR=obj
LDIR =../lib

LIBS= -lpthread

_DEPS = Server.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = Server.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	mkdir -p ./$(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

Server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm  $(ODIR)/*.o *~ core $(INCDIR)/*~ 