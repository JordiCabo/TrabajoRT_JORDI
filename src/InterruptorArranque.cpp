#include "InterruptorArranque.h"

InterruptorArranque::InterruptorArranque() : run_(0) {
}

void InterruptorArranque::setRun(int value) {
    run_ = value;
}

int InterruptorArranque::getRun() const {
    return run_;
}
