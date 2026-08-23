# Conversion guide

1. Read the surviving historical assembly when available.
2. Compare the corresponding final retail instruction stream in the exact preservation repository.
3. Recover types, signedness, fixed-point units and structure fields from all call sites.
4. Write readable C expressing behavior rather than register allocation.
5. Preserve ordering/width/overflow behavior where gameplay depends on it.
6. Add a regression test or oracle note before marking the module converted.

A function may initially remain low-level C if types are uncertain; uncertain semantics must be documented rather than guessed.
