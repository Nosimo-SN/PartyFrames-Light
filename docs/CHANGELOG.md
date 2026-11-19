# Changelog

## 0.4.4 — casual follow exclusion and state diagnostics

- SeverActions casual followers (`1` without recruited-roster bit `1024`) default off. A new setting enables them, with a default-on distance limit of 100 metres adjustable from 10 to 500 m. Unknown space and unloaded casuals are outside range. Turning the range limit off allows them at any distance.
- Only active SeverActions companions defaults off. When enabled, it takes priority over casual/summon options and requires recruited-roster membership plus active follow evidence or the supported Hearth camp package. Independent vanilla/quest followers and unrostered summons are excluded in this mode.
- Both SeverActions casual and recruited follow use its alias/package pool, so package presence alone cannot establish recruitment. This fixes the reported House Cat signal combination without blacklisting animals or adding manual hide controls.
- Logs excluded candidates as well as included ones, with explicit selection state and SeverActions recruitment status. Non-SA engine/package following is marked as recruitment not established by that API.
- Logs update when decision, signals, package or target changes; unchanged states are suppressed. Package plugin names are included, and death, disabled/hostile actors and unavailable references have specific reasons.
- Effective validated settings are logged once on initialization and when changed; unchanged settings echoes are suppressed. Filter decisions identify casual range exclusions and companions-only exclusions.
- All three native suites pass, including the reported cat signals, casual fallback/camp veto, recruited animals, vanilla/quest followers and existing camp/hold cases. This build still needs live-game testing; the guard's generic engine/package follow remains a separate unresolved case.

## 0.4.3 — active follower filtering fix

- A player-teammate flag alone no longer qualifies any actor, including actors outside SeverActions' roster. Quest actors or dismissed companions can keep that flag after following ends.
- Retains current SeverActions follow state, engine follower links, live player-targeted follow/escort/accompany packages, and the exact Hearth camping-package exception.
- Logs inclusion/removal transitions with actor FormID, SeverActions flags, teammate/wait state, engine link, live package, camp state and package/target FormIDs.
- Native regressions cover a quest escort ending while its teammate flag remains, plus recruitment afterward. This models the suspected failure; the reported Hadvar save has not been reproduced.

## 0.4.2 — PartyFrames Light, first public SE/AE package

- Public name and settings title changed from PartyLight to PartyFrames Light.
- Existing DLL, config, logs and asset paths retained for upgrade compatibility.
- Includes active follower filtering, camp occupancy, configurable compact layouts, distance/direction and Canine Drives indicators from the tested local builds.
- Includes the optional, off-by-default SIF overhead debug integration; live-game verification remains pending.
- VR package withheld until a tester requests it and verifies behavior with VR hardware.
