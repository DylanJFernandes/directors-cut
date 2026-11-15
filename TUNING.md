# Tuning: the 5 values most likely to make the game more fun

Edit these in the editor at **Project Settings > Game > Director's Cut Tuning**. They
are saved to `Config/DefaultGame.ini`. No recompile is needed; changes apply the next
time you press Play. Defaults live in
`Source/DirectorsCut/Private/Data/BacklotTuningSettings.cpp`.

| # | Value | Default | Suggested range | What it changes |
|---|---|---|---|---|
| 1 | `GrappleTravelSpeed` | 2400 units/s | 1800 to 3600 | How fast the film reel carries you. The grapple is the main verb, so this sets the feel of the whole game. Higher is snappier; lower gives time to line up the next hook mid-flight. |
| 2 | `GuardDetectionRatePerSecond` | 0.3 | 0.15 to 0.5 | How quickly guards notice you. Measured at the default: standing at 1000 units = alert in 1.3 s, crouched = 3.2 s. Lower it if stealth feels like a coin flip. |
| 3 | `GuardShotWindupSeconds` | 1.0 s | 0.6 to 1.5 | How long the laser warns you before a shot. Tune together with `GuardAimLockSeconds` (0.35 s, try 0.2 to 0.6), the window in which sidestepping dodges. |
| 4 | `FlashCooldownSeconds` | 8 s | 4 to 12 | How often you get the flash. Around 4 the game becomes flash-then-cut on repeat; around 12 the flash is an emergency button and stealth carries the run. |
| 5 | `UploadWaveIntervalSeconds` | 16 s | 10 to 25 | Pressure in the finale. At the default there are 6 waves (2, 3, 4, 5, 5, 5 guards) in the 90 s upload, with at most 7 standing at once. |

## Close runners-up

| Value | Default | Suggested range | Note |
|---|---|---|---|
| `GrappleArcHeight` | 220 | 100 to 400 | Flat zip line vs. big swing. |
| `GrappleAimConeHalfAngleDegrees` | 30 | 15 to 45 | How precisely you must look at a hook. |
| `CrouchedDetectionMultiplier` | 0.35 | 0.2 to 0.6 | How much crouching helps. |
| `GuardChaseSpeed` | 430 | 350 to 550 | Player runs at 600. Above ~550 you cannot escape on foot. |
| `FlashStunSeconds` | 3 | 2 to 5 | Time to reach a stunned guard and cut. |
| `TakedownRange` | 230 | 180 to 320 | Generosity of the F prompt. |
| `UploadDurationSeconds` | 90 | 45 to 120 | Length of the finale. |
| `UploadMaximumPlayerDistance` | 800 | 500 to 1200 | How far you may roam during the upload. |
| `TurretStunSeconds` | 2 | 1 to 3 | Strength of the camera rig. |
| `PlayerMaximumHealth` | 3 | 2 to 5 | Hits before you are caught. |
