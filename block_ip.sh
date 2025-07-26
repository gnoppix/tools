#!/bin/bash

#-----------------------------------------------------------------------------------------------------
# Description: IP blocker4ever  
# Authors: Andreas Mueller
# Licence: 2025 MIT 
# Website: https://www.gnoppix.org
#-----------------------------------------------------------------------------------------------------
# Info: 
#
# This script blocks a specified IP address using iptables and ensures the rule persists after reboot.
# It is designed to work on both Debian/Ubuntu-based and Arch Linux distributions.
#-----------------------------------------------------------------------------------------------------


# --- Global Variables ---
DISTRO="" # To store the detected distribution (debian or arch)

# --- Functions ---

# Function to detect the Linux distribution
detect_distro() {
    if grep -q "ID=debian" /etc/os-release || grep -q "ID=ubuntu" /etc/os-release; then
        DISTRO="debian"
        echo "Detected distribution: Debian/Ubuntu-based"
    elif grep -q "ID=arch" /etc/os-release; then
        DISTRO="arch"
        echo "Detected distribution: Arch Linux"
    else
        echo "Error: Unsupported distribution. This script supports Debian/Ubuntu and Arch Linux."
        exit 1
    fi
}

# Function to check and install iptables based on detected distribution
check_iptables_installation() {
    if [ "$DISTRO" == "debian" ]; then
        if ! dpkg -s iptables-persistent &>/dev/null; then
            echo "iptables-persistent is not installed. Installing now..."
            sudo apt update
            sudo apt install -y iptables-persistent
            if [ $? -ne 0 ]; then
                echo "Error: Failed to install iptables-persistent. Exiting."
                exit 1
            fi
            echo "iptables-persistent installed successfully."
        else
            echo "iptables-persistent is already installed."
        fi
    elif [ "$DISTRO" == "arch" ]; then
        if ! pacman -Qs iptables &>/dev/null; then
            echo "iptables is not installed. Installing now..."
            sudo pacman -Sy iptables --noconfirm
            if [ $? -ne 0 ]; then
                echo "Error: Failed to install iptables. Exiting."
                exit 1
            fi
            echo "iptables installed successfully."
        else
            echo "iptables is already installed."
        fi
    fi
}

# Function to block the IP
block_ip() {
    # Get the IP address from the first command-line argument
    local IP_TO_BLOCK="$1"

    if [ -z "$IP_TO_BLOCK" ]; then
        echo "Error: No IP address provided."
        echo "Usage: sudo ./block_ip.sh <IP_ADDRESS_TO_BLOCK>"
        exit 1
    fi

    echo "Blocking IP address: $IP_TO_BLOCK..."

    # Check if the rule already exists to prevent duplicates
    # Using iptables-save to check current rules for more robust check
    if sudo iptables-save | grep -q -- "-A INPUT -s $IP_TO_BLOCK -j DROP"; then
        echo "Rule to block $IP_TO_BLOCK already exists."
    else
        # Add the rule to drop all traffic from the specified IP
        sudo iptables -A INPUT -s "$IP_TO_BLOCK" -j DROP
        if [ $? -eq 0 ]; then
            echo "Successfully added iptables rule to block $IP_TO_BLOCK."
        else
            echo "Error: Failed to add iptables rule for $IP_TO_BLOCK."
            exit 1
        fi
    fi
}

# Function to save iptables rules for persistence based on detected distribution
save_iptables_rules() {
    echo "Saving iptables rules for persistence..."

    if [ "$DISTRO" == "debian" ]; then
        sudo netfilter-persistent save
        if [ $? -eq 0 ]; then
            echo "iptables rules saved successfully for Debian/Ubuntu."
        else
            echo "Error: Failed to save iptables rules. Check your netfilter-persistent installation."
            exit 1
        fi
    elif [ "$DISTRO" == "arch" ]; then
        # Save current IPv4 rules to the default location for the systemd service
        sudo iptables-save > /etc/iptables/iptables.rules
        if [ $? -ne 0 ]; then
            echo "Error: Failed to save iptables rules to /etc/iptables/iptables.rules."
            exit 1
        fi

        # Enable and start the iptables systemd service to load rules on boot
        echo "Enabling and starting iptables systemd service..."
        sudo systemctl enable iptables.service
        sudo systemctl start iptables.service

        if [ $? -eq 0 ]; then
            echo "iptables rules saved and persistence enabled successfully for Arch Linux."
        else
            echo "Error: Failed to enable/start iptables.service. Check systemd logs."
            exit 1
        fi
    fi
}

# --- Main Script Execution ---

echo "Starting IP blocking script..."

# 1. Detect the distribution
detect_distro

# 2. Check and install iptables/iptables-persistent
check_iptables_installation

# 3. Block the specified IP, passing the first argument to the function
block_ip "$1"

# 4. Save the rules and enable persistence
save_iptables_rules

echo "Script finished. The IP address '$1' is now blocked and the rule will persist after reboot."
echo "You can verify the rule by running: sudo iptables -L INPUT -n --line-numbers"
echo ""
echo "--- To remove the rule ---"
echo "1. Find its line number (e.g., N) by running: sudo iptables -L INPUT -n --line-numbers"
echo "2. Remove the rule: sudo iptables -D INPUT N"
if [ "$DISTRO" == "debian" ]; then
    echo "3. After removing, remember to save changes: sudo netfilter-persistent save"
elif [ "$DISTRO" == "arch" ]; then
    echo "3. After removing, remember to save changes: sudo iptables-save > /etc/iptables/iptables.rules && sudo systemctl restart iptables.service"
fi

