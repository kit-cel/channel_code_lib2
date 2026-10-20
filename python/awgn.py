import numpy as np


def bits_to_bpsk(bits):
    """
    Map bits {0,1} to BPSK symbols {+1,-1}

    0 -> +1
    1 -> -1
    """
    bits = np.array(bits, dtype=np.int8)
    return 1.0 - 2.0 * bits


def awgn_channel(symbols, snr_db):
    """
    Add AWGN noise to symbols.

    Parameters
    
    symbols : np.ndarray
        BPSK symbols (+1/-1)
    snr_db : float
        Eb/N0 in dB

    Returns
 
    noisy_symbols : np.ndarray
    noise_variance : float
    """
    snr_linear = 10 ** (snr_db / 10.0)

    # For BPSK: Eb = 1
    noise_variance = 1.0 / (2.0 * snr_linear)
    noise = np.sqrt(noise_variance) * np.random.randn(len(symbols))

    return symbols + noise, noise_variance


def symbols_to_llr(noisy_symbols, noise_variance):
    """
    Compute LLRs for BPSK over AWGN.

    LLR = 2*y / sigma^2
    """
    return 2.0 * noisy_symbols / noise_variance


def awgn_llr(bits, snr_db):
    """
   
    bits -> BPSK -> AWGN -> LLRs
    """
    symbols = bits_to_bpsk(bits)
    noisy_symbols, noise_var = awgn_channel(symbols, snr_db)
    llr = symbols_to_llr(noisy_symbols, noise_var)
    return llr
