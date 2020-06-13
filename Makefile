CFLAGS += -std=c11 -g3 -Wall -Wextra	
LDLIBS += -lsgutils2

wdled: wdled.o

.PHONY: clean
clean:
	rm -f wdled *.o
