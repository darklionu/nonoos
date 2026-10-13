#include "UsbService.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace chatia {
namespace {

bool isTtyDeviceName(const std::string& name) {
    return name.rfind("ttyUSB", 0) == 0 || name.rfind("ttyACM", 0) == 0 || name.rfind("ttyAMA", 0) == 0;
}

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \n\r\t");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \n\r\t");
    return value.substr(begin, end - begin + 1);
}

} // namespace

std::string UsbService::readTextFile(const std::string& path) const {
    std::ifstream input(path);
    std::string value;
    if (std::getline(input, value)) {
        return trim(value);
    }
    return {};
}

std::string UsbService::readLinkTarget(const std::string& path) const {
    std::filesystem::path fsPath(path);
    std::error_code ec;
    const auto target = std::filesystem::read_symlink(fsPath, ec);
    if (ec) {
        return {};
    }
    return target.filename().string();
}

std::string UsbService::normalizeHexValue(const std::string& value) const {
    std::string cleaned = trim(value);
    if (cleaned.empty()) {
        return {};
    }

    if (cleaned.rfind("0x", 0) == 0 || cleaned.rfind("0X", 0) == 0) {
        return cleaned;
    }

    std::ostringstream oss;
    oss << "0x" << cleaned;
    return oss.str();
}

std::vector<UsbDeviceInfo> UsbService::discoverDevices() const {
    std::vector<UsbDeviceInfo> devices;
    const std::filesystem::path sysUsb = "/sys/bus/usb/devices";
    if (!std::filesystem::exists(sysUsb) || !std::filesystem::is_directory(sysUsb)) {
        return devices;
    }

    for (const auto& entry : std::filesystem::directory_iterator(sysUsb)) {
        if (!entry.is_directory()) {
            continue;
        }

        const std::filesystem::path devicePath = entry.path();
        const std::filesystem::path vendorPath = devicePath / "idVendor";
        const std::filesystem::path productPath = devicePath / "idProduct";
        if (!std::filesystem::exists(vendorPath) || !std::filesystem::exists(productPath)) {
            continue;
        }

        const std::string vendor = normalizeHexValue(readTextFile(vendorPath.string()));
        const std::string product = normalizeHexValue(readTextFile(productPath.string()));
        if (vendor.empty() || product.empty()) {
            continue;
        }

        UsbDeviceInfo info;
        info.name = devicePath.filename().string();
        info.vendor = vendor;
        info.product = product;
        info.path = devicePath.string();
        info.serial = readTextFile((devicePath / "serial").string());
        info.bus = devicePath.filename().string();

        const auto manufacturerPath = devicePath / "manufacturer";
        if (std::filesystem::exists(manufacturerPath)) {
            const std::string manufacturer = readTextFile(manufacturerPath.string());
            if (!manufacturer.empty()) {
                info.name = manufacturer + " " + info.name;
            }
        }

        const auto productNamePath = devicePath / "product";
        if (std::filesystem::exists(productNamePath)) {
            const std::string productName = readTextFile(productNamePath.string());
            if (!productName.empty()) {
                info.name = productName;
            }
        }

        if (std::filesystem::exists(devicePath / "dev")) {
            const std::string dev = readTextFile((devicePath / "dev").string());
            if (!dev.empty()) {
                info.port = "USB device: " + dev;
            }
        }

        bool isSerialPort = false;
        const std::filesystem::path ttyDir = "/sys/class/tty";
        if (std::filesystem::exists(ttyDir) && std::filesystem::is_directory(ttyDir)) {
            for (const auto& ttyEntry : std::filesystem::directory_iterator(ttyDir)) {
                if (!ttyEntry.is_directory()) {
                    continue;
                }

                const std::string ttyName = ttyEntry.path().filename().string();
                if (!isTtyDeviceName(ttyName)) {
                    continue;
                }

                const std::filesystem::path deviceLink = ttyEntry.path() / "device";
                if (!std::filesystem::exists(deviceLink)) {
                    continue;
                }

                const std::string resolved = readLinkTarget(deviceLink.string());
                if (resolved.empty()) {
                    continue;
                }

                if (resolved == devicePath.filename().string() || devicePath.filename().string().rfind(resolved, 0) == 0) {
                    info.port = ttyName;
                    isSerialPort = true;
                    break;
                }
            }
        }

        if (isSerialPort) {
            info.name += " [port série]";
        }

        devices.push_back(info);
    }

    return devices;
}

std::string UsbService::listConnectedDevices() const {
    const std::vector<UsbDeviceInfo> devices = discoverDevices();
    if (devices.empty()) {
        return "Aucun périphérique USB connecté actuellement.";
    }

    std::ostringstream result;
    result << "Périphériques USB détectés :\n";
    for (const auto& device : devices) {
        result << "- " << device.name;
        if (!device.vendor.empty() || !device.product.empty()) {
            result << " (VID: " << device.vendor << ", PID: " << device.product << ")";
        }
        if (!device.port.empty()) {
            result << " | port: " << device.port;
        }
        if (!device.serial.empty()) {
            result << " | serial: " << device.serial;
        }
        result << '\n';
    }
    return result.str();
}

} // namespace chatia
