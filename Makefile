CC = m68k-palmos-gcc
CFLAGS = -O2 -g 

all: LAMPFlash.prc

LAMPFlash.prc: lf bin.stamp
	build-prc LAMPFlash.prc "LAMPFlash" shLF lf *.bin

lf: lf.o
	$(CC) $(CFLAGS) -o lf lf.o

lf.o: lf.c lf.h
	$(CC) $(CFLAGS) -c lf.c

bin.stamp: lf.rcp lf.h icon.bmp
	pilrc lf.rcp

clean:
	-rm -f *.[oa] lf LAMPFlash.prc *.bin *.stamp
