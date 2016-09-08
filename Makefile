
#Modify this to point to the PJSIP location.
PJBASE=../pjproject-2.5.5

include $(PJBASE)/build.mak

#Set to 1 for debug builds
DEBUG := 1

ifeq ($(DEBUG),1)
  CFLAGSD := -g -Og
  LDFLAGSD := -g -Og
  LDLIBSD :=
else
  CFLAGSD :=
  LDFLAGSD :=
  LDLIBSD :=
endif

CC      = $(PJ_CC)
LDFLAGS = $(PJ_LDFLAGS) $(LDFLAGSD)
LXXFLAGS = $(PJ_LDXXFLAGS) $(LDFLAGSD)
LDLIBS  = $(PJ_LDLIBS) $(LDFLIBSD)
LXXLIBS = $(PJ_LDXXLIBS) $(LDFLIBSD)
CFLAGS  = $(PJ_CFLAGS) $(CFLAGSD) -I../tclap/include
#CPPFLAGS= ${CFLAGS}  $(CFLAGSD) -I../tclap/include
CPPFLAGS= $(PJ_CXXFLAGS) $(CFLAGSD) -I../tclap/include

appname := sipcallnotify

SRCDIR := ./src

srcfiles := $(shell find $(SRCDIR) -name "*.cpp")
objects  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname)

$(appname): $(objects)
	$(CC) -o $@ $< $(CPPFLAGS) $(LXXFLAGS) $(LDFLAGS) $(LXXLIBS) $(LDLIBS)


depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	g++ $(CPPFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(objects)
	rm -f $(appname)

dist-clean: clean
	rm -f *~ .depend

include .depend
