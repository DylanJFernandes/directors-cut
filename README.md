# Director's Cut: Backlot Escape (greybox prototype)

A film director escapes a movie studio's backlot with the only copy of his cut, using
director's tools against armed studio guards, then uploads the film while defending the
Editing Bay. Built for the Epic AI MegaJam 2026 (Unreal Engine track) to answer one
question: **is this core loop fun?** Greybox only: no art, no audio, no menus.

- Engine: **UE 5.9 MegaJam Preview** (`D:\Program Files\Epic Games\UE_5.9`)
- This is a separate project from the main jam repo in `..\DirectorsCut`. See
  `DEVLOG.md` for why it lives in `DirectorsCutBacklot\`.

## How to open and play

1. Double-click `DirectorsCut.uproject` (or open it from the UE 5.9 launcher).
2. The editor opens on `Content/Maps/Lvl_Backlot`. The map looks almost empty in the
   editor. That is expected: the whole level is spawned when the game starts.
3. Press **Play** (Alt+P). Click in the viewport once if the mouse is not captured.

The same map is the game default map, so a standalone or packaged run goes straight
into the game too.

To rebuild the C++ with the editor closed:

```
"D:\Program Files\Epic Games\UE_5.9\Engine\Build\BatchFiles\Build.bat" DirectorsCutEditor Win64 Development -Project="D:\O\Documents\Unreal Projects\DirectorsCutBacklot\DirectorsCut.uproject" -WaitMutex
```

## Controls

| Key | Action |
|---|---|
| W A S D | Move |
| Mouse | Look |
| Space | Jump |
| C or Left Ctrl | Crouch toggle. Crouched = silent, seen from less far, detected more slowly |
| E | Film reel grapple to the hook point marked yellow (the one nearest screen center). Works in mid-air and mid-grapple |
| E (aiming at a stunned guard) | Reel him in: instant takedown from range |
| E (at the upload terminal) | Start the upload |
| E (at the camera rig) | Mount / get off |
| Q | Camera flash: stuns every guard in a cone in front of you for 3 s. 8 s cooldown |
| F | Clapperboard takedown. Works on a guard who is stunned, or not alert and facing away, or not alert and below you |
| G | Megaphone: throws a noise to where you aim; nearby guards walk over to look |
| Left mouse (on the camera rig) | Spotlight: stuns the guard you aim at |
| F1 | Toggle guard vision cones |
| R | Restart (on the win screen) |

The controls are also listed on screen while you are in Zone 1.

## The level

Three zones in a straight line, then the finale room. Each zone start is a checkpoint.

1. **New York Street**: tall facades with fire escapes, 2 guards. A barricade forces the
   first grapple.
2. **Western Set**: wide street, balconies on both sides, a bell tower, 4 guards.
3. **Soundstage**: interior with a catwalk cross, 3 guards.
4. **Editing Bay**: upload terminal, 3 red guard doors, low cover, two perches, and a
   camera rig.

Reading the guards: the ball above a guard's head and his vision cone are **green**
(patrolling), **yellow** (suspicious), **orange** (searching), **red** (alert) or
**pale blue** (stunned). The bar above him is his detection meter. Before he shoots, a
laser tracks you (orange), then locks (red) for a moment: move sideways when it turns
red.

## What is finished

All ten must-work items are in and were checked in Play-In-Editor through the Unreal
MCP (real key presses, state read back, screenshots):

| # | Item | Status |
|---|---|---|
| 1 | Run, jump, crouch | Done |
| 2 | Film reel grapple with target marker, arc flight, mid-air chaining | Done |
| 3 | Guards: patrol, AI Perception sight, detection meter, Patrol / Suspicious / Alert / Search, F1 cones | Done |
| 4 | Hitscan shots with laser telegraph, 3 health, slow regeneration | Done |
| 5 | Camera flash (cone stun, cooldown) | Done |
| 6 | Clapperboard takedown, "CUT!" popup, failure knockback on alert guards | Done |
| 7 | Three-zone greybox level built in C++ | Done |
| 8 | Upload finale: 90 s bar, growing waves from 3 doors, pauses beyond 800 units, win | Done |
| 9 | Canvas HUD: health, flash cooldown, zone name, upload %, detection meters | Done |
| 10 | Caught = back to last checkpoint; win screen; R restarts; controls on screen | Done |

Nice-to-haves, all four in:

- **Megaphone (G)**
- **Grapple pull**: E while aiming at a stunned guard
- **Camera rig turret** in the Editing Bay
- **Slow-motion "dramatic cut"** on takedowns

## What is stubbed or simplified

- **No crouch animation.** The mannequin's animation Blueprint has none, so the mesh is
  squashed while crouched.
- **No takedown, flash or shooting animations.** Guards ragdoll when taken down.
- **Grapple passes through thin geometry.** The flight does not collide, so the director
  always arrives. The target must be visible from the camera, not from his body.
- **Guard "weapons" are invisible**; only the laser shows.
- **Wave guards appear inside the room at the red doors**; the doors do not open.
- **Lighting is rough**, with auto exposure clamped by a post process volume.
- **Vision cones use debug drawing**, so they do not show in a Shipping build.
- **The template's Third Person classes** (`Source/DirectorsCut/*.h/.cpp`) and
  `Content/ThirdPerson` are unused leftovers from the template. Safe to delete.

## Known bugs and rough edges

- **Nobody has played this by hand yet.** Every feature was checked by script, one at a
  time. Pacing, difficulty and the feel of a full run are untested. Expect to retune.
- The finale is probably **too hard if you stand still** (two guards drop you in about
  7 s) and may be too easy from the camera rig. See the tuning list.
- Guards chase across zone boundaries and can follow you into the next zone.
- Guards warn each other when one goes alert, which can chain through a zone.
- Being caught resets living guards to their first patrol point; guards you took down
  stay down.
- Zone names change at the checkpoint trigger, so walking backward through one shows the
  wrong name until the next trigger.
- A reeled-in guard's ragdoll sometimes lands short or bounces past you.
- Signs are one-sided text and read mirrored from behind.
- Engine build warning "Plugin 'Fab' does not list plugin 'ToolsetRegistry'" comes from
  the engine's own Fab plugin on 5.9, not from this project.

## Test this first (10 minutes)

1. **Grapple feel.** From the start, look up at the fire escape, press E. Then chain to
   the next hook before you land. Does the arc feel good? Too slow, too floaty?
2. **Stealth read.** Drop past the barricade, crouch, and watch a guard's meter fill.
   Can you tell why you were seen?
3. **Flash into cut.** Run at a guard, Q, then F. Does the slow-motion "CUT!" land?
4. **Reel-in.** Flash two guards from range, then E on one while aiming at him.
5. **Perch takedown.** In the Western Set, grapple to a balcony, wait for a guard to walk
   under you, press F.
6. **Getting shot.** Let a guard go alert and try to dodge the red laser by strafing.
   Fair or cheap?
7. **Megaphone.** From a rooftop, G at the far end of the street and watch them go.
8. **Finale.** Start the upload. Try it once on foot, once from the camera rig. Is 90
   seconds tense or tedious?

To jump straight to the finale while testing: play, press the console key (`` ` ``) and
type `ghost`, run straight through the walls to the Editing Bay (W, the level is a
straight line), then type `walk`.

## The 5 tuning values most likely to make it more fun

All of these are in **Project Settings > Game > Director's Cut Tuning** (saved to
`Config/DefaultGame.ini`). No recompile; changes apply the next time you press Play.

| # | Value | Now | Try | Why it matters |
|---|---|---|---|---|
| 1 | `GrappleTravelSpeed` | 2400 | 1800 to 3600 | The grapple is the star. Faster feels snappier and more Arkham; slower gives time to pick the next hook mid-flight. Pair with `GrappleArcHeight` (220; try 100 to 400). |
| 2 | `GuardDetectionRatePerSecond` | 0.3 | 0.15 to 0.5 | How forgiving stealth is. At 0.3 a standing player at mid range is spotted in about 1.3 s. Lower values give time to react to a yellow meter. Pair with `CrouchedDetectionMultiplier` (0.35). |
| 3 | `GuardShotWindupSeconds` | 1.0 | 0.6 to 1.5 | How long the laser warns you. With `GuardAimLockSeconds` (0.35; try 0.2 to 0.6) this decides whether dodging is skill or luck. |
| 4 | `FlashCooldownSeconds` | 8 | 4 to 12 | How often you get your strongest tool. Short makes combat a flash-cut rhythm; long makes it a panic button. Pair with `FlashStunSeconds` (3). |
| 5 | `UploadWaveIntervalSeconds` | 16 | 10 to 25 | Pressure in the finale. With `UploadFirstWaveGuardCount` (2), `UploadWaveGuardGrowth` (1) and `UploadDurationSeconds` (90) this sets how many guards you face in total (6 waves and about 24 guards at the defaults, never more than `UploadMaximumLivingGuards` = 7 standing at once). |

Runners-up: `PlayerRunSpeed` (600), `GuardChaseSpeed` (430), `TakedownRange` (230),
`UploadMaximumPlayerDistance` (800), `TurretStunSeconds` (2).

## Where things are

```
Source/DirectorsCut/
  Public|Private/
    Data/BacklotTuningSettings      every tuning number
    Character/DirectorCharacter     player: input, camera, owns the tool components
    Character/GuardCharacter        guard state machine, detection, shooting
    AI/GuardAIController            AI Perception sight
    Component/GrappleComponent      hook targeting, arc flight, reel-in
    Component/CameraFlashComponent  cone stun and cooldown
    Component/TakedownComponent     clapperboard rules, CUT! feedback
    Component/MegaphoneComponent    noise lure
    Component/DirectorHealthComponent  3 hits, regeneration
    Component/BeamMeshComponent     laser / tether / spotlight line
    Actor/GreyboxLevelBuilder       the whole level, spawned at BeginPlay
    Actor/GrappleHookPoint          glowing hook sphere
    Actor/ZoneCheckpoint            zone name + restart spot
    Actor/UploadTerminal            upload bar and guard waves
    Actor/CameraRigTurret           mountable spotlight
    Game/BacklotGameMode            checkpoints, caught, win, restart
    Game/BacklotGameState           zone name, upload progress, won flag
    Player/DirectorPlayerController R and F1, HUD access
    UI/HUD/BacklotHUD               everything on screen
Content/Maps/Lvl_Backlot            builder actor, nav bounds, sun, sky, exposure
Content/Materials/M_GreyboxGlow     self-lit color for hooks, lasers, markers
```
