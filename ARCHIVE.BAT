@echo off
rem XVER: 123 or 123p1
set XVER=207
set RETAIL=1
goto Init

rem ** archive files ** (1%RETAIL% geq 1 ... with archive)
:archivelist
set ppcommon=PPLIB%arcbit%%tail%.DLL
if 1%RETAIL% geq 1 set ppc=PPC%tail%.EXE
if 1%RETAIL% geq 1 (set ppv=PPV%tail%.EXE) ELSE set ppv=
if 1%RETAIL% geq 1 (set ppb=PPB%tail%.EXE) ELSE set ppb=
if 1%RETAIL% geq 1 (set ppcust=PPCUST%tail%.EXE) ELSE set ppcust=
if %RETAIL% geq 1 (set pptray=PPTRAY%tail%.EXE) ELSE set pptray=
if %RETAIL% geq 1 (set ppffix=PPFFIX%tail%.EXE) ELSE set ppffix=

tfilesign sp %ppcommon% %ppcommon%
if not "%ppc%"=="" tfilesign sp %ppc% %ppc%
if not "%ppv%"=="" tfilesign sp %ppv% %ppv%
if not "%ppb%"=="" tfilesign sp %ppb% %ppb%
if not "%ppcust%"=="" tfilesign sp %ppcust% %ppcust%
if not "%pptray%"=="" tfilesign sp %pptray% %pptray%
if not "%ppffix%"=="" tfilesign sp %ppffix% %ppffix%
if not "%ppsetup%"=="" tfilesign sp %ppsetup% %ppsetup%

for %%i in (*.exe) do CT %readme% %%i
goto :EOF

rem *** Init ***
:Init
set DIR64=PPx64
if %RETAIL%==1 (set readme=PPX.TXT) ELSE set readme=PPX%XVER%.TXT
if %RETAIL%==1 (set etcfile=PPXF*.TXT) ELSE set etcfile=
if %RETAIL%==1 (set ppsetup=SETUP.EXE) ELSE set ppsetup=
if "%1"=="x64" goto :x64main
if "%1"=="arm64" goto :arm64main
if "%1"=="DW86" goto :DW86main
if "%1"=="DW64" goto :DW64main

WINCLOSE PaperPlaneCUI
WINCLOSE PaperPlaneCUIW
WINCLOSE PaperPlaneVUI
WINCLOSE PaperPlaneVUIW
pptray /c *closeppx
pptrayw /c *closeppx
perl ppxver.pl %XVER%
del /q *.zip 2> NUL
del /q *.map 2> NUL
del /q %DIR64%\*.* 2> NUL
del /q %DIR64%ARM\*.* 2> NUL
set arcbit=32
set arcpath=
set tail=W
set arcx=ppw
call :x86main
set tail=
set arcx=ppx
call :x86main
if %RETAIL%==0 goto :x64call

rem *** Source Archive ***
set help=PPX.CNT PPX.HLP
del PPXDEF.CFG 2> NUL
for %%i in (*) do CT %readme% %%i
for /R %%i in (*.sln) do CT %readme% "%%i"
for /R %%i in (*.vc*) do CT %readme% "%%i"
for %%i in (resource\*) do CT %readme% %%i

ppb /c %%u/7-zip32.dll,a ppx%XVER%s.7z -hide -mx=9 PPXSRC.TXT MAKE* GNUmake* *.BAT *.C *.CPP *.H *.HPJ index.html *.pl *.RC *.RH X*.TXT PPX_TEC.TXT PPX_REC.TXT *.xlsx resource\* VS2008\PPx.sln VS2008\PPb\*.vcproj VS2008\PPc\*.vcproj VS2008\PPcommon\*.vcproj VS2008\PPcust\*.vcproj VS2008\PPffix\*.vcproj VS2008\PPtray\*.vcproj VS2008\PPv\*.vcproj VS2008\SETUP\*.vcproj VS2019\PPx.sln VS2019\PPb\*.vcxproj VS2019\PPc\*.vcxproj VS2019\PPcommon\*.vcxproj VS2019\PPcust\*.vcxproj VS2019\PPffix\*.vcxproj VS2019\PPtray\*.vcxproj VS2019\PPv\*.vcxproj VS2019\SETUP\*.vcxproj
tfilesign sf ppx%XVER%s.7z ppx%XVER%s.7z
CT %readme% ppx%XVER%s.7z

ppb /c %%u/7-zip32.dll,a -tzip -hide -mx=7 ppxhelp.zip %help% ppxcmd.txt
tfilesign s ppxhelp.zip ppxhelp.zip
CT %readme% ppxhelp.zip

rem *** x64 ***
:x64call
if not exist %DIR64% md %DIR64%
if not exist %DIR64%\resource md %DIR64%\resource
del /q %DIR64%\*.* 2> NUL
copy *.* %DIR64% > NUL
del /q %DIR64%\*.map 2> NUL
copy resource\*.* %DIR64%\resource > NUL
cmd /c archive.bat x64 %DIR64% %readme%
ppb /c %%u/7-zip32.dll,a -tzip -hide -mx=7 -r ppx%XVER%map.zip *.map %DIR64%\*.map

rem *** DirectWrite ***
cmd /c archive.bat DW86 %DIR64% %readme%
cmd /c archive.bat DW64 %DIR64% %readme%
ppb /c %%u/7-zip32.dll,a -tzip -hide -mx=7 ppx%XVER%map.zip mapDW32\* mapDW64\*
rd /s /q mapDW32
rd /s /q mapDW64
if %RETAIL%==0 goto :final

rem *** arm64 ***
if not exist %DIR64%ARM md %DIR64%ARM
if not exist %DIR64%ARM\resource md %DIR64%ARM\resource
del /q %DIR64%ARM\*.* 2> NUL
copy *.* %DIR64%ARM > NUL
del /q %DIR64%ARM\*.map 2> NUL
copy resource\*.* %DIR64%ARM\resource > NUL
cmd /c archive.bat arm64 %DIR64%ARM %readme%
ppb /c %%u/7-zip32.dll,a -tzip -hide -mx=7 -r ppx%XVER%map.zip %DIR64%ARM\*.map

rem *** final ***
:final
perl final.pl
goto :EOF

rem *** x86 Multibyte / UNICODE main (BCC 5.02) ***
:x86main
del /q *.obj 2> NUL
del /q *.res 2> NUL
rem make / make -DUseUnicode=1 B makeflit ‚ÍƒƒO‚ÌŠÈ—ª‰»‚Ì‚½‚ßÈ—ª‰Â”\
call make%tail% -DRELEASE | perl makeflit.pl
call :archive
goto :EOF

rem *** x64 main (VS2008 ‚Ì vcvarsall.bat x64) ***
:x64main
CD %2
call VC64.BAT
copy ..\64FILES\*.* .
del *.obj 2> NUL
del *.zip 2> NUL
make -DX64=1 -DRELEASE
for %%i in (*.dll) do CT %3 %%i
for %%i in (*.exe) do CT %3 %%i
del ..\ppx64%XVER%.zip 2> NUL
set arcpath=..\
set arcx=ppx64
set arcbit=64
set tail=W
if %RETAIL%==1 set etcfile=%etcfile% iftwic.sph UNBYPASS.DLL UNBYPASS.EXE
call :archive
del *.c 2> NUL
del *.obj 2> NUL
goto :EOF

rem *** arm64 main (VS2022 ‚Ì vcvarsall.bat x64_arm64) ***
:arm64main
CD %2
call VCARM64.bat
copy ..\64FILESARM\*.* .
del *.obj 2> NUL
del *.zip 2> NUL
make -DARM -DX64=1 -DRELEASE -DUSEDIRECTX -DUSEDIRECTWRITE
for %%i in (*.dll) do CT %3 %%i
for %%i in (*.exe) do CT %3 %%i
del ..\ppxarm64%XVER%.zip 2> NUL
set arcpath=..\
set arcx=ppxarm64
set arcbit=64
set tail=W
if %RETAIL%==1 set etcfile=%etcfile% iftwic.spha UNBYPASS.DLL UNBYPASS.EXE PPXTEXT64A.DLL PPXSCR64A.DLL
call :archive
del *.c 2> NUL
del *.obj 2> NUL
goto :EOF

rem *** DirectWrite main (VS2008 ‚Ì vcvarsall.bat x84 / x64 + SDK7.0) ***
:DW86main
set DESTDIR=PPxDW
if not exist %DESTDIR% md %DESTDIR%
del /q %DESTDIR%\* 2> NUL
if not exist mapDW32 md mapDW32
set arcname=%XVER%
del *.obj 2> NUL
del *.map 2> NUL

cmd /c "VC322008.BAT > nul && make -DUseUnicode=1 -DUseBCC=0 -DUSEDIRECTX -DUSEDIRECTWRITE -DRELEASE
copy PPLIB32W.DLL %DESTDIR%\
if %RETAIL%==1 copy 32FILES\*.spi %DESTDIR%\
copy *.map mapDW32
goto :DWmain

:DW64main
set DESTDIR=PPxDW64
if not exist %DESTDIR% md %DESTDIR%
del /q %DESTDIR%\* 2> NUL
if not exist mapDW64 md mapDW64
set arcname=64%XVER%
del *.obj 2> NUL
del *.map 2> NUL

cmd /c "VC642008.BAT > nul && make -DX64 -DVCTOOLKIT=0 -DUSEDIRECTX -DUSEDIRECTWRITE -DRELEASE
copy PPLIB64W.DLL %DESTDIR%\
if %RETAIL%==1 copy 64FILES\*.sph %DESTDIR%\
copy *.map mapDW64

:DWmain
copy %readme% %DESTDIR%\
set tail=W
call :archivelist
if %RETAIL%==1 copy PPXDW.TXT  %DESTDIR%\
if %RETAIL%==1 copy PPXF*.TXT  %DESTDIR%\
if %RETAIL%==1 copy SETUP.EXE  %DESTDIR%\
if not "%ppb%"=="" copy %ppb%  %DESTDIR%\
if not "%ppc%"=="" copy %ppc%  %DESTDIR%\
if not "%ppv%"=="" copy %ppv%  %DESTDIR%\
if not "%ppcust%"=="" copy %ppcust% %DESTDIR%\
if not "%ppffix%"=="" copy %ppffix% %DESTDIR%\
if not "%pptray%"=="" copy %pptray% %DESTDIR%\

cd %DESTDIR%
for %%i in (*.dll) do tfilesign sp %%i %%i
for %%i in (*.exe) do tfilesign sp %%i %%i
for %%i in (*.dll) do CT %readme% %%i
for %%i in (*.exe) do CT %readme% %%i
ppb /c %%u/7-zip32.dll,a -tzip -hide -mx=7 ..\ppxdw%arcname%.zip -r- *

cd ..
tfilesign s ppxdw%arcname%.zip ppxdw%arcname%.zip
CT %readme% ppxdw%arcname%.zip
goto :EOF

rem ** archive **
:archive
call :archivelist
if %RETAIL%==1 for %%i in (*.html) do CT %readme% %%i
if %RETAIL%==1 for %%i in (PPXF*.TXT) do CT %readme% %%i
CT %readme% PPX.CNT
CT %readme% PPX.HLP
if %RETAIL%==1 CT %readme% ppxcmd.txt
CT %readme% %ppcommon%

del %arcpath%%arcx%%XVER%.zip 2> NUL
ppb /c %%u/7-zip32.dll,a -tzip -hide -mx=7 %arcpath%%arcx%%XVER%.zip %readme% %ppsetup% %ppc% %ppv% %ppb% %ppcust% %pptray% %ppffix% %ppcommon% %etcfile%
tfilesign s %arcpath%%arcx%%XVER%.zip %arcpath%%arcx%%XVER%.zip
CT %readme% %arcpath%%arcx%%XVER%.zip
goto :EOF
