import argparse
import numpy as np
import galois

import channel_code_lib2
from Codes.generate_5G_LDPC import generate_5G_LDPC
from Codes.generate_RM import generate_RM
from Codes.read_AList import read_AList

import show_results

gf2 = galois.GF2


def build_code(args):
    p = s = Z = None

    if args.code == "5g":
        H, p, s, Z, _ = generate_5G_LDPC(
            args.bg, args.k, args.n, return_lifting_size=True
        )
        G = gf2(H).null_space()
        k, n = map(int, G.shape)
        return H, k, n, p, s, Z

    if args.code == "rm":
        n, k, H = generate_RM(args.rm_m, args.rm_r)
        return H, int(k), int(n), None, None, None

    if args.code == "alist":
        n, k, H = read_AList(args.alist_path)
        return H, int(k), int(n), None, None, None

    raise ValueError("Unknown code type")


def run_decoder(decoder, H, k, n, p, s, Z, args, snr_grid):
    sim = channel_code_lib2.Simulation_Env(
        decoder, H, k, n, args.mode
    )
    sim.use_all_zero_codeword = args.use_all_zero

    if p is not None:
        sim.puncturing(p)
    if s is not None:
        sim.shortening(s)
    if Z is not None:
        sim.Z = Z

    sim.init()
    sim.get_error_rates(args.num_blocks, snr_grid)

    return sim.error_rates.get("FER-SNR", None)


def main():
    ap = argparse.ArgumentParser("Compare decoders using Simulation_Env")

    ap.add_argument("--code", choices=["5g", "rm", "alist"], default="5g")
    ap.add_argument("--bg", type=int, default=2)
    ap.add_argument("--n", type=int, default=132)
    ap.add_argument("--k", type=int, default=66)

    ap.add_argument("--rm-m", dest="rm_m", type=int, default=5)
    ap.add_argument("--rm-r", dest="rm_r", type=int, default=2)

    ap.add_argument("--alist-path", type=str,
        default="Codes/CCSDS/CCSDS_ldpc_n128_k64.alist.txt"
    )

    ap.add_argument(
        "--decoders",
        nargs="+",
        default=["bp-spa", "bp-minsum"],
    )

    ap.add_argument("--mode", type=str, default="all")
    ap.add_argument("--use-all-zero", action="store_true")

    ap.add_argument("--num-blocks", type=int, default=20)
    ap.add_argument("--snr-start", type=float, default=1.0)
    ap.add_argument("--snr-stop", type=float, default=4.0)
    ap.add_argument("--snr-points", type=int, default=7)

    args = ap.parse_args()

    H, k, n, p, s, Z = build_code(args)
    snr_grid = np.linspace(args.snr_start, args.snr_stop, args.snr_points)

    curves = []
    for dec in args.decoders:
        print(f"\nRunning decoder: {dec}")
        FER = run_decoder(dec, H, k, n, p, s, Z, args, snr_grid)
        if FER is None:
            print(f"[WARN] No FER returned for {dec}")
            continue
        curves.append((FER, dec))

    show_results.plot_error_rates(*curves)


if __name__ == "__main__":
    main()
