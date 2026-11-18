# Changelog

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
