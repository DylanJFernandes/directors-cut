# Director's Cut

**A noir stealth-action game where a film director fights his way off a
1940s studio backlot with the tools of the trade.**

Made solo in four days for the
[2026 Epic AI MegaJam](https://itch.io/jam/2026-epic-ai-megajam)
(theme: *Director's Cut*) in Unreal Engine 5.9, by Pinto Basto Games.

[![Watch the gameplay video](https://img.youtube.com/vi/x-gfw4gQ1gQ/maxresdefault.jpg)](https://youtu.be/x-gfw4gQ1gQ)

**[Gameplay video](https://youtu.be/x-gfw4gQ1gQ)** ·
**[Play on itch.io](https://dylanjfernandes.itch.io/directors-cut)** ·
**[Jam entry](https://itch.io/jam/2026-epic-ai-megajam/rate/5376566)** ·
**[Download the Windows build](../../releases/latest)**

## The pitch

Every great film needs a final cut. Yours is made with a clapperboard.

You're a director with a reputation for getting the shot, and a second
career nobody at the studio knows about. Tonight the studio's guards are on
to you, the only copy of your cut is in your hands, and the backlot is the
only way out.

Come up out of the subway onto a rain-soaked New York street set, slip or
fight past the guards, grapple up the face of Metropolis Tower, and hold
the roof while your film uploads.

## Features

- **A director's toolkit as weapons.** A film-reel grapple, a camera flash
  that stuns a cone of guards, clapperboard takedowns, and a megaphone that
  lures guards away from your real route.
- **Fast, weighty movement.** Sprint, slide, chain slides out of jumps, and
  carry extra speed off ledges.
- **Zip to a guard.** The grapple pulls you to a guard and keeps the
  momentum: flash first, then cut him on arrival.
- **Readable stealth.** Every guard has a vision cone and a detection
  meter, so you always know how much trouble you're in.
- **Risky takedowns.** A guard who hasn't seen you goes down instantly. A
  guard who has takes time, and you're exposed while you do it.
- **A vertical climb.** Ten checkpoints from the subway exit to the roof,
  with 23 grapple hooks up the tower and patrols on every setback.
- **A rooftop finale.** Defend a 90-second upload against waves of guards
  with a tommy gun built from film reels.
- **Black-and-white noir look.** Night lighting, rain, lightning, neon
  signs and a big-band soundtrack playing from speakers in the level.

## Controls

| Input | Action |
|---|---|
| W A S D / Mouse | Move / look |
| Shift (hold) | Sprint (guards can hear it) |
| Space | Jump |
| C / Left Ctrl | Crouch; C while sprinting to slide |
| E | Grapple to the highlighted hook, or zip to a guard |
| Q | Camera flash (stuns guards in front of you) |
| F | Clapperboard takedown |
| G | Megaphone |
| Left mouse | Fire the tommy gun (once picked up) |
| F1 | Toggle guard vision cones |
| R | Restart on the win screen |

## Play it

Download the Windows build from the
[latest release](../../releases/latest), unzip it and run
`DirectorsCut.exe`. Windows 10 or 11, 64-bit, keyboard and mouse.

## Build it from source

You need Unreal Engine 5.9, Visual Studio 2022 with the C++ game
development workload, and [Git LFS](https://git-lfs.com).

```bash
git lfs install
```

```bash
git clone https://github.com/DylanJFernandes/directors-cut.git
```

Then right-click `DirectorsCut.uproject`, choose **Generate Visual Studio
project files**, and open the project. The game starts on
`/Game/Maps/LVL_NewYorkSet`.

## How it's built

All gameplay is C++ in the `DirectorsCut` module, split into small
components on the player character so each tool can be read and tuned on
its own.

| Area | Code |
|---|---|
| Movement (walk, sprint, slide, edge boost) | `Component/DirectorLocomotionComponent` |
| Film-reel grapple and guard zip | `Component/GrappleComponent`, `Actor/GrappleHookPoint` |
| Clapperboard takedowns | `Component/TakedownComponent` |
| Camera flash and megaphone | `Component/CameraFlashComponent`, `Component/MegaphoneComponent` |
| Tommy gun | `Component/TommyGunComponent`, `Actor/TommyGunPickup` |
| Guards (patrol, sight, hearing, combat) | `Character/GuardCharacter`, `AI/GuardAIController` |
| Checkpoints, takes and the upload finale | `Game/BacklotGameMode`, `Actor/ZoneCheckpoint`, `Actor/UploadTerminal` |
| HUD | `UI/HUD/BacklotHUD` |
| Tuning values | `Data/BacklotTuningSettings` |

Every gameplay number lives in one settings class and can be changed
without recompiling, under **Project Settings > Game > Director's Cut
Tuning**. [TUNING.md](TUNING.md) describes the values.

### Repository layout

| Path | Contents |
|---|---|
| `Source/DirectorsCut` | C++ gameplay code |
| `Content` | Unreal assets (Git LFS) |
| `SourceArt` | Blender files, FBX exports, textures and the scripts that generated them |
| `Tools/BacklotDress` | Editor Python scripts that built and dressed the level |
| `Config` | Project configuration |
| [DEVLOG.md](DEVLOG.md) | Day-by-day development log |
| [UnrealProjectGuidelines.md](UnrealProjectGuidelines.md) | The C++ style the code follows |

## Made with AI agents

The jam invited developers to direct AI agents, and this project was built
that way. I set the design, made the calls and playtested; coding agents
(Claude Code and Codex) wrote and tested the C++ and drove the Unreal
Editor through the Unreal MCP server. The environment kit, neon signs,
weather and level dressing were generated by scripts that are in this
repository. [DEVLOG.md](DEVLOG.md) records what was built, how it was
checked and what went wrong along the way.

## Credits

- Design, direction and development: Dylan Fernandes (Pinto Basto Games)
- "clapperboard" (https://skfb.ly/oAGxF) by clon6767, licensed under
  [Creative Commons Attribution 4.0](http://creativecommons.org/licenses/by/4.0/)
- Big-band music track by Nick Panek
- Director character: "Detective Noir" by SteboStudio, purchased from
  [CGTrader](https://www.cgtrader.com/3d-models/character/man/detective-noir-3d-model-animated),
  with new prop animations made for this project
- Mannequin and template content by Epic Games

### The director model is not in this repository

"Detective Noir" is a paid asset under CGTrader's Royalty Free License,
which allows it inside the packaged game but not as downloadable source
files. Its meshes, textures and animations are therefore left out of this
repository. The released build includes the character. To open the project
with the character in place, buy the model and import it under
`Content/Characters/Director`; without it the director has no visible
body, and the film reel and clapperboard props are still included.

## Status

This is the jam build: four days of work, with rough edges. It is kept
here as submitted. Third-party assets remain under their own licenses.
