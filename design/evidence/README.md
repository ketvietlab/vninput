# Historical visual evidence

The screenshots and comparison reports in this directory were captured before
the product rename on 2026-10-01. They remain historical evidence, not a new
pixel comparison of the renamed interface. Asset references follow the new file
names; the original typing check is described by its sample length.

Fresh Qt captures for the renamed application can be generated with:

```sh
QT_QPA_PLATFORM=offscreen build/dev/tests/vninput-ui-capture build/evidence/rename
```

Run this command from the repository root. Current UI text and the Atlas
presenters both use VNInput. The existing comparison percentages must not be
reported as a newly executed comparison after the rename.
