
setget: setget.c ../ldataobject-dbg.a
	gcc $^ -o $@

clean:
	rm -f setget
	(cd .. ; make clean)

../ldataobject-dbg.a:
	(cd .. ; make debug)

