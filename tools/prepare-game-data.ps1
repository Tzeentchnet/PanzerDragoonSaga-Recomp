[CmdletBinding()]
param(
    [string]$DataDirectory,
    [string]$SourcePath,
    [string]$CacheDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:PreparationVersion = '1'
$script:ImageExtensions = @('.cue', '.iso', '.bin')
$script:ArchiveExtensions = @('.rar', '.zip')

if ([string]::IsNullOrWhiteSpace($DataDirectory)) {
    $repositoryRoot = Split-Path -Parent $PSScriptRoot
    $DataDirectory = Join-Path $repositoryRoot 'data'
}
$dataRoot = [IO.Path]::GetFullPath($DataDirectory)
if (-not (Test-Path -LiteralPath $dataRoot -PathType Container)) {
    throw "Game data directory not found: $dataRoot"
}

if ([string]::IsNullOrWhiteSpace($CacheDirectory)) {
    $CacheDirectory = Join-Path $dataRoot '.prepared'
}
$cacheRoot = [IO.Path]::GetFullPath($CacheDirectory)

function Get-DiscSourceRank {
    param([IO.FileInfo]$File)

    $name = $File.BaseName.ToLowerInvariant()
    $discRank = 100
    if ($name -match '(^|[^a-z0-9])(disc|disk|cd)[ _.-]*0?1([^0-9]|$)') {
        $discRank = 0
    }
    elseif ($name -match '(^|[^a-z0-9])(disc|disk|cd)[ _.-]*0?[2-9]([^0-9]|$)') {
        $discRank = 1000
    }

    $extensionRank = switch ($File.Extension.ToLowerInvariant()) {
        '.cue' { 0 }
        '.iso' { 10 }
        '.bin' { 20 }
        '.zip' { 30 }
        '.rar' { 40 }
        default { 100 }
    }

    return $discRank + $extensionRank
}

function Get-GameDataRoot {
    param([string]$Root)

    $directCommonData = Join-Path $Root 'COMMON.DAT'
    if (Test-Path -LiteralPath $directCommonData -PathType Leaf) {
        return [IO.Path]::GetFullPath($Root)
    }

    $matches = @(
        Get-ChildItem -LiteralPath $Root -Filter 'COMMON.DAT' -File -Recurse |
            Sort-Object FullName
    )
    if ($matches.Count -eq 0) {
        return $null
    }

    return $matches[0].Directory.FullName
}

function Get-SourceFingerprint {
    param([IO.FileInfo]$Source)

    $fingerprintLines = [Collections.Generic.List[string]]::new()
    [void]$fingerprintLines.Add($script:PreparationVersion)
    [void]$fingerprintLines.Add($Source.Name.ToLowerInvariant())
    [void]$fingerprintLines.Add([string]$Source.Length)
    [void]$fingerprintLines.Add([string]$Source.LastWriteTimeUtc.Ticks)

    if ($Source.Extension -ieq '.cue') {
        foreach ($image in @(
            Get-ChildItem -LiteralPath $Source.DirectoryName -File |
                Where-Object {
                    $script:ImageExtensions -contains $_.Extension.ToLowerInvariant()
                } |
                Sort-Object Name
        )) {
            [void]$fingerprintLines.Add($image.Name.ToLowerInvariant())
            [void]$fingerprintLines.Add([string]$image.Length)
            [void]$fingerprintLines.Add([string]$image.LastWriteTimeUtc.Ticks)
        }
    }

    $text = $fingerprintLines -join "`n"
    $bytes = [Text.Encoding]::UTF8.GetBytes($text)
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        $hash = $sha256.ComputeHash($bytes)
    }
    finally {
        $sha256.Dispose()
    }

    return -join ($hash | ForEach-Object { $_.ToString('x2') })
}

function Get-TarExecutable {
    $command = Get-Command 'tar.exe' -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $command) {
        throw (
            'Windows bsdtar (tar.exe) is required to read RAR, ZIP, and ISO ' +
            'game data. Install a current supported Windows release.'
        )
    }
    return $command.Source
}

function Invoke-TarList {
    param(
        [string]$TarExecutable,
        [string]$Path,
        [switch]$VerboseListing
    )

    $listOption = if ($VerboseListing) { '-tvf' } else { '-tf' }
    $output = @(& $TarExecutable $listOption $Path 2>&1)
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        $details = ($output | ForEach-Object { [string]$_ }) -join [Environment]::NewLine
        throw (
            "Could not read '$Path' with Windows bsdtar (exit code " +
            "$exitCode).`n$details"
        )
    }
    return @($output | ForEach-Object { [string]$_ })
}

function Test-SafeArchiveEntries {
    param(
        [string[]]$Entries,
        [string[]]$VerboseEntries,
        [string]$Path
    )

    foreach ($entry in $Entries) {
        $normalized = $entry.Replace('\', '/').Trim()
        if ([string]::IsNullOrWhiteSpace($normalized)) {
            continue
        }
        if (
            $normalized.StartsWith('/') -or
            $normalized -match '^[A-Za-z]:' -or
            ($normalized -split '/') -contains '..'
        ) {
            throw "Archive '$Path' contains an unsafe path: $entry"
        }
    }

    foreach ($entry in $VerboseEntries) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }
        $entryType = $entry[0]
        if ($entryType -ne '-' -and $entryType -ne 'd') {
            throw "Archive '$Path' contains an unsupported link or special entry."
        }
    }
}

function Expand-WithTar {
    param(
        [string]$Path,
        [string]$Destination
    )

    $tarExecutable = Get-TarExecutable
    $entries = Invoke-TarList $tarExecutable $Path
    $verboseEntries = Invoke-TarList $tarExecutable $Path -VerboseListing
    Test-SafeArchiveEntries $entries $verboseEntries $Path

    [void][IO.Directory]::CreateDirectory($Destination)
    $output = @(
        & $tarExecutable '-xf' $Path '-C' $Destination 2>&1
    )
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        $details = ($output | ForEach-Object { [string]$_ }) -join [Environment]::NewLine
        throw (
            "Could not extract '$Path' with Windows bsdtar (exit code " +
            "$exitCode).`n$details"
        )
    }
}

function Convert-CueTimeToFrames {
    param([string]$Time)

    if ($Time -notmatch '^(\d+):([0-5]\d):([0-7]\d)$') {
        throw "Invalid CUE time '$Time'."
    }
    return ([int64]$Matches[1] * 60 * 75) +
        ([int64]$Matches[2] * 75) +
        [int64]$Matches[3]
}

function Resolve-CueImage {
    param(
        [string]$CuePath,
        [string]$ReferencedName
    )

    $cueDirectory = [IO.Path]::GetDirectoryName($CuePath)
    $referencedPath = Join-Path $cueDirectory $ReferencedName
    if (Test-Path -LiteralPath $referencedPath -PathType Leaf) {
        return [IO.FileInfo]::new([IO.Path]::GetFullPath($referencedPath))
    }

    $referencedBaseName = [IO.Path]::GetFileNameWithoutExtension($ReferencedName)
    $sameBaseName = @(
        Get-ChildItem -LiteralPath $cueDirectory -File |
            Where-Object {
                ($_.Extension -ieq '.bin' -or $_.Extension -ieq '.iso') -and
                $_.BaseName -ieq $referencedBaseName
            }
    )
    if ($sameBaseName.Count -eq 1) {
        return $sameBaseName[0]
    }

    $images = @(
        Get-ChildItem -LiteralPath $cueDirectory -File |
            Where-Object {
                $_.Extension -ieq '.bin' -or $_.Extension -ieq '.iso'
            }
    )
    if ($images.Count -eq 1) {
        return $images[0]
    }

    throw (
        "CUE file '$CuePath' references missing image '$ReferencedName', " +
        'and no unique BIN or ISO replacement was found.'
    )
}

function Get-CueLayout {
    param([string]$CuePath)

    $currentFile = $null
    $dataTrack = $null
    foreach ($line in [IO.File]::ReadAllLines($CuePath)) {
        if ($line -match '^\s*FILE\s+(?:"([^"]+)"|(\S+))\s+\S+\s*$') {
            $currentFile = if (
                -not [string]::IsNullOrWhiteSpace($Matches[1])
            ) {
                $Matches[1]
            }
            else {
                $Matches[2]
            }
            continue
        }

        if ($line -match '^\s*TRACK\s+\d+\s+(MODE[12]/\d+)\s*$') {
            if ([string]::IsNullOrWhiteSpace($currentFile)) {
                throw "CUE file '$CuePath' declares a track before a FILE entry."
            }
            if ($null -eq $dataTrack) {
                $dataTrack = [pscustomobject]@{
                    FileName = $currentFile
                    Mode = $Matches[1].ToUpperInvariant()
                    IndexFrames = $null
                }
            }
            continue
        }

        if (
            $null -ne $dataTrack -and
            $null -eq $dataTrack.IndexFrames -and
            $line -match '^\s*INDEX\s+01\s+(\d+:\d+:\d+)\s*$'
        ) {
            $dataTrack.IndexFrames = Convert-CueTimeToFrames $Matches[1]
            break
        }
    }

    if ($null -eq $dataTrack -or $null -eq $dataTrack.IndexFrames) {
        throw "CUE file '$CuePath' does not contain a supported data track."
    }

    $sectorSize = 0
    $userDataOffset = 0
    switch ($dataTrack.Mode) {
        'MODE1/2048' {
            $sectorSize = 2048
            $userDataOffset = 0
        }
        'MODE1/2352' {
            $sectorSize = 2352
            $userDataOffset = 16
        }
        'MODE2/2048' {
            $sectorSize = 2048
            $userDataOffset = 0
        }
        'MODE2/2336' {
            $sectorSize = 2336
            $userDataOffset = 8
        }
        'MODE2/2352' {
            $sectorSize = 2352
            $userDataOffset = 24
        }
        default {
            throw "CUE track mode '$($dataTrack.Mode)' is not supported."
        }
    }

    $image = Resolve-CueImage $CuePath $dataTrack.FileName
    return [pscustomobject]@{
        Path = $image.FullName
        SectorSize = $sectorSize
        UserDataOffset = $userDataOffset
        TrackStartBytes = [int64]$dataTrack.IndexFrames * $sectorSize
    }
}

function Read-ExactBytes {
    param(
        [IO.FileStream]$Stream,
        [byte[]]$Buffer
    )

    $offset = 0
    while ($offset -lt $Buffer.Length) {
        $read = $Stream.Read($Buffer, $offset, $Buffer.Length - $offset)
        if ($read -eq 0) {
            return $false
        }
        $offset += $read
    }
    return $true
}

function Test-ImageLayout {
    param(
        [string]$Path,
        [int]$SectorSize,
        [int]$UserDataOffset,
        [int64]$TrackStartBytes
    )

    $stream = [IO.File]::Open(
        $Path,
        [IO.FileMode]::Open,
        [IO.FileAccess]::Read,
        [IO.FileShare]::Read
    )
    try {
        $pvdOffset = $TrackStartBytes + (16L * $SectorSize) + $UserDataOffset
        if ($pvdOffset -lt 0 -or ($pvdOffset + 2048) -gt $stream.Length) {
            return $null
        }

        [void]$stream.Seek($pvdOffset, [IO.SeekOrigin]::Begin)
        $pvd = [byte[]]::new(2048)
        if (-not (Read-ExactBytes $stream $pvd)) {
            return $null
        }

        $signature = [Text.Encoding]::ASCII.GetString($pvd, 1, 5)
        if ($pvd[0] -ne 1 -or $signature -cne 'CD001' -or $pvd[6] -ne 1) {
            return $null
        }

        $volumeSectors = [BitConverter]::ToUInt32($pvd, 80)
        if ($volumeSectors -le 16) {
            return $null
        }

        $availableSectors = [math]::Floor(
            ($stream.Length - $TrackStartBytes) / $SectorSize
        )
        if ($availableSectors -le 16) {
            return $null
        }
        $readableSectors = [math]::Min($volumeSectors, $availableSectors)

        return [pscustomobject]@{
            Path = [IO.Path]::GetFullPath($Path)
            SectorSize = $SectorSize
            UserDataOffset = $UserDataOffset
            TrackStartBytes = $TrackStartBytes
            VolumeSectors = [uint32]$readableSectors
        }
    }
    finally {
        $stream.Dispose()
    }
}

function Get-ImageLayout {
    param([string]$ImagePath)

    foreach ($candidate in @(
        [pscustomobject]@{ SectorSize = 2048; UserDataOffset = 0 },
        [pscustomobject]@{ SectorSize = 2352; UserDataOffset = 16 },
        [pscustomobject]@{ SectorSize = 2352; UserDataOffset = 24 },
        [pscustomobject]@{ SectorSize = 2336; UserDataOffset = 8 }
    )) {
        $layout = Test-ImageLayout `
            $ImagePath `
            $candidate.SectorSize `
            $candidate.UserDataOffset `
            0
        if ($null -ne $layout) {
            return $layout
        }
    }

    throw (
        "Disc image '$ImagePath' does not contain a supported ISO9660 " +
        'MODE1 or MODE2 data track.'
    )
}

function Initialize-IsoExtractor {
    if ('Pds.Tools.Iso9660Extractor' -as [type]) {
        return
    }

    $null = Add-Type -TypeDefinition @'
namespace Pds.Tools
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;

    public sealed class Iso9660Extractor : IDisposable
    {
        private const int LogicalSectorSize = 2048;

        private readonly FileStream source;
        private readonly long trackStartBytes;
        private readonly int sectorSize;
        private readonly int userDataOffset;
        private readonly uint volumeSectors;
        private readonly byte[] sectorBuffer = new byte[LogicalSectorSize];
        private readonly HashSet<ulong> activeDirectories = new HashSet<ulong>();

        private Iso9660Extractor(
            string sourcePath,
            long trackStartBytes,
            int sectorSize,
            int userDataOffset,
            uint sectorCount)
        {
            if (trackStartBytes < 0 ||
                sectorSize < LogicalSectorSize ||
                userDataOffset < 0 ||
                userDataOffset + LogicalSectorSize > sectorSize ||
                sectorCount <= 16)
            {
                throw new ArgumentOutOfRangeException("Invalid sector layout.");
            }

            this.source = new FileStream(
                sourcePath,
                FileMode.Open,
                FileAccess.Read,
                FileShare.Read,
                sectorSize,
                FileOptions.RandomAccess);
            this.trackStartBytes = trackStartBytes;
            this.sectorSize = sectorSize;
            this.userDataOffset = userDataOffset;
            this.volumeSectors = sectorCount;
        }

        public static void Extract(
            string sourcePath,
            string destinationPath,
            long trackStartBytes,
            int sectorSize,
            int userDataOffset,
            uint sectorCount)
        {
            Directory.CreateDirectory(destinationPath);
            using (Iso9660Extractor extractor = new Iso9660Extractor(
                sourcePath,
                trackStartBytes,
                sectorSize,
                userDataOffset,
                sectorCount))
            {
                byte[] primaryVolumeDescriptor = extractor.ReadSector(16);
                uint rootExtent = ReadBothEndianUInt32(
                    primaryVolumeDescriptor,
                    158,
                    "root directory extent");
                uint rootSize = ReadBothEndianUInt32(
                    primaryVolumeDescriptor,
                    166,
                    "root directory size");
                extractor.ExtractDirectory(
                    rootExtent,
                    rootSize,
                    Path.GetFullPath(destinationPath),
                    0);
            }
        }

        public void Dispose()
        {
            source.Dispose();
        }

        private byte[] ReadSector(uint logicalSector)
        {
            if (logicalSector >= volumeSectors)
            {
                throw new InvalidDataException(
                    "ISO9660 extent is outside the declared volume.");
            }

            long offset = checked(
                trackStartBytes +
                ((long)logicalSector * sectorSize) +
                userDataOffset);
            if (offset < 0 || offset + LogicalSectorSize > source.Length)
            {
                throw new EndOfStreamException(
                    "Disc image ended before its ISO9660 volume.");
            }

            source.Position = offset;
            int read = 0;
            while (read < LogicalSectorSize)
            {
                int current = source.Read(
                    sectorBuffer,
                    read,
                    LogicalSectorSize - read);
                if (current == 0)
                {
                    throw new EndOfStreamException(
                        "Disc image ended while reading an ISO9660 sector.");
                }
                read += current;
            }

            return sectorBuffer;
        }

        private byte[] ReadExtent(uint extent, uint size)
        {
            if (size > int.MaxValue)
            {
                throw new InvalidDataException(
                    "ISO9660 directory is too large to inspect safely.");
            }

            byte[] result = new byte[(int)size];
            int destinationOffset = 0;
            uint logicalSector = extent;
            while (destinationOffset < result.Length)
            {
                byte[] sector = ReadSector(logicalSector++);
                int length = Math.Min(
                    LogicalSectorSize,
                    result.Length - destinationOffset);
                Buffer.BlockCopy(
                    sector,
                    0,
                    result,
                    destinationOffset,
                    length);
                destinationOffset += length;
            }
            return result;
        }

        private void ExtractDirectory(
            uint extent,
            uint size,
            string destinationPath,
            int depth)
        {
            if (depth > 64)
            {
                throw new InvalidDataException(
                    "ISO9660 directory nesting exceeds the safety limit.");
            }

            ulong directoryKey = ((ulong)extent << 32) | size;
            if (!activeDirectories.Add(directoryKey))
            {
                throw new InvalidDataException(
                    "ISO9660 directory cycle detected.");
            }

            try
            {
                Directory.CreateDirectory(destinationPath);
                byte[] directory = ReadExtent(extent, size);
                int offset = 0;
                while (offset < directory.Length)
                {
                    int recordLength = directory[offset];
                    if (recordLength == 0)
                    {
                        offset = checked(
                            ((offset / LogicalSectorSize) + 1) *
                            LogicalSectorSize);
                        continue;
                    }
                    if (recordLength < 34 ||
                        offset + recordLength > directory.Length)
                    {
                        throw new InvalidDataException(
                            "Invalid ISO9660 directory record.");
                    }

                    int identifierLength = directory[offset + 32];
                    if (identifierLength == 0 ||
                        33 + identifierLength > recordLength)
                    {
                        throw new InvalidDataException(
                            "Invalid ISO9660 file identifier.");
                    }

                    int identifierOffset = offset + 33;
                    if (identifierLength == 1 &&
                        (directory[identifierOffset] == 0 ||
                         directory[identifierOffset] == 1))
                    {
                        offset += recordLength;
                        continue;
                    }

                    string name = Encoding.ASCII.GetString(
                        directory,
                        identifierOffset,
                        identifierLength);
                    name = NormalizeFileName(name);
                    ValidateFileName(name);

                    byte flags = directory[offset + 25];
                    if ((flags & 0x80) != 0)
                    {
                        throw new InvalidDataException(
                            "Multi-extent ISO9660 files are not supported.");
                    }
                    if (directory[offset + 26] != 0 ||
                        directory[offset + 27] != 0)
                    {
                        throw new InvalidDataException(
                            "Interleaved ISO9660 files are not supported.");
                    }

                    uint childExtent = ReadBothEndianUInt32(
                        directory,
                        offset + 2,
                        "file extent");
                    childExtent = checked(
                        childExtent + directory[offset + 1]);
                    uint childSize = ReadBothEndianUInt32(
                        directory,
                        offset + 10,
                        "file size");
                    string childPath = GetSafeChildPath(
                        destinationPath,
                        name);

                    if ((flags & 0x02) != 0)
                    {
                        ExtractDirectory(
                            childExtent,
                            childSize,
                            childPath,
                            depth + 1);
                    }
                    else
                    {
                        ExtractFile(childExtent, childSize, childPath);
                    }

                    offset += recordLength;
                }
            }
            finally
            {
                activeDirectories.Remove(directoryKey);
            }
        }

        private void ExtractFile(uint extent, uint size, string path)
        {
            using (FileStream destination = new FileStream(
                path,
                FileMode.CreateNew,
                FileAccess.Write,
                FileShare.None,
                LogicalSectorSize,
                FileOptions.SequentialScan))
            {
                uint remaining = size;
                uint logicalSector = extent;
                while (remaining > 0)
                {
                    byte[] sector = ReadSector(logicalSector++);
                    int length = (int)Math.Min(
                        (uint)LogicalSectorSize,
                        remaining);
                    destination.Write(sector, 0, length);
                    remaining -= (uint)length;
                }
            }
        }

        private static uint ReadBothEndianUInt32(
            byte[] data,
            int offset,
            string field)
        {
            if (offset < 0 || offset + 8 > data.Length)
            {
                throw new InvalidDataException(
                    "Truncated ISO9660 " + field + ".");
            }

            uint littleEndian =
                (uint)data[offset] |
                ((uint)data[offset + 1] << 8) |
                ((uint)data[offset + 2] << 16) |
                ((uint)data[offset + 3] << 24);
            uint bigEndian =
                ((uint)data[offset + 4] << 24) |
                ((uint)data[offset + 5] << 16) |
                ((uint)data[offset + 6] << 8) |
                data[offset + 7];
            if (littleEndian != bigEndian)
            {
                throw new InvalidDataException(
                    "Mismatched ISO9660 " + field + ".");
            }
            return littleEndian;
        }

        private static string NormalizeFileName(string name)
        {
            int versionSeparator = name.LastIndexOf(';');
            if (versionSeparator > 0)
            {
                bool numericVersion = true;
                for (int index = versionSeparator + 1;
                     index < name.Length;
                     ++index)
                {
                    if (name[index] < '0' || name[index] > '9')
                    {
                        numericVersion = false;
                        break;
                    }
                }
                if (numericVersion)
                {
                    name = name.Substring(0, versionSeparator);
                }
            }
            return name.TrimEnd('.');
        }

        private static void ValidateFileName(string name)
        {
            if (String.IsNullOrEmpty(name) ||
                name == "." ||
                name == ".." ||
                name.EndsWith(" ", StringComparison.Ordinal) ||
                name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
            {
                throw new InvalidDataException(
                    "Unsafe ISO9660 file name.");
            }
        }

        private static string GetSafeChildPath(
            string directory,
            string name)
        {
            string root = Path.GetFullPath(directory);
            string child = Path.GetFullPath(Path.Combine(root, name));
            string prefix = root.TrimEnd(
                Path.DirectorySeparatorChar,
                Path.AltDirectorySeparatorChar) +
                Path.DirectorySeparatorChar;
            if (!child.StartsWith(
                prefix,
                StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException(
                    "ISO9660 entry escapes the destination directory.");
            }
            return child;
        }
    }
}
'@
}

function Expand-DiscImage {
    param(
        [IO.FileInfo]$Image,
        [string]$WorkDirectory
    )

    if ($Image.Extension -ieq '.cue') {
        $cueLayout = Get-CueLayout $Image.FullName
        $layout = Test-ImageLayout `
            $cueLayout.Path `
            $cueLayout.SectorSize `
            $cueLayout.UserDataOffset `
            $cueLayout.TrackStartBytes
        if ($null -eq $layout) {
            throw (
                "The data track described by '$($Image.FullName)' does not " +
                'contain a valid ISO9660 primary volume.'
            )
        }
    }
    else {
        $layout = Get-ImageLayout $Image.FullName
    }

    $outputDirectory = Join-Path $WorkDirectory 'disc'
    Initialize-IsoExtractor
    [Pds.Tools.Iso9660Extractor]::Extract(
        $layout.Path,
        $outputDirectory,
        $layout.TrackStartBytes,
        $layout.SectorSize,
        $layout.UserDataOffset,
        $layout.VolumeSectors
    )
    $gameDataRoot = Get-GameDataRoot $outputDirectory
    if ($null -eq $gameDataRoot) {
        throw "Disc image '$($Image.FullName)' does not contain COMMON.DAT."
    }
    return $gameDataRoot
}

function Get-ImageCandidate {
    param([string]$Root)

    $images = @(
        Get-ChildItem -LiteralPath $Root -File -Recurse |
            Where-Object {
                $script:ImageExtensions -contains $_.Extension.ToLowerInvariant()
            } |
            Sort-Object `
                @{ Expression = { Get-DiscSourceRank $_ } }, `
                @{ Expression = { $_.FullName } }
    )
    if ($images.Count -eq 0) {
        return $null
    }
    return $images[0]
}

function Materialize-GameData {
    param(
        [IO.FileInfo]$Source,
        [string]$WorkDirectory
    )

    $extension = $Source.Extension.ToLowerInvariant()
    if ($script:ArchiveExtensions -contains $extension) {
        $archiveDirectory = Join-Path $WorkDirectory 'archive'
        Expand-WithTar $Source.FullName $archiveDirectory

        $looseDataRoot = Get-GameDataRoot $archiveDirectory
        if ($null -ne $looseDataRoot) {
            return $looseDataRoot
        }

        $image = Get-ImageCandidate $archiveDirectory
        if ($null -eq $image) {
            throw (
                "Archive '$($Source.FullName)' contains neither extracted " +
                'CD1 data nor a supported CUE, BIN, or ISO image.'
            )
        }
        return Expand-DiscImage $image $WorkDirectory
    }

    if ($script:ImageExtensions -contains $extension) {
        return Expand-DiscImage $Source $WorkDirectory
    }

    throw "Unsupported game data source: $($Source.FullName)"
}

function Write-SourceManifest {
    param(
        [string]$GameDataRoot,
        [IO.FileInfo]$Source
    )

    $manifest = [ordered]@{
        preparation_version = $script:PreparationVersion
        source_name = $Source.Name
        source_size = $Source.Length
        source_last_write_utc = $Source.LastWriteTimeUtc.ToString('o')
    }
    $json = $manifest | ConvertTo-Json
    $encoding = [Text.UTF8Encoding]::new($false)
    [IO.File]::WriteAllText(
        (Join-Path $GameDataRoot '.pds-data-source.json'),
        $json,
        $encoding
    )
}

function Prepare-Source {
    param([IO.FileInfo]$Source)

    $fingerprint = Get-SourceFingerprint $Source
    $cacheTarget = Join-Path $cacheRoot $fingerprint
    $cachedDataRoot = $null
    if (Test-Path -LiteralPath $cacheTarget -PathType Container) {
        $cachedDataRoot = Get-GameDataRoot $cacheTarget
    }
    if ($null -ne $cachedDataRoot) {
        return $cachedDataRoot
    }

    [void][IO.Directory]::CreateDirectory($cacheRoot)
    if (Test-Path -LiteralPath $cacheTarget) {
        Remove-Item -LiteralPath $cacheTarget -Recurse -Force
    }

    $temporaryDirectory = Join-Path $cacheRoot (
        "$fingerprint.tmp-$PID-$([Guid]::NewGuid().ToString('N'))"
    )
    [void][IO.Directory]::CreateDirectory($temporaryDirectory)
    try {
        $preparedRoot = Materialize-GameData $Source $temporaryDirectory
        Write-SourceManifest $preparedRoot $Source
        Move-Item -LiteralPath $preparedRoot -Destination $cacheTarget
        return $cacheTarget
    }
    finally {
        if (Test-Path -LiteralPath $temporaryDirectory) {
            Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force
        }
    }
}

if ([string]::IsNullOrWhiteSpace($SourcePath)) {
    $directCommonData = Join-Path $dataRoot 'COMMON.DAT'
    if (Test-Path -LiteralPath $directCommonData -PathType Leaf) {
        Write-Output $dataRoot
        return
    }

    foreach ($directory in @(
        Get-ChildItem -LiteralPath $dataRoot -Directory |
            Where-Object {
                -not $_.FullName.Equals(
                    $cacheRoot,
                    [StringComparison]::OrdinalIgnoreCase
                )
            } |
            Sort-Object Name
    )) {
        $looseDataRoot = Get-GameDataRoot $directory.FullName
        if ($null -ne $looseDataRoot) {
            Write-Output $looseDataRoot
            return
        }
    }

    $existingCachedDataRoot = $null
    if (Test-Path -LiteralPath $cacheRoot -PathType Container) {
        foreach ($cacheEntry in @(
            Get-ChildItem -LiteralPath $cacheRoot -Directory |
                Sort-Object LastWriteTimeUtc -Descending
        )) {
            $existingCachedDataRoot = Get-GameDataRoot $cacheEntry.FullName
            if ($null -ne $existingCachedDataRoot) {
                break
            }
        }
    }

    $sources = @(
        Get-ChildItem -LiteralPath $dataRoot -File |
            Where-Object {
                $extension = $_.Extension.ToLowerInvariant()
                ($script:ImageExtensions -contains $extension) -or
                ($script:ArchiveExtensions -contains $extension)
            } |
            Sort-Object `
                @{ Expression = { Get-DiscSourceRank $_ } }, `
                @{ Expression = { $_.Name } }
    )
    $namedDiscOneSources = @(
        $sources | Where-Object {
            $_.BaseName.ToLowerInvariant() -match (
                '(^|[^a-z0-9])(disc|disk|cd)[ _.-]*0?1([^0-9]|$)'
            )
        }
    )
    if ($namedDiscOneSources.Count -gt 0) {
        $sources = $namedDiscOneSources
    }
}
else {
    $resolvedSource = [IO.Path]::GetFullPath($SourcePath)
    if (-not (Test-Path -LiteralPath $resolvedSource -PathType Leaf)) {
        throw "Game data source not found: $resolvedSource"
    }
    $sources = @([IO.FileInfo]::new($resolvedSource))
}

if ($sources.Count -eq 0) {
    if ($null -ne $existingCachedDataRoot) {
        Write-Output $existingCachedDataRoot
        return
    }
    throw (
        "No game data was found in '$dataRoot'. Add extracted CD1 files, a " +
        'BIN/CUE or ISO image, or a ZIP/RAR archive containing one.'
    )
}

$failures = [Collections.Generic.List[string]]::new()
foreach ($source in $sources) {
    try {
        $prepared = Prepare-Source $source
        Write-Output $prepared
        return
    }
    catch {
        [void]$failures.Add("$($source.Name): $($_.Exception.Message)")
        if (-not [string]::IsNullOrWhiteSpace($SourcePath)) {
            throw
        }
    }
}

throw (
    "No usable CD1 game data source was found.`n`n" +
    ($failures -join "`n`n")
)
