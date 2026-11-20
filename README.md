# mod-temucraft-pushback

Choose how damage pushes back a player's spells (`Temucraft.Pushback.Mode` in `mod-temucraft-pushback.conf`):

| Mode | Casts | Channels |
|---|---|---|
| 0 Off | never pushed back, as in today's game | never cut short |
| 1 Wrath (default) | 0.5 seconds a hit, twice at most (the 3.3.5 rule, unchanged) | 25% of the duration a hit, twice at most |
| 2 Classic | `Classic.CastDelay` (1 second by default) a hit, every hit | 25% of the duration a hit, every hit |

- Talents, auras and items that reduce pushback (Concentration Aura, Burning Soul and the like) still work in every mode.
- Spells that never get pushed back (Slam, bombs) stay that way. As in the core, only players are pushed back.
- Server side only: the client follows the server's cast bar, so no client change is needed.

Install: put the folder in `modules/`, re-run CMake, build, and copy the conf file. `.reload config` picks up a new mode.

Not tested in game yet.
