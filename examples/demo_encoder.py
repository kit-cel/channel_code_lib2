
import numpy as np
import galois
import channel_code_lib2
from Codes.generate_5G_LDPC import generate_5G_LDPC

gf2 = galois.GF2


H, p, s, Z, BG = generate_5G_LDPC(2, 66, 132, return_lifting_size=True)


n = int(H.shape[1])
G = gf2(H).null_space()
k = int(G.shape[0])   
u = np.random.randint(0, 2, size=k).tolist()

enc = channel_code_lib2.Encoder(H, k, n, 0)  
c = enc.encode(u)

print("k =", k, "n =", n)
print("len(u) =", len(u))
print("len(c) =", len(c))
print("c[:20] =", c[:20])