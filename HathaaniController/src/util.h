#pragma once

#include "../include/definitions.h"

namespace Util {
    template<typename T>
    void linspace(T start, T end, size_t N, T* array) {
        if (!array || N <= 0)
            return;

        for (int i = 0; i < N; ++i) {
            array[i] = start + ((i * 1.f / (N - 1)) * (end - start));
        }
    }

    // refer: https://cs.gmu.edu/~kosecka/cs685/cs685-trajectories.pdf
    template<typename T>
    bool interp(T q0, T qf, size_t N, float tb_cent = 0.2, T* curve = nullptr) {
        if (!curve)
            return false;

        if (q0 == qf) {
            for (int i = 0; i < N; ++i)
                curve[i] = q0;
            return true;
        }

        int nb = int(tb_cent * N);
        float a_2 = 0.5 * (qf - q0) / (nb * (N - nb));
        T tmp;

        for (int i = 0; i < nb; ++i) {
            tmp = a_2 * pow(i, 2);
            curve[i] = q0 + tmp;
            //  N-1, N-2, ... , N - nb
            curve[N - i - 1] = qf - tmp;
        }

        tmp = a_2 * pow(nb, 2);
        T qa = q0 + tmp;
        T qb = qf - tmp;
        linspace(qa, qb, N - (2 * nb), &curve[nb]);

        return true;
    }

} // namespace Util
