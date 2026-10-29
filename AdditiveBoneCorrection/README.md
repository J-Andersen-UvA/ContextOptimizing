# Additive Bone Correction

This Unreal Engine plugin solves position-only IK on an explicitly selected, contiguous bone chain and stores the result as sparse additive correction keys.

## Modules

- `AdditiveBoneCorrection`: runtime CCD IK, correction types, and the additive correction-layer data asset.
- `AdditiveBoneCorrectionEditor`: animation-pose evaluation, Blueprint nodes for solving and keying, and baking to a new animation asset.

## First test workflow

1. Create a correction layer with `Create Correction Layer Asset`.
2. Call `Solve Position IK At Frame` with the target animation, target skeletal mesh, representative frame, target interaction bone and its bone-local point, component-space goal, and an ordered IK chain.
3. Inspect `Reached Goal`, `Final Error`, and `Component Space Solved Point`.
4. Pass the result to `Add Correction Key`.
5. Add identity keys around the correction before evaluating interpolation over an animation.
6. Call `Bake Correction Layer` to create a new animation. The base animation remains unchanged.

The chain must be ordered from its root control to its end control and must be contiguous. For the first arm test, use upper arm, lower arm, and hand. Add the clavicle only when the arm-only goal is unreachable.

The interaction point may be attached to a descendant finger bone. The solver treats its current offset from the final chain bone as rigid, so finger-local transforms remain unchanged.
