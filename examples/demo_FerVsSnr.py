import numpy as np
import channel_code_lib2
import show_results

from Codes.generate_5G_LDPC import generate_5G_LDPC
import galois
gf2 = galois.GF2


# Generate a5G LDPC code
n_ = 132
k_ = 66

H, p, s, Z, BG = generate_5G_LDPC(
    2, k_, n_, return_lifting_size=True
)

G = gf2(H).null_space()
k, n = G.shape

decoder_type = "bp-spa"   # SPA
sim = channel_code_lib2.Simulation_Env(decoder_type, H, k, n, "all")

sim.use_all_zero_codeword = True   
sim.puncturing(p)
sim.shortening(s)
sim.Z = Z

sim.init()


snr_db = np.linspace(1.0, 4.0, 7)
num_frames = 100  

sim.get_error_rates(num_frames, snr_db)


#  results

FER = sim.error_rates["FER-SNR"]


BER = sim.error_rates.get("BER-SNR", None)

print("SNR (dB):", snr_db)
print("FER:", FER)
if BER is not None:
    print("BER:", BER)

#Show plot
show_results.plot_error_rates((FER, "BP-SPA"))