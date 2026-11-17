$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
$owner = 'vurdich'
$repo = 'NeoNyx'
$currentVersion = [version]$args[0]
try {
    $headers = @{ 'User-Agent' = 'NeoNyx-Updater'; 'Accept' = 'application/vnd.github+json' }
    $release = Invoke-RestMethod -Uri "https://api.github.com/repos/$owner/$repo/releases/latest" -Headers $headers -TimeoutSec 15
    $latestText = ([string]$release.tag_name).TrimStart('v')
    $latestVersion = [version]$latestText
    if ($latestVersion -le $currentVersion) { exit 0 }
    $asset = $release.assets | Where-Object { $_.name -eq 'NeoNyx_Setup.exe' } | Select-Object -First 1
    if (-not $asset) { exit 0 }
    $answer = [System.Windows.Forms.MessageBox]::Show("Доступна новая версия NeoNyx $latestText. Установить обновление сейчас?", 'Обновление NeoNyx', [System.Windows.Forms.MessageBoxButtons]::YesNo, [System.Windows.Forms.MessageBoxIcon]::Information)
    if ($answer -ne [System.Windows.Forms.DialogResult]::Yes) { exit 0 }
    $installer = Join-Path $env:TEMP 'NeoNyx_Update_Setup.exe'
    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $installer -Headers $headers -TimeoutSec 180
    Start-Process -FilePath $installer -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CLOSEAPPLICATIONS' -Wait
} catch {
    # Updates are optional; do not interrupt normal application startup.
    exit 0
}
