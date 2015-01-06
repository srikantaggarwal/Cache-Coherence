include Makefile.inc

DIRS	= protocols sim
EXE	= sim_trace
OBJS	= 
OBJLIBS	= lib/libprotocols.a lib/libsim.a 
LIBS	= -Llib/ -lsim -lprotocols

all : $(EXE)

$(EXE) : $(OBJLIBS)
	g++ -o $(EXE) $(OBJS) $(LIBS)

lib/libprotocols.a : force_look
	cd protocols; $(MAKE) $(MFLAGS)

lib/libsim.a : force_look
	cd sim; $(MAKE) $(MFLAGS)

clean :
	$(ECHO) cleaning up in .
	-$(RM) -f $(EXE) $(OBJS) $(OBJLIBS)
	-for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

force_look :
	true
