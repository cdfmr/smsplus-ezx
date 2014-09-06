XPREFIX=arm-linux-
CXX=$(XPREFIX)g++
CC=$(XPREFIX)gcc
STRIP=$(XPREFIX)strip
CXXFLAGS=-fno-exceptions -fno-rtti -DQWS -Wall -O2 -march=iwmmxt
MOC=$(EZX_BASE)/moc/moc
INCLUDE=-I$(EZX_BASE)/include/qt -I$(EZX_BASE)/include/ezx
LIBDIRS=-L$(EZX_BASE)/lib -L$(EZX_BASE)/lib/ezx/lib
LIBS=-lezxappbase-xscale-r -lqte-mt-xscale-r -lezxjpeg-xscale-r -lezxnotification-xscale-r

OBJS=spfe.o
APP=spfe

all: $(APP)

spfe.o: spfe.cpp spfe.moc

clean:
	rm -f $(OBJS) $(APP) $(APP).mpkg *.moc

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $<

%.moc: %.cpp
	$(MOC) $< -o $@

$(APP): $(OBJS)
	$(CXX) $^ -o $@ $(LIBDIRS) $(LIBS)
	$(STRIP) $@
