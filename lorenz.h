extern
struct QsSource *qsLorenz_create(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    struct QsSource *group);
