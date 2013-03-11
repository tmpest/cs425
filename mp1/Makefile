all: chat

SOURCEFILES = unicast.cc mcast.cc chat.cc
HEADERS = mp1.h

chat: $(SOURCEFILES) $(HEADERS)
	g++ ${CFLAGS} -g -pthread -o $@ $(SOURCEFILES)

clean:	restart
	-rm -f chat *.o *~

restart:
	-rm -f GROUPLIST
