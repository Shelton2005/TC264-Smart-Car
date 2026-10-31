/*
 * Agorithm.h
 *
 *  Created on: 2026Äê3ÔÂ28ÈÕ
 *      Author: Shelton Liu
 */

#ifndef CODE_PID_H_
#define CODE_PID_H_

float LowPassFilter(float current, float last, float alpha);

float StepApproach(float target, float step_size);

float AngleErrorNormalize(float error);

float StepApproachAngleMode(float target, float real_angle, float step_size, int mode);

#endif /* CODE_PID_H_ */
