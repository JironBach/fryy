AS=nasm
CC=bcc
LD=ld86
INC=include
SRC=source
CFLAGS=-0 -I$(INC) -ansi -c

all: boot.img kernel.bin

boot.img: $(SRC)/boot.nasm
	$(AS) -o $@ $(SRC)/boot.nasm
	dd if=/dev/zero of=$@ bs=512 count=2879 seek=1

kernel.o: $(SRC)/kernel.c $(INC)/kernel.h
	$(CC) $(CFLAGS) $(SRC)/kernel.c -o $@ 

task.o: $(SRC)/task.c $(INC)/task.h $(INC)/list.h
	$(CC) $(CFLAGS) $(SRC)/task.c -o $@

io.o: $(SRC)/io.c $(INC)/io.h
	$(CC) $(CFLAGS) $(SRC)/io.c -o $@

shell.o: $(SRC)/shell.c $(INC)/shell.h
	$(CC) $(CFLAGS) $(SRC)/shell.c -o $@

fs.o: $(SRC)/fs.c $(INC)/fs.h
	$(CC) $(CFLAGS) $(SRC)/fs.c -o $@

syscall.o: $(SRC)/syscall.c $(INC)/syscall.h
	$(CC) $(CFLAGS) $(SRC)/syscall.c -o $@

kernel.bin: kernel.o task.o io.o shell.o fs.o syscall.o
	$(LD) -d -M kernel.o task.o io.o shell.o fs.o syscall.o -L/usr/lib/bcc/ -lc -o $@

clean:
	rm -f kernel.bin
	rm -f *.o
	rm -rf floppy
	rm boot.img

run:
	mkdir ./floppy
	mount -o loop boot.img ./floppy/
	cp kernel.bin ./floppy/
	umount ./floppy/
	rmdir floppy
	qemu-system-i386 -fda boot.img
