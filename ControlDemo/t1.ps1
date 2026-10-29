Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WM3 {
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@
function Click-L($x, $y) {
    [WM3]::SetCursorPos($script:rr.L + $x, $script:rr.T + $y) | Out-Null
    Start-Sleep -Milliseconds 150
    [WM3]::mouse_event(2,0,0,0,[UIntPtr]::Zero); [WM3]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 400
}
function Click-R($x, $y) {
    [WM3]::SetCursorPos($script:rr.L + $x, $script:rr.T + $y) | Out-Null
    Start-Sleep -Milliseconds 200
    [WM3]::mouse_event(8,0,0,0,[UIntPtr]::Zero); [WM3]::mouse_event(16,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 600
}
$proc = Get-Process -Name 'ControlDemoD' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) { Write-Host "启动失败"; exit 1 }
$h = [IntPtr]$proc.MainWindowHandle
[WM3]::ShowWindow($h, 3) | Out-Null
Start-Sleep -Seconds 1
$script:rr = New-Object WM3+RECT
[WM3]::GetWindowRect($h, [ref]$script:rr) | Out-Null
Write-Host "rect ok"
# 切到第6页（窗口与嵌入）
[void](Click-L 100 330)
# 右键菜单区
[void](Click-R 500 560)
$pp = Get-Process -Name 'ControlDemoD' | Select-Object -First 1
Write-Host "弹出菜单: responding=$($pp.Responding)"
# 菜单宽180，从点击点向下弹。"打开"第一项 y+20
[void](Click-L 500 + 90 - $rNull, 0) 2>$null
