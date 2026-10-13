#pragma once

#include <string>
#include <vector>

namespace chatia {

struct UsbDeviceInfo {
    std::string name;
    std::string vendor;
    std::string product;
    std::string serial;
    std::string bus;
    std::string path;
    std::string port;
};

class UsbService {
public:
    std::string listConnectedDevices() const;

private:
    std::vector<UsbDeviceInfo> discoverDevices() const;
    std::string readTextFile(const std::string& path) const;
    std::string readLinkTarget(const std::string& path) const;
    std::string normalizeHexValue(const std::string& value) const;
};

} // namespace chatia
