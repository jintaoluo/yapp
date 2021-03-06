/*
 * @file yapp_subtract.c
 * Program to (incoherently) subtract one time series from another. This is
 *  useful for baseline subtraction (subtracting a suitably-smoothed time
 *  series from the original).
 *
 * @verbatim
 * Usage: yapp_subtract [options] <data-file-A> <data-file-B>
 *     -h  --help                           Display this usage information
 *     -s  --skip <time>                    The length of data in seconds, to be
 *                                          skipped
 *                                          (default is 0 s)
 *     -p  --proc <time>                    The length of data in seconds, to be
 *                                          processed
 *                                          (default is all)
 *     -n  --nsamp <samples>                Number of samples read in one block
 *     -g  --graphics                       Turn on plotting
 *                                          (default is 4096 samples)
 *     -i  --invert                         Invert the background and foreground
 *                                          colours in plots
 *     -e  --non-interactive                Run in non-interactive mode
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2013.05.06
 */

#include "yapp.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

/* PGPLOT device ID */
extern int g_iPGDev;

/* data file */
FILE *g_pFDataA;
FILE *g_pFDataB;

/* the following are global only to enable cleaning up in case of abnormal
   termination, such as those triggered by SIGINT or SIGTERM */
float *g_pfBufA = NULL;
float *g_pfBufB = NULL;
float *g_pfOutBuf = NULL;
float *g_pfXAxis = NULL;

int main(int argc, char *argv[])
{
    FILE *pFOut = NULL;
    char *pcFileDataA = NULL;
    char *pcFileDataB = NULL;
    char *pcFileOut = NULL;
    char acFileOut[LEN_GENSTRING] = {0};
    int iFormat = DEF_FORMAT;
    double dDataSkipTime = 0.0;
    double dDataProcTime = 0.0;
    YUM_t stYUM = {{0}};
    int iTotSampsPerBlock = 0;  /* iBlockSize */
    double dTSampInSec = 0.0;   /* holds sampling time in s */
    long lBytesToSkip = 0;
    long lBytesToProc = 0;
    int iTimeSampsToSkip = 0;
    int iTimeSampsToProc = 0;
    int iBlockSize = DEF_SIZE_BLOCK;
    int iNumReads = 0;
    int iReadBlockCount = 0;
    char cIsLastBlock = YAPP_FALSE;
    int iRet = YAPP_RET_SUCCESS;
    float fDataMin = 0.0;
    float fDataMax = 0.0;
    int iReadItems = 0;
    float fButX = 0.0;
    float fButY = 0.0;
    char cCurChar = 0;
    int iNumSamps = 0;
    int iDiff = 0;
    int i = 0;
    float fMeanOrig = 0.0;
    float fRMSOrig = 0.0;
    float fMeanOrigAll = 0.0;
    float fRMSOrigAll = 0.0;
    float fMeanSubed = 0.0;
    float fRMSSubed = 0.0;
    float fMeanSubedAll = 0.0;
    float fRMSSubedAll = 0.0;
    char cHasGraphics = YAPP_FALSE;
    int iInvCols = YAPP_FALSE;
    char cIsNonInteractive = YAPP_FALSE;
    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hs:p:w:giev";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
        { "skip",                   1, NULL, 's' },
        { "proc",                   1, NULL, 'p' },
        { "width",                  1, NULL, 'w' },
        { "graphics",               0, NULL, 'g' },
        { "invert",                 0, NULL, 'i' },
        { "non-interactive",        0, NULL, 'e' },
        { "version",                0, NULL, 'v' },
        { NULL,                     0, NULL, 0   }
    };

    /* get the filename of the program from the argument list */
    pcProgName = argv[0];

    /* parse the input */
    do
    {
        iNextOpt = getopt_long(argc, argv, pcOptsShort, stOptsLong, NULL);
        switch (iNextOpt)
        {
            case 'h':   /* -h or --help */
                /* print usage info and terminate */
                PrintUsage(pcProgName);
                return YAPP_RET_SUCCESS;

            case 's':   /* -s or --skip */
                /* set option */
                dDataSkipTime = atof(optarg);
                break;

            case 'p':   /* -p or --proc */
                /* set option */
                dDataProcTime = atof(optarg);
                break;

            case 'n':   /* -n or --nsamp */
                /* set option */
                iBlockSize = atoi(optarg);
                /* validate - PGPLOT does not like iBlockSize = 1 */
                if (iBlockSize < 2)
                {
                    (void) fprintf(stderr,
                                   "ERROR: Number of samples must be > 1!\n");
                    PrintUsage(pcProgName);
                    return YAPP_RET_ERROR;
                }
                break;

            case 'g':   /* -g or --graphics */
                /* set option */
                cHasGraphics = YAPP_TRUE;
                break;

            case 'i':  /* -i or --invert */
                /* set option */
                iInvCols = YAPP_TRUE;
                break;

            case 'e':  /* -e or --non-interactive */
                /* set option */
                cIsNonInteractive = YAPP_TRUE;
                break;

            case 'v':   /* -v or --version */
                /* display the version */
                (void) printf("%s\n", g_pcVersion);
                return YAPP_RET_SUCCESS;

            case '?':   /* user specified an invalid option */
                /* print usage info and terminate with error */
                (void) fprintf(stderr, "ERROR: Invalid option!\n");
                PrintUsage(pcProgName);
                return YAPP_RET_ERROR;

            case -1:    /* done with options */
                break;

            default:    /* unexpected */
                assert(0);
        }
    } while (iNextOpt != -1);

    /* no arguments */
    if (argc <= optind)
    {
        (void) fprintf(stderr, "ERROR: Input file not specified!\n");
        PrintUsage(pcProgName);
        return YAPP_RET_ERROR;
    }

    /* register the signal-handling function */
    iRet = YAPP_RegisterSignalHandlers();
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Handler registration failed!\n");
        return YAPP_RET_ERROR;
    }

    /* get the input filenames */
    pcFileDataA = argv[optind];
    pcFileDataB = argv[optind+1];

    /* determine the file type */
    iFormat = YAPP_GetFileType(pcFileDataA);
    if (YAPP_RET_ERROR == iFormat)
    {
        (void) fprintf(stderr,
                       "ERROR: File type determination failed!\n");
        return YAPP_RET_ERROR;
    }
    if (iFormat != YAPP_FORMAT_DTS_TIM)
    {
        (void) fprintf(stderr,
                       "ERROR: Invalid file type!\n");
        return YAPP_RET_ERROR;
    }
    iFormat = YAPP_GetFileType(pcFileDataB);
    if (YAPP_RET_ERROR == iFormat)
    {
        (void) fprintf(stderr,
                       "ERROR: File type determination failed!\n");
        return YAPP_RET_ERROR;
    }
    if (iFormat != YAPP_FORMAT_DTS_TIM)
    {
        (void) fprintf(stderr,
                       "ERROR: Invalid file type!\n");
        return YAPP_RET_ERROR;
    }

    /* read metadata from first file */
    /* ASSUMPTION: second data is smoothed version of first, so metadata is
       same */
    iRet = YAPP_ReadMetadata(pcFileDataA, iFormat, &stYUM);
    if (iRet != YAPP_RET_SUCCESS)
    {
        (void) fprintf(stderr,
                       "ERROR: Reading metadata failed for file %s!\n",
                       pcFileDataA);
        return YAPP_RET_ERROR;
    }

    /* convert sampling interval to seconds */
    dTSampInSec = stYUM.dTSamp / 1e3;

    /* calculate bytes to skip and read */
    if (0.0 == dDataProcTime)
    {
        dDataProcTime = (stYUM.iTimeSamps * dTSampInSec) - dDataSkipTime;
    }
    /* check if the input time duration is less than the length of the
       data */
    else if (dDataProcTime > (stYUM.iTimeSamps * dTSampInSec))
    {
        (void) fprintf(stderr,
                       "WARNING: Input time is longer than length of "
                       "data!\n");
    }

    lBytesToSkip = (long) floor((dDataSkipTime / dTSampInSec)
                                                    /* number of samples */
                           * stYUM.fSampSize);
    lBytesToProc = (long) floor((dDataProcTime / dTSampInSec)
                                                    /* number of samples */
                           * stYUM.fSampSize);

    if (lBytesToSkip >= stYUM.lDataSizeTotal)
    {
        (void) fprintf(stderr,
                       "ERROR: Data to be skipped is greater than or equal to "
                       "the size of the file!\n");
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    if ((lBytesToSkip + lBytesToProc) > stYUM.lDataSizeTotal)
    {
        (void) printf("WARNING: Total data to be read (skipped and processed) "
                      "is more than the size of the file! ");
        lBytesToProc = stYUM.lDataSizeTotal - lBytesToSkip;
        (void) printf("Newly calculated size of data to be processed: %ld "
                      "bytes\n",
                      lBytesToProc);
    }

    iTimeSampsToSkip = (int) (lBytesToSkip / (stYUM.fSampSize));
    (void) printf("Skipping\n"
                  "    %ld of %ld bytes\n"
                  "    %d of %d time samples\n"
                  "    %.10g of %.10g seconds\n",
                  lBytesToSkip,
                  stYUM.lDataSizeTotal,
                  iTimeSampsToSkip,
                  stYUM.iTimeSamps,
                  (iTimeSampsToSkip * dTSampInSec),
                  (stYUM.iTimeSamps * dTSampInSec));

    iTimeSampsToProc = (int) (lBytesToProc / (stYUM.fSampSize));
    /* change block size according to the number of samples to be processed */
    if (iTimeSampsToProc < iBlockSize)
    {
        iBlockSize = (int) ceil(dDataProcTime / dTSampInSec);
    }
    /* calculate the actual number of samples that will be processed in one
       iteration */
    iNumReads = (int) ceilf(((float) iTimeSampsToProc) / iBlockSize);

    /* optimisation - store some commonly used values in variables */
    iTotSampsPerBlock = iBlockSize;

    (void) printf("Processing\n"
                  "    %ld of %ld bytes\n"
                  "    %d of %d time samples\n"
                  "    %.10g of %.10g seconds\n"
                  "in %d reads with block size %d time samples...\n",
                  lBytesToProc,
                  stYUM.lDataSizeTotal,
                  iTimeSampsToProc,
                  stYUM.iTimeSamps,
                  (iTimeSampsToProc * dTSampInSec),
                  (stYUM.iTimeSamps * dTSampInSec),
                  iNumReads,
                  iBlockSize);

    /* open the first time series data file for reading */
    g_pFDataA = fopen(pcFileDataA, "r");
    if (NULL == g_pFDataA)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       pcFileDataA,
                       strerror(errno));
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    /* open the second time series data file for reading */
    g_pFDataB = fopen(pcFileDataB, "r");
    if (NULL == g_pFDataB)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       pcFileDataB,
                       strerror(errno));
        (void) fclose(g_pFDataA);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    /* allocate memory for the buffer, based on the number of channels and time
       samples */
    g_pfBufA = (float *) YAPP_Malloc((size_t) iBlockSize,
                                     sizeof(float),
                                     YAPP_FALSE);
    if (NULL == g_pfBufA)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed! %s!\n",
                       strerror(errno));
        (void) fclose(g_pFDataA);
        (void) fclose(g_pFDataB);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }
    g_pfBufB = (float *) YAPP_Malloc((size_t) iBlockSize,
                                     sizeof(float),
                                     YAPP_FALSE);
    if (NULL == g_pfBufB)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation failed! %s!\n",
                       strerror(errno));
        (void) fclose(g_pFDataA);
        (void) fclose(g_pFDataB);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    if (1 == iNumReads)
    {
        cIsLastBlock = YAPP_TRUE;
    }

    /* open the time series data file for writing */
    pcFileOut = YAPP_GetFilenameFromPath(pcFileDataA);
    (void) sprintf(acFileOut,
                   "%s.%s%s",
                   pcFileOut,
                   INFIX_SUB,
                   EXT_TIM);
    pFOut = fopen(acFileOut, "w");
    if (NULL == pFOut)
    {
        (void) fprintf(stderr,
                       "ERROR: Opening file %s failed! %s.\n",
                       acFileOut,
                       strerror(errno));
        (void) fclose(g_pFDataA);
        (void) fclose(g_pFDataB);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    /* skip the header by copying it to the output file */
    char acBuf[stYUM.iHeaderLen];
    (void) fread(acBuf, sizeof(char), (long) stYUM.iHeaderLen, g_pFDataA);
    (void) fread(acBuf, sizeof(char), (long) stYUM.iHeaderLen, g_pFDataB);
    (void) fwrite(acBuf, sizeof(char), (long) stYUM.iHeaderLen, pFOut);
    /* skip data, if any are to be skipped */
    (void) fseek(g_pFDataA, lBytesToSkip, SEEK_CUR);
    (void) fseek(g_pFDataB, lBytesToSkip, SEEK_CUR);

    /* open the PGPLOT graphics device */
    if (cHasGraphics)
    {
        g_iPGDev = cpgopen(PG_DEV);
        if (g_iPGDev <= 0)
        {
            (void) fprintf(stderr,
                           "ERROR: Opening graphics device %s failed!\n",
                           PG_DEV);
            (void) fclose(pFOut);
            (void) fclose(g_pFDataA);
            (void) fclose(g_pFDataB);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }

        /* set the background colour to white and the foreground colour to
           black, if user requires so */
        if (YAPP_TRUE == iInvCols)
        {
            cpgscr(0, 1.0, 1.0, 1.0);
            cpgscr(1, 0.0, 0.0, 0.0);
        }

        cpgsubp(1, 3);
        cpgsch(PG_CH_3P);

        /* set up the plot's X-axis */
        g_pfXAxis = (float *) YAPP_Malloc(iBlockSize,
                                          sizeof(float),
                                          YAPP_FALSE);
        if (NULL == g_pfXAxis)
        {
            (void) fprintf(stderr,
                           "ERROR: Memory allocation for X-axis failed! %s!\n",
                           strerror(errno));
            (void) fclose(pFOut);
            (void) fclose(g_pFDataA);
            (void) fclose(g_pFDataB);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
    }

    /* allocate memory for the accumulation buffer */
    g_pfOutBuf = (float *) YAPP_Malloc((size_t) iBlockSize,
                                       sizeof(float),
                                       YAPP_TRUE);
    if (NULL == g_pfOutBuf)
    {
        (void) fprintf(stderr,
                       "ERROR: Memory allocation for plot buffer failed! "
                       "%s!\n",
                       strerror(errno));
        (void) fclose(pFOut);
        (void) fclose(g_pFDataA);
        (void) fclose(g_pFDataB);
        YAPP_CleanUp();
        return YAPP_RET_ERROR;
    }

    while (iNumReads > 0)
    {
        /* read data */
        (void) printf("\rReading data block %d.", iReadBlockCount);
        (void) fflush(stdout);
        iReadItems = YAPP_ReadData(g_pFDataA,
                                   g_pfBufA,
                                   stYUM.fSampSize,
                                   iTotSampsPerBlock);
        if (YAPP_RET_ERROR == iReadItems)
        {
            (void) fprintf(stderr, "ERROR: Reading data failed!\n");
            (void) fclose(pFOut);
            (void) fclose(g_pFDataA);
            (void) fclose(g_pFDataB);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        iReadItems = YAPP_ReadData(g_pFDataB,
                                   g_pfBufB,
                                   stYUM.fSampSize,
                                   iTotSampsPerBlock);
        if (YAPP_RET_ERROR == iReadItems)
        {
            (void) fprintf(stderr, "ERROR: Reading data failed!\n");
            (void) fclose(pFOut);
            (void) fclose(g_pFDataA);
            (void) fclose(g_pFDataB);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        --iNumReads;
        ++iReadBlockCount;

        if (iReadItems < iTotSampsPerBlock)
        {
            iDiff = iBlockSize - iReadItems;

            /* reset remaining elements to '\0' */
            (void) memset((g_pfBufA + iReadItems),
                          '\0',
                          (sizeof(float) * iDiff));
            (void) memset((g_pfBufB + iReadItems),
                          '\0',
                          (sizeof(float) * iDiff));
        }

        /* calculate the number of time samples in the block - this may not
           be iBlockSize for the last block, and should be iBlockSize for
           all other blocks */
        iNumSamps = iReadItems;

        /* subtract data */
        for (i = 0; i < iNumSamps; ++i)
        {
            g_pfOutBuf[i] = g_pfBufA[i] - g_pfBufB[i];
        }
        /* write output data to file */
        (void) fwrite(g_pfOutBuf,
                      sizeof(float),
                      (long) iNumSamps,
                      pFOut);

        /* original signal */
        fMeanOrig = YAPP_CalcMean(g_pfBufA, iNumSamps, 0, 1);
        fMeanOrigAll += fMeanOrig;
        fRMSOrig = YAPP_CalcRMS(g_pfBufA,
                                iNumSamps,
                                0,
                                1,
                                fMeanOrig);
        fRMSOrig *= fRMSOrig;
        fRMSOrig *= (iNumSamps - 1);
        fRMSOrigAll += fRMSOrig;

        /* output signal */
        fMeanSubed = YAPP_CalcMean(g_pfOutBuf, iNumSamps, 0, 1);
        fMeanSubedAll += fMeanSubed;
        fRMSSubed = YAPP_CalcRMS(g_pfOutBuf,
                                 iNumSamps,
                                 0,
                                 1,
                                 fMeanSubed);
        fRMSSubed *= fRMSSubed;
        fRMSSubed *= (iNumSamps - 1);
        fRMSSubedAll += fRMSSubed;

        if (cHasGraphics)
        {
            fDataMin = g_pfBufA[0];
            fDataMax = g_pfBufA[0];
            for (i = 0; i < iBlockSize; ++i)
            {
                if (g_pfBufA[i] < fDataMin)
                {
                    fDataMin = g_pfBufA[i];
                }
                if (g_pfBufA[i] > fDataMax)
                {
                    fDataMax = g_pfBufA[i];
                }
            }

            #ifdef DEBUG
            (void) printf("Minimum value of data             : %g\n",
                          fDataMin);
            (void) printf("Maximum value of data             : %g\n",
                          fDataMax);
            #endif

            cpgpanl(1, 1);
            /* erase just before plotting, to reduce flicker */
            cpgeras();
            for (i = 0; i < iBlockSize; ++i)
            {
                g_pfXAxis[i] = (float) (dDataSkipTime
                                        + (((iReadBlockCount - 1)
                                            * iBlockSize
                                            * dTSampInSec)
                                           + (i * dTSampInSec)));
            }

            cpgsvp(PG_VP_ML, PG_VP_MR, PG_VP_MB, PG_VP_MT);
            cpgswin(g_pfXAxis[0],
                    g_pfXAxis[iBlockSize-1],
                    fDataMin,
                    fDataMax);
            cpglab("Time (s)", "", "Signal A");
            cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
            cpgsci(PG_CI_PLOT);
            cpgline(iBlockSize, g_pfXAxis, g_pfBufA);
            cpgsci(PG_CI_DEF);

            fDataMin = g_pfBufB[0];
            fDataMax = g_pfBufB[0];
            for (i = 0; i < iBlockSize; ++i)
            {
                if (g_pfBufB[i] < fDataMin)
                {
                    fDataMin = g_pfBufB[i];
                }
                if (g_pfBufB[i] > fDataMax)
                {
                    fDataMax = g_pfBufB[i];
                }
            }

            #ifdef DEBUG
            (void) printf("Minimum value of data             : %g\n",
                          fDataMin);
            (void) printf("Maximum value of data             : %g\n",
                          fDataMax);
            #endif

            cpgpanl(1, 2);
            /* erase just before plotting, to reduce flicker */
            cpgeras();
            for (i = 0; i < iBlockSize; ++i)
            {
                g_pfXAxis[i] = (float) (dDataSkipTime
                                        + (((iReadBlockCount - 1)
                                            * iBlockSize
                                            * dTSampInSec)
                                           + (i * dTSampInSec)));
            }

            cpgsvp(PG_VP_ML, PG_VP_MR, PG_VP_MB, PG_VP_MT);
            cpgswin(g_pfXAxis[0],
                    g_pfXAxis[iBlockSize-1],
                    fDataMin,
                    fDataMax);
            cpglab("Time (s)", "", "Signal A");
            cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
            cpgsci(PG_CI_PLOT);
            cpgline(iBlockSize, g_pfXAxis, g_pfBufB);
            cpgsci(PG_CI_DEF);

            fDataMin = g_pfOutBuf[0];
            fDataMax = g_pfOutBuf[0];
            for (i = 0; i < iNumSamps; ++i)
            {
                if (g_pfOutBuf[i] < fDataMin)
                {
                    fDataMin = g_pfOutBuf[i];
                }
                if (g_pfOutBuf[i] > fDataMax)
                {
                    fDataMax = g_pfOutBuf[i];
                }
            }

            #ifdef DEBUG
            (void) printf("Minimum value of data             : %g\n",
                          fDataMin);
            (void) printf("Maximum value of data             : %g\n",
                          fDataMax);
            #endif

            cpgpanl(1, 3);
            /* erase just before plotting, to reduce flicker */
            cpgeras();
            for (i = 0; i < iBlockSize; ++i)
            {
                g_pfXAxis[i] = (float) (dDataSkipTime
                                        + (((iReadBlockCount - 1)
                                            * iBlockSize
                                            * dTSampInSec)
                                           + (i * dTSampInSec)));
            }

            cpgsvp(PG_VP_ML, PG_VP_MR, PG_VP_MB, PG_VP_MT);
            cpgswin(g_pfXAxis[0],
                    g_pfXAxis[iBlockSize-1],
                    fDataMin,
                    fDataMax);
            cpglab("Time (s)", "", "After Subtracting");
            cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
            cpgsci(PG_CI_PLOT);
            cpgline(iBlockSize, g_pfXAxis, g_pfOutBuf);
            cpgsci(PG_CI_DEF);

            if (!(cIsLastBlock))
            {
                if (!(cIsNonInteractive))
                {
                    /* draw the 'next' and 'exit' buttons */
                    cpgsvp(PG_VP_BUT_ML, PG_VP_BUT_MR, PG_VP_BUT_MB, PG_VP_BUT_MT);
                    cpgswin(PG_BUT_L, PG_BUT_R, PG_BUT_B, PG_BUT_T);
                    cpgsci(PG_BUT_FILLCOL); /* set the fill colour */
                    cpgrect(PG_BUTNEXT_L, PG_BUTNEXT_R, PG_BUTNEXT_B, PG_BUTNEXT_T);
                    cpgrect(PG_BUTEXIT_L, PG_BUTEXIT_R, PG_BUTEXIT_B, PG_BUTEXIT_T);
                    cpgsci(0);  /* set colour index to white */
                    cpgtext(PG_BUTNEXT_TEXT_L, PG_BUTNEXT_TEXT_B, "Next");
                    cpgtext(PG_BUTEXIT_TEXT_L, PG_BUTEXIT_TEXT_B, "Exit");

                    fButX = (PG_BUTNEXT_R - PG_BUTNEXT_L) / 2;
                    fButY = (PG_BUTNEXT_T - PG_BUTNEXT_B) / 2;

                    while (YAPP_TRUE)
                    {
                        iRet = cpgcurs(&fButX, &fButY, &cCurChar);
                        if (0 == iRet)
                        {
                            (void) fprintf(stderr,
                                           "WARNING: "
                                           "Reading cursor parameters failed!\n");
                            break;
                        }

                        if (((fButX >= PG_BUTNEXT_L) && (fButX <= PG_BUTNEXT_R))
                            && ((fButY >= PG_BUTNEXT_B) && (fButY <= PG_BUTNEXT_T)))
                        {
                            /* animate button click */
                            cpgsci(PG_BUT_FILLCOL);
                            cpgtext(PG_BUTNEXT_TEXT_L, PG_BUTNEXT_TEXT_B, "Next");
                            cpgsci(0);  /* set colour index to white */
                            cpgtext(PG_BUTNEXT_CL_TEXT_L, PG_BUTNEXT_CL_TEXT_B, "Next");
                            (void) usleep(PG_BUT_CL_SLEEP);
                            cpgsci(PG_BUT_FILLCOL); /* set colour index to fill
                                                       colour */
                            cpgtext(PG_BUTNEXT_CL_TEXT_L, PG_BUTNEXT_CL_TEXT_B, "Next");
                            cpgsci(0);  /* set colour index to white */
                            cpgtext(PG_BUTNEXT_TEXT_L, PG_BUTNEXT_TEXT_B, "Next");
                            cpgsci(1);  /* reset colour index to black */
                            (void) usleep(PG_BUT_CL_SLEEP);

                            break;
                        }
                        else if (((fButX >= PG_BUTEXIT_L) && (fButX <= PG_BUTEXIT_R))
                            && ((fButY >= PG_BUTEXIT_B) && (fButY <= PG_BUTEXIT_T)))
                        {
                            /* animate button click */
                            cpgsci(PG_BUT_FILLCOL);
                            cpgtext(PG_BUTEXIT_TEXT_L, PG_BUTEXIT_TEXT_B, "Exit");
                            cpgsci(0);  /* set colour index to white */
                            cpgtext(PG_BUTEXIT_CL_TEXT_L, PG_BUTEXIT_CL_TEXT_B, "Exit");
                            (void) usleep(PG_BUT_CL_SLEEP);
                            cpgsci(PG_BUT_FILLCOL); /* set colour index to fill
                                                       colour */
                            cpgtext(PG_BUTEXIT_CL_TEXT_L, PG_BUTEXIT_CL_TEXT_B, "Exit");
                            cpgsci(0);  /* set colour index to white */
                            cpgtext(PG_BUTEXIT_TEXT_L, PG_BUTEXIT_TEXT_B, "Exit");
                            cpgsci(1);  /* reset colour index to black */
                            (void) usleep(PG_BUT_CL_SLEEP);

                            (void) fclose(pFOut);
                            (void) fclose(g_pFDataA);
                            (void) fclose(g_pFDataB);
                            YAPP_CleanUp();
                            return YAPP_RET_SUCCESS;
                        }
                    }
                }
                else
                {
                    /* pause before erasing */
                    (void) usleep(PG_PLOT_SLEEP);
                }
            }
        }

        if (1 == iNumReads)
        {
            cIsLastBlock = YAPP_TRUE;
        }
    }

    (void) printf("DONE!\n");

    /* print statistics */
    fMeanOrigAll /= iReadBlockCount;
    fRMSOrigAll /= (stYUM.iTimeSamps - 1);
    fRMSOrigAll = sqrtf(fRMSOrigAll);
    (void) printf("Original signal mean = %g\n", fMeanOrigAll);
    (void) printf("Original signal RMS = %g\n", fRMSOrigAll);
    fMeanSubedAll /= iReadBlockCount;
    fRMSSubedAll /= (stYUM.iTimeSamps - 1);
    fRMSSubedAll = sqrtf(fRMSSubedAll);
    (void) printf("Subed signal mean = %g\n", fMeanSubedAll);
    (void) printf("Subed signal RMS = %g\n", fRMSSubedAll);

    (void) fclose(g_pFDataA);
    (void) fclose(g_pFDataB);
    (void) fclose(pFOut);
    YAPP_CleanUp();

    return YAPP_RET_SUCCESS;
}

/*
 * Prints usage information
 */
void PrintUsage(const char *pcProgName)
{
    (void) printf("Usage: %s [options] <data-file>\n",
                  pcProgName);
    (void) printf("    -h  --help                          ");
    (void) printf("Display this usage information\n");
    (void) printf("    -s  --skip <time>                   ");
    (void) printf("The length of data in seconds, to be\n");
    (void) printf("                                        ");
    (void) printf("skipped\n");
    (void) printf("                                        ");
    (void) printf("(default is 0 s)\n");
    (void) printf("    -p  --proc <time>                   ");
    (void) printf("The length of data in seconds, to be\n");
    (void) printf("                                        ");
    (void) printf("processed\n");
    (void) printf("                                        ");
    (void) printf("(default is all)\n");
    (void) printf("    -g  --graphics                      ");
    (void) printf("Turn on plotting\n");
    (void) printf("    -i  --invert                        ");
    (void) printf("Invert background and foreground\n");
    (void) printf("                                        ");
    (void) printf("colours in plots\n");
    (void) printf("    -e  --non-interactive               ");
    (void) printf("Run in non-interactive mode\n");
    (void) printf("    -v  --version                       ");
    (void) printf("Display the version\n");

    return;
}

