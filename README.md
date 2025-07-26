*  block_ip.sh
*  block_ip.cpp


### Why? 
I was frustrated with maintaining and supporting multiple systems that used different scripts, so I merged them. However, my clients do not accept shell scripts, which required me to convert the final version into a C application. You can find both the script and the C application here.

#### How to Compile and Run the C++ Program:

    Save the code:
    Copy the C++ code above and save it to a file named block_ip.cpp.

    Install a C++ compiler (if you don't have one):
    Most Linux distributions come with g++ (GCC C++ compiler) pre-installed or easily available.

#### On Debian/Ubuntu:


sudo apt install build-essential

#### On Arch Linux:

    sudo pacman -Sy base-devel

#### Compile the program:
Open your terminal, navigate to the directory where you saved block_ip.cpp, and run the following command:

g++ block_ip.cpp -o block_ip_cpp

    g++: The C++ compiler.

    block_ip.cpp: Your source code file.

    -o block_ip_cpp: Specifies the output executable file name (block_ip_cpp).

If the compilation is successful, you will have an executable file named block_ip_cpp in the same directory.

#### Run the program:

Just like the shell script, this C++ program needs root privileges to modify iptables rules.

    sudo ./block_ip_cpp 1.2.3.4

    Replace 1.2.3.4 with the actual IP address you want to block.

#### Explanation of the C++ Code:

#include directives: Include necessary libraries for I/O, strings, system calls, and file operations.

exec_command(const std::string& cmd): This helper function is included for completeness, although system() is used for most commands for simplicity. popen() is generally preferred for capturing command output, but system() is sufficient for commands where you only care about the exit status.

is_debian_package_installed() and is_arch_package_installed(): These functions use dpkg -s (Debian) and pacman -Qs (Arch) to check for package installation status.

detect_distro(): Reads /etc/os-release to identify the distribution, similar to the shell script.

check_and_install_iptables(): Conditionally installs iptables-persistent (Debian/Ubuntu) or iptables (Arch) using sudo apt or sudo pacman.

block_ip(): Adds the iptables rule. It uses system() to execute the iptables commands. It also checks for existing rules using iptables-save and grep.

save_iptables_rules(): Handles persistence by calling netfilter-persistent save for Debian/Ubuntu or saving rules to /etc/iptables/iptables.rules and enabling/starting the iptables.service for Arch Linux.

main(int argc, char* argv[]): The entry point of the program. It parses the command-line argument for the IP address and orchestrates the calls to the other functions.

This C++ version provides a compiled executable, but fundamentally, it still relies on the underlying shell commands for iptables and package management, executed with root privileges.# tools
