add_library(decoder OBJECT
  Decoder/Decoder.cpp
  Decoder/Ensemble/Ensemble_Decoder.cpp
  Decoder/BP/BP_Decoder.cpp
  Decoder/BP/BP_Scheduling/BP_Column_Layered.cpp
  Decoder/BP/BP_Scheduling/BP_Row_Layered.cpp
  Decoder/BP/BP_Scheduling/BP_Flooding.cpp
  Decoder/BP/BP_CN_Processing/BP_MSA.cpp
  Decoder/BP/BP_CN_Processing/BP_SPA_Phi.cpp
  Decoder/BP/BP_CN_Processing/BP_SPA.cpp
  Decoder/Ensemble/Processing_Diversity/Automorphism_Processing.cpp
  Decoder/Ensemble/DecoderSelector/FullParallelSelector.cpp
)

target_include_directories(decoder PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:include>
  ${nlohmann_json_SOURCE_DIR}
)

add_library(encoder OBJECT
  Encoder/G_based_Encoder.cpp
  Encoder/PCM_based_Encoder.cpp
)
target_include_directories(encoder PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:include>
  ${nlohmann_json_SOURCE_DIR}
)

add_library(simenv OBJECT
  Simulation_Env.cpp
  GFqField.cpp
)
target_include_directories(simenv PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:include>
  ${nlohmann_json_SOURCE_DIR}
)
