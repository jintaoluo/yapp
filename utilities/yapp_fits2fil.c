/*
 * @file yapp_fits2fil.c
 * Program to convert multiple PSRFITS files to a single filterbank file.
 *
 * @verbatim
 * Usage: yapp_fits2fil [options] <data-files>
 *     -h  --help                           Display this usage information
 *     -s  --sum                            Sum polarizations in the case of
 *                                          dual-polarization data
 *     -v  --version                        Display the version @endverbatim
 *
 * @author Jayanth Chennamangalam
 * @date 2012.02.25
 */

#include "yapp.h"
#include "yapp_fits2fil.h"
#include "yapp_sigproc.h"   /* for SIGPROC filterbank file format support */
#include "yapp_psrfits.h"
#include <fitsio.h>

/**
 * The build version string, maintained in the file version.c, which is
 * generated by makever.c.
 */
extern const char *g_pcVersion;

/* data file */
extern FILE *g_pFData;

/* the following are global only to enable cleaning up in case of abnormal
   termination, such as those triggered by SIGINT or SIGTERM */
void *g_pvBuf = NULL;
void *g_pvPolX = NULL;
void *g_pvPolY = NULL;
void *g_pvPolSum = NULL;
FILE *g_pFDataSec = NULL;

int main(int argc, char *argv[])
{
    char *pcFileSpec = NULL;
    char *pcFileOut = NULL;
    char acFileOut[LEN_GENSTRING] = {0};
    fitsfile *pstFileData = NULL;
    int iFormat = DEF_FORMAT;
    YUM_t stYUM = {{0}};
    int iRet = YAPP_RET_SUCCESS;
    int iNumSubInt = 0;
    int iColNum = 0;
    char acErrMsg[LEN_GENSTRING] = {0};
    int iSampsPerSubInt = 0;
    long int lBytesPerSubInt = 0;
    int iStatus = 0;
    int i = 0;
    int iDataType = 0;
    char cToSum = YAPP_FALSE;
    char cIsFirst = YAPP_TRUE;
    const char *pcProgName = NULL;
    int iNextOpt = 0;
    /* valid short options */
    const char* const pcOptsShort = "hsv";
    /* valid long options */
    const struct option stOptsLong[] = {
        { "help",                   0, NULL, 'h' },
        { "sum",                    0, NULL, 's' },
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

            case 's':   /* -s or --sum */
                /* set option */
                cToSum = YAPP_TRUE;
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

    /* handle expanded wildcards */
    iNextOpt = optind;
    while ((argc - iNextOpt) != 0)
    {
        /* get the input filename */
        pcFileSpec = argv[iNextOpt];

        if ((argc - optind) > 1)    /* more than one input file */
        {
            (void) printf("\rProcessing file %s.", pcFileSpec);
            (void) fflush(stdout);
        }

        /* determine the file type */
        iFormat = YAPP_GetFileType(pcFileSpec);
        if (YAPP_RET_ERROR == iFormat)
        {
            (void) fprintf(stderr,
                           "ERROR: File type determination failed!\n");
            return YAPP_RET_ERROR;
        }
        if (iFormat != YAPP_FORMAT_PSRFITS)
        {
            (void) fprintf(stderr,
                           "ERROR: Invalid file type!\n");
            return YAPP_RET_ERROR;
        }

        if (cIsFirst)
        {
            /* read metadata */
            iRet = YAPP_ReadMetadata(pcFileSpec, iFormat, &stYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: Reading metadata failed for file %s!\n",
                               pcFileSpec);
                return YAPP_RET_ERROR;
            }

            if (stYUM.iNumPol > YAPP_MAX_NPOL)
            {
                (void) fprintf(stderr,
                               "ERROR: Unsupported number of polarizations!"
                               "\n");
                return YAPP_RET_ERROR;
            }

            if ((1 == stYUM.iNumPol) && cToSum)
            {
                (void) printf("WARNING: Cannot sum polarizations in "
                              "single-polarization data!\n");
            }

            /* build output file name */
            pcFileOut = YAPP_GetFilenameFromPath(pcFileSpec);
            (void) strcpy(acFileOut, pcFileOut);
            if ((YAPP_MAX_NPOL == stYUM.iNumPol)
                && (!cToSum))
            {
                (void) strcat(acFileOut, ".X");
            }
            (void) strcat(acFileOut, EXT_FIL);

            /* write metadata */
            iFormat = YAPP_FORMAT_FIL;
            iRet = YAPP_WriteMetadata(acFileOut, iFormat, stYUM);
            if (iRet != YAPP_RET_SUCCESS)
            {
                (void) fprintf(stderr,
                               "ERROR: Writing metadata failed for file %s!\n",
                               acFileOut);
                YAPP_CleanUp();
                return YAPP_RET_ERROR;
            }
        }

        /*  open PSRFITS file */
        (void) fits_open_file(&pstFileData, pcFileSpec, READONLY, &iStatus);
        if  (iStatus != 0)
        {
            fits_get_errstatus(iStatus, acErrMsg); 
            (void) fprintf(stderr,
                           "ERROR: Opening file failed! %s\n",
                           acErrMsg);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        /* read SUBINT HDU header to get data parameters */
        (void) fits_movnam_hdu(pstFileData,
                               BINARY_TBL,
                               YAPP_PF_HDUNAME_SUBINT,
                               0,
                               &iStatus);
        if  (iStatus != 0)
        {
            fits_get_errstatus(iStatus, acErrMsg); 
            (void) fprintf(stderr,
                           "ERROR: Moving to HDU %s failed! %s\n",
                           YAPP_PF_HDUNAME_SUBINT,
                           acErrMsg);
            (void) fits_close_file(pstFileData, &iStatus);
            YAPP_CleanUp();
            return YAPP_RET_ERROR;
        }
        /* NOTE: the number of rows may be different in the last file, so this
                 needs to be read for every file */
        (void) fits_read_key(pstFileData,
                             TINT,
                             YAPP_PF_LABEL_NSUBINT,
                             &iNumSubInt,
                             NULL,
                             &iStatus);
        if (cIsFirst)
        {
            (void) fits_read_key(pstFileData,
                                 TINT,
                                 YAPP_PF_LABEL_NSBLK,
                                 &iSampsPerSubInt,
                                 NULL,
                                 &iStatus);
            (void) fits_get_colnum(pstFileData,
                                   CASESEN,
                                   YAPP_PF_LABEL_DATA,
                                   &iColNum,
                                   &iStatus);
            if (iStatus != 0)
            {
                fits_get_errstatus(iStatus, acErrMsg); 
                (void) fprintf(stderr,
                               "ERROR: Getting column number failed! %s\n",
                               acErrMsg);
                (void) fits_close_file(pstFileData, &iStatus);
                YAPP_CleanUp();
                return YAPP_RET_ERROR;
            }

            /* allocate memory for data array */
            lBytesPerSubInt = (long int) stYUM.iNumPol * iSampsPerSubInt
                              * stYUM.iNumChans
                              * ((float) stYUM.iNumBits
                                 / YAPP_BYTE2BIT_FACTOR);
            g_pvBuf = YAPP_Malloc(lBytesPerSubInt, sizeof(char), YAPP_FALSE);
            if (NULL == g_pvBuf)
            {
                (void) fprintf(stderr,
                               "ERROR: Memory allocation failed! %s!\n",
                               strerror(errno));
                (void) fits_close_file(pstFileData, &iStatus);
                YAPP_CleanUp();
                return YAPP_RET_ERROR;
            }

            if (YAPP_MAX_NPOL == stYUM.iNumPol)
            {
                g_pvPolX = YAPP_Malloc((lBytesPerSubInt / 2),
                                       sizeof(char),
                                       YAPP_FALSE);
                if (NULL == g_pvPolX)
                {
                    (void) fprintf(stderr,
                                   "ERROR: Memory allocation failed! %s!\n",
                                   strerror(errno));
                    (void) fits_close_file(pstFileData, &iStatus);
                    YAPP_CleanUp();
                    return YAPP_RET_ERROR;
                }
                g_pvPolY = YAPP_Malloc((lBytesPerSubInt / 2),
                                       sizeof(char),
                                       YAPP_FALSE);
                if (NULL == g_pvPolY)
                {
                    (void) fprintf(stderr,
                                   "ERROR: Memory allocation failed! %s!\n",
                                   strerror(errno));
                    (void) fits_close_file(pstFileData, &iStatus);
                    YAPP_CleanUp();
                    return YAPP_RET_ERROR;
                }
                if (cToSum)
                {
                    g_pvPolSum = YAPP_Malloc((lBytesPerSubInt / 2),
                                             sizeof(char),
                                             YAPP_FALSE);
                    if (NULL == g_pvPolSum)
                    {
                        (void) fprintf(stderr,
                                       "ERROR: Memory allocation failed! "
                                       "%s!\n",
                                       strerror(errno));
                        (void) fits_close_file(pstFileData, &iStatus);
                        YAPP_CleanUp();
                        return YAPP_RET_ERROR;
                    }
                }
            }

            /* open .fil file */
            g_pFData = fopen(acFileOut, "a");
            if (NULL == g_pFData)
            {
                (void) fprintf(stderr,
                               "ERROR: Opening file %s failed! %s.\n",
                               acFileOut,
                               strerror(errno));
                (void) fits_close_file(pstFileData, &iStatus);
                YAPP_CleanUp();
                return YAPP_RET_ERROR;
            }

            if ((YAPP_MAX_NPOL == stYUM.iNumPol)
                && (!cToSum))
            {
                /* open second .fil file */
                (void) strcpy(acFileOut, pcFileOut);
                (void) strcat(acFileOut, ".Y");
                (void) strcat(acFileOut, EXT_FIL);
            
                /* write metadata */
                iRet = YAPP_WriteMetadata(acFileOut, iFormat, stYUM);
                if (iRet != YAPP_RET_SUCCESS)
                {
                    (void) fprintf(stderr,
                                   "ERROR: Writing metadata failed for file "
                                   "%s!\n",
                                   acFileOut);
                    YAPP_CleanUp();
                    return YAPP_RET_ERROR;
                }

                g_pFDataSec = fopen(acFileOut, "a");
                if (NULL == g_pFDataSec)
                {
                    (void) fprintf(stderr,
                                   "ERROR: Opening file %s failed! %s.\n",
                                   acFileOut,
                                   strerror(errno));
                    (void) fits_close_file(pstFileData, &iStatus);
                    YAPP_CleanUp();
                    return YAPP_RET_ERROR;
                }
            }

            /* read data */
            switch (stYUM.iNumBits)
            {
                case YAPP_SAMPSIZE_4:
                    iDataType = TBYTE;
                    /* update iSampsPerSubInt */
                    iSampsPerSubInt /= 2;
                    break;

                case YAPP_SAMPSIZE_8:
                    iDataType = TBYTE;
                    break;

                case YAPP_SAMPSIZE_16:
                    iDataType = TSHORT;
                    break;

                case YAPP_SAMPSIZE_32:
                    iDataType = TLONG;
                    break;

                default:
                    (void) fprintf(stderr,
                                   "ERROR: Unexpected number of bits!\n");
                    (void) fits_close_file(pstFileData, &iStatus);
                    YAPP_CleanUp();
                    return YAPP_RET_ERROR;
            }
        }
        for (i = 1; i <= iNumSubInt; ++i)
        {
            (void) fits_read_col(pstFileData,
                                 iDataType,
                                 iColNum,
                                 i,
                                 1,
                                 ((long int) stYUM.iNumPol * stYUM.iNumChans 
                                  * iSampsPerSubInt),
                                 NULL,
                                 g_pvBuf,
                                 NULL,
                                 &iStatus);
            if (iStatus != 0)
            {
                fits_get_errstatus(iStatus, acErrMsg); 
                (void) fprintf(stderr,
                               "ERROR: Getting column number failed! %s\n",
                               acErrMsg);
                (void) fits_close_file(pstFileData, &iStatus);
                YAPP_CleanUp();
                return YAPP_RET_ERROR;
            }
            if (YAPP_MAX_NPOL == stYUM.iNumPol)
            {
                (void) YAPP_WritePolSelection(stYUM.iNumBits,
                                              lBytesPerSubInt,
                                              cToSum);
            }
            else
            {
                (void) fwrite(g_pvBuf,
                              sizeof(char),
                              lBytesPerSubInt,
                              g_pFData);
            }
        }

        (void) fits_close_file(pstFileData, &iStatus);
        cIsFirst = YAPP_FALSE;
        ++iNextOpt;
    }

    (void) printf("\n");

    /* clean up */
    YAPP_CleanUp();

    return YAPP_RET_SUCCESS;
}


/*
 * Write the selected polarization output format to file
 */
int YAPP_WritePolSelection(int iNumBits, long int lLen, char cToSum)
{
    long int i = 0;
    long int j = 0;
    long int lNumSamps = 0;

    lNumSamps = (int) ((float) lLen
                       / ((float) iNumBits / YAPP_BYTE2BIT_FACTOR));

    switch (iNumBits)
    {
        case YAPP_SAMPSIZE_4:
        {
            unsigned char *pcBuf = (unsigned char *) g_pvBuf;
            unsigned char *pcPolX = (unsigned char *) g_pvPolX;
            unsigned char *pcPolY = (unsigned char *) g_pvPolY;
            unsigned char *pcPolSum = (unsigned char *) g_pvPolSum;
            j = 0;
            for (i = 0; i < lNumSamps; i += 2)
            {
                pcPolX[j] = pcBuf[i] & 0x0F;
                pcPolY[j] = (pcBuf[i] & 0xF0) >> 4;
                pcPolX[j] |= ((pcBuf[i+1] & 0x0F) << 4);
                pcPolY[j] |= (pcBuf[i+1] & 0xF0);
                if (cToSum)
                {
                    /* NOTE: the assumption is that there are no overflows */
                    pcPolSum[j] = pcPolX[j] + pcPolY[j];
                }
                ++j;
            }
            break;
        }

        case YAPP_SAMPSIZE_8:
        {
            char *pcBuf = (char *) g_pvBuf;
            char *pcPolX = (char *) g_pvPolX;
            char *pcPolY = (char *) g_pvPolY;
            char *pcPolSum = (char *) g_pvPolSum;
            j = 0;
            for (i = 0; i < lNumSamps; i += 2)
            {
                pcPolX[j] = pcBuf[i];
                pcPolY[j] = pcBuf[i+1];
                if (cToSum)
                {
                    pcPolSum[j] = pcPolX[j] + pcPolY[j];
                }
                ++j;
            }
            break;
        }

        case YAPP_SAMPSIZE_16:
        {
            short int *psBuf = (short int *) g_pvBuf;
            short int *psPolX = (short int *) g_pvPolX;
            short int *psPolY = (short int *) g_pvPolY;
            short int *psPolSum = (short int *) g_pvPolSum;
            j = 0;
            for (i = 0; i < lNumSamps; i += 2)
            {
                psPolX[j] = psBuf[i];
                psPolY[j] = psBuf[i+1];
                if (cToSum)
                {
                    psPolSum[j] = psPolX[j] + psPolY[j];
                }
                ++j;
            }
            break;
        }

        case YAPP_SAMPSIZE_32:
        {
            int *piBuf = (int *) g_pvBuf;
            int *piPolX = (int *) g_pvPolX;
            int *piPolY = (int *) g_pvPolY;
            int *piPolSum = (int *) g_pvPolSum;
            j = 0;
            for (i = 0; i < lNumSamps; i += 2)
            {
                piPolX[j] = piBuf[i];
                piPolY[j] = piBuf[i+1];
                if (cToSum)
                {
                    piPolSum[j] = piPolX[j] + piPolY[j];
                }
                ++j;
            }
            break;
        }

        default:
        {
            (void) fprintf(stderr,
                           "ERROR: Unexpected number of bits!\n");
            return YAPP_RET_ERROR;
        }
    }

    if (!cToSum)
    {
        (void) fwrite(g_pvPolX,
                      sizeof(char),
                      (lLen / 2),
                      g_pFData);
        (void) fwrite(g_pvPolY,
                      sizeof(char),
                      (lLen / 2),
                      g_pFDataSec);
    }
    else
    {
        (void) fwrite(g_pvPolSum,
                      sizeof(char),
                      (lLen / 2),
                      g_pFData);
    }

    return YAPP_RET_SUCCESS;
}


/*
 * Prints usage information
 */
void PrintUsage(const char *pcProgName)
{
    (void) printf("Usage: %s [options] <data-files>\n",
                  pcProgName);
    (void) printf("    -h  --help                           ");
    (void) printf("Display this usage information\n");
    (void) printf("    -s  --sum                            ");
    (void) printf("Sum polarizations in the case of\n");
    (void) printf("                                         ");
    (void) printf("dual-polarization data\n");
    (void) printf("    -v  --version                        ");
    (void) printf("Display the version\n");

    return;
}

