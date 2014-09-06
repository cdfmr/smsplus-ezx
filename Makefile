# SMS Plus - Sega Master System / Game Gear emulator
# (c) 1998-2003  Charles MacDonald

# -DLSB_FIRST   - Leave undefined for big-endian processors.
# -DALIGN_DWORD - Align 32-bit memory transfers
# -DDOS		- Set when compiling the DOS version

CC	=	arm-linux-gcc
CPP	=	arm-linux-g++
STRIP	=	arm-linux-strip
LDFLAGS	=	
FLAGS	=	-I. -Izlib -Icpu -Isound -Iezx -I$(QTDIR)/include \
		-Wall -O9 -fomit-frame-pointer -ffast-math -mcpu=xscale \
		-DLSB_FIRST \
		-D__cdecl= \
		-DQWS -DQT_THREAD_SUPPORT -fno-rtti

LIBS	=	-L$(QTDIR)/lib -lqte-mt -lezxjpeg-xscale-r -lz

OBJ	=       obj/z80.oa	\
		obj/sms.o	\
		obj/pio.o	\
		obj/memz80.o	\
		obj/render.o	\
		obj/vdp.o	\
		obj/system.o	\
		obj/error.o

OBJ	+=	obj/fileio.o	\
		obj/state.o	\
		obj/loadrom.o

OBJ	+=	obj/unzip.o

OBJ	+=      obj/sound.o	\
		obj/sn76489.o	\
		obj/emu2413.o	\
		obj/ym2413.o	\
		obj/fmintf.o	\
		obj/stream.o

OBJ	+=	obj/qmainapp.o	\
		obj/qmainwnd.o	\
		obj/moc_qmainwnd.o \
		obj/qsmsplus.o	\
		obj/qspthread.o

EXE	=	bin/smsplus

all	:	$(EXE)

$(EXE)	:	$(OBJ)
		$(CPP) -o $(EXE) $(OBJ) $(LIBS) $(LDFLAGS)
		$(STRIP) $(EXE)

obj/%.oa :	cpu/%.c cpu/%.h
		$(CC) -c $< -o $@ $(FLAGS)

obj/%.o : 	%.c %.h
		$(CC) -c $< -o $@ $(FLAGS)

obj/%.o :	zlib/%.c
		$(CC) -c $< -o $@ $(FLAGS)

obj/%.o :	sound/%.c sound/%.h	        
		$(CC) -c $< -o $@ $(FLAGS)

ezx/moc_qmainwnd.cpp: ezx/qmainwnd.h
		$(QTDIR)/bin/moc ezx/qmainwnd.h -o ezx/moc_qmainwnd.cpp

obj/moc_qmainwnd.o: ezx/moc_qmainwnd.cpp
		$(CPP) -c ezx/moc_qmainwnd.cpp -o obj/moc_qmainwnd.o $(FLAGS)

obj/%.o :	ezx/%.cpp
		$(CPP) -c $< -o $@ $(FLAGS)

obj/%.o :	ezx/%.c
		$(CC) -c $< -o $@ $(FLAGS)

clean	:
		rm -f obj/*.o
		rm -f ezx/moc_qmainwnd.cpp
		rm -f *.bak
		rm -f *.exe
		rm -f $(EXE)
		rm -f *.log
		rm -f *.wav
		rm -f *.zip
cleancpu :		
		rm -f obj/z80.oa

makedir :
		mkdir -p obj
		mkdir -p bin

#
# end of makefile
#
