// Chatbot.h
#pragma once
#include <string>
#include "IntentDetector.h"
#include "UsbService.h"
#include "WeatherService.h"

namespace chatia {

class Chatbot {
public:
    Chatbot();

    std::string generateReply(const std::string& input);

private:
    IntentDetector intentDetector_;
    WeatherService weatherService_;
    UsbService usbService_;
};

}