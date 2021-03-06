#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include "Point.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include "Cluster.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace utils {


    /**
    * This is a versatile function to create lists containing arithmetic progressions.
    * It is most often used in for loops
    *
    * @param start The starting value of the sequence.
    * @param stop The end value of the sequence, unless endpoint is set to False. In that case, the sequence
    * consists of all but the last of num + 1 evenly spaced samples, so that stop is excluded.
    * @param step Stepsize used to generate the samples. Default is 1. Can also be negative.
    * @param endpoint If True, stop is the last sample. Otherwise, it is not included. Default is False.
    */
    std::vector<double> range(double start, double stop, double step = 1, bool endpoint = false);

    /**
    * Return evenly spaced numbers over a specified interval.
    *
    * @param start The starting value of the sequence.
    * @param stop The end value of the sequence, unless endpoint is set to False. In that case, the sequence
    * consists of all but the last of num + 1 evenly spaced samples, so that stop is excluded.
    * Note that the step size changes when endpoint is False.
    * @param num Number of samples to generate. Default is 50. Must be non-negative.
    * @param endpoint If True, stop is the last sample. Otherwise, it is not included. Default is True.
    */
    std::vector<double> linspace(double start, double stop, uint64_t num = 50, bool endpoint = true);

    inline double deg2rad(double degrees) {
        static const double pi_on_180 = M_PI / 180.0L;
        return degrees * pi_on_180;
    }


    std::vector<Point *> minZinSec(int start, int stop, Point (&points)[2000][16]);

    int max(int a, int b);

    int min(int a, int b);


}


