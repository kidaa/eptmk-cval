
# Edit this line if necessary to contain an executable C++ compiler
# No other changes are required

CC=g++ -g

INCPATH=lib

all : tagger tokenizer MPtokEX1 MPtokEX2 MPtokEX3 MPtagEX1 MPtagEX2 MPparEX1 MPparEX2 libmp.a

libmp.a : MPtag.o MPlex.o MPtok.o MPpar.o lib/Btree.o lib/runn.o
	ar -rc libmp.a MPtag.o MPlex.o MPtok.o MPpar.o lib/Btree.o lib/runn.o

tagger: tagger.o libmp.a
	$(CC) -o ../util/tagger $^

tokenizer: tokenizer.o libmp.a
	$(CC) -o ../util/tokenizer $^

MPtokEX1: MPtokEX1.o libmp.a
	$(CC) -o MPtokEX1 $^

MPtokEX2: MPtokEX2.o libmp.a
	$(CC) -o MPtokEX2 $^

MPtokEX3: MPtokEX3.o libmp.a
	$(CC) -o MPtokEX3 $^

MPtagEX1: MPtagEX1.o libmp.a
	$(CC) -o MPtagEX1 $^

MPtagEX2: MPtagEX2.o libmp.a
	$(CC) -o MPtagEX2 $^

MPparEX1: MPparEX1.o libmp.a
	$(CC) -o MPparEX1 $^ -lpcre

MPparEX2: MPparEX2.o libmp.a
	$(CC) -o MPparEX2 $^ -lpcre

clean:
	rm -f *.o lib/*.o libmp.a
	rm -f ../util/tagger
	rm -f ../util/tokenizer
	rm -f MPtokEX1 MPtokEX2 MPtokEX3 MPtagEX1 MPtagEX2 MPparEX1

.SUFFIXES:

%.o: %.C
	$(CC) -c -I$(INCPATH) $< -o $@ 
