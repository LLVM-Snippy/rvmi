include(testgen-model-interface-launcher-tests)

set(MODEL_PLUGIN_LIBRARY "${RISCVSpikeModel_DIR}/libRISCVModel.so")
set(MODEL_LAUNCHER_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
add_model_tests()
