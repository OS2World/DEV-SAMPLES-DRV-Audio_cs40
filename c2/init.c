//
// init.c
// 3-Feb-99
//
// USHORT stratMode2Init(REQPACK __far *rpPtr);
//
// will make call into slam basedev to get resources (won't use RM... but have to
// write a slam, with IDC hook, probably use assembly like wacker...?)

#include "cs40.h"

// if these are put in initdata they'll increase file size by 8K

const CHAR   signOnMsg[] = "\r\nCS423x & OPL3SAx audio driver [CS40:990929]\r\n";
const CHAR      irqMsg[] = "     IRQ=  ";
const CHAR basePortMsg[] = "basePort=0x";
const CHAR ctrlPortMsg[] = "ctrlPort=0x";
const CHAR     modeMsg[] = "    mode=  ";
const CHAR   deviceMsg[] = "  device=  ";
const CHAR  playDMAMsg[] = "play DMA=  ";
const CHAR playSizeMsg[] = "    size=0x";
const CHAR  playIPBMsg[] = "     IPB=  ";
const CHAR playModeMsg[] = "    mode=  ";
const CHAR   recDMAMsg[] = " rec DMA=  ";
const CHAR  recSizeMsg[] = "    size=0x";
const CHAR   recIPBMsg[] = "     IPB=  ";
const CHAR  recModeMsg[] = "    mode=  ";
const CHAR  clFlagsMsg[] = "   flags=0x";

const CHAR  errorPrepMsg[] = "CS40.SYS cmdline parse error: ";
const CHAR *errorMsg[] = {"--X--\r\n",
                          "IRQ (-i:n)\r\n",
                          "Base port (-bp:x)\r\n",
                          "Control port (-cp:x)\r\n",
                          "Mode (-xm:n)\r\n",
                          "Device (-dev:n)\r\n",
                          "Play DMA (-dp:n)\r\n",
                          "Play DMA size (-dps:x)\r\n",
                          "Play DMA IPB (-dpi:n)\r\n",
                          "Play DMA mode (-dpm:n)\r\n",
                          "Rec DMA (-dr:n)\r\n",
                          "Rec DMA size (-drs:x)\r\n",
                          "Rec DMA IPB (-dri:n)\r\n",
                          "Rec DMA mode (-drm:n)\r\n",
                          "Flags (various)\r\n",
                          "Unknown (?)\r\n"};

const CHAR *deviceStr[] = {" (CS423x)",
                           " (1?)",
                           " (2?)",
                           " (OPL3SA3)"};

const CHAR  initExitMsg[] = "CS40.SYS install error, rc=";

#pragma code_seg ("_INITTEXT");
#pragma data_seg ("_INITDATA","ENDDS");

#pragma intrinsic (strlen,strcat);
#include "d:\w10a\h\string.h"   // use explicit else uses ddk, etc.

extern USHORT endCode;
extern USHORT endData;

USHORT ParseCL(UCHAR __far *clPtr);
char __far * __cdecl _itoa (int val,char __far *buf,int radix);
char __far * __cdecl _ltoa (long val,char __far *buf,int radix);
char __far * __cdecl _ultoa (unsigned long val,char __far *buf,int radix);
long __cdecl strtol (const char __far *nptr,char __far * __far *endptr,int ibase);
unsigned long __cdecl strtoul (const char __far *nptr,char __far * __far *endptr,int ibase);

// -------------------------------------
// in:
//out:
//nts:

USHORT stratMode2Init(REQPACK __far *rpPtr) {

 USHORT rc = 0;
 USHORT heapSize;
 USHORT flags = 0;
 CHAR __far *errPtr = 0;
 CHAR valueMsg[64];

 Device_Help = rpPtr->init.Device_Help;

 rpPtr->status = RPDONE | RPERR | RPGENFAIL;
 rpPtr->init.sizeCS = 0;
 rpPtr->init.sizeDS = 0;

 heapSize = HeapInit(8192);

 // do any log setup here...

 rc = ParseCL(rpPtr->init.argsPtr);
 if (rc) goto ExitRightNow;

 errPtr = "CS40: Installed\r\n";

 // resource manager access here (create, detect PnP, etc.) here...

 // had created IRQ object for timer here...
 // had setup more hardware types here (FMSYNTH or MPU...)

 flags = 0;
 if (gDMAplay != gDMArec) flags = flags | FLAGS_WAVEAUDIO_FULLDUPLEX;
 if (gDMAplay >= 4)       flags = flags | FLAGS_WAVEAUDIO_DMA16;
 if (gDMAplayMode != 0)   flags = flags | FLAGS_WAVEAUDIO_FTYPEDMA;

 rc = waveplayInit(gDMAplay, flags, gIRQ);

 if (rc) {
if (gCLflags & FLAGS_CL_DDPRINT) ddprintf("waveplayInit() failed, rc=%u\n");
    goto ExitNow;
 }

 flags = 0;
 if (gDMAplay != gDMArec) flags = flags | FLAGS_WAVEAUDIO_FULLDUPLEX;
 if (gDMArec >= 4)        flags = flags | FLAGS_WAVEAUDIO_DMA16;
 if (gDMArecMode != 0)    flags = flags | FLAGS_WAVEAUDIO_FTYPEDMA;

 rc = waverecInit(gDMArec, flags, gIRQ);

 if (rc) {
    errPtr = "waverecInit() failed";
if (gCLflags & FLAGS_CL_DDPRINT) ddprintf("waverecInit() failed, rc=%u\n");
    goto ExitNow;
 }

 rc = chipsetInit();
 if (rc) goto ExitNow;

 if (gCLflags & FLAGS_CL_SETDTM) chipsetSetDTM(gCLflags & FLAGS_CL_SETDTM);

 // had init'ed mixer here...
 //InitMixer();

 // had done VDD setup here...
 // fill in the ADAPTERINFO
 //codec_info.ulNumPorts = NUMIORANGES;
 //codec_info.Range[0].ulPort  =  pResourcesWSS->uIOBase[0];
 //codec_info.Range[0].ulRange =  pResourcesWSS->uIOLength[0];
 //...
 //// set up the addressing to the codec data for the vdd
 //pfcodec_info = (ADAPTERINFO __far *)&codec_info;
 //DevHelp_VirtToLin (SELECTOROF(pfcodec_info), (ULONG)(OFFSETOF(pfcodec_info)),(PLIN)&pLincodec);
 // copy the pdd name out of the header.
 //for (i = 0; i < sizeof(szPddName)-1 ; i++) {
 //   if (phdr->abName[i] <= ' ')
 //      break;
 //   szPddName[i] = phdr->abName[i];
 //}
 //// register the VDD IDC entry point..
 //DevHelp_RegisterPDD ((NPSZ)szPddName, (PFN)IDCEntry_VDD);

 if (gCLflags & FLAGS_CL_TESTONLY) rc = 666;

 if (rc) goto ExitNow;

 rpPtr->status = RPDONE;
 rpPtr->init.sizeCS = (USHORT)&endCode;
 rpPtr->init.sizeDS = (USHORT)&endData;

if (gCLflags & FLAGS_CL_DDPRINT) ddprintf("stratMode2Init:endCode=%x, endData=%x, ds=%x\n",rpPtr->init.sizeCS,rpPtr->init.sizeDS,_DS());

ExitNow:

 if (rc && (gCLflags & FLAGS_CL_QUIET) == 0) {
    DosPutMessage(STDOUT,_fstrlen(initExitMsg),initExitMsg);
    _ultoa(rc,valueMsg,10);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);
 }

ExitRightNow:
 return rc;
}

// ------------------------------------------------------
//USHORT gIRQ          = 99;       // 1  -i:5
//USHORT gBasePort     = 0;        // 2  -bp:534
//USHORT gCtrlPort     = 0;        // 3  -cp:390
//USHORT gMode         = 2;        // 4  -xm:2
//USHORT gDevice       = 99;       // 5  -dev:3       0=CS 3=OPL3SA3
//USHORT gDMAplay      = 99;       // 6  -dp:1
//USHORT gDMAplaySize  = 0x8000;   // 7  -dps:8000    DMA buffer size (max is 60KB)
//USHORT gDMAplayIPB   = 2;        // 8  -dpi:2       ints per buffer -- power of 2 (2,4,8,16,32)
//USHORT gDMAplayMode  = 1;        // 9  -dpm:1       1=typeF DMA, else not
//USHORT gDMArec       = 99;       //10  -dr:0        can be same -dp: (then is not full duplex)
//USHORT gDMArecSize   = 0x8000;   //11  -drs:8000    DMA buffer size (max is 60KB)
//USHORT gDMArecIPB    = 2;        //12  -dri:2       ints per buffer -- power of 2 (2,4,8,16,32)
//USHORT gDMArecMode   = 1;        //13  -drm:1       1=typeF DMA, else not
//USHORT gCLflags      = 0;        //14  various flags (-v bit0=1, ...)
//                                 //15 none of the above
//#define FLAGS_CL_VERBOSE  1

USHORT ParseCL(UCHAR __far *clPtr) {

 USHORT rc = 0;
 ULONG tVal;
 UCHAR __far *tPtr = clPtr;
 UCHAR __far *ePtr = 0;
 UCHAR __far *argListPtr = 0;

 UCHAR sw0, sw1, sw2, sw3;
 CHAR valueMsg[64];

 // cs40.sys -
 // skip over filename

 while (*tPtr && *tPtr != ' ') {
    tPtr++;
 }

 while (*tPtr && (rc == 0)) {

    tPtr++;            // skip to next

    if (*tPtr == '-') {

       if (argListPtr == 0) argListPtr = tPtr;

       tPtr++;          // after - switch
       sw0 = *(tPtr+0); // first (eg, d)
       sw1 = *(tPtr+1); // next char, eg, ':'
       sw2 = *(tPtr+2); // ...
       sw3 = *(tPtr+3); // ... (if used can only be :)

       switch(sw0) {
       case 'i':
          rc = 1;
          if (sw1 == ':') {
             tVal = strtoul((tPtr+2),&ePtr,10);
             switch(tVal) {
             case 3:
             case 5:
             case 7:
             case 9:
             case 10:
             case 11:
             case 12: // i12 not for OPL3SA3 though
                gIRQ = tVal;
                rc = 0;
                break;
             }
             tPtr = ePtr;
          }
          break;

       case 'b':
          rc = 2;
          if (sw1 == 'p' && sw2 == ':') {
int3();
             tVal = strtoul((tPtr+3),&ePtr,16);
             if (tVal >= 0x100 && tVal <= 0xFFF) {
                gBasePort = tVal;
                rc = 0;
             }
             tPtr = ePtr;
          }
          break;

       case 'c':
          rc = 3;
          if (sw1 == 'p' && sw2 == ':') {
             tVal = strtoul((tPtr+3),&ePtr,16);
             if (tVal >= 0x100 && tVal <= 0xFFF) {
                gCtrlPort = tVal;
                rc = 0;
             }
             tPtr = ePtr;
          }
          break;

       case 'x':
          rc = 4;
          if (sw1 == 'm' && sw2 == ':') {
             // only know mode2
             rc = 0;
          }
          break;

       case 'd':
          rc = 5;
          switch(sw1) {
          case 'e':                                    // dev:
             if (sw2 == 'v' && sw3 == ':') {
                tVal = strtoul((tPtr+4),&ePtr,10);
                if (tVal == 0 || tVal == 3) {
                   gDevice = tVal;
                   rc = 0;
                }
                tPtr = ePtr;
             }
             break;

          case 'p':                                    // dp*:
             rc = 6;
             switch(sw2) {
             case ':':                                 // dp:
                tVal = strtoul((tPtr+3),&ePtr,10);
                switch(tVal) {
                case 0:
                case 1:
                case 3:
                   gDMAplay = tVal;
                   rc = 0;
                }
                tPtr = ePtr;
                break; // case dp:

             case 's':
                rc = 7;
                if (sw3 == ':') {
                   tVal = strtoul((tPtr+4),&ePtr,16);
                   if ((tVal & 0xFFF) == 0) {  // align 4K
                      gDMAplaySize = tVal;
                      rc = 0;
                   }
                   tPtr = ePtr;
                }
                break; // case dps

             case 'i':
                rc = 8;
                if (sw3 == ':') {
                   tVal = strtoul((tPtr+4),&ePtr,10);
                   switch(tVal) {
                   case 2:
                   case 4:
                   case 8:
                   case 16:
                   case 32:
                      gDMAplayIPB = tVal;
                      rc = 0;
                   }
                   tPtr = ePtr;
                }
                break; // case dpi

             case 'm':
                rc = 9;
                if (sw3 == ':') {
                   tVal = strtoul((tPtr+4),&ePtr,10);
                   if (tVal == 0 || tVal == 1) {
                      gDMAplayMode = tVal;
                      rc = 0;
                   }
                   tPtr = ePtr;
                }
                break;  // case dpm
             }
             break;  // case dp

          case 'r':                                    // dr*:
             rc = 10;
             switch(sw2) {
             case ':':                                 // dp:
                tVal = strtoul((tPtr+3),&ePtr,10);
                switch(tVal) {
                case 0:
                case 1:
                case 3:
                   gDMArec = tVal;
                   rc = 0;
                }
                tPtr = ePtr;
                break; // case dr:

             case 's':
                rc = 11;
                if (sw3 == ':') {
                   tVal = strtoul((tPtr+4),&ePtr,16);
                   if ((tVal & 0xFFF) == 0) {  // align 4K
                      gDMArecSize = tVal;
                      rc = 0;
                   }
                   tPtr = ePtr;
                }
                break; // case drs

             case 'i':
                rc = 12;
                if (sw3 == ':') {
                   tVal = strtoul((tPtr+4),&ePtr,10);
                   switch(tVal) {
                   case 2:
                   case 4:
                   case 8:
                   case 16:
                   case 32:
                      gDMArecIPB = tVal;
                      rc = 0;
                   }
                   tPtr = ePtr;
                }
                break; // case dri

             case 'm':
                rc = 13;
                if (sw3 == ':') {
                   tVal = strtoul((tPtr+4),&ePtr,10);
                   if (tVal == 0 || tVal == 1) {
                      gDMArecMode = tVal;
                      rc = 0;
                   }
                   tPtr = ePtr;
                }
                break; // case drm
                //
             }
             break; // case 'dr'
          }
          break;  // case 'd'

       case 'v':
          rc = 30;
          if (sw1 == ' ' || sw1 == 0) {
             gCLflags = gCLflags | FLAGS_CL_VERBOSE;
             rc = 0;
          }
          break;

       case 'z':
          rc = 30;
          switch(sw1) {
          case '1':
             gCLflags = gCLflags | FLAGS_CL_SETDTM;
             rc = 0;
             break;
          case '2':
             gCLflags = gCLflags | FLAGS_CL_SETACAL;
             rc = 0;
             break;

          case 'e':
             gCLflags = gCLflags | FLAGS_CL_DDPRINT;
             rc = 0;
             break;
          }
          break;

       case 'Q':
          rc = 30;
          if (sw1 == ' ' || sw1 == 0) {
             gCLflags = gCLflags | FLAGS_CL_QUIET;
             rc = 0;
          }
          break;

       case 't':
          rc = 30;
          if (sw1 == ' ' || sw1 == 0) {
             gCLflags = gCLflags | FLAGS_CL_TESTONLY;
             rc = 0;
          }
          break;

       default:
          rc = 31;
       }

    } // if *tPtr = '-'
 }    // while (*tPtr && (rc == 0)) {

 if ((gCLflags & FLAGS_CL_QUIET) == 0) {
    DosPutMessage(STDOUT,_fstrlen(signOnMsg),signOnMsg);
 }

 if (gCLflags & FLAGS_CL_VERBOSE) {

    if (argListPtr) {
       _fstrcpy(valueMsg,"device=cs40.sys ");
       DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);
       DosPutMessage(STDOUT,_fstrlen(argListPtr),argListPtr);
       _fstrcpy(valueMsg,"\r\n");
       DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);
    }

    DosPutMessage(STDOUT,_fstrlen(irqMsg),irqMsg);
    if (gIRQ == 99) {
       _fstrcpy(valueMsg," *not set*\r\n");
    }
    else {
       valueMsg[0] = 0;
       _ultoa(gIRQ,valueMsg,10);
       _fstrcat(valueMsg,"\r\n");
    }
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(basePortMsg),basePortMsg);
    if (gBasePort == 99) {
       _fstrcpy(valueMsg," *not set*\r\n");
    }
    else {
       valueMsg[0] = 0;
       _ultoa(gBasePort,valueMsg,16);
       _fstrcat(valueMsg,"\r\n");
    }
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(ctrlPortMsg),ctrlPortMsg);
    if (gCtrlPort == 99) {
       _fstrcpy(valueMsg," *not set*\r\n");
    }
    else {
       valueMsg[0] = 0;
       _ultoa(gCtrlPort,valueMsg,16);
       _fstrcat(valueMsg,"\r\n");
    }
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(modeMsg),modeMsg);
    valueMsg[0] = 0;
    _ultoa(gMode,valueMsg,10);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(deviceMsg),deviceMsg);
    if (gDevice > 3) gDevice = 99;
    if (gDevice == 99) {
       _fstrcpy(valueMsg," *not set*\r\n");
    }
    else {
       valueMsg[0] = 0;
       _ultoa(gDevice,valueMsg,10);
       _fstrcat(valueMsg,deviceStr[gDevice]);
       _fstrcat(valueMsg,"\r\n");
    }
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(playDMAMsg),playDMAMsg);
    if (gDMAplay == 99) {
       _fstrcpy(valueMsg," *not set*\r\n");
    }
    else {
       valueMsg[0] = 0;
       _ultoa(gDMAplay,valueMsg,10);
       _fstrcat(valueMsg,"\r\n");
    }
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(playSizeMsg),playSizeMsg);
    valueMsg[0] = 0;
    _ultoa(gDMAplaySize,valueMsg,16);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(playIPBMsg),playIPBMsg);
    valueMsg[0] = 0;
    _ultoa(gDMAplayIPB,valueMsg,10);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(playModeMsg),playModeMsg);
    valueMsg[0] = 0;
    _ultoa(gDMAplayMode,valueMsg,10);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(recDMAMsg),recDMAMsg);
    if (gDMArec == 99) {
       _fstrcpy(valueMsg," *not set*\r\n");
    }
    else {
       valueMsg[0] = 0;
       _ultoa(gDMArec,valueMsg,10);
       _fstrcat(valueMsg,"\r\n");
    }
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(recSizeMsg),recSizeMsg);
    valueMsg[0] = 0;
    _ultoa(gDMArecSize,valueMsg,16);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(recIPBMsg),recIPBMsg);
    valueMsg[0] = 0;
    _ultoa(gDMArecIPB,valueMsg,10);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(recModeMsg),recModeMsg);
    valueMsg[0] = 0;
    _ultoa(gDMArecMode,valueMsg,10);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

    DosPutMessage(STDOUT,_fstrlen(clFlagsMsg),clFlagsMsg);
    valueMsg[0] = 0;
    _ultoa(gCLflags,valueMsg,16);
    _fstrcat(valueMsg,"\r\n");
    DosPutMessage(STDOUT,_fstrlen(valueMsg),valueMsg);

 }

 if (gIRQ == 99)           rc = 1;
 else if (gBasePort == 99) rc = 2;
 else if (gCtrlPort == 99) rc = 3;
 else if (gDevice == 99)   rc = 5;
 else if (gDMAplay == 99)  rc = 6;
 else if (gDMArec == 99)   rc = 10;
 // rest are optional

 if (rc) {
    USHORT trc = rc;
    if (trc > 14) trc = 15;
    DosPutMessage(STDOUT,_fstrlen(errorPrepMsg),errorPrepMsg);
    DosPutMessage(STDOUT,_fstrlen(errorMsg[trc]),errorMsg[trc]);
 }

 return rc;

}

//*******************************************************************************
//*strtol, strtoul(nptr,endptr,ibase) - Convert ascii string to long un/signed int
//
//*       Convert an ascii string to a long 32-bit value.  The base
//*       used for the caculations is supplied by the caller.  The base
//*       must be in the range 0, 2-36.  If a base of 0 is supplied, the
//*       ascii string must be examined to determine the base of the
//*       number:
//*               (a) First char = '0', second char = 'x' or 'X', use base 16
//*               (b) First char = '0', use base 8
//*               (c) First char in range '1' - '9', use base 10
//*
//*       If the 'endptr' value is non-NULL, then strtol/strtoul places
//*       a pointer to the terminating character in this value
//*
//*Entry:
//*       nptr == NEAR/FAR pointer to the start of string.
//*       endptr == NEAR/FAR pointer to the end of the string.
//*       ibase == integer base to use for the calculations.
//*
//*       string format: [whitespace] [sign] [0] [x] [digits/letters]
//*
//*Exit:
//*       Good return:
//*               result
//*
//*       Overflow return:
//*               strtol -- LONG_MAX or LONG_MIN
//*               strtoul -- ULONG_MAX
//*               strtol/strtoul -- errno == ERANGE
//*
//*       No digits or bad base return:
//*               0
//*               endptr = nptr*
//*******************************************************************************

/* flag values */
#define FL_UNSIGNED   1       /* strtoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

#define ULONG_MAX 0xFFFFFFFF
#define LONG_MAX  0x7FFFFFFF
#define LONG_MIN  0x80000000

static unsigned long __cdecl strtoxl (const char __far *nptr,const char __far * __far *endptr,int ibase,int flags) {

 const char __far *p;
 char c;
 unsigned long number;
 unsigned digval;
 unsigned long maxval;

 p = nptr;                       /* p is our scanning pointer */
 number = 0;                     /* start with zero */

 c = *p++;                       /* read char */
 while ((unsigned char)c == ' ')
         c = *p++;               /* skip whitespace */

 if (c == '-') {
         flags |= FL_NEG;        /* remember minus sign */
         c = *p++;
 }
 else if (c == '+')
         c = *p++;               /* skip sign */

 if (ibase < 0 || ibase == 1 || ibase > 36) {
         /* bad base! */
         if (endptr)
                 /* store beginning of string in endptr */
                 *endptr = nptr;
         return 0L;              /* return 0 */
 }
 else if (ibase == 0) {
         /* determine base free-lance, based on first two chars of
            string */
         if (c != '0')
                 ibase = 10;
         else if (*p == 'x' || *p == 'X')
                 ibase = 16;
         else
                 ibase = 8;
 }

 if (ibase == 16) {
         /* we might have 0x in front of number; remove if there */
         if (c == '0' && (*p == 'x' || *p == 'X')) {
                 ++p;
                 c = *p++;       /* advance past prefix */
         }
 }

 /* if our number exceeds this, we will overflow on multiply */
 maxval = ULONG_MAX / ibase;


 for (;;) {      /* exit in middle of loop */
         /* convert c to value */
         //if ( isdigit((int)(unsigned char)c) )
         //        digval = c - '0';
         //else if ( isalpha((int)(unsigned char)c) )
         //        digval = toupper(c) - 'A' + 10;
         //else
         //        break;

         if (c >= '0' && c <= '9') {
            digval = c - '0';
         }
         else if (c >= 'a' && c <= 'z') {
            digval = c - 'a' + 10;
         }
         else if (c >= 'A' && c <= 'Z') {
            digval = c - 'A' + 10;
         }
         else {
            break;
         }

         if (digval >= (unsigned)ibase) break;          /* exit loop if bad digit found */

         /* record the fact we have read one digit */
         flags |= FL_READDIGIT;

         /* we now need to compute number = number * base + digval,
            but we need to know if overflow occured.  This requires
            a tricky pre-check. */

         if (number < maxval || (number == maxval &&
         (unsigned long)digval <= ULONG_MAX % ibase)) {
                 /* we won't overflow, go ahead and multiply */
                 number = number * ibase + digval;
         }
         else {
                 /* we would have overflowed -- set the overflow flag */
                 flags |= FL_OVERFLOW;
         }

         c = *p++;               /* read next digit */
 }

 --p;                            /* point to place that stopped scan */

 if (!(flags & FL_READDIGIT)) {
         /* no number there; return 0 and point to beginning of string */
         if (endptr)
                 /* store beginning of string in endptr later on */
                 p = nptr;
         number = 0L;            /* return 0 */
 }
 else if ( (flags & FL_OVERFLOW) ||
           ( !(flags & FL_UNSIGNED) &&
             ( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
               ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
 {
         /* overflow or signed overflow occurred */
         //chh removederrno = ERANGE;
         if ( flags & FL_UNSIGNED )
                 number = ULONG_MAX;
         else if ( flags & FL_NEG )
                 number = (unsigned long)(-LONG_MIN);
         else
                 number = LONG_MAX;
 }

 if (endptr != NULL) *endptr = p;
 if (flags & FL_NEG) number = (unsigned long)(-(long)number);

 return number;     
}

long __cdecl strtol (const char __far *nptr,char __far * __far *endptr,int ibase) {
 return (long) strtoxl(nptr, endptr, ibase, 0);
}

unsigned long __cdecl strtoul (const char __far *nptr,char __far * __far *endptr,int ibase) {
 return strtoxl(nptr, endptr, ibase, FL_UNSIGNED);
}


//*******************************************************************************
//*char *_itoa, *_ltoa, *_ultoa(val, buf, radix) - convert int to ASCII string
//*Entry:
//*       val - number to be converted (int, long or unsigned long)
//*       int radix - base to convert into
//*       char *buf - ptr to buffer to place result
//*Exit:
//*       fills in space pointed to by buf with string result
//*       returns a pointer to this buffer
//*******************************************************************************

/* helper routine that does the main job. */

static void __cdecl xtoa (unsigned long val,char __far *buf,unsigned radix,int is_neg) {

 char __far *p;                /* pointer to traverse string */
 char __far *firstdig;         /* pointer to first digit */
 char temp;              /* temp char */
 unsigned digval;        /* value of digit */

 p = buf;

 if (is_neg) {
     /* negative, so output '-' and negate */
     *p++ = '-';
     val = (unsigned long)(-(long)val);
 }

 firstdig = p;           /* save pointer to first digit */

 do {
     digval = (unsigned) (val % radix);
     val /= radix;       /* get next digit */

     /* convert to ascii and store */
     if (digval > 9)
         *p++ = (char) (digval - 10 + 'a');  /* a letter */
     else
         *p++ = (char) (digval + '0');       /* a digit */
 } while (val > 0);

 /* We now have the digit of the number in the buffer, but in reverse
    order.  Thus we reverse them now. */

 *p-- = '\0';            /* terminate string; p points to last digit */

 do {
     temp = *p;
     *p = *firstdig;
     *firstdig = temp;   /* swap *p and *firstdig */
     --p;
     ++firstdig;         /* advance to next two digits */
 } while (firstdig < p); /* repeat until halfway */
}

/* Actual functions just call conversion helper with neg flag set correctly,
   and return pointer to buffer. */

char __far * __cdecl _itoa (int val,char __far *buf,int radix) {
 if (radix == 10 && val < 0)
    xtoa((unsigned long)val, buf, radix, 1);
 else
    xtoa((unsigned long)(unsigned int)val, buf, radix, 0);
 return buf;
}

char __far * __cdecl _ltoa (long val,char __far *buf,int radix) {
 xtoa((unsigned long)val, buf, radix, (radix == 10 && val < 0));
 return buf;
}

char __far * __cdecl _ultoa (unsigned long val,char __far *buf,int radix) {
 xtoa(val, buf, radix, 0);
 return buf;
}


