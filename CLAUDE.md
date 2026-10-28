# WheelPlugin

GTA V (single player) Script Hook V plugin: steering wheel force feedback, clutch + H-shifter, paddles, handbrake, in-game menu.

Before changing anything read `.claude/skills/wheelplugin-dev/SKILL.md` (rules, dev loop, pitfalls) and `docs/ARCHITECTURE.md`
(source map, game memory offsets, force and transmission models). Symptom → cause table: `.claude/skills/wheelplugin-dev/references/debug-playbook.md`.

- Build: `build.bat` (needs the Script Hook V SDK in `sdk\`, which is not in the repo and must not be committed).
- Deploy: `scripts\deploy.ps1 -Game "<GTA V folder>" [-Launch]`.
- Analyse a telemetry capture: `scripts\analyze-capture.ps1 -Csv <file>`.
- Sources: `src\main.cpp`, `src\thai.h` (Thai strings and text renderer), `src\font_data.h` (embedded font, see `src\FONT_NOTICE.txt`).
- You cannot run the game yourself: say "built, not tested in game" and state what the user should check.
