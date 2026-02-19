# Thread Safety And Performance

Author: watsonryan

## Thread safety

- Parsed model data is immutable after `Model` construction.
- No shared global mutable caches are used.
- Separate `Model` instances are safe to use concurrently.

## Performance policy

- Correctness and parity lock first.
- Profile only after parity tests are in place for the implemented paths.
- Avoid optimization changes that alter validated numerical behavior.

## Planned optimizations (post-parity)

- Batch evaluation APIs for amortized overhead.
- Optional caller-owned workspace for reusing temporary buffers.
- Optional cache mode gated behind explicit options.
