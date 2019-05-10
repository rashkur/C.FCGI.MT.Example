CC ?=clang
#CFLAGS=-O0 -g -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include
CFLAGS=-O2 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include
EXECUTABLE=main
LIBS=-lfcgi -lpthread -lcurl
PKGCONF=pkg-config

OBJ=tsearch.o querys.o helpers.o main.o
PREFIX=/usr/local

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS) `$(PKGCONF) --cflags --libs glib-2.0`
	
clean:
	rm -f $(EXECUTABLE) *.o

install: $(EXECUTABLE)
	install -m 775 $(EXECUTABLE) $(PREFIX)/bin

rminstall:
	rm -f $(PREFIX)/bin/$(EXECUTABLE)

