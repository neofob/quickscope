extern
struct QsSource *qsLorenz_create(int maxNumFrames,
    float rate/*play rate multiplier*/,
    float sigma, float rho, float beta,
    QsRK4Source_projectionFunc_t projectionCallback,
    void *cbdata,
    int numChannels/*number source channels written*/,
    struct QsSource *group);
