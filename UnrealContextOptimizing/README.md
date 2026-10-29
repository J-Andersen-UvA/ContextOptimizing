# Unreal Context Optimizing

The plugin is split into three modules:

- `ContextOptimizingCore`: engine-independent animation math and optimization.
- `UnrealContextOptimizing`: source semantic-event and target safety-event data.
- `UnrealContextOptimizingEditor`: animated mesh extraction, event detection, diagnostics, and animation writing.

The runtime module does not read or modify animation assets. Editor-only asset access stays in `UnrealContextOptimizingEditor`.

The plugin depends on the sibling `SemanticSurfacePatches` plugin. That plugin owns surface-patch authoring, stored patch geometry, runtime queries, and patch-proximity mapping.

## Primary event source

Semantic events are detected from proximity between the animated source hand mesh and source face patches. The source patch, patch-relative position, signed normal offset, hand orientation, active contact set, and timing define the intended event.

The corresponding target pose is corrected toward the mapped target event whether the initial target hand penetrates, floats outside the face, or points at the wrong facial region.

## Secondary safety scan

Target penetration is scanned separately after semantic corrections. It catches accidental target collisions that do not correspond to a source semantic event. A safety correction may remove collision but may not redefine an existing semantic event.

## Current event pipeline

Source and target actors author independent `SurfacePatches` components matched by stable `patchId`. Source interaction points are evaluated across the source animation, neighboring detections are grouped into semantic events, and each selected source patch location is reconstructed on the corresponding target patch.

`SemanticEventTimingEditorLibrary` samples bone trajectories around semantic events. It exposes approach-start detection, the earlier low-speed experiment, and an interactive position/speed graph for diagnostics.

`Bake Semantic Event Corrections` runs the current editor pipeline in one call: extract and map source events, detect approach and recovery timing, solve target position IK, create sparse additive keys, and bake a new animation asset. Its result retains per-event timing and IK diagnostics.

