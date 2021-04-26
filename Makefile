#
#
#


LIBRARY := ldataobject.a
LIBDBG := ldataobject-dbg.a

SOURCES := src/dataobject.c src/dataobject_json.c src/dataobject_protobuf.c src/dataobject_dump.c src/dataobject_tmpbuf.c

HEADERS := dataobject.h lib/dataobject_private.h

#
#
#

OBJECTS := ${SOURCES:.c=.o}
DBGOBJS := ${SOURCES:.c=.d}

default: ${LIBRARY}

all: ${LIBRARY} ${LIBDBG}

debug: ${LIBDBG}

clean: 
	/bin/rm -f ${LIBRARY} ${LIBDBG} ${OBJECTS} ${DBGOBJS}


${LIBRARY}: ${OBJECTS}
	ar -rcs $@ $^

${LIBDBG}: ${DBGOBJS}
	ar -rcs $@ $^

%.o : %.c
	gcc -c -o $@ $^

%.d : %.c 
	gcc -g -D DEBUG -c -o $@ $^

%.c : %.h ${HEADERS}

dataobjecttest: dataobjecttest.c ${LIBDBG}
	gcc -g -D DEBUG -o $@ $^
