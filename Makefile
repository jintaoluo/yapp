#
# Makefile for Yet Another Pulsar Processor
#
# Created by Jayanth Chennamangalam on 2008.11.17
#

# C compiler and flags
# this may need to be changed to g77 in some cases
CC = gcc

# include path for other libraries
CFLAGS_INC_PGPLOT =# define if needed (as -I[...])
CFLAGS_INC_FFTW3 =# define if needed (as -I[...])
CFLAGS_INC_CFITSIO =# define if needed (as -I[...])

CFLAGS = -std=gnu99 -pedantic -Wall $(CFLAGS_INC_PGPLOT) $(CFLAGS_INC_FFTW3)  \
	$(CFLAGS_INC_CFITSIO)
CFLAGS_C_DEBUG = $(CFLAGS) -g -c
CFLAGS_C_RELEASE = $(CFLAGS) -O3 -c
ifeq ($(OPT_DEBUG), yes)
CFLAGS_C = $(CFLAGS_C_DEBUG)
else
CFLAGS_C = $(CFLAGS_C_RELEASE)
endif
CFLAGS_L_DEBUG = $(CFLAGS) -g
CFLAGS_L_RELEASE = $(CFLAGS) -O3
ifeq ($(OPT_DEBUG), yes)
CFLAGS_L = $(CFLAGS_L_DEBUG)
else
CFLAGS_L = $(CFLAGS_L_RELEASE)
endif

# enable/disable the debug flag
ifeq ($(OPT_DEBUG), yes)
DDEBUG = -DDEBUG
endif

# linker flags
LFLAGS_PGPLOT_DIR =# define if not in $PATH (as -L[...])
LFLAGS_FFTW3_DIR =# define if not in $PATH (as -L[...])
LFLAGS_CFITSIO_DIR =# define if not in $PATH (as -L[...])
LFLAGS_FFTW3 = $(LFLAGS_FFTW3_DIR) -lfftw3f
LFLAGS_CFITSIO = $(LFLAGS_CFITSIO_DIR) -lcfitsio
# in some cases, linking needs to be done with the X11 library, in which case
# append '-lX11' (and possibly the path to the library) to the line below.
# libgfortran may also be needed in some case, in which case append
# '-lgfortran' (and possibly the path to the library) to the line below
LFLAGS_PGPLOT = $(LFLAGS_PGPLOT_DIR) -lcpgplot
LFLAGS_MATH = -lm

# directories
SRCDIR = src
UTILDIR = utilities
MANDIR = man
IDIR = src
BINDIR = bin
# binary installation directory - modify if needed
BININSTALLDIR = /usr/local/bin
# man page installation directory - modify if needed
MANINSTALLDIR = /usr/local/share/man/man1

# command definitions
DELCMD = rm

all: yapp_makever \
	 yapp_version.o \
	 yapp_erflookup.o \
	 yapp_common.o \
	 yapp_viewmetadata.o \
	 yapp_viewmetadata \
	 colourmap.o \
	 yapp_viewdata.o \
	 yapp_viewdata \
	 yapp_ft.o \
	 yapp_ft \
	 yapp_dedisperse.o \
	 yapp_dedisperse \
	 yapp_smooth.o \
	 yapp_smooth \
	 yapp_filter.o \
	 yapp_filter \
	 yapp_fold.o \
	 yapp_fold \
	 yapp_add.o \
	 yapp_add \
	 yapp_fits2fil.o \
	 yapp_fits2fil \
	 yapp_dat2tim.o \
	 yapp_dat2tim \
	 yapp_tim2dat.o \
	 yapp_tim2dat \
	 yapp_subtract.o \
	 yapp_subtract \
	 yapp_siftpulses.o \
	 yapp_siftpulses \
	 yapp_stacktim.o \
	 yapp_stacktim

yapp_makever: $(SRCDIR)/yapp_makever.c
	$(CC) $(CFLAGS_L) $< -o $(IDIR)/$@
	$(IDIR)/$@
	$(DELCMD) $(IDIR)/$@

yapp_version.o: $(SRCDIR)/yapp_version.c
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

yapp_erflookup.o: $(SRCDIR)/yapp_erflookup.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

yapp_common.o: $(SRCDIR)/yapp_common.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_viewmetadata.o: $(SRCDIR)/yapp_viewmetadata.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

yapp_viewmetadata: $(IDIR)/yapp_viewmetadata.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

colourmap.o: $(SRCDIR)/colourmap.c $(SRCDIR)/colourmap.h
	$(CC) $(CFLAGS_C) $< -o $(IDIR)/$@

yapp_viewdata.o: $(SRCDIR)/yapp_viewdata.c $(SRCDIR)/yapp.h \
	$(SRCDIR)/yapp_erflookup.c
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_viewdata: $(IDIR)/yapp_viewdata.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o $(IDIR)/colourmap.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_ft.o: $(SRCDIR)/yapp_ft.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_ft: $(IDIR)/yapp_ft.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_FFTW3) \
		$(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_dedisperse.o: $(SRCDIR)/yapp_dedisperse.c $(SRCDIR)/yapp.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $(DFC) $(SRCDIR)/yapp_dedisperse.c \
		-o $(IDIR)/$@

yapp_dedisperse: $(IDIR)/yapp_dedisperse.o
	$(CC) $(IDIR)/yapp_dedisperse.o $(IDIR)/yapp_version.o \
		$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o $(IDIR)/colourmap.o \
		$(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_smooth.o: $(SRCDIR)/yapp_smooth.c $(SRCDIR)/yapp.h \
	$(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_smooth: $(IDIR)/yapp_smooth.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_filter.o: $(SRCDIR)/yapp_filter.c $(SRCDIR)/yapp.h \
	$(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_filter: $(IDIR)/yapp_filter.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_FFTW3) \
		$(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_fold.o: $(SRCDIR)/yapp_fold.c $(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_fold: $(IDIR)/yapp_fold.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o $(IDIR)/colourmap.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_add.o: $(SRCDIR)/yapp_add.c $(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_add: $(IDIR)/yapp_add.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_subtract.o: $(SRCDIR)/yapp_subtract.c $(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_subtract: $(IDIR)/yapp_subtract.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_fits2fil.o: $(UTILDIR)/yapp_fits2fil.c $(UTILDIR)/yapp_fits2fil.h \
	$(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h $(SRCDIR)/yapp_psrfits.h
	$(CC) $(CFLAGS_C) -I$(SRCDIR) $(DDEBUG) $< -o $(UTILDIR)/$@

yapp_fits2fil: $(UTILDIR)/yapp_fits2fil.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_dat2tim.o: $(UTILDIR)/yapp_dat2tim.c $(UTILDIR)/yapp_dat2tim.h \
	$(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) -I$(SRCDIR) $(DDEBUG) $< -o $(UTILDIR)/$@

yapp_dat2tim: $(UTILDIR)/yapp_dat2tim.o $(SRCDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_tim2dat.o: $(UTILDIR)/yapp_tim2dat.c $(UTILDIR)/yapp_tim2dat.h \
	$(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) -I$(SRCDIR) $(DDEBUG) $< -o $(UTILDIR)/$@

yapp_tim2dat: $(UTILDIR)/yapp_tim2dat.o $(SRCDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_siftpulses.o: $(SRCDIR)/yapp_siftpulses.c $(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_siftpulses: $(IDIR)/yapp_siftpulses.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

yapp_stacktim.o: $(SRCDIR)/yapp_stacktim.c $(SRCDIR)/yapp.h $(SRCDIR)/yapp_sigproc.h
	$(CC) $(CFLAGS_C) $(DDEBUG) $< -o $(IDIR)/$@

yapp_stacktim: $(IDIR)/yapp_stacktim.o $(IDIR)/yapp_version.o \
	$(IDIR)/yapp_erflookup.o $(IDIR)/yapp_common.o $(IDIR)/colourmap.o
	$(CC) $^ $(LFLAGS_PGPLOT) $(LFLAGS_MATH) $(LFLAGS_CFITSIO) -o $(BINDIR)/$@

# install the man pages
install:
	@echo Copying binaries...
	cp $(BINDIR)/* $(BININSTALLDIR)
	@echo DONE
	@echo Copying man pages...
	cp $(MANDIR)/*.1 $(MANINSTALLDIR)
	@echo DONE

clean:
	$(DELCMD) $(SRCDIR)/yapp_version.c
	$(DELCMD) $(IDIR)/yapp_version.o
	$(DELCMD) $(IDIR)/yapp_erflookup.o
	$(DELCMD) $(IDIR)/yapp_common.o
	$(DELCMD) $(IDIR)/yapp_viewmetadata.o
	$(DELCMD) $(IDIR)/colourmap.o
	$(DELCMD) $(IDIR)/yapp_viewdata.o
	$(DELCMD) $(IDIR)/yapp_ft.o
	$(DELCMD) $(IDIR)/yapp_dedisperse.o
	$(DELCMD) $(IDIR)/yapp_smooth.o
	$(DELCMD) $(IDIR)/yapp_filter.o
	$(DELCMD) $(IDIR)/yapp_fold.o
	$(DELCMD) $(IDIR)/yapp_add.o
	$(DELCMD) $(UTILDIR)/yapp_fits2fil.o
	$(DELCMD) $(UTILDIR)/yapp_dat2tim.o
	$(DELCMD) $(UTILDIR)/yapp_tim2dat.o
	$(DELCMD) $(IDIR)/yapp_subtract.o
	$(DELCMD) $(IDIR)/yapp_siftpulses.o
	$(DELCMD) $(IDIR)/yapp_stacktim.o

