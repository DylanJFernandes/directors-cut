# Tuning: the 5 values most likely to make the game more fun

Edit these in the editor at **Project Settings > Game > Director's Cut Tuning**. They
are saved to `Config/DefaultGame.ini`. No recompile is needed; changes apply the next
time you press Play. Defaults live in
`Source/DirectorsCut/Private/Data/BacklotTuningSettings.cpp`. Values marked (slider)
show a drag slider in Project Settings.

| # | Value | Default | Suggested range | What it changes |
|---|---|---|---|---|
| 1 | `GrappleCooldownSeconds` (slider) | 1.25 s | 0 to 3 | Time from one grapple launching until the next can. A typical flight lasts 0.5 to 0.9 s, so at 1.25 you fall briefly between hooks. 0 turns the cooldown off. |
| 2 | `SlideStartSpeed` | 1100 units/s | 900 to 1400 | The burst a slide gives. Sprint is 780, so the default is a 40% boost. Tune with `SlideDeceleration` (650; 400 to 900): at the defaults a slide lasts 1.1 s and covers about 700 units. |
| 3 | `PlayerGravityScale` (slider) | 2.0 | 1.5 to 3 | How heavy the director feels. Measured at the default with `PlayerJumpVelocity` 800: jump peaks at 163 units, 0.83 s in the air. Raise both together to keep the jump height. |
| 4 | `GuardShotCooldownSeconds` | 0.45 s | 0.2 to 1.2 | Guard fire rate. Measured at the default with `GuardShotWindupSeconds` 0.55: one hit a second on a player standing still. `GuardAimLockSeconds` (0.2) is the window in which moving dodges. |
| 5 | `AlertTakedownSeconds` (slider) | 1.2 s | 0.6 to 2.5 | How long you are stuck while cutting a guard who sees you. |

## Close runners-up

| Value | Default | Suggested range | Note |
|---|---|---|---|
| `PlayerWalkSpeed` / `PlayerSprintSpeed` | 420 / 780 | 350 to 500 / 650 to 900 | Guards chase at 430, so walking does not escape them. |
| `SlideChainSpeedMultiplier` (slider) | 0.85 | 0.7 to 1.0 | Start speed lost per chained slide. 1.0 lets slide-hopping hold top speed forever. |
| `SlideEdgeBoostSpeed` | 350 | 200 to 600 | Bonus for sliding off a ledge. |
| `SlideMinimumEntrySpeed` | 600 | 450 to 750 | Above walk speed, so only a sprint or a grapple exit can start a slide. |
| `GrappleGuardExitForwardSpeed` | 950 | 600 to 1400 | Speed you keep after zipping to a guard. Above 600 you can slide out of it. |
| `GrappleGuardRange` | 1800 | 1200 to 2500 | How far away a guard can be zipped to. |
| `GrappleTravelSpeed` | 2400 | 1800 to 3600 | Flight speed of every grapple. |
| `GuardMaximumHealth` | 6 | 3 to 12 | Tommy gun hits per guard. At 6 a guard drops in about 0.6 s of fire on target. |
| `TommyGunFireIntervalSeconds` | 0.09 | 0.06 to 0.15 | Rate of fire. |
| `TommyGunSpreadHalfAngleDegrees` | 1.5 | 0.5 to 4 | How much it sprays. |
| `GuardDetectionRatePerSecond` | 0.3 | 0.15 to 0.5 | How quickly guards notice you. |
| `MegaphoneNoiseRadius` | 1400 | 800 to 2000 | How far the shout carries. The rings on screen grow to this size. |
| `FlashCooldownSeconds` | 8 | 4 to 12 | How often you get the flash. |
| `UploadWaveIntervalSeconds` | 16 | 10 to 25 | Pressure in the finale: 6 waves (2, 3, 4, 5, 5, 5 guards) in 90 s, at most 7 standing at once. |
| `PlayerMaximumHealth` | 3 | 2 to 5 | Hits before you are caught. |