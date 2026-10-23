// IntentDetector.h
#pragma once
#include <string>
#include <vector>

namespace chatia {

enum class Intent {
    Greeting,
    Who,
    Weather,
    Time,
    Date,
    Thanks,
    Goodbye,
    Calculator,
    Usb,
    Help,
    Unknown,
    Rapelle
};

class IntentDetector {
public:
    Intent detect(const std::string& input) const;
};

}