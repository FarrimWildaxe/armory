BOFNAME := fw-is
CC_x64 := x86_64-w64-mingw32-gcc
CC_x86 := i686-w64-mingw32-gcc
CC=x86_64-w64-mingw32-clang

all:
	$(CC_x64) -Wall -Werror -o $(BOFNAME).x64.o -Os -c fw-is.c 
	$(CC_x86) -Wall -Werror -o $(BOFNAME).x86.o -Os -c fw-is.c 

test:
	$(CC_x64) -Wall -Werror fw-is.c -o $(BOFNAME).x64.exe
	$(CC_x86) -Wall -Werror fw-is.c -o $(BOFNAME).x86.exe

clean:
	rm -f $(PWD)/*.o