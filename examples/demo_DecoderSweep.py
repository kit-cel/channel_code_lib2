import numpy as np
import galois
gf2 = galois.GF2

import channel_code_lib2
import show_results
from Codes.generate_5G_LDPC import generate_5G_LDPC

H, p, s, Z, BG = generate_5G_LDPC(2, 66, 132, return_lifting_size=True)
G = gf2(H).null_space()
k, n = G.shape

snr = np.linspace(1, 4, 7).tolist()
iters = 30

decoders = [
    "bp-spa",
    "bp-minsum",
    "bp-flooding",
    "bp-row-layered",
    "bp-column-layered",
]

curves = []

for dec in decoders:
    try:
        sim = channel_code_lib2.Simulation_Env(dec, H, k, n, "all")
        sim.use_all_zero_codeword = True
        sim.puncturing(p)
        sim.shortening(s)
        sim.Z = Z
        sim.init()
        sim.get_error_rates(iters, snr)

        curves.append((sim.error_rates["FER-SNR"], dec))
        print(f"{dec}: OK")
    except Exception as e:
        print(f"{dec}: FAILED -> {e}")

if curves:
    show_results.plot_error_rates(*curves)