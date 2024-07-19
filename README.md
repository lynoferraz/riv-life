# Life is Hard - Riv Cartridge

A Conway's Game of Life-based puzzle, that you set up life and watch it evolve

## Description

How many and for how long life can survive? Set up Conway's Game of Life patterns and let it roll!

The Life Points (LP) are spent to keep cells alive. But too few and stale cells are boring, so they consume more LP. Leveling up increase score income, but it also consumes LP faster. Birth award bonus, while deaths... And those pesky blue cells? They are just part of life, you'll have to live with them.

Game controls:

- Press ARROWS to move the cursor and keep it pressed to move faster.
- Press Z/A1 to set/unset cell.
- Keep X/A2 pressed to start simulation.

## Game arguments

| Argument | Type | Default Value | Description |
| - | - | - | - |
| -life-points | int | 20000 | Starting LP, to be consumed |
| -bonus-remaining-time | int | 0 | Bonus multiplier for remaining setup time |
| -birth-bonus | int | 1 | LP bonus for each new cell |
| -moving-bonus | int | 4 | LP bonus for new cell that was blank since 2 generations before |
| -starting-alive-bonus | int | 0 | Score bonus for each one of the remaining starting cells |
| -death-penalty | int | 0 | LP penalty for each cell that dies |
| -stale-penalty | int | 3 | LP penalty for each new cell that stayed alive |
| -boring-threshold | int | 20 | Minimum number of cells to avoid boring penalty |
| -boring-penalty | int | 100 | LP penalty for having less than boring-threshold |
| -starting-cells | int | 30 | Number of starting unremovable cells |
| -level-increase | float | 500 | Score/level to reach to upgrade level |
| -level-increase-factor | float | 2 | Factor to multiply level-increase after upgrading level |
| -efficiency | float | 1 | Rate of LP consumed by alive cells (rounded up) |
| -setup-time | int | 60 | Time to set up life board |
| -show-stats | bool | 1 | Display/Hide stats during simulation |
| -updates-sec | int | 1 | Number of seconds to do updates (divided by level) |
