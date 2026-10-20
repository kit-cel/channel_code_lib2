# channel_code_lib2

A C++ channel-coding library with Python bindings, built around three pieces that
compose: **encoders**, **belief-propagation decoders**, and an **ensemble decoder**
that runs several decoding paths over diversified views of the same code and
combines them.

It was developed at the Communications Engineering Lab (CEL), KIT, and is the
basis for the numerical results on automorphism/affine-structured ensemble
decoding (aSCED) produced with it.

## Why an ensemble decoder

A single belief-propagation decoder gets stuck on the same trapping sets every
time it sees the same channel-induced error pattern, because it always runs on
the same Tanner graph. The ensemble decoder in this library instead runs
several BP decoders in parallel, each seeing a *different* view of the code:

- **Automorphism / affine processing diversity** — permute the codeword (via a
  code automorphism or an affine structure) before decoding and undo the
  permutation afterward, so each path explores different cycles in the Tanner
  graph for the same underlying codeword.
- **Alternative parity-check matrices** — decode the same code against
  different (redundant or overcomplete) parity-check matrices, e.g. structured
  sub-matrices derived from a base graph.

Results are merged across paths (`Ensemble_config` + a `Processing_config` such
as `Automorphism_config` or `Identity_config`), giving substantial coding gain
over a single BP decoder at comparable or lower total decoding effort.

## What's in the box

- **Encoders**: generator-matrix based (`G_Encoder_config`) and parity-check-matrix
  based (`PCM_Encoder_config`), both over GF(2).
- **BP decoding**: sum-product (`spa`), sum-product with the exact `phi` function
  (`spa_phi`), and min-sum (`msa`) check-node updates, each with flooding,
  row-layered, or column-layered scheduling (`BP_config`).
- **Ensemble decoding**: `Ensemble_config` combines a set of `Decoder_config`s
  (e.g. several `BP_config`s with different PCMs) with a `Processing_config`
  for automorphism/affine diversity.
- **Linear algebra over finite fields**: `GF2` and `GF4` matrix/vector types used
  internally by the encoders and decoders (`include/GF2`, `include/GFq`).
- **Simulation harness**: `Simulation_Env` drives Monte-Carlo transmission over
  an AWGN channel, tracks frame/bit error rates down to a target error count,
  and can save results to disk.

## Installation

Prerequisites: a working install of [conda](https://www.anaconda.com/docs/getting-started/miniconda/main).

```bash
conda env create -f environment.yml
conda activate channel_coding_lib2
uv run examples/demo.py   # builds the C++ extension and runs the demo
uv pip install .          # install the library into .venv
```

The build compiles a C++23 core (via CMake + pybind11/scikit-build-core) and
exposes it as the `channel_code_lib2` Python package.

## Examples

`examples/` contains runnable demos: encoding/decoding over an AWGN channel,
comparing decoder variants, sweeping decoder parameters, and FER-vs-SNR
evaluation. `examples/Codes/` has helpers to generate or read in codes used in
the examples (5G NR LDPC base graphs, BCH codes, CCSDS LDPC, Reed-Muller,
non-binary LDPC).

## Tests

C++ unit tests for the GF(2)/GF(4) arithmetic and the encoders live in
`tests/` (enable with `-Dtesting=ON` when configuring CMake).
