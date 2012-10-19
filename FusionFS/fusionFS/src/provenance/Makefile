libprovenance.a: provenance.o
	ar -rcs libprovenance.a provenance.o

provenance.o :
	gcc -g -Wall -fPIC -o provenance.o -c provenance.c -lpthread

clean:
	rm *.a
