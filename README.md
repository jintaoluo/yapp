# Yet Another Pulsar Processor (YAPP) 3.4-beta
## README

Yet Another Pulsar Processor (YAPP) is a suite of radio pulsar data analysis software.

The YAPP tools available with this release are:

* `yapp_viewmetadata` : Prints metadata to standard output.
* `yapp_viewdata` : Plots data to PGPLOT device.
* `yapp_ft` : Performs PFB/FFT on 8-bit, complex, dual-pol. baseband data.
* `yapp_dedisperse` : Dedisperses filterbank format data.
* `yapp_smooth` : Boxcar-smoothes dedispersed time series data
* `yapp_filter` : Processes dedispersed time series data with a custom frequency-domain filter.
* `yapp_add` : Coherently add dedispersed time series data from multiple frequency bands
* `yapp_fold` : Folds filterbank and dedispersed time series data.
* `yapp_subtract` : Subtracts two dedispersed time series files.
* `yapp_siftpulses` : Sifts multiple dedispersed time series files for bright pulses.
* `yapp_stacktim` : Stacks time series data to form filterbank data.

YAPP also comes with the following utilities:

* `yapp_fits2fil` : Converts PSRFITS data to SIGPROC `.fil`
* `yapp_dat2tim` : Converts PRESTO '.dat' to SIGPROC `.tim`
* `yapp_tim2dat` : Converts SIGPROC '.tim' to PRESTO `.dat`

YAPP includes the following scripts:

* `yapp_genpfbcoeff.py` : Generate PFB pre-filter co-efficients for `yapp_ft`.
* `yapp_genfiltermask.py` : Generate filter response for `yapp_filter`.
* `yapp_calcspecidx.py` : Calculate spectral index from a sequence of time series files corresponding to multiple bands.
* `yapp_stackprof.py` : Stacks folded profiles from multiple bands to show a plot of phase versus frequency.
* `yapp_addprof.py` : Add [calibrated] profiles from two polarisations.
* `yapp_viewcand.rb` : Converts prepfold candidate plots in PS format to PNG, and generates a set of HTML pages displaying a tiled set of plots.

The supported file formats are are DAS `.spec`, SIGPROC `.fil`, and SIGPROC `.tim`, with limited support for DAS `.dds`, PSRFITS, and PRESTO `.dat`.

For detailed usage instructions, refer the man pages or online documentation.

System requirements: Linux/OS X, PGPLOT with C binding, FFTW3, CFITSIO, Python with Matplotlib, Ruby, mogrify

Installation instructions: On a typical Ubuntu-based machine in which PGPLOT, FFTW3, and CFITSIO were installed via APT, running `make` followed by `sudo make install` should work, with the binaries being copied to `/usr/local/bin`. For different operating systems and/or different PGPLOT/FFTW3/CFITSIO installation directories and/or a different choice of YAPP installation directory, the makefile may need to be modified by hand.

Created by Jayanth Chennamangalam  
[http://jayanthc.github.io/yapp/](http://jayanthc.github.io/yapp/)
