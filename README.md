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
| W A S D | Walk |
| Mouse | Look |
| Left Shift (hold) | Sprint. Guards nearby hear sprinting footsteps |
| Space | Jump. Pressed during a slide it keeps the slide's speed |
| C or Left Ctrl | Crouch toggle. Crouched = silent, seen from less far, detected more slowly |
| C while sprinting | Slide. Also works in the air: press C during a sprint jump and you slide when you land |
| E | Film reel grapple to the hook point marked yellow (the one nearest screen center). 1.25 s cooldown from launch |
| E (aiming at a guard) | Zip to him and keep the speed. If he is stunned you cut him on arrival |
| E (at the upload terminal) | Start the upload |
| Q | Camera flash: stuns every guard in a cone in front of you for 3 s. 8 s cooldown |
| F | Clapperboard takedown. Instant on a guard who is stunned, or unaware and facing away, or unaware and below you. On a guard who sees you it takes 1.2 s, during which you cannot move |
| G | Megaphone: shouts from where you stand; guards within earshot walk over to look. 5 s cooldown |
| Left mouse (hold) | Fire the tommy gun, once you have picked it up in the Editing Bay |
| F1 | Toggle guard vision cones |
| R | Restart (on the win screen) |

The controls are also listed on screen while you are in Zone 1.

### Movement

- **Walk 420, sprint 780, slide starts at 1100** (units per second). A slide loses 650
  per second and ends at 380, so it covers about 700 units.
- **Chaining slides**: slide, jump, press C in the air, slide again. Each chained slide
  starts 15% slower than the last (1100, 935, 795), so two in a row is strong and a
  third gives almost nothing. The penalty resets after 1.5 s without sliding.
- **Sliding off an edge** adds 350 forward speed.
- **Gravity is twice the engine default** and the jump is stronger to match: the jump
  now peaks at 163 units and lasts 0.83 s (was 196 units and 1.27 s).

## The level

Three zones in a straight line, then the finale room. Each zone start is a checkpoint.

1. **New York Street**: tall facades with fire escapes, 2 guards. A barricade forces the
   first grapple.
2. **Western Set**: wide street, balconies on both sides, a bell tower, 4 guards.
3. **Soundstage**: interior with a catwalk cross, 3 guards.
4. **Editing Bay**: the floating tommy gun, upload terminal, 3 red guard doors, low
   cover and two perches.

Reading the guards: the ball above a guard's head and his vision cone are **green**
(patrolling), **yellow** (suspicious), **orange** (searching), **red** (alert) or
**pale blue** (stunned). The bar above him is his detection meter. Before he shoots, a
laser tracks you (orange), then locks (red) for 0.2 s. A guard now fires about once a
second, so keep moving: standing still in front of one guard costs all three health in
about three seconds.

## What is finished

All ten must-work items from the first brief are in. The second round of changes
(Nov 15) is in too, each checked in Play-In-Editor through the Unreal MCP (real key
presses, state read back, screenshots):

| Change | Status |
|---|---|
| Grapple cooldown, tunable with a slider | Done |
| Takedown works on guards who see you, but takes time | Done |
| Megaphone shouts from where you stand, with a sound, a popup and spreading rings | Done |
| Camera rig turret removed | Done |
| Floating tommy gun in the Editing Bay, film reels for barrel and drum | Done |
| Guards fire faster and take 6 tommy gun hits | Done |
| Walk and sprint | Done |
| Slide, chained slides, edge boost | Done |
| Grappling a guard pulls you to him and keeps the speed | Done |
| Heavier gravity | Done |

Kept from the first round: camera flash, checkpoints, the 90 s upload finale with
waves, the Canvas HUD, win screen and R to restart, and the slow-motion "dramatic cut".

## What is stubbed or simplified

- **No crouch animation.** The mannequin's animation Blueprint has none, so the mesh is
  squashed while crouched.
- **No takedown, flash or shooting animations.** Guards ragdoll when taken down.
- **Grapple passes through thin geometry.** The flight does not collide, so the director
  always arrives. The target must be visible from the camera, not from his body.
- **Guard guns are a black box** held in front of them.
- **The four sounds are synthesized placeholders** (megaphone, tommy gun, guard shot,
  clapperboard) in `Content/Audio`. Nothing else makes a sound.
- **The tommy gun has no ammo or reload** and you keep it after being caught.
- **Zipping to a guard does not hurt him** unless he is stunned. It is a movement tool.
- **Wave guards appear inside the room at the red doors**; the doors do not open.
- **Lighting is rough**, with auto exposure clamped by a post process volume.
- **Vision cones use debug drawing**, so they do not show in a Shipping build.
- **The template's Third Person classes** (`Source/DirectorsCut/*.h/.cpp`) and
  `Content/ThirdPerson` are unused leftovers from the template. Safe to delete.

## Known bugs and rough edges

- **Nobody has played this by hand yet.** Every feature was checked by script, one at a
  time. Pacing, difficulty and the feel of a full run are untested. Expect to retune.
- **Nobody has played the new movement by hand either.** The numbers were measured by
  script; whether the slide and the heavier gravity feel right is yours to judge.
- **The finale balance is a guess.** Guards now hit about once a second, and the tommy
  gun drops one in about 0.6 s of fire on target. It was not tested past the first
  wave.
- The slow takedown is not interrupted when another guard shoots you; you just take
  the hit.
- A guard held in a slow takedown shows "STUNNED", so the label does not tell the two
  apart.
- The grapple cooldown starts at launch, so chaining hook to hook now includes a short
  fall. Set `GrappleCooldownSeconds` to 0 to get the old chaining back.
- Guards chase across zone boundaries and can follow you into the next zone.
- Guards warn each other when one goes alert, which can chain through a zone.
- Being caught resets living guards to their first patrol point; guards you took down
  stay down.
- Zone names change at the checkpoint trigger, so walking backward through one shows the
  wrong name until the next trigger.
- Signs are one-sided text and read mirrored from behind.
- Engine build warning "Plugin 'Fab' does not list plugin 'ToolsetRegistry'" comes from
  the engine's own Fab plugin on 5.9, not from this project.

## Test this first (10 minutes)

1. **Movement.** Sprint (Shift), jump, press C in the air, slide on landing, jump out,
   slide again. Does it feel quick and nimble? Is gravity right now?
2. **Edge boost.** Grapple to a fire escape, sprint along it, slide off the end.
3. **Grapple cooldown.** Chain two hooks. Is 1.25 s too long between them?
4. **Zip to a guard.** Aim at a guard, E. You fly to him and past him with speed. Then
   try Q first, E second: flash, zip, cut.
5. **Slow takedown.** Let a guard see you, run up, F. You are stuck for 1.2 s. Try it
   with a second guard watching.
6. **Megaphone.** Press G near two guards, then grapple away and watch them walk to
   where you were. Can you hear and see it?
7. **Getting shot.** Stand in front of one alert guard. Fair or cheap at this rate?
8. **Finale.** Walk into the tommy gun, start the upload, hold the left mouse button.
   Is 90 seconds tense or tedious? Do you still want the flash and the clapperboard?

To jump straight to the finale while testing: play, press the console key (`` ` ``) and
type `ghost`, run straight through the walls to the Editing Bay (W, the level is a
straight line), then type `walk`.

## The 5 tuning values most likely to make it more fun

All of these are in **Project Settings > Game > Director's Cut Tuning** (saved to
`Config/DefaultGame.ini`). No recompile; changes apply the next time you press Play.
`TUNING.md` has the full list with runners-up.

| # | Value | Now | Try | Why it matters |
|---|---|---|---|---|
| 1 | `GrappleCooldownSeconds` (slider) | 1.25 | 0 to 3 | The anti-spam cooldown you asked for. It starts when a grapple launches. 0 turns it off. |
| 2 | `SlideStartSpeed` | 1100 | 900 to 1400 | How much of a burst the slide is. Pair with `SlideDeceleration` (650; try 400 to 900) for how far it carries and `SlideChainSpeedMultiplier` (0.85) for how well slides chain. |
| 3 | `PlayerGravityScale` (slider) | 2.0 | 1.5 to 3 | How heavy the director feels. Pair with `PlayerJumpVelocity` (800): raising gravity alone shrinks the jump. |
| 4 | `GuardShotCooldownSeconds` | 0.45 | 0.2 to 1.2 | Guard fire rate. With `GuardShotWindupSeconds` (0.55) a guard fires about once a second. This is the "is open combat a bad idea" dial. |
| 5 | `AlertTakedownSeconds` (slider) | 1.2 | 0.6 to 2.5 | How long you are exposed while cutting a guard who sees you. |

Runners-up: `PlayerSprintSpeed` (780), `GuardMaximumHealth` (6),
`TommyGunFireIntervalSeconds` (0.09), `GrappleGuardExitForwardSpeed` (950),
`GuardDetectionRatePerSecond` (0.3), `UploadWaveIntervalSeconds` (16).

## Where things are

```
Source/DirectorsCut/
  Public|Private/
    Data/BacklotTuningSettings      every tuning number
    Character/DirectorCharacter     player: input, camera, owns the tool components
    Character/GuardCharacter        guard state machine, detection, shooting
    AI/GuardAIController            AI Perception sight
    Component/DirectorLocomotionComponent  sprint, slide, edge boost
    Component/GrappleComponent      hook and guard targeting, arc flight, cooldown
    Component/TommyGunComponent     hitscan spray, tracer
    Component/CameraFlashComponent  cone stun and cooldown
    Component/TakedownComponent     clapperboard rules, slow takedown, CUT! feedback
    Component/MegaphoneComponent    shout that draws guards over
    Component/DirectorHealthComponent  3 hits, regeneration
    Component/BeamMeshComponent     laser / tether / spotlight line
    Actor/GreyboxLevelBuilder       the whole level, spawned at BeginPlay
    Actor/GrappleHookPoint          glowing hook sphere
    Actor/ZoneCheckpoint            zone name + restart spot
    Actor/UploadTerminal            upload bar and guard waves
    Actor/TommyGunPickup            floating gun in the Editing Bay
    Game/BacklotGameMode            checkpoints, caught, win, restart
    Game/BacklotGameState           zone name, upload progress, won flag
    Game/BacklotSoundLibrary        plays the placeholder sounds
    Game/TommyGunShape              greybox gun built from cubes and reels
    Player/DirectorPlayerController R and F1, HUD access
    UI/HUD/BacklotHUD               everything on screen
Content/Maps/Lvl_Backlot            builder actor, nav bounds, sun, sky, exposure
Content/Materials/M_GreyboxGlow     self-lit color for hooks, lasers, markers
Content/Audio/S_*                   four synthesized placeholder sounds
```
