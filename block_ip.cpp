//-----------------------------------------------------------------------------------------------------
// Description: IP blocker4ever
// Disable Login Sound
// Authors: Andreas Mueller
// Licence: 2025 MIT
// Website: https://www.gnoppix.org
//-----------------------------------------------------------------------------------------------------
// Info:
//
// This script blocks a specified IP address using iptables and ensures the rule persists after reboot.
// It is designed to work on both Debian/Ubuntu-based and Arch Linux distributions.
//-----------------------------------------------------------------------------------------------------

#include <iostream> // For input/output operations (cout, cerr)
#include <string>   // For string manipulation
#include <vector>   // For dynamic arrays (not strictly used here, but good practice)
#include <cstdlib>  // For system() function
#include <fstream>  // For file operations (reading /etc/os-release)

// Function to execute a shell command and return its output
// Note: This is a simplified version and might not capture all output or errors
std::string exec_command(const std::string& cmd) {
    char buffer[128];
    std::string result = "";
    // Use popen to execute the command and read its output
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: popen() failed for command: " << cmd << std::endl;
        return "";
    }
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

// Function to check if a package is installed (for Debian/Ubuntu)
bool is_debian_package_installed(const std::string& package_name) {
    std::string cmd = "dpkg -s " + package_name + " &>/dev/null";
    return system(cmd.c_str()) == 0;
}

// Function to check if a package is installed (for Arch Linux)
bool is_arch_package_installed(const std::string& package_name) {
    std::string cmd = "pacman -Qs " + package_name + " &>/dev/null";
    return system(cmd.c_str()) == 0;
}

// Function to detect the Linux distribution
std::string detect_distro() {
    std::ifstream os_release_file("/etc/os-release");
    std::string line;
    std::string distro = "unknown";

    if (os_release_file.is_open()) {
        while (getline(os_release_file, line)) {
            if (line.find("ID=debian") != std::string::npos || line.find("ID=ubuntu") != std::string::npos) {
                distro = "debian";
                break;
            } else if (line.find("ID=arch") != std::string::npos) {
                distro = "arch";
                break;
            }
        }
        os_release_file.close();
    }
    return distro;
}

// Function to check and install iptables based on detected distribution
bool check_and_install_iptables(const std::string& distro) {
    std::cout << "Checking iptables installation..." << std::endl;
    if (distro == "debian") {
        if (!is_debian_package_installed("iptables-persistent")) {
            std::cout << "iptables-persistent is not installed. Installing now..." << std::endl;
            if (system("sudo apt update && sudo apt install -y iptables-persistent") != 0) {
                std::cerr << "Error: Failed to install iptables-persistent. Exiting." << std::endl;
                return false;
            }
            std::cout << "iptables-persistent installed successfully." << std::endl;
        } else {
            std::cout << "iptables-persistent is already installed." << std::endl;
        }
    } else if (distro == "arch") {
        if (!is_arch_package_installed("iptables")) {
            std::cout << "iptables is not installed. Installing now..." << std::endl;
            if (system("sudo pacman -Sy iptables --noconfirm") != 0) {
                std::cerr << "Error: Failed to install iptables. Exiting." << std::endl;
                return false;
            }
            std::cout << "iptables installed successfully." << std::endl;
        } else {
            std::cout << "iptables is already installed." << std::endl;
        }
    }
    return true;
}

// Function to block the IP
bool block_ip(const std::string& ip_to_block) {
    if (ip_to_block.empty()) {
        std::cerr << "Error: No IP address provided." << std::endl;
        std::cerr << "Usage: sudo ./block_ip_cpp <IP_ADDRESS_TO_BLOCK>" << std::endl;
        return false;
    }

    std::cout << "Blocking IP address: " << ip_to_block << "..." << std::endl;

    // Check if the rule already exists to prevent duplicates
    std::string check_cmd = "sudo iptables-save | grep -q -- \"-A INPUT -s " + ip_to_block + " -j DROP\"";
    if (system(check_cmd.c_str()) == 0) {
        std::cout << "Rule to block " << ip_to_block << " already exists." << std::endl;
    } else {
        // Add the rule to drop all traffic from the specified IP
        std::string add_rule_cmd = "sudo iptables -A INPUT -s " + ip_to_block + " -j DROP";
        if (system(add_rule_cmd.c_str()) != 0) {
            std::cerr << "Error: Failed to add iptables rule for " << ip_to_block << "." << std::endl;
            return false;
        }
        std::cout << "Successfully added iptables rule to block " << ip_to_block << "." << std::endl;
    }
    return true;
}

// Function to save iptables rules for persistence
bool save_iptables_rules(const std::string& distro) {
    std::cout << "Saving iptables rules for persistence..." << std::endl;

    if (distro == "debian") {
        if (system("sudo netfilter-persistent save") != 0) {
            std::cerr << "Error: Failed to save iptables rules. Check your netfilter-persistent installation." << std::endl;
            return false;
        }
        std::cout << "iptables rules saved successfully for Debian/Ubuntu." << std::endl;
    } else if (distro == "arch") {
        // Save current IPv4 rules to the default location for the systemd service
        if (system("sudo iptables-save > /etc/iptables/iptables.rules") != 0) {
            std::cerr << "Error: Failed to save iptables rules to /etc/iptables/iptables.rules." << std::endl;
            return false;
        }

        // Enable and start the iptables systemd service to load rules on boot
        std::cout << "Enabling and starting iptables systemd service..." << std::endl;
        if (system("sudo systemctl enable iptables.service && sudo systemctl start iptables.service") != 0) {
            std::cerr << "Error: Failed to enable/start iptables.service. Check systemd logs." << std::endl;
            return false;
        }
        std::cout << "iptables rules saved and persistence enabled successfully for Arch Linux." << std::endl;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "Starting IP blocking program..." << std::endl;

    if (argc < 2) {
        std::cerr << "Error: No IP address provided." << std::endl;
        std::cerr << "Usage: sudo ./block_ip_cpp <IP_ADDRESS_TO_BLOCK>" << std::endl;
        return 1;
    }

    std::string ip_to_block = argv[1];
    std::string detected_distro = detect_distro();

    if (detected_distro == "unknown") {
        std::cerr << "Error: Unsupported distribution. Exiting." << std::endl;
        return 1;
    }

    if (!check_and_install_iptables(detected_distro)) {
        return 1;
    }

    if (!block_ip(ip_to_block)) {
        return 1;
    }

    if (!save_iptables_rules(detected_distro)) {
        return 1;
    }

    std::cout << "Program finished. The IP address '" << ip_to_block << "' is now blocked and the rule will persist after reboot." << std::endl;
    std::cout << "You can verify the rule by running: sudo iptables -L INPUT -n --line-numbers" << std::endl;
    std::cout << std::endl;
    std::cout << "--- To remove the rule ---" << std::endl;
    std::cout << "1. Find its line number (e.g., N) by running: sudo iptables -L INPUT -n --line-numbers" << std::endl;
    std::cout << "2. Remove the rule: sudo iptables -D INPUT N" << std::endl;

    if (detected_distro == "debian") {
        std::cout << "3. After removing, remember to save changes: sudo netfilter-persistent save" << std::endl;
    } else if (detected_distro == "arch") {
        std::cout << "3. After removing, remember to save changes: sudo iptables-save > /etc/iptables/iptables.rules && sudo systemctl restart iptables.service" << std::endl;
    }

    return 0;
}
