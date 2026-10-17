# First playable prototype

## Included
- Opening scene for Chapter 1: "A Fenda Entre Mundos".
- Character selection: Goku, Ichigo Kurosaki, and Roronoa Zoro.
- Goku starts without transformations.
- First combat loop with health bars, normal attacks, special attacks, energy, guarding, victory, and defeat screens.
- PSP build workflow that produces an `EBOOT.PBP` artifact when compilation succeeds.

## Controls
- D-pad left/right: select a character.
- X: confirm / normal attack.
- Circle: special attack.
- Triangle: guard the next rival attack.
- Start: restart combat.

## Current limitations
This is a functional gameplay scaffold using PSP debug text, not the final visual fighting game. It does not yet include character sprites, animation, music, voice acting, save data, stages, or a full chapter campaign. The rival is a simple timed opponent. Those features will be developed in later milestones.

## Build
The GitHub Actions workflow uses the PSP development container and runs `make`. On a successful run, download the artifact named `SaibaUltimate-PSP` from the workflow run page; it contains `EBOOT.PBP` for testing in PPSSPP.
