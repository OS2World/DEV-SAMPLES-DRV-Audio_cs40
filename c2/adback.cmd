@echo off
if !%1==! goto nogo
zip -9 -r ad9%1 * d:\dev\csrc\trace\*.c  -x *.smp -x *.err -x *.lb1 -x *.lib -x *.lk1  -x *.mk* -x *.DLL -x *.db? -x *.ix4 -x testdata.* -x *.tmp -x *.dmp -x INDEX -x *.OBJ -x *.EXE -x *.LIB -x *.BAK -x *.LST -x *.MAP -x *.BIN -x *.ZIP -x *.INF -x *.SYM
ECHO.
ECHO Used zip -9 -r ad9%1 * d:\dev\csrc\trace\*.c -x *.smp -x *.err -x *.lb1 -x *.lib -x *.lk1 -x *.mk* -x *.DLL  -x *.db? -x *.ix4 -x testdata.* -x *.tmp -x *.dmp -x INDEX -x *.OBJ -x *.EXE -x *.LIB -x *.BAK -x *.LST -x *.MAP -x *.BIN -x *.ZIP -x *.INF -x *.SYM
ECHO Copying to e:\CBU\ad\ad9%1.ZIP
copy ad9%1.ZIP e:\CBU\ad

ECHO Copying to \\box2\drivee\cbu\ad\ad9%1.ZIP
copy ad9%1.ZIP  \\box2\drivee\cbu\ad\ad9%1.ZIP

ECHO Copying to \\box3\drived\cbu\ad\ad9%1.ZIP
copy ad9%1.ZIP  \\box3\drived\cbu\ad\ad9%1.ZIP

dir ad9%1.ZIP
goto exit
:nogo
ECHO Date required is format MMDDv, as in
ECHO.
ECHO    adback 0117a
ECHO.
ECHO for ZIP file ad90117a.ZIP
:exit
