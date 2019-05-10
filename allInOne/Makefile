CC := clang
CFLAGS := -std=c99 -O2 -Wall -g
LDFLAGS := -Wl,-rpath,/usr/local/lib64
LIBS := -lpthread -lcurl -lfcgi

TARGET := ./tt

$(TARGET): main.c Makefile
	$(CC) $(CFLAGS) main.c -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
