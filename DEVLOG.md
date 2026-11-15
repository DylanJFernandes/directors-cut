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
