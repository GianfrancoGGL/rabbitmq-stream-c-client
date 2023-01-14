//---------------------------------------------------------------------------
#include <ctype.h>
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsLogger.h"
#include "rmqsGlobal.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
#define BYTES_TO_DUMP_PER_ROW  16
#define LOG_SEPARATOR          "=========================================================================="
//---------------------------------------------------------------------------
rmqsLogger_t * rmqsLoggerCreate(char_t *FileName, uint8_t AppendToFile)
{
    rmqsLogger_t *Logger = (rmqsLogger_t *)rmqsAllocateMemory(sizeof(rmqsLogger_t));

    Logger->FileName = (char_t *)rmqsAllocateMemory(strlen(FileName) + 1);
    strcpy(Logger->FileName, FileName);

    Logger->AppendToFile = AppendToFile;

    Logger->File = fopen(Logger->FileName, (AppendToFile ? "a": "w"));

    Logger->Mutex = rmqsMutexCreate();
    Logger->Timer = rmqsTimerCreate();

    rmqsTimerStart(Logger->Timer);

    return Logger;
}
//---------------------------------------------------------------------------
void rmqsLoggerDestroy(rmqsLogger_t *Logger)
{
    rmqsFreeMemory((void *)Logger->FileName);

    fclose(Logger->File);

    rmqsMutexDestroy(Logger->Mutex);
    rmqsTimerDestroy(Logger->Timer);

    rmqsFreeMemory((void *)Logger);
}
//---------------------------------------------------------------------------
void rmqsLoggerRegisterLog(rmqsLogger_t *Logger, char_t *Message, char_t *Comment)
{
    char_t DateTimeString[20], TimeSinceLastLog[10];
    size_t i;

    rmqsMutexLock(Logger->Mutex);

    rmqsGetCurrentDateTimeString(DateTimeString, sizeof(DateTimeString));
    sprintf(TimeSinceLastLog, "%08d", (int32_t)rmqsTimerGetTime(Logger->Timer));

    if (Comment && *Comment != '\0')
    {
        fputs(Comment, Logger->File);
        fputs("\n", Logger->File);
    }

    fputs(DateTimeString, Logger->File);
    fputs(" - ", Logger->File);
    fputs(TimeSinceLastLog, Logger->File);
    fputs(" - ", Logger->File);
    fputs(Message, Logger->File);
    fputs("\n", Logger->File);

    for (i = 0; i < 80; i++)
    {
        fputc('-', Logger->File);
    }

    fputs("\n", Logger->File);
    fflush(Logger->File);

    rmqsTimerStart(Logger->Timer); // Reset time since last log

    rmqsMutexUnlock(Logger->Mutex);
}
//---------------------------------------------------------------------------
void rmqsLoggerRegisterDump(rmqsLogger_t *Logger, void *Data, size_t DataLen, char_t *Comment1, char_t *Comment2)
{
    char_t DateTimeString[20], TimeSinceLastLog[10];
    uchar_t *Pointer = (uchar_t *)Data;
    uchar_t Byte;
    size_t i, TotalBytesWritten = 0, RowBytesWritten = 0;
    char_t Temp[32];
    char_t AsciiVals[BYTES_TO_DUMP_PER_ROW][2], HexVals[BYTES_TO_DUMP_PER_ROW][4];

    rmqsMutexLock(Logger->Mutex);

    rmqsGetCurrentDateTimeString(DateTimeString, sizeof(DateTimeString));
    sprintf(TimeSinceLastLog, "%08d", (int32_t)rmqsTimerGetTime(Logger->Timer));

    while (TotalBytesWritten < DataLen)
    {
        if (TotalBytesWritten == 0)
        {
            fputs(DateTimeString, Logger->File);
            fputs(" - ", Logger->File);
            fputs(TimeSinceLastLog, Logger->File);
            fputs("\n", Logger->File);

            if (Comment1 && *Comment1)
            {
                fputs(Comment1, Logger->File);
                fputs("\n", Logger->File);
            }

            if (Comment2 && *Comment2)
            {
                fputs(Comment2, Logger->File);
                fputs("\n", Logger->File);
            }

            sprintf(Temp, "Bytes: %d\n\n", (int32_t)DataLen);
            fputs(Temp, Logger->File);
        }

        if (RowBytesWritten == 0)
        {
            sprintf(Temp, "%05d  ", (int32_t)TotalBytesWritten);
            fputs(Temp, Logger->File);

            for (i = 0; i < BYTES_TO_DUMP_PER_ROW; i++)
            {
                AsciiVals[i][0] = ' ';
                AsciiVals[i][1] = 0;

                HexVals[i][0] = ' ';
                HexVals[i][1] = ' ';
                HexVals[i][2] = ' ';
                HexVals[i][3] = 0;
            }
        }

        Byte = *(Pointer++);

        sprintf(HexVals[RowBytesWritten], " %02X", Byte);

        if (iscntrl((int32_t)Byte)) Byte = '.';

        AsciiVals[RowBytesWritten][0] = Byte;

        ++RowBytesWritten;
        ++TotalBytesWritten;

        if (RowBytesWritten == BYTES_TO_DUMP_PER_ROW || TotalBytesWritten == DataLen)
        {
            for (i = 0; i < BYTES_TO_DUMP_PER_ROW; i++)
            {
                fputs(HexVals[i], Logger->File);

                if (i == 7)
                {
                    //
                    // Byte separator with an additional blank
                    //
                    fputc(' ', Logger->File);
                }
            }

            fputs("   ", Logger->File);

            for (i = 0; i < BYTES_TO_DUMP_PER_ROW; i++)
            {
                fputs(AsciiVals[i], Logger->File);

                if (i == 7)
                {
                    //
                    // Byte separator with an additional blank
                    //
                    fputc(' ', Logger->File);
                }
            }

            fputs("\n", Logger->File);

            RowBytesWritten = 0;
        }
    }

    fputs(LOG_SEPARATOR, Logger->File);
    fputs("\n", Logger->File);
    fflush(Logger->File);

    rmqsTimerStart(Logger->Timer); // Reset time since last log

    rmqsMutexUnlock(Logger->Mutex);
}
//---------------------------------------------------------------------------


