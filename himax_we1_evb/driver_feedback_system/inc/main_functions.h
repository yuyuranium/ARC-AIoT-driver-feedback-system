#ifndef MAIN_FUNCTIONS_H_
#define MAIN_FUNCTIONS_H_

// Expose a C friendly interface for main functions.
#ifdef __cplusplus
extern "C" {
#endif

void Setup();
void Calibration();
void Inference();

#ifdef __cplusplus
}
#endif

#endif  // MAIN_FUNCTIONS_H_
