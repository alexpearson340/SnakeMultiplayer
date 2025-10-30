#include <random>
#include <ncurses.h>

class Engine {
public:
    Engine(int width, int height);
    ~Engine();

    void run();

protected:
    void initCurses();
    void cleanupCurses();

    virtual void handleInput();
    virtual void create();
    virtual void update();
    virtual void render();
    virtual void cleanup();

    int napMs [100];
    int width;
    int height;
    bool running;
    int lastKey;
    int score;

    // random
    std::mt19937 gen;
};
