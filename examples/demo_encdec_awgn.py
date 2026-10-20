
# F demo: 5G LDPC Encoder -> AWGN (LLR) -> Decoder

import numpy as np

from Codes.generate_5G_LDPC import generate_5G_LDPC
from channel_code_lib2 import Encoder, decoders


def awgn_llr(bits, snr_db):
    """
    bits   : list/np.array of 0/1
    snr_db : Eb/N0 in dB (BPSK)
    returns: list of LLR floats
    """
    bits = np.array(bits, dtype=np.int8)

    # BPSK mapping: 0 -> +1, 1 -> -1
    x = 1.0 - 2.0 * bits

    # Eb/N0
    snr_linear = 10 ** (snr_db / 10.0)

    # For BPSK with Eb=1: sigma^2 = 1/(2*SNR)
    sigma2 = 1.0 / (2.0 * snr_linear)
    sigma = np.sqrt(sigma2)

    # AWGN
    y = x + sigma * np.random.randn(len(x))

    # LLR for BPSK over AWGN: 2y/sigma^2
    llr = (2.0 / sigma2) * y
    return llr.tolist()


def main():
    
    # 1) Generate a small 5G LDPC code
 
    bg = 2
    k = 66
    n = 132

    H, p, s, Z, BG = generate_5G_LDPC(bg, k, n, return_lifting_size=True)

    print("=== Code ===")
    print("H shape:", H.shape)
    print("BG:", BG, "Z:", Z)
    print("puncturing bits:", len(p) if p is not None else 0)
    print("shortening bits:", len(s) if s is not None else 0)

 
    # 2) Encoder: message -> codeword
   
    enc = Encoder(H, k, n, min_col=0)

    msg = np.random.randint(0, 2, size=k).tolist()
    cw = enc.encode(msg)

    print("\n=== Encoder ===")
    print("message length:", len(msg))
    print("codeword length:", len(cw))

  
    # 3) Channel: codeword -> LLR
    
    snr_db = 2.0
    llr = awgn_llr(cw, snr_db)

    print("\n=== Channel ===")
    print("SNR(dB):", snr_db)
    print("LLR length:", len(llr))

    # 4) Decoder: LLR -> decoded codeword


    dec = decoders.make("bp-spa")
    dec.init(H)

   
    try:
        dec.Z = int(Z)
    except Exception:
        pass

    max_iter = 20
    dec.decode(llr, max_iter)

    c_hat = dec.get_codeword()
    ok = dec.syndrome_is_zero()

    print("\n=== Decoder ===")
    print("max_iter:", max_iter)
    print("decoded length:", len(c_hat))
    print("syndrome_is_zero:", ok)

    # Optional: compare decoded with transmitted codeword
    if len(c_hat) == len(cw):
        bit_errors = sum(int(a != b) for a, b in zip(c_hat, cw))
        print("bit errors vs transmitted codeword:", bit_errors, "/", len(cw))

    print("\nDone.")


if __name__ == "__main__":
    main()
