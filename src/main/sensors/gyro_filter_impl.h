static FAST_CODE void GYRO_FILTER_FUNCTION_NAME(gyroSensor_t *gyroSensor)
{

    DEBUG_SET(DEBUG_KALMAN, 0, gyroSensor->gyroDev.gyroADC[X] * gyroSensor->gyroDev.scale);                               //Gyro input

    for (int axis = 0; axis < XYZ_AXIS_COUNT; axis++) {
        GYRO_FILTER_DEBUG_SET(DEBUG_GYRO_RAW, axis, gyroSensor->gyroDev.gyroADCRaw[axis]);
        // scale gyro output to degrees per second
        float gyroADCf = gyroSensor->gyroDev.gyroADC[axis] * gyroSensor->gyroDev.scale;
        // DEBUG_GYRO_SCALED records the unfiltered, scaled gyro output
        GYRO_FILTER_DEBUG_SET(DEBUG_GYRO_SCALED, axis, lrintf(gyroADCf));

#ifdef USE_GYRO_DATA_ANALYSE
        if (isDynamicFilterActive()) {
            if (axis == X) {
                GYRO_FILTER_DEBUG_SET(DEBUG_FFT, 0, lrintf(gyroADCf)); // store raw data
                GYRO_FILTER_DEBUG_SET(DEBUG_FFT_FREQ, 3, lrintf(gyroADCf)); // store raw data
            }
        }
#endif

        // apply static notch filters and software lowpass filters
        gyroADCf = gyroSensor->lowpass2FilterApplyFn((filter_t *)&gyroSensor->lowpass2Filter[axis], gyroADCf);
        gyroADCf = gyroSensor->lowpassFilterApplyFn((filter_t *)&gyroSensor->lowpassFilter[axis], gyroADCf);
        gyroADCf = gyroSensor->notchFilter1ApplyFn((filter_t *)&gyroSensor->notchFilter1[axis], gyroADCf);
        gyroADCf = gyroSensor->notchFilter2ApplyFn((filter_t *)&gyroSensor->notchFilter2[axis], gyroADCf);

#ifdef USE_GYRO_DATA_ANALYSE
        if (isDynamicFilterActive()) {
            gyroDataAnalysePush(&gyroSensor->gyroAnalyseState, axis, gyroADCf);
            gyroADCf = gyroSensor->notchFilterDynApplyFn((filter_t *)&gyroSensor->notchFilterDyn[axis], gyroADCf);
            if (axis == X) {
                GYRO_FILTER_DEBUG_SET(DEBUG_FFT, 1, lrintf(gyroADCf)); // store data after dynamic notch
            }
        }
#endif

        // DEBUG_GYRO_FILTERED records the scaled, filtered, after all software filtering has been applied.
        GYRO_FILTER_DEBUG_SET(DEBUG_GYRO_FILTERED, axis, lrintf(gyroADCf));

        gyroSensor->gyroDev.gyroADCf[axis] = gyroADCf;

    }

    float input[XYZ_AXIS_COUNT];
    float output[XYZ_AXIS_COUNT];

    input[X] = gyroSensor->gyroDev.gyroADCf[X];
    input[Y] = gyroSensor->gyroDev.gyroADCf[Y];
    input[Z] = gyroSensor->gyroDev.gyroADCf[Z];

    kalman_update(input, output);

    gyroSensor->gyroDev.gyroADCf[X] = output[X];
    gyroSensor->gyroDev.gyroADCf[Y] = output[Y];
    gyroSensor->gyroDev.gyroADCf[Z] = output[Z];
}
