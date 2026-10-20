import sys
import os

# if sys.platform == "darwin" and os.getenv("HOMEBREW_PREFIX"):
#     os.environ["DYLD_LIBRARY_PATH"] = (
#         f"{os.getenv('HOMEBREW_PREFIX', '/opt/homebrew')}/opt/llvm/lib/c++"
#     )
# print(os.environ["DYLD_FALLBACK_LIBRARY_PATH"])
# print(os.getenv("DYLD_LIBRARY_PATH"))
from . import channel_coding
from .channel_coding import (  # type: ignore
    Simulation_Env,
    BP_config,
    Ensemble_config,
    Identity_config,
    Automorphism_config,
    G_Encoder_config,
    PCM_Encoder_config,
)  # , Encoder, Decoder
