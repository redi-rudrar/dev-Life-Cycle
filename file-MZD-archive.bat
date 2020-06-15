echo off

set RUNFOLDER=%cd%
set FolderToArchive=%1
set filename=%2
set ARCHIVEDAYS=%3
set ArchiveFolder=%RUNFOLDER%\Archive\%FolderToArchive%
set LogStart=ViewServer:%d%:%t%:

echo %LogStart%Checking folders to archive
echo %LogStart%------------------------------------------------------------

IF not EXIST "%RUNFOLDER%\Archive\" mkdir  "%RUNFOLDER%\Archive"
IF not EXIST "%ArchiveFolder%"  mkdir "%ArchiveFolder%"

echo %LogStart%Checking folders to DONE
echo %LogStart%------------------------------------------------------------


echo %LogStart%Deleting archives .zip files
echo %LogStart%------------------------------------------------------------

if EXIST %ArchiveFolder%\*.zip  ( ForFiles /p "%ArchiveFolder%" /s /d -%ARCHIVEDAYS% /c "cmd /c del @file" )

echo %LogStart%Deleting DONE
echo %LogStart%------------------------------------------------------------

pause

echo %LogStart%Move below files  to %ArchiveFolder% folder
echo %LogStart%------------------------------------------------------------

copy /y "%RUNFOLDER%\data\*.FIX, %RUNFOLDER%\data\*.ZMAP" "%ArchiveLogsFolder%"

for /f "tokens=* delims=" %%i in (%filename%) do copy /y "%RUNFOLDER%\%FolderToArchive%\%%i" "%ArchiveFolder%" 

echo %LogStart%------------------------------------------------------------
echo %LogStart%START ZIPPING
echo %LogStart%------------------------------------------------------------

ForFiles /p "%ArchiveFolder%" /s /d -0 /c "cmd /c 7z a -tzip %ArchiveFolder%\Archive_%FolderToArchive%Folder_%d%_%t%.zip  @%RUNFOLDER%\%filename%" 

echo %LogStart%------------------------------------------------------------
echo %LogStart%DONE ZIPPING
echo %LogStart%------------------------------------------------------------

echo %LogStart%Delete below files in %ArchiveFolder% folder
echo %LogStart%------------------------------------------------------------

for /f "tokens=* delims=" %%i in (%filename%) do  del /s /q %ArchiveFolder%\%%i

echo %LogStart%DONE deleting files
echo %LogStart%------------------------------------------------------------


::echo %LogStart%Press ENTER to exit
::pause

exit 1