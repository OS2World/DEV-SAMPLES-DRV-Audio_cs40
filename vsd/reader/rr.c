//
// 12-Jan-99
// Resource Reader - Cornel Huth
//
// read and xlate info in resource dlls (specifically MM dlls)
//

#define PRG_DATE __DATE__  //"12-Jan-99"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DO_ERR      0
#define DO_VSD      1
#define DO_CARDINFO 2

typedef struct _RS {
 short typeID;
 short nameID;
 long  size;
 short object;
 long  offset;
} RS;

short nameID;

typedef struct _DST {   // DATATYPE_* subtype
 long id;
 char *namePtr;
} T_DST;

typedef struct _DT {    // DATATYPE_*
 long id;
 char *namePtr;
 T_DST *dstPtr;
} T_DT;

char *devIDname[] = {"MINIDD","ACPA","MACPA","MPU401","SOUND_BLASTER","IMF","PS1","PAS16"};

T_DST dstNone[]= {{0,"SUBTYPE_NONE,    "},
                  {-1,""}};
T_DST dstPCM[]=  {{1,"WAVE_FORMAT_1M08,"},
                  {2,"WAVE_FORMAT_1S08,"},
                  {3,"WAVE_FORMAT_1M16,"},
                  {4,"WAVE_FORMAT_1S16,"},
                  {5,"WAVE_FORMAT_2M08,"},
                  {6,"WAVE_FORMAT_2S08,"},
                  {7,"WAVE_FORMAT_2M16,"},
                  {8,"WAVE_FORMAT_2S16,"},
                  {9,"WAVE_FORMAT_4M08,"},
                 {10,"WAVE_FORMAT_4S08,"},
                 {11,"WAVE_FORMAT_4M16,"},
                 {12,"WAVE_FORMAT_4S16,"},
                 {13,"WAVE_FORMAT_8M08,"},
                 {14,"WAVE_FORMAT_8S08,"},
                 {15,"WAVE_FORMAT_8M16,"},
                 {16,"WAVE_FORMAT_8S16,"},
                  {-1,""}};
T_DST dstMu[]=   {{1,"MULAW_8B8KS,     "},
                  {2,"MULAW_8B11KS,    "},
                  {3,"MULAW_8B22KS,    "},
                  {4,"MULAW_8B44KS,    "},
                  {5,"MULAW_8B8KM,     "},
                  {6,"MULAW_8B11KM,    "},
                  {7,"MULAW_8B22KM,    "},
                  {8,"MULAW_8B44KM,    "},
                  {-1,""}};
T_DST dstA[]=    {{1,"ALAW_8B8KS,      "},
                  {2,"ALAW_8B11KS,     "},
                  {3,"ALAW_8B22KS,     "},
                  {4,"ALAW_8B44KS,     "},
                  {5,"ALAW_8B8KM,      "},
                  {6,"ALAW_8B11KM,     "},
                  {7,"ALAW_8B22KM,     "},
                  {8,"ALAW_8B44KM,     "},
                  {-1,""}};
T_DST dstAVC[]=  {{1,"ADPCM_AVC_VOICE, "},
                  {2,"ADPCM_AVC_MUSIC, "},
                  {3,"ADPCM_AVC_STEREO,"},
                  {4,"ADPCM_AVC_HQ,    "},
                  {-1,""}};
T_DST dstCT[]=   {{1,"ADPCM_CT_16B8KS, "},
                  {2,"ADPCM_CT_16B11KS,"},
                  {3,"ADPCM_CT_16B22KS,"},
                  {4,"ADPCM_CT_16B44KS,"},
                  {5,"ADPCM_CT_16B8KM, "},
                  {6,"ADPCM_CT_16B11KM,"},
                  {7,"ADPCM_CT_16B22KM,"},
                  {8,"ADPCM_CT_16B44KM,"},
                  {-1,""}};

// these are to be prefixed by "DATATYPE_" for output

T_DT dt[] = {{0,        "NULL,     ",  dstNone},
             {1,        "WAVEFORM, ",  dstPCM},
             {0x101,    "MULAW,    ",  dstMu},
             {7,        "RIFF_MULAW,", dstMu},
             {0x102,    "ALAW,     ",  dstA},
             {6,        "RIFF_ALAW,",  dstA},
             {0x103,    "ADPCM_AVC,",  dstAVC},
             {0x200,    "CT_ADPCM, ",  dstCT},
             {0x201,    "MIDI,     ",  dstNone}};


char *c10msg[] = {"ID number of this adapter",
                  "adapter type"};

char *c11msg[] = {"max adapters this adapter",
                  "helpfile name (max 39 chars)",
                  "adapter specific dll name (max 19 chars)",
                  "adapter specific dll entry point (max 39 chars)",
                  "config.sys lines (max is 6 lines)",
                  "# of drivers to install (max is 6)",
                  "product name (max 39 chars)",
                  "version of software (max 5 chars)",
                  "PDD name (max 6 chars)",
                  "MCD table name (max 19 chars)",
                  "VSD table name (max 19 chars)"};

char *c12msg[] = {"install name (max 17 chars)",
                  "device type (max 3 chars)",
                  "device flag (max 3 chars)",
                  "MCD driver name (max 19 chars)",
                  "VSD driver name (max 19 chars)",
                  "share type (max 3 chars)",
                  "resource name (max 17 chars)",
                  "# of resource units (max 2 chars)",
                  "# of resource classes (max 2 chars)",
                  "# valid resource class combos (max 2 chars)",
                  "# of connectors  (max 2 chars)",
                  "# of extensions (max 2 chars)",
                  "extended attributes (max 255 chars)",
                  "alias name (max 17 chars)",
                  "device parms (max 255 chars)"};


// get short/long values

long GetShort(char **currPtr) {
 short *itPtr, it;
 itPtr = (short *)*currPtr;
 it = *itPtr;
 *currPtr = *currPtr + 2;
 return it;
}

long GetLong(char **currPtr) {
 long *itPtr, it;
 itPtr = (long *)*currPtr;
 it = *itPtr;
 *currPtr = *currPtr + 4;
 return it;
}


int main(int argc,char *argv[]) {

 int rc = 0;
 int i, j, k, sl;

 int rrType = 0, tLen;
 //char zeroMsg[] = "\\0";        // to display a null string character (don't need it)

 unsigned short tmpS;
 unsigned long tmpL;

 RS *rsPtr = 0;

 FILE *fh = 0;
 char *fnPtr = 0;
 char *buffPtr = 0;
 char *currPtr = 0;
 char *tPtr = 0;

 char *cPtr;
 //short *sPtr;
 long *lPtr;

 unsigned long dataOffset, resOffset, resCount, resEnd, skipFlag = 0, so = 0;

 long dtCount = sizeof(dt) / sizeof(T_DT);

 setbuf(stdout,NULL);
 printf("/* RR 1.0  Copyright (C)1999 Cornel Huth  All rights reserved [%s] */\n",PRG_DATE);

 for (i=1; i < argc; i++) {

    sl = strlen(argv[i]);
    if (strnicmp(argv[i],";",1) == 0) break;

    if (sl >= 2) {
       if (strnicmp(argv[i],"-h",2) == 0) {
          argc = 1;
          break;
       }
    }

    if (sl >= 2) {
       if (strnicmp(argv[i],"-z",2) == 0) {
          skipFlag = 1;
          continue;
       }
    }

    if (sl >= 4) {
       if (strnicmp(argv[i],"-so:",4) == 0) {
          if (*(argv[i]+5) == 'x') {
             sscanf(argv[i]+4,"%X",&so);
          }
          else {
             so = atol(argv[i]+4);
          }
          continue;
       }
    }

    if (sl >= 4) {
       if (strnicmp(argv[i],"-vsd",4) == 0) {
          rrType = DO_VSD;
          continue;
       }
    }

    if (sl >= 5) {
       if (strnicmp(argv[i],"-card",5) == 0) {
          rrType = DO_CARDINFO;
          continue;
       }
    }

    if (*argv[i] != '-') {
       if (fnPtr != 0) {
          argc = 999;
          break;
       }
       else {
          fnPtr = argv[i];
          continue;
       }
    }
    else {
       argc = 999;
       break;
    }
 }

 if (argc == 999) {
    printf("\n%s<---what's that supposed to do?\n",argv[i]);
    argc = 1;
 }

 if (argc < 2 || fnPtr == NULL) {
    printf("Use: [C:\\]rr [-switch] any.dll\n\n");
    printf("  -vsd      process as MM audio vsd\n");
    printf("  -card     process as MM cardinfo\n");
    printf("  -so:n     n=absolute file offset to resource data start\n");
    printf("  -z        add 0x10 to start offset\n");
    printf("  -h        show this\n");
    printf("            nothing then show resource table\n");
    return 1;
 }

 // go

 fh = fopen(fnPtr, "r+b");
 if (fh == 0) {
    printf("*** can't open file '%s', errno=%u\n",fnPtr,errno);
    goto ExitNow;
 }

 fseek(fh,0x3C,SEEK_SET);
 if (fread(&tmpL,1,sizeof(tmpL),fh) != 4) {
    printf("*** can't read LX offset dword \n");
    goto ExitNow;
 }

 fseek(fh,tmpL+0x88,SEEK_SET);
 if (fread(&resEnd,1,sizeof(resEnd),fh) != 4) {  // resEnd is relative start of EXE
    printf("*** can't read res end dword \n");   // though is not really a "res end", just a start of next
    goto ExitNow;
 }

 buffPtr = malloc(resEnd);
 if (buffPtr == 0) {
    printf("*** can't allocate %d bytes for buffer\n",resEnd);
    goto ExitNow;
 }

 fseek(fh,0,SEEK_SET);
 if (fread(buffPtr,1,resEnd,fh) != resEnd) {
    printf("*** can't read the resource data (%d bytes) \n",resEnd);
    goto ExitNow;
 }


 cPtr = buffPtr;
 cPtr = cPtr + 0x3C;
 lPtr = (long *)cPtr;        // lPtr -> LX start offset
 cPtr = buffPtr + *lPtr;     // cPtr -> LX start ('LX')

 resOffset = *((long *)(cPtr + 0x50));       // relative LX header
 resCount = *((long *)(cPtr + 0x54));        // the number of resources
 dataOffset = *((long *)(cPtr + 0x80));      // relative EXE header

 // HACK! ---------------------------------------- !!!!!
 // HACK! ---------------------------------------- !!!!!
 // HACK! ---------------------------------------- !!!!!

 if (skipFlag) dataOffset = dataOffset + 0x10; // quick hack to cover up RET (C3) code in rcstub (very temp)
 if (so) {
    dataOffset = so;                      // -so:n for bogus cardinfo.dll like from yam 3.1.4
    printf("using absolute file offset of 0x%X to data start\n",so);
 }

 currPtr = cPtr + resOffset;

 if (resCount == 0) {
    printf("*** no resources \n");
    goto ExitNow;
 }

 rsPtr = malloc(resCount * sizeof(RS));
 if (rsPtr == 0) {
    printf("*** can't allocated %u rs[] elements \n",resCount);
    goto ExitNow;
 }
 else {
    for (i=0; i < resCount; i++) {
       rsPtr[i].typeID = GetShort(&currPtr);
       rsPtr[i].nameID = GetShort(&currPtr);
       rsPtr[i].size   = GetLong(&currPtr);
       rsPtr[i].object = GetShort(&currPtr); // dunno
       rsPtr[i].offset = GetLong(&currPtr);
       if (rrType == 0) {
          printf("%2d: typeID=%d   nameID=%2d   size=%5d   obj#=%d   offset=0x%.4X \n",(i+1),
                   rsPtr[i].typeID,rsPtr[i].nameID,rsPtr[i].size,rsPtr[i].object,rsPtr[i].offset);
       }
    }
 }

 if (rrType == DO_VSD) {

    for (i=0; i < resCount; i++) {

       long rows;
       int xLen;

       currPtr = buffPtr + dataOffset + rsPtr[i].offset; // relative EXE header
       xLen = strlen(currPtr);

       printf("\n/* resource typeID=%d (size=%d bytes) */\n",rsPtr[i].typeID,rsPtr[i].size);
       printf("RCDATA %d\nBEGIN\n",rsPtr[i].nameID);
       printf("\"%s\",\n",currPtr);
       currPtr = currPtr + (xLen+1);

       tmpS = GetShort(&currPtr);            // get device ID
       if (tmpS <= 7) {
          tPtr = devIDname[tmpS];
          printf("%s,",tPtr);
       }
       else {
          printf("%hu,",tmpS);
       }

       printf("%hu,\n",GetShort(&currPtr));  // get rez1
       printf("%uL,\n",GetLong(&currPtr));   // get rez2
       rows = GetLong(&currPtr);             // get rows
       printf("%uL,\n",rows);

       for (j=0; j < rows; j++) {

          T_DST *dstPtr = 0;

          tmpL = GetLong(&currPtr);          // data type
          for (k=0; k < dtCount; k++) {
             if (tmpL == dt[k].id) {
                printf("DATATYPE_%s",dt[k].namePtr);
                dstPtr = dt[k].dstPtr;
             }
          }
          if (dstPtr==0) printf("0x%X, ",tmpL);// if no match put up literal

          tmpL = GetLong(&currPtr);          // data subtype
          if (dstPtr != 0) {                 // search for text only if found a data type
             do {
                if (tmpL == dstPtr->id) {
                   printf("%s",dstPtr->namePtr);
                   break;
                }
                dstPtr++;
             } while(dstPtr->id != -1);
          }
          if (tmpL == -1) printf("0x%X, ",tmpL);// if no match put up literal

          printf("%6hu, ",GetShort(&currPtr)); // data sample rate (signed)
          printf("%2hu, ",GetShort(&currPtr)); // data sample size
          printf("%1hu, ",GetShort(&currPtr)); // data channels

          tmpL = GetLong(&currPtr);          // sampling description
          if (tmpL == 0) printf("STATIC_RATE,     ");
          if (tmpL == 1) printf("BEGIN_CONTINUOUS,");
          if (tmpL == 2) printf("END_CONTINUOUS,  ");
          if (tmpL > 2)  printf("%u,               ");

          tmpS = GetShort(&currPtr);            // play resource class
          if (tmpS == 0) printf("0,         ");
          if (tmpS == 1) printf("PCM_CLASS, ");
          if (tmpS == 2) printf("MIDI_CLASS,"); // no ,space needed since using %2 in next output (usually 1 digit)
          if (tmpS > 2)  printf("%u,         ");

          printf("%2u, ",GetShort(&currPtr));   // play resource units used

          tmpS = GetShort(&currPtr);            // record resource class
          if (tmpS == 0) printf("0,         "); // -0- indicates not possible to record at this row type
          if (tmpS == 1) printf("PCM_CLASS, ");
          if (tmpS == 2) printf("MIDI_CLASS,");
          if (tmpS > 2)  printf("%u,         ");

          printf("%2u, ",GetShort(&currPtr));    // record resource units used
          printf("\n");
       }
       printf("END\n");
    }
 }
 else if (rrType == DO_CARDINFO) {

    for (i=0; i < resCount; i++) {

       currPtr = buffPtr + dataOffset + rsPtr[i].offset; // relative EXE header

       printf("\n/* resource typeID=%d (size=%d bytes) */\n",rsPtr[i].typeID,rsPtr[i].size);
       printf("RCDATA %d\nBEGIN\n",rsPtr[i].nameID);

       nameID = rsPtr[i].nameID;
       if (nameID > 19) nameID = (nameID % 10)+10; //1,10-19,20-29(is 10-19 of second adapter),30-39(),...

       switch(nameID) {     // switch on mod 10, but output actual nameID number
       case  1:
          printf("   \"%s\"\t\t/* number of adapters in this file */\n",currPtr);
          currPtr++;
          break;

       case 10:
          for (j=0; j < 2; j++) {
             tLen = strlen(currPtr);
             printf("   \"%s\",\t/* %s */\n",currPtr,c10msg[j]);
             currPtr = currPtr + tLen + 1;
          }
          break;

       case 11:
          for (j=0; j < 4; j++) {
             tLen = strlen(currPtr);
             printf("   \"%s\",\t/* %s */\n",currPtr,c11msg[j]);
             currPtr = currPtr + tLen + 1;
          }

          printf("\n   /* -- data for config.sys -- */\n\n");
          tLen = strlen(currPtr);
          tmpL = atol(currPtr);
          printf("   \"%s\",\t/* %s */\n",currPtr,c11msg[4]);
          currPtr = currPtr + tLen + 1;
          while(tmpL) {


             tLen = strlen(currPtr);
             cPtr = strstr(currPtr,"\\");       // have to change single backslash to a double backslash
             if (cPtr) {

                char *currPtr2 = currPtr;       // use to preserve currPtr for later ...+tLen+1

                *cPtr = 0;
                printf("   \"%s\\\\",currPtr2); // which means doing double back twice for each in this source
                while (cPtr) {
                   currPtr2 = cPtr + 1;
                   cPtr = strstr(currPtr2,"\\");
                   if (cPtr) *cPtr = 0;
                   printf("%s",currPtr2);
                }
                printf("\n");
             }
             else {
                printf("   \"%s\",\n",currPtr);
             }
             currPtr = currPtr + tLen + 1;
             tmpL--;
          }
          printf("\n   /* -- data for ini file -- */\n\n");
          for (j=5; j < 11; j++) {
             tLen = strlen(currPtr);
             printf("   \"%s\",\t/* %s */\n",currPtr,c11msg[j]);
             currPtr = currPtr + tLen + 1;
          }
          break;

       case 12:
       case 13:
       case 14:
       case 15:
       case 16:
       case 17:
       case 18:
          for (j=0; j < 15; j++) {
             tLen = strlen(currPtr);

             if (j==8) {

                printf("   \"%s\",\t/* %s */\n",currPtr,c12msg[j]);
                tmpL = atol(currPtr);
                for (k=0; k < tmpL; k++) {
                   if (k==0) printf("   ");
                   currPtr = currPtr + tLen + 1;
                   tLen = strlen(currPtr);
                   printf("\"%s\",",currPtr);
                }
                printf("\t/* resource classes (max 2 chars ea) */\n");
             }

             else if (j==9) {

                printf("   \"%s\",\t/* %s */\n",currPtr,c12msg[j]);
                tmpL = atol(currPtr);
                for (k=0; k < tmpL; k++) {
                   if (k==0) printf("   ");
                   currPtr = currPtr + tLen + 1;
                   tLen = strlen(currPtr);
                   printf("\"%s\",",currPtr);
                   currPtr = currPtr + tLen + 1;
                   tLen = strlen(currPtr);
                   printf("\"%s\",",currPtr);
                }
                printf("\t/* valid resource class combos */\n");
             }

             else if (j==10) {

                printf("   \"%s\",\t/* %s */\n",currPtr,c12msg[j]);
                tmpL = atol(currPtr);
                for (k=0; k < tmpL; k++) {
                   if (k==0) printf("   ");
                   currPtr = currPtr + tLen + 1;
                   tLen = strlen(currPtr);
                   printf("\"%s\",",currPtr);
                   currPtr = currPtr + tLen + 1;
                   tLen = strlen(currPtr);
                   printf("\"%s\",",currPtr);
                   currPtr = currPtr + tLen + 1;
                   tLen = strlen(currPtr);
                   printf("\"%s\",",currPtr);
                }
                printf("\t/* connectors */\n");
             }

             else if (j==11) {

                printf("   \"%s\",\t/* %s */\n",currPtr,c12msg[j]);
                tmpL = atol(currPtr);
                for (k=0; k < tmpL; k++) {
                   if (k==0) printf("   ");
                   currPtr = currPtr + tLen + 1;
                   tLen = strlen(currPtr);
                   printf("\"%s\",",currPtr);
                }
                printf("\t/* extensions */\n");
             }

             else {
                printf("   \"%s\",\t/* %s */\n",currPtr,c12msg[j]);
             }
             currPtr = currPtr + tLen + 1;
          }
          break;

       case 19:

          tmpL = atol(currPtr);
          printf("   \"%s\",\t/* number of prompts */\n\n",currPtr);
          tLen = strlen(currPtr);
          currPtr = currPtr + tLen + 1;
          while(tmpL) {

             long tmpL2, tmpL3;

             printf("   \"%s\", /* prompt */\n",currPtr);
             tLen = strlen(currPtr);
             currPtr = currPtr + tLen + 1;

             tmpL2 = atol(currPtr);
             tmpL3 = tmpL2;

             printf("   \"%s\", /* # of valid values */\n",currPtr);
             tLen = strlen(currPtr);
             currPtr = currPtr + tLen + 1;

             while (tmpL2) {
                if (tmpL2 == tmpL3) printf("   ");
                printf("\"%s\",",currPtr);
                tLen = strlen(currPtr);
                if (tmpL2 > 1 && tLen > 2) printf("\n   ");  // adds a \n<spX3> onto "Interrupt 10"... type prompts
                currPtr = currPtr + tLen + 1;
                tmpL2--;
             }
             printf(" /* valid values (max 20 chars ea) */\n");
             tmpL2 = tmpL3;  // just for flag use below

             while (tmpL3) {
                if (tmpL2 == tmpL3) printf("   ");
                printf("\"%s\",",currPtr);
                tLen = strlen(currPtr);
                currPtr = currPtr + tLen + 1;
                tmpL3--;
             }
             printf(" /* corresponding config.sys values */\n");

             printf("   \"%s\", /* index to default value (1-based)*/\n\n",currPtr);
             tLen = strlen(currPtr);
             currPtr = currPtr + tLen + 1;

             tmpL--;
          }

          break;

       default:
          printf("/* unhandled nameID * (#%d)/\n",rsPtr[i].nameID);
       }

       printf("END\n");
    }
 }
 else {
    printf("*** no rr type specified (use -h switch) \n");
 }

ExitNow:
 if (buffPtr) free(buffPtr);
 if (rsPtr) free(rsPtr);

 return rc;
}
