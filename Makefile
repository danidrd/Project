TARNAME=DanieleCaliandro-543970
###########################################################

CC		=  gcc
AR              =	ar
ARFLAGS         =	rvs
INCLUDES	= -I.
LDFLAGS 	= -L.
OPTFLAGS	= #-O3
LIBS            =	-lpthread
SRCDIR =	src
LIBDIR =	lib


# aggiungere qui altri targets se necessario
TARGETS		= 	farm \
		  	generafile

# aggiungere qui i file oggetto da compilare
OBJECTS		= 	master.o \
			worker.o \
			queue.o \
			MasterWorker.o \
			collector.o

# aggiungere qui gli altri include 
INCLUDE_FILES   =	$(SRCDIR)/master.h \
		  $(SRCDIR)/MasterWorker.h     \
		  $(SRCDIR)/queue.h	  	\
		  $(SRCDIR)/worker.h	\
		  $(SRCDIR)/collector.h


.PHONY: all clean cleanall test
.SUFFIXES: .c .h

%: $(SRCDIR)/%.c
	$(CC) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) 

%.o: $(SRCDIR)/%.c
	$(CC)  $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all		: $(TARGETS)


farm: MasterWorker.o $(LIBDIR)/libmyf.a $(INCLUDE_FILES)
	$(CC)  $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)



############################ non modificare da qui in poi

$(LIBDIR)/libmyf.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

clean		: 
	rm -f $(TARGETS)

cleanall	: clean
	\rm -f *.o *~ lib/libmyf.a

test	:
			make cleanall
			make all
			./test.sh