/**
 * @file yapp_dat2tim.h
 * Header file for yapp_dat2tim
 *
 * @author Jayanth Chennamangalam
 * @date 2012.12.18
 */

#ifndef __YAPP_DAT2TIM_H__
#define __YAPP_DAT2TIM_H__

#define SIZE_BUF    1048576 /* 1 MB */

int YAPP_CopyData(char *pcFileData, FILE *pFTim);

#endif  /* __YAPP_DAT2TIM_H__ */

