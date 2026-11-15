# Director's Cut: Backlot Escape Prototype — Dev Log

Epic AI MegaJam 2026 (Unreal Engine track). Theme: **Director's Cut**.

This log was written by Claude Code (Fable 5.2) during one unattended overnight session
on Saturday Nov 14, 2026, while Dylan was at work. All times are Pacific. Dylan's
brief was: build a greybox prototype of the "director escapes the studio backlot" idea,
make every decision without asking, and log what was chosen and why.

---

## Summary of the session

- **Path taken: Unreal MCP tools + C++.** The editor's MCP server came up on the first
  try (`http://127.0.0.1:8000/mcp`). It exposes editor tools only (112 toolsets: level,
  actors, Python, PIE control, input injection, screenshots, logs). There is **no
  in-editor agent or prompt input** that accepts a task, so the "hand the spec to an
  agent" option did not apply. The C++ fallback of Phase 2 was not needed, but the level
  is still built by a C++ `AGreyboxLevelBuilder` at BeginPlay, because that is the
  fastest way to make a level without hand-placing anything.
- **What MCP was used for:** creating the map, placing the builder / nav bounds /
  lights, saving, starting and stopping Play-In-Editor, pressing real keys in the
  running game, reading actor state back, and taking screenshots to check each feature.
- **What C++ was used for:** all gameplay.

---

## 17:12 — Phase 0: finding things

- **Projects folder:** `D:\O\Documents\Unreal Projects\` (contains `DirectorsCut`, `GG`,
  `Goblins`, `MegaJamSmokeTest`). None of these were modified.
- **Engines found:** `D:\Program Files\Epic Games\UE_5.8` (5.8.3) and
  `D:\Program Files\Epic Games\UE_5.9` (5.9.0 MegaJam Preview, CL 60351404).
  `C:\Program Files\Epic Games\` only has 5.5 and 5.6.
- **Engine chosen: UE 5.9.** The brief said "newest 5.x, ideally 5.8". 5.9 is the
  newest, and the existing jam project is already on 5.9, so code written here can be
  copied across without an engine change. `JamSetup.md` also says not to switch engines
  mid-jam.

### Decision: the project lives in `DirectorsCutBacklot\`, not `DirectorsCut\`

The brief asked for a new project named "DirectorsCut" in the projects folder. **A
project with that exact name already exists there** (`D:\O\Documents\Unreal
Projects\DirectorsCut`, the film-vs-fire jam repo with its own git history). The brief
also said existing projects are read-only. So:

- New folder: `D:\O\Documents\Unreal Projects\DirectorsCutBacklot\`
- The project file and C++ module are still named **DirectorsCut**
  (`DirectorsCut.uproject`, module `DirectorsCut`), as asked. That also means classes can
  be copied into the main jam project without renaming an API macro.
- Nothing inside the existing `DirectorsCut` folder was touched.

## 17:20 — Phase 0: creating the project

1. Copied `UE_5.9\Templates\TP_ThirdPerson` (the C++ Third Person template): `Source`,
   `Config`, and `Content\ThirdPerson`.
2. Renamed `TP_ThirdPerson` to `DirectorsCut` in file names and file contents, the same
   replacements the engine's own `TemplateDefs.ini` lists.
3. Copied the shared content packs the template depends on from
   `Templates\TemplateResources\High`: `Characters` (mannequins and `ABP_Unarmed`),
   `LevelPrototyping` and `Input`.
4. Added class and package redirects to `DefaultEngine.ini` so the template Blueprints
   find the renamed C++ classes.
5. **Dropped the three template variants** (Combat, Platforming, SideScrolling). They add
   ~80 source files and the StateTree plugins, and none of it is used. The base Third
   Person classes are kept in `Source/DirectorsCut/` as reference.
6. **Unreal MCP enabled the same way as the existing projects:** `ModelContextProtocol`
   and `AllToolsets` in the `.uproject` (both are engine plugins, so nothing was copied
   into `Plugins/`), plus `bAutoStartServer=True`, port 8000 in
   `Config/DefaultEditorPerProjectUserSettings.ini`. Also copied two fixes from the main
   project: the `GameFeatureData` asset manager rule (otherwise cooking fails with All
   Toolsets on) and `bThrottleCPUWhenNotForeground=False` (so the game keeps running at
   full speed while the editor is in the background).
7. Built with `Engine\Build\BatchFiles\Build.bat DirectorsCutEditor Win64 Development`.
   **First build succeeded in 37 s**, no errors.
8. `git init` on `main`, Git LFS for `*.uasset` / `*.umap`, `.gitignore` copied from the
   main project's rules, first commit. Commits are under Dylan's git identity with no AI
   trailer, as `UnrealProjectGuidelines.md` asks; AI use is disclosed here instead.
9. Launched the editor. The MCP server answered within about 40 s.

## 17:30 — Phase 1: Unreal MCP

- `initialize` returned `unreal-mcp 5.9.0-60351404` for this project.
- The server exposes four top-level tools (`list_toolsets`, `describe_toolsets`,
  `describe_tools`, `call_tool`) and 112 toolsets behind them.
- `AIAssistantEditor.AIAssistantToolset` only reports editor context. Nothing accepts a
  task prompt. **So the game was built with MCP editor tools plus C++.**
- Claude Code in this session cannot load a new MCP server mid-session, so the calls were
  made with a small PowerShell script that speaks MCP's streamable HTTP directly. A
  `.mcp.json` was also written to the project root so a Claude Code session opened in
  this folder connects on its own.

---

## 17:35 — Architecture decisions

**One tuning class instead of per-class properties.** Every tuning number lives in
`UBacklotTuningSettings` (`Source/DirectorsCut/Public/Data/`), a `UDeveloperSettings`
class. It shows up in **Project Settings > Game > Director's Cut Tuning**, every value is
`UPROPERTY(Config, EditAnywhere)`, and edits are saved to `Config/DefaultGame.ini`.
Why: guards, hook points and the terminal are spawned from C++ at BeginPlay, so there
are no placed actors or Blueprints whose defaults could be edited. A settings class is
the only way to make "tweak in the editor without recompiling" true for spawned actors.
Changes apply the next time Play is pressed.

**Tools are components on the character** (`UGrappleComponent`, `UCameraFlashComponent`,
`UTakedownComponent`, `UDirectorHealthComponent`), following the composition rule in the
guidelines. The megaphone or any new tool can be added the same way.

**Guard brain is a plain state machine in `AGuardCharacter::Tick`**, not a Behavior Tree
or State Tree. Six states, each a small function. It is easier to read, tune and debug
for a prototype, and needs no assets. AI Perception (sight) is still used as the spec
asks; it lives on `AGuardAIController` and only tells the guard "I can / cannot see the
director". The detection meter is the guard's own logic on top of that.

**Input and HUD are code only.** Input actions and the mapping context are created at
runtime in `ADirectorCharacter` (same approach as the main jam project), and the HUD is
drawn on the canvas by `ABacklotHUD`. No Blueprint or widget assets to keep in sync.

**Level is code.** `AGreyboxLevelBuilder` sits in the map and spawns every box, hook
point, guard, checkpoint and the upload terminal at BeginPlay. The game mode also spawns
a builder if a map does not have one, so pressing Play on an empty map still works.

---

## 17:40 — First playable (must-work items 1 to 6)

All of the first six items were written in one pass, because they share the tuning
class, the HUD and the level builder, then compiled together and tested one by one.
**This is a deviation from "build one item, compile, test, commit, then the next".** It
was faster, and each item still got its own test before the commit, but it means items
1 to 6 share one commit (`Add player, grapple, guards, flash and takedown`).

The first full compile had four errors, all fixed in two more builds:

- `GetWorld()->GetTimeSeconds()` returns a double; brace-initializing a `float` from it
  is a narrowing error. Used `double` locals.
- `UCharacterMovementComponent::bUseAccelerationForPaths` moved to
  `GetNavMovementProperties()->bUseAccelerationForPaths` in 5.5+.
- A local named `bIsCrouched` hid `ACharacter::bIsCrouched`. Renamed.

### How the pieces work

**1. Player (`ADirectorCharacter`).** A third person character using the template's
Quinn mannequin and `ABP_Unarmed`. Camera boom and camera are created in the
constructor. Run speed, crouch speed, jump and air control come from the tuning class in
`BeginPlay`. Crouch is a toggle (C or Left Ctrl). The mannequin has no crouch
animation, so `OnStartCrouch` squashes the mesh to 62% height; it is ugly but reads.

**2. Film reel grapple (`UGrappleComponent` + `AGrappleHookPoint`).** Every frame the
component looks at every hook point and picks the one with the smallest angle from the
center of the screen, provided it is within `GrappleRange` of the director, inside the
aim cone, and a line trace from the camera reaches it. That hook turns yellow and gets a
HUD bracket. E starts the flight: the position is interpolated from start to hook with
ease-in/ease-out, plus a sine bump of `GrappleArcHeight` for the arc. Targeting keeps
running during the flight, so E again retargets mid-air.

- *Problem:* the first version swept for collision and ended the grapple when it hit
  something. It also flung the player forward on arrival. Result: the player bounced off
  the ledge edge and landed on the street past it.
- *Fix (simplification):* the flight no longer collides at all, hook points were moved
  to sit above the spot you stand on, and the exit speed was cut to a small hop. Now the
  director always arrives and lands on the ledge. The cost is that he can pass through a
  thin ledge on the way.

**3. Guards (`AGuardCharacter` + `AGuardAIController`).** The controller owns AI
Perception with a sight sense (range and half-angle from tuning). Its only job is to
call `HandlePlayerSightChanged(true/false)` on the guard. The guard runs the rest:

- *Detection meter:* while perception sees the director, the meter fills at
  `GuardDetectionRatePerSecond`, scaled up to `GuardCloseRangeDetectionMultiplier` when
  close and down by `CrouchedDetectionMultiplier` when crouched. A crouched director
  beyond 60% of sight range is not seen at all. Running (not crouched) within
  `GuardFootstepHearingRadius` also fills the meter, even from behind.
- *States:* Patrol (walk the loop, wait at each point) → Suspicious at 35% (walk to the
  last known spot, turn on the spot) → Alert at 100% (chase, shoot, warn guards nearby)
  → Search after 4 s without line of sight → back to Patrol. Stunned and TakenDown are
  the other two.
- *Movement:* `AAIController::MoveToLocation` on a navmesh that is generated at runtime
  (`RuntimeGeneration=Dynamic`) from the spawned boxes. If a path request fails, the
  guard walks straight at the goal instead, so it never just stands there.
- *Debug cones:* a flat fan drawn with debug lines in the state color, toggled by F1.
  The first version used `DrawDebugCone` with 12 sides and it filled the screen with
  lines; the fan is much easier to read.

**4. Shooting.** An alert guard with line of sight inside `GuardShootingRange` stops and
winds up for `GuardShotWindupSeconds`. A laser (a stretched glowing box) tracks the
director in orange, then freezes and turns red for the last `GuardAimLockSeconds`. The
shot hits if the frozen line passes within `GuardShotHitRadius` of the director, so
moving sideways during the red phase dodges it. Health is 3 hits with a 0.6 s grace
after each, and comes back one point at a time after 6 s without a guard aiming at you.

**5. Camera flash (`UCameraFlashComponent`).** Q stuns every guard inside the cone and
range that has a clear line to the director. A point light pops for an instant and the
HUD whites out briefly.

- *Decision:* the spec says "~45°". That is used as the **half** angle (a 90° cone)
  because a 45° total cone was hard to hit anyone with from third person. The guard
  vision cone is treated the other way: "~70°" is the full cone (35° half angle).

**6. Clapperboard takedown (`UTakedownComponent`).** F finds the nearest guard in reach
and checks the rules: stunned always works; alert never works; otherwise it works if the
guard is facing away **or the director is above him**. Success ragdolls the guard and
shows "CUT!". Failure makes the guard alert and launches the director backward.

- *Addition:* the "from above" rule is not in the spec. Zone 2 is supposed to teach
  takedowns from perches, and a ground-range check alone cannot do that, so F from up to
  9 m above a guard drops the director onto him.

### How each was tested (through the Unreal MCP, in PIE)

- Run / jump / crouch: held W for 1 s (moved 526 units), Space (airborne), C (capsule
  dropped, crouch-walk covered 239 units in 1 s).
- Grapple: aimed the camera at three hooks in turn; landed on the first fire escape,
  chained to the second.
- Guards: logged one guard's state every 0.5 s. Shown the director for 1 s: Suspicious
  → Alert → 4 s later Search → 6 s later Patrol, and it resumed its loop.
- Navmesh: path queries around the bell tower, through both doorways and across zones
  all returned valid multi-point paths.
- Shooting: stood in front of a guard; health went 3 → 2 → 1 → caught, respawned.
- Flash, takedown from behind, takedown from above, failed takedown on an alert guard
  (knockback of ~750 units/s measured): each confirmed by reading guard state back.

## 17:55 — Items 7 to 10

**7. Level (`AGreyboxLevelBuilder`).** One function per zone, each a list of boxes
(lowest corner + size + color), ledges with hook points, guards with patrol points, a
checkpoint and signs. Layout runs along +X: New York Street (0 to 5300), Western Set
(5300 to 11400), Soundstage (11400 to 16400), Editing Bay (16400 to 19200). The
soundstage roof is beams rather than a slab, so sunlight still gets in.

**8. Upload finale (`AUploadTerminal`).** E within reach starts it. Progress only
advances while the director is within `UploadMaximumPlayerDistance`. A wave timer spawns
guards at the three doors, already alert and "relentless" (they go back to hunting
instead of giving up). Wave size grows by one each time, capped per wave and by the
number still standing. Tested by temporarily setting the duration to 3 s (win screen
appeared) and by standing still at full duration (first wave at 4 s, caught at 10.5 s,
upload reset to 0 and the wave guards were removed).

**9. HUD (`ABacklotHUD`).** All canvas drawing. Text is outlined in black after the
first screenshot showed white text vanishing against the sky.

**10. Lose / win.** Health 0 → `ABacklotGameMode::HandlePlayerCaught` → next tick the
director is moved to the last checkpoint with full health, living guards go back to
their first patrol point, and any upload is reset. The one-tick delay is deliberate: the
call arrives from inside a guard's own shot, and resetting guards mid-shot is asking for
trouble. Upload at 100% → win screen → R reloads the level.

## 18:00 — Nice-to-haves (all four)

Added only after the ten must-work items passed their checks.

- **Megaphone (G)** `UMegaphoneComponent`: traces from the camera to the aimed spot;
  guards within `MegaphoneNoiseRadius` of it get `InvestigateLocation`. Tested: a
  patrolling guard turned Suspicious and walked toward the noise.
- **Grapple pull**: E while aiming at a stunned guard within `GrapplePullRange` knocks
  him out and throws his ragdoll toward the director. First version threw him at a fixed
  speed and he sailed past; now the throw is sized to land about 80% of the way.
- **Dramatic cut**: every takedown sets global time dilation to 0.25 for 0.45 real
  seconds.
- **Camera rig turret** `ACameraRigTurret`: E to mount in the Editing Bay. Mounted, the
  director cannot move; left mouse fires a spotlight that stuns the aimed guard for 2 s,
  every 0.7 s. Tested during a real wave: the aimed guard stayed stunned while the other
  one kept shooting and eventually caught the director, which is the intended trade.

## 18:05 — Polish from screenshots

- **Lighting took five tries.** With default auto exposure the scene was washed out to
  pastel; with a fixed exposure the shadowed street was black. Settled on: no height
  fog, sun pointing down the street, sky light at 7, and an unbound post process volume
  that clamps auto exposure. It is still rough.
- **Glow material.** `M_GreyboxGlow` (unlit, color × strength into emissive) was created
  through MCP Python so hook points, lasers and state markers actually glow. The C++
  falls back to the flat engine material if the asset is missing.
- **F1 conflict.** The engine binds F1 to "viewmode wireframe" in non-shipping builds, so
  the first F1 test turned the whole game to wireframe. Removed that one binding in
  `Config/DefaultInput.ini` (it lives under `[/Script/Engine.PlayerInput]`; the first
  attempt put it under the wrong section and did nothing).
- **Detection slowed** from 0.55 to 0.3 per second after measuring that a standing
  player at 800 units went from unseen to alert in under a second.
- **Measured after tuning:** standing at 1000 units = alert in 1.3 s; crouched = 3.2 s.
  Health regenerates after 6 s hidden.

## 18:10 — Final checks

- Editor target builds with no errors or warnings from project code. (One warning,
  "Plugin 'Fab' does not list plugin 'ToolsetRegistry'", is from the engine's own Fab
  plugin on 5.9.)
- Standalone run (`UnrealEditor.exe DirectorsCut.uproject -game`): loads
  `/Game/Maps/Lvl_Backlot` with `BacklotGameMode`, no errors in the log.
- No source line is over 70 characters (guideline 6); checked with a script.

## What was not done, and honest caveats

- **No human has played it.** Every check above is a script pressing keys and reading
  state. That proves the features work; it says nothing about whether the run is fun,
  fair or well paced. Tuning numbers are first guesses.
- **Not packaged.** Only the editor target and a standalone `-game` run were tested.
- **The seven judge agents, the code reviewer and the devlog producer were not run.**
  The session rules say not to spawn subagents unless asked.
- **Function length.** Most functions are within the 5 to 20 line guideline, but the
  level builder's zone functions and a few guard functions are longer.
- **Code not reviewed against every guideline rule** beyond line length, Allman braces,
  names, comments, constructor initialization and access levels.

## Declarations for the jam submission

- AI use: all code, the map, the glow material, this dev log and the README in this
  project were produced by Claude Code (Fable 5.2) in one session on Nov 14, 2026,
  following Dylan's written brief. Dylan did not edit anything during the session.
- Engine content used: UE 5.9 Third Person template (mannequins, `ABP_Unarmed`, input
  and level prototyping packs) and engine basic shapes.
- No third-party assets.
