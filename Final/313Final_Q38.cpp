class Mutex {
private:
    const int LOCKED = 1;
    const int UNLOCKED = 0;
    int reg = 1;
    int temp = 0;
public:
    lock() {
        while(true) {
            swap(temp, reg);
            if(temp == UNLOCKED) {
                swap(temp, reg);
                break;
            } else {
                swap(temp, reg);
            }
        }
    }
    unlock() {
        temp = UNLOCKED;
        swap(temp, reg);
    }
};