# PPSSPP MultiSplit: autorizacion voluntaria y restringida al propio equipo.
# No desactiva Windows Defender Firewall y no permite conexiones desde la red.
param([string]$OriginalLocalAppData = $env:LOCALAPPDATA)
$ErrorActionPreference = 'Stop'
$pauseAtEnd = $true
try {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        $args = '-NoProfile -ExecutionPolicy Bypass -File "' + $PSCommandPath + '" -OriginalLocalAppData "' + $OriginalLocalAppData + '"'
        Start-Process -FilePath 'powershell.exe' -ArgumentList $args -Verb RunAs -ErrorAction Stop
        $pauseAtEnd = $false
        return
    }

    $workerSource = Join-Path $PSScriptRoot 'psp_worker.exe'
    if (-not (Test-Path -LiteralPath $workerSource -PathType Leaf)) {
        throw 'No se encuentra psp_worker.exe junto al instalador.'
    }

    $installFolder = Join-Path $OriginalLocalAppData 'PPSSPP-MultiSplit'
    New-Item -ItemType Directory -Force -Path $installFolder | Out-Null
    $workerPath = Join-Path $installFolder 'psp_worker.exe'
    Copy-Item -LiteralPath $workerSource -Destination $workerPath -Force

    $ruleName = 'PPSSPP MultiSplit - Ad Hoc local (127.0.0.0/8)'
    $existing = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue
    if ($existing) { $existing | Remove-NetFirewallRule -ErrorAction Stop }
    New-NetFirewallRule -DisplayName $ruleName -Description 'Solo conexiones entrantes locales para el multijugador Ad Hoc en el mismo PC.' `
        -Program $workerPath -Direction Inbound -Action Allow -Enabled True -Profile Any `
        -Protocol Any -RemoteAddress '127.0.0.0/8', '::1' -ErrorAction Stop | Out-Null
    Write-Host ''
    Write-Host 'PERMISO LOCAL INSTALADO CORRECTAMENTE' -ForegroundColor Green
    Write-Host ('Aplicacion: ' + $workerPath)
    Write-Host 'Solo origen 127.0.0.0/8 y ::1 (este mismo ordenador).'
    Write-Host 'No se ha desactivado el firewall y no hay una excepcion para Internet.'
    Write-Host 'Ya puedes iniciar el nucleo desde RetroArch.'
} catch {
    Write-Host ('ERROR: ' + $_.Exception.Message) -ForegroundColor Red
    exit 1
} finally {
    if ($pauseAtEnd) { Read-Host 'Pulsa Intro para cerrar esta ventana' | Out-Null }
}
