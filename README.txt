NHC — NapoleonHorossCentral
GitHub Actions ARM64 build scaffold
Version: 0.1.0-dev

WHAT THIS PACKAGE DOES
- Uses the official AML template headers included in this source package.
- Configures a GitHub Actions workflow to compile an ARM64 plugin.
- Produces a development artifact named NHC-ARM64-dev containing libnhc.so.

WHAT IT DOES NOT DO
- It does not install or inject AML into GTA SA.
- It does not contain a working in-game menu yet.
- It does not implement Infinite Money, Invincibility, Jetpack, or Vehicle Spawn.
- It does not establish compatibility with GTA SA 2.11.311.
- It does not include a weapon feature.

WHY
The AML Plugin SDK documents GTA SA v2.00 and v2.10 64-bit, not 2.11.311.
The supplied AML source README says AML must be loaded into the target app
after game libraries, e.g. through app integration. A plugin build alone does
not load AML into the game.

NEXT TEST
1. Upload the contents of this folder to a new GitHub repository.
2. Open the repository Actions tab and run "NHC ARM64 Plugin Build".
3. Download the NHC-ARM64-dev artifact only if the build succeeds.
4. Do not install the artifact until AML loading and target-game compatibility
   are verified. The first test is only to confirm the plugin's load log.

CORE FEATURE ROADMAP
- Player / Infinite Money
- Player / Invincibility
- Player / Jetpack
- Vehicles / Vehicle Spawn
- Settings / About

UI language: English.
Target: GTA SA Android 2.11.311, ARM64-v8a, Android 15–16, no root.
