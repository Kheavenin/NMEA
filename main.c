#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <stdbool.h>


#define READ_BUFFER_SIZE 128
//.h
#define READ_BUFFER_SIZE 128
#define PREAMBLE_MAX_SIZE 8
#define PREAMBLE_TABLE_SIZE 6
#define PREAMBLE_NOT_DETECTED 0xFFFF //default value

#define NMEA_MAX_SIZE 96
#define GGA_FIELDS_SIZE 14
#define VTG_FIELDS_SIZE 9

typedef struct
{
    FILE *pReadFile;
    char tReadBuffer[READ_BUFFER_SIZE];
    char *ptReadBuffer;
    uint16_t uReadSize;
} NMEA_Read_Struct_t;

typedef enum
{
    GGA = 0,
    GSA,
    GSV,
    HDT,
    RMC,
    VTG
} NMEA_PreambleNum;

typedef struct
{
    float UTC;
    char sUTC[16];
    double latitude; //N or S
    double longitude;
    float altitude;
    char latitudeDirection;
    char longitudeDirection;

    uint16_t checkSum;
} NMEA_GGA_Struct_t;

typedef struct
{
    double course;
    double magneticCourse;
    double speedKnots;
    double speedMeters;
    uint16_t checkSum;
} NMEA_VTG_Struct_t;

typedef struct
{
    bool bPreambleDectionFlag : 1;
    char *ptMsgNMEA;
    char *ptDefaultMsgNMEA;
    size_t sizeMsgNMEA ;
    uint16_t uDetectedPreamble;

    NMEA_GGA_Struct_t sGGA;
    NMEA_VTG_Struct_t sVTG;
    NMEA_GGA_Struct_t *psGGA;
    NMEA_VTG_Struct_t *psVTG;
    
} NMEA_0183_Struct_t;

const char *InputFileName = "testInput.txt";

bool InitNMEAStruct(NMEA_0183_Struct_t *psNMEA_0183_struct);
bool DeinitNMEAStruct(NMEA_0183_Struct_t *psNMEA_0183_struct);
void setPreambleDetection(NMEA_0183_Struct_t *psNMEA_0183_struct);
void resetPreambleDetection(NMEA_0183_Struct_t *psNMEA_0183_struct);
bool getPreambleDetection(NMEA_0183_Struct_t *psNMEA_0183_struct);

void setPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct, uint16_t preamble);
void resetPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct);
uint16_t getPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct);

void RunDecodeNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct, char *s, size_t size);

bool setMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct, char *s, size_t n);
bool resetMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct);
char* getMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct);

bool findPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct, char *s, size_t size);
bool decodeNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct, char *pMsgNMEA);
void decodeGGA(NMEA_0183_Struct_t *psNMEA_0183_struct);
void decodeVTG(NMEA_0183_Struct_t *psNMEA_0183_struct);

char *findSperator(char *str, char sep);
void UTCtoString(char *s, double d);
// end of .h



NMEA_Read_Struct_t sReadBuffer;
NMEA_Read_Struct_t *psReadBuffer = &sReadBuffer;
NMEA_0183_Struct_t sNMEA;
NMEA_0183_Struct_t *psNMEA = &sNMEA;

bool InitReadStruct(NMEA_Read_Struct_t *psNMEA_ReadStruct);
void openFile(NMEA_Read_Struct_t *psNMEA_ReadStruct, const char *FileName);
void closeFile(NMEA_Read_Struct_t *psNMEA_ReadStruct);
void getLine(NMEA_Read_Struct_t *psNMEA_ReadStruct);
void putLine(NMEA_Read_Struct_t *psNMEA_ReadStruct);
void clearBuffer(NMEA_Read_Struct_t *psNMEA_ReadStruct);

static const char *const tNMEA_Preamble[PREAMBLE_TABLE_SIZE] = {
    [GGA] = "$GPGGA",
    [GSA] = "$GPGSA",
    [GSV] = "$GPGSV",
    [HDT] = "$GPHDT",
    [RMC] = "$GPRMC",
    [VTG] = "$GPVTG"};



//** MAINA */
int main()
{
    InitReadStruct(psReadBuffer);
    InitNMEAStruct(psNMEA);

    openFile(psReadBuffer, InputFileName);
    size_t k = 0;
    while (k < 100)
    {
        getLine(psReadBuffer);
        putLine(psReadBuffer);
        RunDecodeNMEA(psNMEA, psReadBuffer->tReadBuffer, psReadBuffer->uReadSize);
        memset(psReadBuffer->tReadBuffer, 0, READ_BUFFER_SIZE); //clear buffer
        printf("%s\n", psNMEA->psGGA->sUTC);
        k++;
    }

    DeinitNMEAStruct(psNMEA);
    closeFile(psReadBuffer);

    return 0;
}

//* Read file */
bool InitReadStruct(NMEA_Read_Struct_t *psNMEA_ReadStruct)
{
    if (psNMEA_ReadStruct != NULL)
    {
        psNMEA_ReadStruct->pReadFile = NULL;
        psNMEA_ReadStruct->ptReadBuffer = psNMEA_ReadStruct->tReadBuffer;
        psNMEA_ReadStruct->uReadSize = 0;

        return true;
    }
    return false;
}
void openFile(NMEA_Read_Struct_t *psNMEA_ReadStruct, const char *FileName)
{
    const char ReadMode = 'r';
    psNMEA_ReadStruct->pReadFile = fopen(FileName, &ReadMode);
    printf("Open input file.\nRead Status: ");
    if (psNMEA_ReadStruct->pReadFile != NULL)
    {
        printf("open correct.\n");
    }
    else
    {
        printf("open failed!\n");
    }
}
void closeFile(NMEA_Read_Struct_t *psNMEA_ReadStruct)
{
    if (psNMEA_ReadStruct != NULL)
    {
        fclose(psNMEA_ReadStruct->pReadFile);
    }
}
void getLine(NMEA_Read_Struct_t *psNMEA_ReadStruct)
{
    if ((psNMEA_ReadStruct != NULL) && (psNMEA_ReadStruct->pReadFile != NULL))
    {
        fgets(psNMEA_ReadStruct->ptReadBuffer, READ_BUFFER_SIZE, psNMEA_ReadStruct->pReadFile);
        //    fscanf(psNMEA_ReadStruct->pReadFile, "%[^\n]", psNMEA_ReadStruct->ptReadBuffer);
        psNMEA_ReadStruct->uReadSize = strlen(psNMEA_ReadStruct->ptReadBuffer);
    }
}
void putLine(NMEA_Read_Struct_t *psNMEA_ReadStruct)
{
    if (psNMEA_ReadStruct != NULL)
    {
        for (size_t i = 0; i < psNMEA_ReadStruct->uReadSize; i++)
        {
            printf("%c", psNMEA_ReadStruct->ptReadBuffer[i]);
            if (i == psNMEA_ReadStruct->uReadSize - 1)
            {
                printf("\n");
            }
        }
    }
}
void clearBuffer(NMEA_Read_Struct_t *psNMEA_ReadStruct)
{
    if (psNMEA_ReadStruct != NULL)
    {
        memset(psNMEA_ReadStruct->ptReadBuffer, 0, READ_BUFFER_SIZE);
        psNMEA_ReadStruct->uReadSize = 0;
    }
}


//.c

static void *InitBufferMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct, size_t bufferSize)
{
    if ((psNMEA_0183_struct != NULL) && (bufferSize > 0) && (bufferSize <= NMEA_MAX_SIZE))
    {
        psNMEA_0183_struct->ptMsgNMEA = (char *)calloc(bufferSize, sizeof(char));
        psNMEA_0183_struct->ptDefaultMsgNMEA = psNMEA_0183_struct->ptMsgNMEA;
        return (psNMEA_0183_struct->ptMsgNMEA);
    }
    return NULL;
}
static void DeinitBufferMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if ((psNMEA_0183_struct != NULL) && (psNMEA_0183_struct->ptMsgNMEA != NULL))
    {
        free(psNMEA_0183_struct->ptDefaultMsgNMEA);
        psNMEA_0183_struct->ptMsgNMEA = NULL;
    }
}
static void InitGGAStruct(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        psNMEA_0183_struct->psGGA = &psNMEA_0183_struct->sGGA;

        psNMEA_0183_struct->psGGA->altitude = 0.0;
        psNMEA_0183_struct->psGGA->latitude = 0.0;
        psNMEA_0183_struct->psGGA->longitude = 0.0;
        psNMEA_0183_struct->psGGA->latitudeDirection = 0;
        psNMEA_0183_struct->psGGA->longitudeDirection = 0;
        psNMEA_0183_struct->psGGA->UTC = 0.0;
        psNMEA_0183_struct->psGGA->checkSum = 0;
    }
}
static void InitVTGStruct(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        psNMEA_0183_struct->psVTG = &psNMEA_0183_struct->sVTG;
        psNMEA_0183_struct->psVTG->course = 0.0;
        psNMEA_0183_struct->psVTG->magneticCourse = 0.0;
        psNMEA_0183_struct->psVTG->speedKnots = 0.0;
        psNMEA_0183_struct->psVTG->speedMeters = 0.0;
        psNMEA_0183_struct->psVTG->checkSum = 0;
    }
}

bool InitNMEAStruct(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        psNMEA_0183_struct->bPreambleDectionFlag = false;
        psNMEA_0183_struct->ptMsgNMEA = NULL;
        psNMEA_0183_struct->ptDefaultMsgNMEA = NULL;
        psNMEA_0183_struct->sizeMsgNMEA = 0;
        psNMEA_0183_struct->uDetectedPreamble = PREAMBLE_NOT_DETECTED;
        
        InitBufferMsgNMEA(psNMEA_0183_struct, NMEA_MAX_SIZE);
        
        InitGGAStruct(psNMEA_0183_struct);
        InitVTGStruct(psNMEA_0183_struct);

        return true;
    }
    return false;
}
bool DeinitNMEAStruct(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        DeinitBufferMsgNMEA(psNMEA_0183_struct);
        return true;
    }
    return false;
}

void RunDecodeNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct, char *s, size_t size)
{
    setMsgNMEA(psNMEA_0183_struct, s, size);

    findPreamble(psNMEA_0183_struct, psNMEA_0183_struct->ptMsgNMEA, psNMEA_0183_struct->sizeMsgNMEA);
    decodeNMEA(psNMEA_0183_struct, s);
    //after decode
    psNMEA_0183_struct->bPreambleDectionFlag = false;
    psNMEA_0183_struct->uDetectedPreamble = PREAMBLE_NOT_DETECTED;
    resetMsgNMEA(psNMEA_0183_struct);
}

void setPreambleDetection(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        psNMEA_0183_struct->bPreambleDectionFlag = true;
    }
}
void resetPreambleDetection(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        psNMEA_0183_struct->bPreambleDectionFlag = false;
    }
}
bool getPreambleDetection(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        return (psNMEA_0183_struct->bPreambleDectionFlag);
    }
    return false;
}

void setPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct, uint16_t preamble)
{
    if ((psNMEA_0183_struct != NULL) && (preamble < PREAMBLE_TABLE_SIZE))
    {
        psNMEA_0183_struct->uDetectedPreamble = preamble;
    }
}
void resetPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if ((psNMEA_0183_struct != NULL))
    {
        psNMEA_0183_struct->uDetectedPreamble = PREAMBLE_NOT_DETECTED;
    }
}
uint16_t getPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    if (psNMEA_0183_struct != NULL)
    {
        return (psNMEA_0183_struct->uDetectedPreamble);
    }
    return PREAMBLE_NOT_DETECTED;
}

bool findPreamble(NMEA_0183_Struct_t *psNMEA_0183_struct, char *s, size_t n)
{
    if ((psNMEA_0183_struct != NULL) && (s != NULL) && (n > 5))
    {
        uint8_t i = 0;
        char *preambleOccur = NULL;
        do
        {
            preambleOccur = strstr(s, tNMEA_Preamble[i]);
            i++;
        } while ((preambleOccur == NULL) && (i < (PREAMBLE_TABLE_SIZE)));

        if (preambleOccur != NULL)
        {
            setPreambleDetection(psNMEA_0183_struct); // if preamble detected set flag
            setPreamble(psNMEA_0183_struct, (i - 1));
            return true;
        }
        return false;
    }
    return false;
}
bool decodeNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct, char *pMsgNMEA)
{
    if ((psNMEA_0183_struct != NULL) && (pMsgNMEA != NULL))
    {
        psNMEA_0183_struct->ptMsgNMEA = pMsgNMEA;
        switch (psNMEA_0183_struct->uDetectedPreamble)
        {
        case GGA:
            decodeGGA(psNMEA_0183_struct);
            break;
        case GSA:
            break;
        case GSV:
            break;
        case HDT:
            break;
        case RMC:
            break;
        case VTG:
            decodeVTG(psNMEA_0183_struct);
        default:
            break;
        }

        return true;
    }
    return false;
}
void decodeGGA(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    uint8_t fieldsNum = GGA_FIELDS_SIZE; //number of fileds in GGS msg
    long dataLength[16];
    char tmp[16];
    char *ptFieldsList[GGA_FIELDS_SIZE];
    memset((void *)dataLength, 0, sizeof(dataLength));
    memset((void *)tmp, 0, sizeof(tmp));
    memset((void *)ptFieldsList, 0, sizeof(char *) * GGA_FIELDS_SIZE);

    /** Disassemble */
    ptFieldsList[0] = strchr(psNMEA_0183_struct->ptMsgNMEA, ',') + 1;
    for (size_t i = 0; i < fieldsNum - 1; i++)
    {
        ptFieldsList[i + 1] = strchr(ptFieldsList[i] + 1, ',') + 1;
    }
    //find and catch check sum
    ptFieldsList[GGA_FIELDS_SIZE - 1] = strchr(ptFieldsList[GGA_FIELDS_SIZE - 2] + 1, '*') + 1;
    /** Assigning */
    for (size_t i = 0; i < fieldsNum - 1; i++)
    {
        dataLength[i] = ptFieldsList[i + 1] - ptFieldsList[i];
    }

    //Check sum
    psNMEA_0183_struct->psGGA->checkSum = atoi(ptFieldsList[GGA_FIELDS_SIZE - 1]);
    //UTC
    strncpy(tmp, ptFieldsList[0], dataLength[0]);
    psNMEA_0183_struct->psGGA->UTC = atof(tmp);
    UTCtoString(psNMEA_0183_struct->psGGA->sUTC, psNMEA_0183_struct->psGGA->UTC);

    //latitude
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[1], dataLength[1] - 1);
    psNMEA_0183_struct->psGGA->latitude = atof(tmp);
    strncpy(&(psNMEA_0183_struct->psGGA->latitudeDirection), ptFieldsList[2], 1);

    //longitude
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[3], dataLength[3] - 1);
    psNMEA_0183_struct->psGGA->longitude = atof(tmp);
    strncpy(&(psNMEA_0183_struct->psGGA->longitudeDirection), ptFieldsList[4], 1);

    //altitude
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[8], dataLength[8] - 1);
    psNMEA_0183_struct->psGGA->altitude = atof(tmp);
}
void decodeVTG(NMEA_0183_Struct_t *psNMEA_0183_struct)
{
    uint8_t fieldsNum = VTG_FIELDS_SIZE; //number of fileds in GGS msg
    long dataLength[16];
    char tmp[16];
    char *ptFieldsList[VTG_FIELDS_SIZE];
    memset((void *)dataLength, 0, sizeof(dataLength));
    memset((void *)ptFieldsList, 0, sizeof(char *) * VTG_FIELDS_SIZE);

    /** Disassemble */
    ptFieldsList[0] = strchr(psNMEA_0183_struct->ptMsgNMEA, ',') + 1;
    for (size_t i = 0; i < fieldsNum - 1; i++)
    {
        ptFieldsList[i + 1] = strchr(ptFieldsList[i] + 1, ',') + 1;
        if (ptFieldsList[i + 1] == NULL)
        {
            break;
        }
    }
    //find and catch check sum
    ptFieldsList[VTG_FIELDS_SIZE - 1] = strchr(ptFieldsList[VTG_FIELDS_SIZE - 2] + 1, '*') + 1;

    /** Assigning */
    for (size_t i = 0; i < fieldsNum - 1; i++)
    {
        dataLength[i] = ptFieldsList[i + 1] - ptFieldsList[i];
    }

    //real course
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[0], dataLength[0] - 1); //  -1 remove ','
    psNMEA_0183_struct->psVTG->course = atof(tmp);

    //magnetic course
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[2], dataLength[2] - 1);
    psNMEA_0183_struct->psVTG->magneticCourse = atof(tmp);

    //Speed in knots
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[4], dataLength[4] - 1);
    psNMEA_0183_struct->psVTG->speedKnots = atof(tmp);

    //Speed in kilometers per hour
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[6], dataLength[6] - 1);
    psNMEA_0183_struct->psVTG->speedMeters = atof(tmp);

    //check sum
    memset((void *)tmp, 0, sizeof(tmp));
    strncpy(tmp, ptFieldsList[VTG_FIELDS_SIZE - 1], dataLength[VTG_FIELDS_SIZE - 2]);
    psNMEA_0183_struct->psVTG->checkSum = atoi(tmp);
}

void UTCtoString(char *s, double d)
{
    if ((s != NULL) && (d >= 0))
    {
        char t[16];
        memset((void *)t, 0, sizeof(t));
        uint32_t integral = (uint32_t)d;
        double fraction = d - integral;
        uint16_t hours = (uint16_t)(integral / 10000);
        uint16_t minutes = (uint16_t)(integral / 100) - (hours * 100);
        uint16_t seconds = integral - (hours * 10000) - (minutes * 100);
        uint16_t useconds = (uint16_t)(fraction * 1000);
        snprintf(t, 16, "%02d:%02d:%02d.%03d", hours, minutes, seconds, useconds);
        strncpy(s, t, 16);
    }
}


bool setMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct, char *s, size_t n) {
    if ((psNMEA_0183_struct != NULL) && (s != NULL) && (n>0 && n<NMEA_MAX_SIZE))
    {
        if (strncpy(psNMEA_0183_struct->ptMsgNMEA, s, n) != NULL)
        {
            psNMEA_0183_struct->sizeMsgNMEA = n;
            return true;
        }
        return false;
    }
    return false;
}
bool resetMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct){
    if ((psNMEA_0183_struct != NULL) && (psNMEA_0183_struct->ptMsgNMEA != NULL ))
    {
        memset((void*) psNMEA_0183_struct->ptMsgNMEA, 0,NMEA_MAX_SIZE);
        psNMEA_0183_struct->sizeMsgNMEA = 0;
        return true;
    }
    return false;
}
char* getMsgNMEA(NMEA_0183_Struct_t *psNMEA_0183_struct) { 
    if ((psNMEA_0183_struct != NULL) && (psNMEA_0183_struct->ptMsgNMEA != NULL))
    {
        return (psNMEA_0183_struct->ptMsgNMEA);
    }
    return NULL;
}

