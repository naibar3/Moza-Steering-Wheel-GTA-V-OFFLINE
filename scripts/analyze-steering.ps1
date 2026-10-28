<#
.SYNOPSIS
  Summarise WheelPlugin_steering.csv: how the car's real steering angle follows the rim angle.
.PARAMETER Csv
  Path of WheelPlugin_steering.csv (in the GTA V folder).
.NOTES
  Columns: seconds, model, speed_kmh, rim_deg (your rim, + = right), steer_value (what the plugin computed, -1..1),
  input_in_game (what the game holds, read back), angle_field_deg (the game's own steering angle field, + = left),
  yaw_rate_deg_s (how fast the car turns), wheel_range, steer_lock.
#>
param([Parameter(Mandatory = $true)][string]$Csv)
$rows = Import-Csv $Csv | ForEach-Object {
    [pscustomobject]@{
        t = [double]$_.seconds; model = $_.model; v = [double]$_.speed_kmh; rim = [double]$_.rim_deg; steer = [double]$_.steer_value
        inp = [double]$_.input_in_game; ang = [double]$_.angle_field_deg; yaw = [double]$_.yaw_rate_deg_s
        range = [double]$_.wheel_range; lock = [double]$_.steer_lock
    }
}
"rows: $($rows.Count)   models: $(($rows | Select-Object -ExpandProperty model -Unique) -join ', ')"
foreach ($m in ($rows | Group-Object model)) {
    $r = $m.Group
    "`n=== model $($m.Name): $($r.Count) rows"
    $still = $r | Where-Object { $_.v -lt 3 }
    if ($still) {
        $maxAng = ($still | ForEach-Object { [math]::Abs($_.ang) } | Measure-Object -Maximum).Maximum
        $maxRim = ($still | ForEach-Object { [math]::Abs($_.rim) } | Measure-Object -Maximum).Maximum
        "standing still: max |wheel angle field| = {0:N1} deg at max |rim| = {1:N0} deg" -f $maxAng, $maxRim
        "ratio at full lock (rim deg per wheel deg): {0:N1} : 1" -f ($(if ($maxAng -gt 0.1) { $maxRim / $maxAng } else { 0 }))
    }
    "`nrim angle (deg) -> wheel angle field (deg), by speed band"
    foreach ($band in @(@(0, 3), @(3, 30), @(30, 70), @(70, 120), @(120, 400))) {
        $b = $r | Where-Object { $_.v -ge $band[0] -and $_.v -lt $band[1] }
        if (-not $b) { continue }
        "  speed {0}-{1} km/h ({2} rows)" -f $band[0], $band[1], @($b).Count
        foreach ($rimBin in @(45, 90, 180, 270, 360, 450)) {
            $s = $b | Where-Object { [math]::Abs([math]::Abs($_.rim) - $rimBin) -lt 20 }
            if ($s) {
                $avg = ($s | ForEach-Object { [math]::Abs($_.ang) } | Measure-Object -Average).Average
                $inp = ($s | ForEach-Object { [math]::Abs($_.inp) } | Measure-Object -Average).Average
                "    rim ~{0,3} deg -> game input {1:N2}, wheel angle {2:N1} deg (ratio {3:N1}:1)" -f $rimBin, $inp, $avg, ($(if ($avg -gt 0.05) { $rimBin / $avg } else { 0 }))
            }
        }
    }
    $diff = $r | Where-Object { [math]::Abs($_.steer) -lt 1 } | ForEach-Object { [math]::Abs($_.inp - $_.steer) } | Measure-Object -Maximum -Average
    "`nwritten vs read back input: max difference {0:N3}, average {1:N3} (large = the game overwrites the value)" -f $diff.Maximum, $diff.Average
}
