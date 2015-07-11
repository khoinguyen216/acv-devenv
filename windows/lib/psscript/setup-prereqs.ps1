Write-Verbose "Set up prerequisites"

# Module imports
Add-Type -assembly "system.io.compression.filesystem"

If (-Not (Test-Path package)) {
	mkdir package
}

# Python
$PythonPackage = "$PWD\package\python-2.7.10.amd64.msi"
$PythonBinUrl = "https://www.python.org/ftp/python/2.7.10/python-2.7.10.amd64.msi"
$PythonDir = "$PWD\python"
Write-Verbose "Verify Python installation in $PythonDir"
If (-Not (Test-Path $PythonPackage)) {
	Write-Verbose "  Download from $PythonBinUrl"
	(New-Object System.Net.WebClient).DownloadFile("$PythonBinUrl", "$PythonPackage")	
}
If (-Not (Test-Path $PythonDir)) {
	Write-Verbose "  Extract $PythonPackage"
	Start-Process -FilePath msiexec -ArgumentList /a, $PythonPackage, /qb, TARGETDIR=$PythonDir -Wait
}
$ENV:PATH="$PythonDir;$ENV:PATH"

# Powershell Community Extensions
$PscxPackage = "$PWD\package\Pscx-3.2.0.msi"
$PscxBinUrl = "https://pscx.codeplex.com/downloads/get/923562"
$PscxDir = "$PWD\pscx"
Write-Verbose "Verify Powershell Community Extensions installation in $PscxDir"
If (-Not (Test-Path $PscxPackage)) {
	Write-Verbose "  Download from $PscxBinUrl"	
	Invoke-WebRequest $PscxBinUrl -OutFile $PscxPackage
}
If (-Not (Test-Path $PscxDir)) {
	Write-Verbose "  Extract $PscxPackage"
	Start-Process -FilePath msiexec.exe -ArgumentList /a, $PscxPackage, /qb, TARGETDIR=$PscxDir -Wait
}
$ENV:PsModulePath="$PscxDir\PowerShell Community Extensions\Pscx3;$ENV:PsModulePath"

# 7zip
$7zipPackage = "$PWD\package\7za920.zip"
$7zipBinUrl = "http://www.7-zip.org/a/7za920.zip"
$7zipBinDir = [System.IO.Path]::GetFileNameWithoutExtension($7zipPackage)
$7zipBinDir = "$PWD\$7zipBinDir"
Write-Verbose "Verify 7zip installation in $7zipBinDir"
If (-Not (Test-Path $7zipPackage)) {
	Write-Verbose "  Download from 7zipBinUrl"
	(New-Object System.Net.WebClient).DownloadFile("$7zipBinUrl", "$7zipPackage")
}
If (-Not (Test-Path $7zipBinDir)) {
	Write-Verbose "  Extract $7zipPackage"
	[io.compression.zipfile]::ExtractToDirectory("$7zipPackage", $7zipBinDir)
}
$ENV:PATH = "$7zipBinDir;$ENV:PATH"