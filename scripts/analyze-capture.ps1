<#
.SYNOPSIS
  Print a readable summary of a WheelPlugin_capture_N.csv (saved with the capture key in the game).
.PARAMETER Csv
  Path of the capture file.
.PARAMETER Every
  Print every Nth frame (default 12, about 5 rows per second at 60 fps).
.PARAMETER Gear
  Only frames where this gear is engaged (e.g. 1). Default: all frames.
#>
param(
    [Parameter(Mandatory = $true)][string]$Csv,
    [int]$Every = 12,
    [int]$Gear = [int]::MinValue
)
$rows = Import-Csv $Csv
if (-not $rows) { throw "empty file" }
"frames: {0}   span: {1}s .. {2}s" -f $rows.Count, $rows[0].seconds_before_mark, $rows[-1].seconds_before_mark
"engaged gears: " + (($rows | Group-Object engaged_gear | Sort-Object { [int]$_.Name } | ForEach-Object { "$($_.Name):$($_.Count)" }) -join "  ")
$speed = $rows | ForEach-Object { [double]$_.speed_ms }
"max speed {0:N1} m/s, max rpm {1:N2}" -f ($speed | Measure-Object -Maximum).Maximum, (($rows | ForEach-Object { [double]$_.rpm }) | Measure-Object -Maximum).Maximum

# frames where the game fights the plugin: it wanted another gear, or it overwrote the clutch we had written
$fight = $rows | Where-Object { $_.engaged_gear -ne "99" -and $_.engaged_gear -ne "0" -and ([double]$_.game_clutch_before_write -lt 0.5) -and ([double]$_.clutch_written -ge 0.99) }
"frames where the game set the clutch below 0.5 although the plugin wrote ~1.0: $(@($fight).Count)"
$next = $rows | Where-Object { $_.engaged_gear -ne "99" -and $_.engaged_gear -ne "0" -and $_.game_next_gear_before -ne $_.engaged_gear }
"frames where the game's own next gear differs from the engaged gear: $(@($next).Count)"
""
$i = 0
$rows | ForEach-Object {
    if (($Gear -eq [int]::MinValue -or [int]$_.engaged_gear -eq $Gear) -and ($i % $Every -eq 0)) {
        "{0,7}s spd={1,5} rpm={2,5} thr={3}->{4} brk={5} clPed={6} clOut={7,5} sh={8,2} eng={9,2} run={10} gameClutch={11,6} gNext={12} gCur={13}" -f `
            $_.seconds_before_mark, $_.speed_ms, $_.rpm, $_.throttle_raw, $_.throttle_after_logic, $_.brake, $_.clutch_pedal, $_.clutch_written,
            $_.shifter_gear, $_.engaged_gear, $_.engine_running, $_.game_clutch_before_write, $_.game_next_gear_before, $_.game_cur_gear_before
    }
    $i++
}
