$ErrorActionPreference = "Stop"
# Verbosity
$VerbosePreference = "Continue"

# Global variables
$TOPDIR = $PWD
$VCDIR = "$ENV:VS120COMNTOOLS\..\..\VC"

# Verify prerequisites
psscript/setup-prereqs

# Common modules
Write-Verbose "Set up common utility modules"
Add-Type -assembly "system.io.compression.filesystem"
Import-Module Pscx

# CMake
cd $TOPDIR
$CMakePackage = "$PWD\package\cmake-3.2.3-win32-x86.zip"
$CMakeBinUrl = "http://www.cmake.org/files/v3.2/cmake-3.2.3-win32-x86.zip"
$CMakeDir = "$PWD\cmake-3.2.3-win32-x86"
Write-Verbose "Verify CMake installation in $CMakeDir"
If (-Not (Test-Path $CMakeDir)) {
	If (-Not (Test-Path $CMakePackage)) {
		Write-Verbose "  Download from $CMakeBinUrl"
		(New-Object System.Net.WebClient).DownloadFile("$CMakeBinUrl", "$CMakePackage")
	}		
	Write-Verbose "  Extract $CMakePackage"
	[io.compression.zipfile]::ExtractToDirectory("$CMakePackage", $PWD)
}
$ENV:PATH = "$CMakeDir/bin;$ENV:PATH"

# Boost
cd $TOPDIR
$BoostPackage = "$PWD\package\boost_1_58_0.zip"
$BoostSrcUrl = "http://downloads.sourceforge.net/project/boost/boost/1.58.0/boost_1_58_0.zip?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Fboost%2Ffiles%2Fboost%2F1.58.0%2F&ts=1435194500&use_mirror=jaist"
$BoostDir = "$PWD\boost_1_58_0"
Write-Verbose "Verify Boost installation in $BoostDir"
If (-Not (Test-Path $BoostPackage)) {
	Write-Verbose "  Download from $BoostSrcUrl"
	(New-Object System.Net.WebClient).DownloadFile("$BoostSrcUrl", "$BoostPackage")
}
If (-Not (Test-Path $BoostDir)) {
	Write-Verbose "  Extract $BoostPackage"
	[io.compression.zipfile]::ExtractToDirectory("$BoostPackage", $PWD)
}
If (-Not (Test-Path $BoostDir/stage)) {
	Write-Verbose "  Bootstrap"
	cd $BoostDir
	.\bootstrap.bat
	Write-Verbose "  Build"
	.\b2.exe address-model=64 link=shared
}

# OpenCV
cd $TOPDIR
$OpenCVPackage = "$PWD\package\opencv-2.4.11.exe"
$OpenCVBinUrl = "http://downloads.sourceforge.net/project/opencvlibrary/opencv-win/2.4.11/opencv-2.4.11.exe?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Fopencvlibrary%2Ffiles%2Fopencv-win%2F2.4.11%2Fopencv-2.4.11.exe%2Fdownload%3Fuse_mirror%3Djaist%26r%3Dhttp%253A%252F%252Fopencv.org%252Fdownloads.html%26use_mirror%3Djaist&ts=1435196111&use_mirror=skylink"
$OpenCVDir = [System.IO.Path]::GetFileNameWithoutExtension($OpenCVPackage)
Write-Verbose "Verify OpenCV istallation in $OpenCVDir"
If (-Not (Test-Path $OpenCVPackage)) {
	Write-Verbose "  Download from $OpenCVBinUrl"
	(New-Object System.Net.WebClient).DownloadFile("$OpenCVBinUrl", "$OpenCVPackage")
}
If (-Not (Test-Path $OpenCVDir)) {
	Write-Verbose "  Extract $OpenCVPackage"
	Start-Process "$OpenCVPackage" -ArgumentList '-o"." -y' -Wait
	Rename-Item -path $PWD\opencv -newName $PWD\$OpenCVDir
}

# Qt
cd $TOPDIR
$QtPackage = "$PWD\package\qt-everywhere-opensource-src-5.4.2.zip"
$QtSrcUrl = "http://download.qt.io/official_releases/qt/5.4/5.4.2/single/qt-everywhere-opensource-src-5.4.2.zip"
$QtSrcDir = [System.IO.Path]::GetFileNameWithoutExtension($QtPackage)
$QtSrcDir = "$PWD\$QtSrcDir"
$QtBinDir = "$PWD\qt-5.4.2"
Write-Verbose "Verify Qt istallation in $QtBinDir"
If (-Not (Test-Path $QtPackage)) {
	Write-Verbose "  Download from $QtSrcUrl"
	(New-Object System.Net.WebClient).DownloadFile("$QtSrcUrl", "$QtPackage")
}
If (-Not (Test-Path $QtSrcDir)) {
	Write-Verbose "  Extract $QtPackage"
	[io.compression.zipfile]::ExtractToDirectory("$QtPackage", "$PWD")
}
If (-Not (Test-Path $QtBinDir)) {
	Write-Verbose "  Prepare env for MSVC"
	Invoke-BatchFile $VCDIR\vcvarsall.bat amd64
	Write-Verbose "  Configure"
	cd $QtSrcDir
	.\configure -prefix $TOPDIR\qt-5.4.2 -opensource -confirm-license -mp `
	-nomake examples -nomake tests -debug-and-release -c++11 -platform win32-msvc2013 `
	-no-qml-debug -skip activeqt -skip connectivity -skip declarative -skip enginio `
	-skip graphicaleffects -skip imageformats -skip location -skip multimedia `
	-skip quickcontrols -skip script -skip sensors -skip svg -skip serialport `
	-skip webchannel -skip webkit -skip webkit-examples -skip websockets -skip webengine
	nmake
	nmake install
}

# Eigen
cd $TOPDIR
$EigenPackage = "$PWD\package\eigen-3.2.4.tar.bz2"
$EigenSrcUrl = "http://bitbucket.org/eigen/eigen/get/3.2.4.tar.bz2"
$EigenSrcDir = [System.IO.Path]::GetFileNameWithoutExtension($EigenPackage)
$EigenSrcDir = [System.IO.Path]::GetFileNameWithoutExtension($EigenSrcDir)
$EigenSrcDir = "$PWD\$EigenSrcDir"
Write-Verbose "Verify Eigen installation in $EigenSrcDir"
If (-Not (Test-Path $EigenPackage)) {
	Write-Verbose "  Download from EigenSrcUrl"	
	(New-Object System.Net.WebClient).DownloadFile("$EigenSrcUrl", "$EigenPackage")
}
If (-Not (Test-Path $EigenSrcDir)) {
	Write-Verbose "  Extract $EigenPackage"
	7za e -y -o"$PWD\package" $EigenPackage
	7za x -y -o"$EigenSrcDir" $PWD\package\eigen-3.2.4.tar	
}

# Live555
cd $TOPDIR
$LivePackage = "$PWD\package\live555-latest.tar.gz"
$LiveSrcUrl = "http://www.live555.com/liveMedia/public/live555-latest.tar.gz"
$LiveSrcDir = "$PWD\live"
Write-Verbose "Verify Live555 installation in $LiveSrcDir"
If (-Not (Test-Path $LivePackage)) {
	Write-Verbose "  Download from $LiveSrcUrl"
	(New-Object System.Net.WebClient).DownloadFile("$LiveSrcUrl", "$LivePackage")
}
If (-Not (Test-Path $LiveSrcDir)) {
	Write-Verbose "  Extract $LivePackage"
	7za e -y -o"$PWD\package" $LivePackage
	7za x -y -o"$PWD" $PWD\package\live555-latest.tar
}
If (-Not (Test-Path $LiveSrcDir\liveMedia\libliveMedia.lib)) {
	Write-Verbose "  Configure"
	cd $LiveSrcDir
	Write-Verbose "  Adapt Makefile parameters"
	(Get-Content win32config) | Foreach-Object {$_ -replace 'c:\\Program Files\\DevStudio\\Vc', "`"$VCDIR`""} | Foreach-Object {$_ -replace "i386", "amd64"} | Foreach-Object {$_ -replace "\(TOOLS32\)", "(TOOLS32:`"=)"} | Foreach-Object {$_ -replace "\\bin\\cl", "\bin\amd64\cl"} | Foreach-Object {$_ -replace "msvcirt\.lib", "msvcrt.lib /machine:x64"}| Out-File win32config
	./genWindowsMakefiles
	Write-Verbose "  Prepare env for MSVC"
	Invoke-BatchFile $VCDIR\vcvarsall.bat amd64
	Write-Verbose "  Build"
	Write-Verbose "  If you encounter errors related to ntwin32.mak or win32.mak"
	Write-Verbose "  copy these files from Windows SDK\<versions>\include"
	Write-Verbose "  to Microsoft Visual Studio <version>\VC\include"
	cd groupsock
	nmake /B -f groupsock.mak
	cd ../liveMedia
	nmake /B -f liveMedia.mak
	cd ../BasicUsageEnvironment
	nmake /B -f BasicUsageEnvironment.mak
	cd ../UsageEnvironment
	nmake /B -f UsageEnvironment.mak
}

# FFmpeg
cd $TOPDIR
$FFmpegBinPackage = "$PWD\package\ffmpeg-20150619-git-bb3703a-win64-shared.7z"
$FFmpegBinUrl = "http://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-20150619-git-bb3703a-win64-shared.7z"
$FFmpegBinDir = [System.IO.Path]::GetFileNameWithoutExtension($FFmpegBinPackage)
$FFmpegDevPackage = "$PWD\package\ffmpeg-20150619-git-bb3703a-win64-dev.7z"
$FFmpegDevUrl = "http://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-20150619-git-bb3703a-win64-dev.7z"
$FFmpegDevDir = [System.IO.Path]::GetFileNameWithoutExtension($FFmpegDevPackage)
Write-Verbose "Verify FFmpeg installation in $LiveSrcDir"
If (-Not (Test-Path $FFmpegBinPackage)) {
	Write-Verbose "  Download from $FFmpegBinUrl"
	(New-Object System.Net.WebClient).DownloadFile("$FFmpegBinUrl", "$FFmpegBinPackage")
}
If (-Not (Test-Path $FFmpegBinDir)) {
	Write-Verbose "  Extract $FFmpegBinPackage"
	7za x -y -o"$PWD" $FFmpegBinPackage
}
If (-Not (Test-Path $FFmpegDevPackage)) {
	Write-Verbose "  Download from $FFmpegDevUrl"
	(New-Object System.Net.WebClient).DownloadFile("$FFmpegDevUrl", "$FFmpegDevPackage")
}
If (-Not (Test-Path $FFmpegDevDir)) {
	Write-Verbose "  Extract $FFmpegDevPackage"
	7za x -y -o"$PWD" $FFmpegDevPackage
}


cd $TOPDIR















