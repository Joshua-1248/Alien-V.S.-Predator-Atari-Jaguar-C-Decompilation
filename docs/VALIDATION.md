# Validation

Run:

```sh
./tools/validate.sh
```

The script performs a fresh strict C99 build, executes the regression suite, forces every object in `libavp_c.a` into a single link to catch hidden undefined/duplicate symbols, scans active source for unfinished implementation markers, and audits common prohibited ROM/media/resource extensions.

Current release gate:

```text
strict build                PASS
regression tests            PASS
whole-archive link          PASS
public payload audit        PASS
```

The byte-exact preservation repository remains the final behavioral/binary oracle. This readable tree is intentionally not expected to reproduce identical historical 68000 object bytes.
