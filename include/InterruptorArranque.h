#pragma once

class InterruptorArranque {
private:
    volatile int run_;

public:
    InterruptorArranque();
    
    void setRun(int value);
    int getRun() const;
};
