fusionfs : fusionfs.o util.o log.o
	gcc -g -o fusionfs fusionfs.o util.o log.o -L/mnt/common/cshou/usr/lib -L./ffsnet -lffsnet_bridger -L./zht/lib -lzht -I./provenance -L./provenance -lprovenance  -lstdc++ -lprotobuf -lprotobuf-c -lpthread -I/mnt/common/cshou/usr/include `pkg-config fuse --libs`

fusionfs.o : fusionfs.c log.h params.h util.h
	gcc -g -Wall -c fusionfs.c -L/mnt/common/cshou/usr/lib -L./ffsnet -lffsnet_bridger -I./provenance -L./provenance -lprovenance -lpthread -I/mnt/common/cshou/usr/include `pkg-config fuse --cflags`
	
util.o : util.c log.h params.h
	gcc -g -Wall -c util.c -L./zht/lib -lzht -lstdc++ -lprotobuf -lprotobuf-c -lpthread -I/mnt/common/cshou/usr/include `pkg-config fuse --cflags` 

log.o : log.c log.h params.h
	gcc -g -Wall -c log.c `pkg-config fuse --cflags`

clean:
	rm -f fusionfs *.o 

dist:
	rm -rf fuse-tutorial/
	mkdir fuse-tutorial/
	cp ../*.html fuse-tutorial/
	mkdir fuse-tutorial/example/
	mkdir fuse-tutorial/example/mountdir/
	mkdir fuse-tutorial/example/rootdir/
	echo "a bogus file" > fuse-tutorial/example/rootdir/bogus.txt
	mkdir fuse-tutorial/src
	cp Makefile *.c *.h fuse-tutorial/src/
	tar cvzf ../../fuse-tutorial.tgz fuse-tutorial/
	rm -rf fuse-tutorial/
