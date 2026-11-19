# PartyFrames Light

A small follower HUD for **Skyrim Special Edition and Anniversary Edition**, built with [Magelight UI](https://github.com/Severause/MagelightUI). Portraits, names and readable health, stamina and magicka bars, with just enough settings to put them where you want them.

**[Download the SE/AE release](https://github.com/Nosimo-SN/PartyFrames-Light/releases/latest)** · [Report a problem](https://github.com/Nosimo-SN/PartyFrames-Light/issues)

<img src="docs/preview.png" alt="PartyFrames Light in game, showing Celestine and Hjalti" width="229">

*SE/AE development screenshot. Portraits come from SeverActions; the dog magicka bar is replaced by Canine Drives indicators.*

## Features

- Shows actively following companions, including the vanilla follower slot and quest companions using standard follower state. SeverActions roster membership or Skyrim teammate status alone does not qualify. A recruited SeverActions companion currently following, an independent engine follower link, live player-targeted follow/escort/accompany package or supported Hearth camp package is required. SeverActions casual followers default off and can be enabled in settings. Their optional distance limit defaults on at 100 metres. A separate default-off setting limits the HUD to active recruited SeverActions companions.
- Reads existing SeverActions portraits and follows its selected portrait-frame/font theme. A monogram appears when no portrait is available.
- Move the party as a group or place individual cards. Change sizes, spacing, portrait visibility and numbers.
- Limit rows and columns; extra cards are omitted. Minions and summons are optional.
- Combat indicator, distance and a direction arrow relative to player facing. SeverActions survival icons appear only when needs reach debuff conditions.
- Keeps camp occupants visible while they use Sever's Hearth camping package.
- Optional Canine Drives integration replaces its tracked dog's magicka bar with hunger, tiredness and activity indicators. Uses the provider's warning flags and configured thresholds.
- Optional, off-by-default Hjalti overhead debugging through Status Indicator Framework.
- Hides the normal HUD while a Magelight menu owns UI focus.

One SKSE DLL, web assets and settings; **no ESP, ESL or Papyrus scripts**. No AI requests or portrait generation.

## Requirements

- Skyrim SE/AE with the matching [SKSE](https://skse.silverlock.org/).
- Address Library for SKSE Plugins for your game version.
- **Magelight UI 1.9.0 or newer**.
- Microsoft Visual C++ 2015–2022 x64 runtime.

Optional integrations:

| Mod | Adds |
| --- | --- |
| SeverActions, native API v2+ (4.8.0+) | Existing portraits; native follow state, theme and survival integration where available |
| Sever's Hearth | Recognition of its `Severs_CampingPackage` camp sandbox |
| Canine Drives 0.26.6+, API v1 | Tracked dog's authoritative need warnings and published activities |
| Status Indicator Framework – Floating Icons, API v1 | Optional overhead canine debugging icons; also requires Canine Drives |

Canine Drives is a soft requirement: without it, the normal follower HUD works and dog-specific indicators are unavailable. Install optional mods separately. Ordinary follower frames work without those native APIs. Some custom quest companions expose no standard follower state and may need a compatibility rule; every quest and follower framework has not been tested.

**SE/AE only is offered here. VR is withheld pending a request and testing by someone with VR hardware.**

## Install and configure

1. Install the SE/AE ZIP through MO2 or your preferred mod manager. Start Skyrim through SKSE.
2. Press **Shift+F8** to open settings. Change the key and optional Shift/Ctrl/Alt modifiers there. Escape or Done closes settings; the game pauses while they are open.
3. Drag **Move party** to move the stack, or drag a follower card to place it separately. Select a follower for individual size and portrait controls.

Under **Follower selection**, **Show SeverActions casual followers** defaults off. **Hide casual followers beyond range** defaults on, with a 100 m default range adjustable from 10 to 500 m. Casuals in another interior/worldspace or without loaded 3D count as outside range; turning the distance limit off allows them regardless of distance. These controls affect casual followers only. **Only active SeverActions companions** defaults off and overrides the casual/summon options: only recruited SeverActions companions with active follow evidence or the supported camp package qualify. Independent vanilla/quest followers and unrostered summons are excluded in that mode. The new fields receive their defaults when loading an older JSON. Filter changes apply on the next one-second follower scan. Effective settings are recorded in PartyLight.log at initialization and whenever changed.

Settings save automatically. Existing users keep their selected hotkey: updating does not reset it. If another mod manager file overrides the JSON, preserve the winning copy.

The public name is **PartyFrames Light**. Internal paths remain `SKSE/Plugins/PartyLight.dll`, `SKSE/Plugins/PartyLight.json`, `Magelight/PartyLight/` and `PartyLight.log` for compatibility with earlier local builds. Upgrade by replacing the earlier PartyLight mod; enable only one copy. Keep your existing JSON when prompted. Disable another party HUD if it produces duplicate frames.

## Canine indicators

Food and tiredness icons appear only when Canine Drives publishes their warning flags. The activity slot can show awake/unknown, eating, napping, hunting, sniffing, playing, fishing, scavenging, greeting, warming or fighting. Unknown future activities use a paw. When the API is unavailable, the ordinary magicka bar returns.

For overhead debugging, enable **Hjalti overhead debug icons** under **Dog debugging**. It defaults off, requires SIF and Canine Drives, and changes no dog behavior. Only its tracked dog receives these icons. The overhead integration still needs live-game confirmation.

## Testing and troubleshooting

The developer's SE/AE game has exercised portraits, active follower filtering, camp visibility, direction arrows, settings sliders and a live Canine Drives API snapshot. Version 0.4.4 also excludes SeverActions casual followers and adds classified inclusion/exclusion state logs, recruitment status and package plugin names. Its DLL builds and all three native regression suites pass, including the reported casual-follow cat signals, recruited animals, quest escort lifecycle and camp exceptions. Existing browser checks cover layout, controls, overflow, canine activities and warning visibility; the UI is unchanged in 0.4.4. The 0.4.4 engine integration still needs live-game testing. The reported Hadvar save has not been reproduced, so confirmation in that scenario is still needed. These checks do not establish compatibility with every runtime, quest or mod combination. Overhead positioning and live toggling remain unverified.

If a frame is missing, confirm the actor is actively following or using the supported camp package. Waiting/home-rostered companions are intentionally excluded. Check row/column limits and the summons toggle. Another interior/worldspace displays **Elsewhere** because coordinates cannot be compared reliably.

For a bug report, include your Skyrim and SKSE versions, integration versions, steps to reproduce, and `Documents/My Games/Skyrim Special Edition/SKSE/PartyLight.log`. Avoid posting private data from unrelated logs.

## Build

See [BUILDING.md](docs/BUILDING.md). Source is GPL-3.0-or-later; third-party headers and libraries retain their own notices under `include/` and `package/SKSE/Plugins/PartyLight/Licenses/`.

Severause created Magelight UI and SeverActions. Thanks also to the CommonLibSSE-NG contributors and the creator of Status Indicator Framework.
