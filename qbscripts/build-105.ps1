# Build Server Script for Meridian59 105
# Telnet code: http://thesurlyadmin.com/2013/04/04/using-powershell-as-a-telnet-client/
# Requires psexec.exe: https://technet.microsoft.com/en-us/sysinternals/psexec.aspx
Param(
  #Source Repository Paths
  [string]$PatchListGen="C:\qbscripts\PatchListGenerator.exe",
  [string]$RootPath="C:\Meridian59",
  [string]$SolutionPath="\",
  [string]$MeridianSolution="Meridian59.sln",
  [string]$ClientPath="\run\localclient\",
  [string]$ServerPath="\run\server\",
  [string]$ResourcePath="\resource\",

  #Destination Client Paths
  [string]$OutputPackagePath="C:\patchserver\106\clientpatch\", # Root Path for Uncompressed Client
  [string]$ProdPackagePath="C:\patchserver\105\clientpatch\",
  [string]$TestPackagePath="C:\patchserver\106\clientpatch\",
  [string]$OutputParentPath="C:\patchserver\106", # Root path for patchinfo.txt
  [string]$ProdParentPath="C:\patchserver\105",
  [string]$TestParentPath="C:\patchserver\106",
  [string]$PackageResourcePath="resource\", #Resources subdirectory
  [string]$LatestZip="latest.zip",

  #Destination Server Paths
  [string]$OutputServerPath="C:\Server106\",
  [string]$ProdServerPath="Z:\",
  [string]$TestServerPath="C:\Server106\",

  #DNS
  [string]$OutputIP="127.0.0.1",
  [string]$ProdServerIP="172.31.51.182",
  [string]$TestServerIP="127.0.0.1",

  #Control Parameters
  [string]$BuildID="latest",
  [ValidateSet('build','clean','package','checkout','killserver','startserver','copyserver')]$Action,
  [ValidateSet('develop','master','release')]$Branch,
  [ValidateSet('production','test')]$Server,
  [ValidateSet('true','false')]$Hotfix,
  [switch]$WhatIf
)

function DisplaySyntax
{
   Write-Host
   Write-Host "Usage Scenarios:"
   Write-Host "1) Build or Clean Project and Platform"
   Write-Host "   build-105.ps1 -Action [build|clean]"
   Write-Host 
   Write-Host "2) Package Client for distribution"
   Write-Host "   build-105.ps1 -Action package"
   Write-Host 
   Write-Host "3) Checkout latest code"
   Write-Host "   build-105.ps1 -Action checkout"
   Write-Host 
   Write-Host "4) Copy server files to destination"
   Write-Host "   build-105.ps1 -Action copyserver"
   Write-Host
   Write-Host "5) Shutdown server"
   Write-Host "   build-105.ps1 -Action killserver"
   Write-Host
   Write-Host "6) Start server and recreate"
   Write-Host "   build-105.ps1 -Action startserver"
   Write-Host
   Write-Host "Optional Parameters:"
   Write-Host "-PatchListGen the executable file to generate patchinfo.txt"
   Write-Host "-RootPath the root path to work under for all operations"
   Write-Host "-SolutionPath the path to the client solution files"
   Write-Host "-MeridianSolution the file name of the Meridian solution file"
   Write-Host "-ClientPath the path to copy client files from"
   Write-Host "-ResourcePath the path to copy client resources from"
   Write-Host "-OutputPackagePath the path to output a packaged client"
   Write-Host "-OutputParentPath the path to output patchinfo.txt"
   Write-Host "-PackageResourcePath the path to copy resources to"
   Write-Host "-Branch which branch to build from (develop, master, release)"
   Write-Host "-Server either production (105) or test (106)"
   Write-Host "-Hotfix true if production server should reuse kodbase.txt and not restart"
   Write-Host "-BuildID used by the build server to name output files"
   Write-Host "-WhatIf simulates the actions and outputs resulting command to be run instead of performing said command"
   Write-Host
}

########### Path Creation Code ###########
function TestRootPath
{
   if (-not (Test-Path -Path $RootPath -PathType container))
   {
      Write-Host "$RootPath does not exist, creating"
      New-Item -Path $RootPath -ItemType Directory
   }
}

function TestPackagePath
{
   if (-not (Test-Path -Path "$OutputPackagePath"-PathType container))
   {
      Write-Host "$OutputPackagePath does not exist, creating"
      New-Item -Path "$OutputPackagePath" -ItemType Directory
   }
}

function TestResourcesPath
{
   if (-not (Test-Path -Path "$OutputPackagePath$PackageResourcePath"-PathType container))
   {
      Write-Host "$OutputPackagePath$PackageResourcePath does not exist, creating"
      New-Item -Path "$OutputPackagePath$PackageResourcePath" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\mail\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\mail\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\mail\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\resource\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\resource\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\resource\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\ads\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\ads\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\ads\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\download\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\download\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\download\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\help\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\help\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\help\\" -ItemType Directory
   }
}

########### Checkout Code ###########
function CheckoutAction
{
   Set-Location -Path $RootPath
   TestRootPath
   CheckoutAndUpdateBranch
}

function CheckoutAndUpdateBranch
{
   $Command = "git checkout " + $Branch
   if ($WhatIf) { Write-Host $Command }
   else
   {
      Write-Host "Running: $Command"
      powershell "& $Command "
   }

   $Command = "git pull origin " + $Branch
   if ($WhatIf) { Write-Host $Command }
   else
   {
      Write-Host "Running: $Command"
      powershell "& $Command "
   }
}

########### Build Action Code ###########
function Get-Batchfile ($file) {
   $cmd = "`"$file`" & set"
   cmd /c $cmd | Foreach-Object {
      $p, $v = $_.split('=')
      Set-Item -path env:$p -value $v
   }
}

function VsVars32()
{
   $vs120comntools = (Get-ChildItem env:VS120COMNTOOLS).Value
   $batchFile = [System.IO.Path]::Combine($vs120comntools, "vsvars32.bat")
   Get-Batchfile $BatchFile
}

function CopyProdKodbase
{
   if ($Hotfix -eq "true")
   {
      Write-Host "Using production kodbase.txt"

      Copy-Item "$ProdServerPath\kodbase.txt" -Destination "$RootPath\kod" -Recurse -Force
   }
   else
   {
      Write-Host "Making new kodbase.txt"
   }
}

function BuildCodebase
{
   $Command = "nmake release=1 nocopyfiles=1"
   if (-not ($ENV:INCLUDE))   { VsVars32 }
   if ($WhatIf) { Write-Host $Command }
   else 
   { 
      Write-Host "Running: $Command"
      powershell "& $Command "
   }
}

function BuildAction
{
   CheckServerParams
   CopyProdKodbase
   BuildCodebase
}

########### Clean Action Code ###########
function CleanAction
{
   $Command = "nmake clean release=1"

   if (-not ($ENV:INCLUDE))   { VsVars32 }
   if ($WhatIf) { Write-Host $Command }
   else 
   { 
      Write-Host "Running: $Command"
      powershell "& $Command "
   }
}

########### Parameter Checking ###########
function CheckServerParams
{
   if (($Server -ne "production") -and ($Server -ne "test"))
   {
      Write-Error "Incorrect -Server parameter!"
      DisplaySyntax
      Exit -1
   }

   switch ($Server)
   {
      production
      {
         $script:OutputPackagePath = $ProdPackagePath
         $script:OutputParentPath = $ProdParentPath
         $script:OutputServerPath = $ProdServerPath
         $script:OutputIP = $ProdServerIP
      }
      test
      {
         $script:OutputPackagePath = $TestPackagePath
         $script:OutputParentPath = $TestParentPath
         $script:OutputServerPath = $TestServerPath
         $script:OutputIP = $TestServerIP
      }
   }
}

########### Copy Server Action Code ###########
function TestServerDirs
{
   if (-not (Test-Path -Path $OutputServerPath -PathType container))
   {
      Write-Host "$OutputServerPath does not exist, creating"
      New-Item -Path $OutputServerPath -ItemType Directory
   }

   if (-not (Test-Path -Path "$OutputServerPath\\channel\\"-PathType container))
   {
      Write-Host "$OutputServerPath\\channel\\ does not exist, creating"
      New-Item -Path "$OutputServerPath\\channel\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputServerPath\\savegame\\"-PathType container))
   {
      Write-Host "$OutputServerPath\\savegame\\ does not exist, creating"
      New-Item -Path "$OutputServerPath\\savegame\\" -ItemType Directory
   }
}

function CopyServerFiles
{
   $BinaryItems=@("blakserv.exe","blakston.khd","jansson.dll","kodbase.txt",
                  "libcurl.dll","libmysql.dll","packages.txt","protocol.khd")

   $BinarySource = "$RootPath$ServerPath"
   $BinaryDestination = "$OutputServerPath"

   foreach ($file in $BinaryItems)
   {
      Copy-Item "$BinarySource$file" -destination $BinaryDestination -Force
   }
}

function CopyServerBofs
{
   $ServerSource = "$RootPath$ServerPath"
   $ServerDest = "$OutputServerPath"
   $BofDir = "loadkod"

   Write-Host "$ServerDest"

   $GitIgnore = Get-ChildItem -Path "$ServerSource$BofDir" -Filter .gitignore
   Copy-Item "$ServerSource$BofDir" -Destination $ServerDest -Recurse -Force -exclude $GitIgnore
}

function CopyServerDirs
{
   $ServerDirs=@("loadkod","memmap","rooms","rsc")

   $ServerSource = "$RootPath$ServerPath"
   $ServerDest = "$OutputServerPath"

   foreach ($folder in $ServerDirs)
   {
      $GitIgnore = Get-ChildItem -Path "$ServerSource$folder" -Filter .gitignore
      Copy-Item "$ServerSource$folder" -Destination $ServerDest -Recurse -Force -exclude $GitIgnore
   }
}

function CopySaveGame
{
   if ($Server -eq "test")
   {
      $SaveLocation = "savegame\lastsave.txt"
      $ServerSource = "$ProdServerPath"
      $ServerDest = "$OutputServerPath"

      Remove-Item "$ServerDest\savegame\*"

      $SaveFile = (Get-Content $ServerSource$SaveLocation)[12]
      $SaveExt = $SaveFile.Replace("LASTSAVE ", ".")

      Write-Host "Copying save game $SaveExt to test"

      $SaveFiles=@("accounts","dynarscs","gameuser","striings")
      foreach ($file in $SaveFiles)
      {
         Copy-Item "$ServerSource\savegame\$file$SaveExt" -Destination "$ServerDest\savegame" -Recurse -Force
      }
      Copy-Item "$ServerSource$SaveLocation" -Destination "$ServerDest\savegame" -Recurse -Force
   }
}

function CopyServerKodbase
{
   Copy-Item "$RootPath\kod\kodbase.txt" -Destination "$ProdServerPath" -Recurse -Force
}

function CopyServerAction
{
   CheckServerParams
   TestServerDirs

   if (($Hotfix -eq "true") -and ($Server -eq "production"))
   {
      Write-Host "Production hotfix"
      CopyServerBofs
      CopyServerKodbase
   }
   else
   {
      CopyServerFiles
      CopyServerDirs
      CopySaveGame
   }
}

########### Package Action Code ###########
function CopyFiles
{
   $BinaryItems=@("club.exe","m59bind.exe","meridian.exe","ikpMP3.dll",
                  "irrKlang.dll","Heidelb1.ttf","license.rtf")
   
   $BinarySource = "$RootPath$ClientPath"
   $BinaryDestination = "$OutputPackagePath"
   
   foreach ($file in $BinaryItems)
   {
      Copy-Item "$BinarySource$file" -destination $BinaryDestination -Force
   }
}

function CopyResources
{
   $ResourceSource = "$RootPath$ClientPath$PackageResourcePath"
   $ResourceDestination = "$OutputPackagePath$PackageResourcePath"

   $GitIgnore = Get-ChildItem -Path $ResourceSource -Filter .gitignore
   $files = Get-ChildItem -Path $ResourceSource -Exclude $GitIgnore
   $files | Copy-Item -Destination $ResourceDestination -Force
}

function CompressAll
{
   if (Test-Path $OutputPackagePath$LatestZip)
   {
      Remove-Item $OutputPackagePath$LatestZip
   }

   Get-ChildItem -Recurse $OutputPackagePath |
   Write-Zip -OutputPath $OutputPackagePath$LatestZip -IncludeEmptyDirectories -EntryPathRoot $OutputPackagePath
}

function PatchListGen
{
   $Args = "--client=$OutputPackagePath", "--outfile=$OutputParentPath\patchinfo.txt", "--type=classic"

   if ($WhatIf) { Write-Host $PatchListGen $Args }
   else 
   { 
      Write-Host "Running: $PatchListGen $Args"
      powershell "& $PatchListGen $Args "
   }
}

function PackageAction
{
   CheckServerParams
   TestPackagePath
   TestResourcesPath
   CopyFiles
   CopyResources
   CompressAll
   PatchListGen
}

########### KillServer Action Code ###########
function KillServerAction
{
   CheckServerParams

   $Commands=@("save game","terminate save")

   $Socket = New-Object System.Net.Sockets.TcpClient("$OutputIP", 9999)
   if ($Socket)
   {
      $Stream = $Socket.GetStream()
      $Writer = New-Object System.IO.StreamWriter($Stream)
      $Buffer = New-Object System.Byte[] 1024
      $Encoding = New-Object System.Text.AsciiEncoding

      foreach ($Command in $Commands)
      {
         $Writer.WriteLine($Command)
         $Writer.Flush()
         Start-Sleep -Milliseconds (4000)
      }
   }
}

########### Start Server Action Code ###########
function StartServer
{
   if ($Server -eq "production")
   {
      Start-Process -FilePath "c:\qbscripts\psexec.exe" -ArgumentList ('-i 2', '-d', '\\172.31.51.182', '-u username', '-p password', 'cmd /c start /D c:\meridian\ blakserv.exe') -Wait
      Start-Sleep -Milliseconds (2000)
   }
   else
   {
      Start-Process -WorkingDirectory $OutputServerPath -FilePath "blakserv.exe"
      Start-Sleep -Milliseconds (2000)
   }
}

function RecreateServer
{
   $Commands=@("lock updating","send o 0 recreateall","unlock")

   $Socket = New-Object System.Net.Sockets.TcpClient("$OutputIP", 9999)
   if ($Socket)
   {
      $Stream = $Socket.GetStream()
      $Writer = New-Object System.IO.StreamWriter($Stream)
      $Buffer = New-Object System.Byte[] 1024
      $Encoding = New-Object System.Text.AsciiEncoding

      foreach ($Command in $Commands)
      {
         $Writer.WriteLine($Command)
         $Writer.Flush()
         Start-Sleep -Milliseconds (8000)
      }
   }
}

function ReloadServer
{
   $Commands=@("reload system")

   $Socket = New-Object System.Net.Sockets.TcpClient("$OutputIP", 9999)
   if ($Socket)
   {
      $Stream = $Socket.GetStream()
      $Writer = New-Object System.IO.StreamWriter($Stream)
      $Buffer = New-Object System.Byte[] 1024
      $Encoding = New-Object System.Text.AsciiEncoding

      foreach ($Command in $Commands)
      {
         $Writer.WriteLine($Command)
         $Writer.Flush()
         Start-Sleep -Milliseconds (1000)
      }
   }
}

function StartServerAction
{
   CheckServerParams
   if (($Hotfix -eq "true") -and ($Server -eq "production"))
   {
      Write-Host "Production hotfix"
      ReloadServer
   }
   else
   {
      StartServer
      RecreateServer
   }
}

########### Main Code ###########
if (($Action -ne "build") -and ($Action -ne "clean") -and ($Action -ne "package") -and ($Action -ne "checkout") -and ($Action -ne "startserver") -and ($Action -ne "killserver") -and ($Action -ne "copyserver"))
{
   Write-Error "Incorrect -Action parameter!"
   DisplaySyntax
   Exit -1
}

switch ($Action)
{
   checkout { CheckoutAction }
   build { BuildAction }
   clean { CleanAction }
   package { PackageAction }
   startserver { StartServerAction }
   killserver { KillServerAction }
   copyserver { CopyServerAction }
}