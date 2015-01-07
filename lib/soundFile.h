/* This makes a QsSource that reads a sound file using libsndfile */
struct QsSource *qsSoundFile_create(const char *filename,
    int maxNumFrames/*scope frames per scope source read call*/,
    float sampleRate, /* play back rate, 0 for same as file play rate*/
    const struct QsSource *group);
