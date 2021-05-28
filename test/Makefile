
FILES:= dotest.c parsejson.c embeddedstring.c setget.c

dotest: $(FILES) ../ldataobject-dbg.a
	gcc -g $^ -o $@

rund: dotest
	gdbgui --args ./dotest

clean:
	rm -f dotest
	(cd .. ; make clean)

../ldataobject-dbg.a:
	(cd .. ; make debug)

