echo off 
cls

REM --------- IMPORTANT to run the below command on first time of this script (with proper zip exe path)
REM SET PATH=%PATH%;"C:\Program Files\7-Zip"


echo %LogStart%Executing batch script 

REM --------- Get time and Date
for /f "tokens=1,2,3 delims=:. " %%x in ("%time%") do set t=%%x%%y%%z
for /f "tokens=3,2,4 delims=/- " %%x in ("%date%") do set d=%%y%%x%%z

set LogStart=ViewServer:%d%:%t%:

set ARCHIVEDELETEDAYS=30
echo %LogStart%Configuration is to be delete .zip files olders than %ARCHIVEDELETEDAYS% days %NL%
echo %LogStart%------------------------------------------------------------

set ExtFile=datafiles.txt

:: Execute the batch file with folder to srchive with file extensions in it and number of days to archive 

:: logs folder to archive
start file-MZD-archive.bat logs,%ExtFile%,%ARCHIVEDELETEDAYS%

:: data folder to archive
start file-MZD-archive.bat data,%ExtFile%,%ARCHIVEDELETEDAYS%