
embeddedstring: embeddedstring.c ../ldataobject-dbg.a
	gcc $^ -o $@

setget: setget.c ../ldataobject-dbg.a
	gcc $^ -o $@

clean:
	rm -f setget embeddedstring
	(cd .. ; make clean)

../ldataobject-dbg.a:
	(cd .. ; make debug)

