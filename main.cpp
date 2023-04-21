#include <unistd.h>             // getpagesize()
#include <sys/mman.h>           // munmap()
#include <fcntl.h>              // O_ flags
#include <sys/stat.h>
#include <stdint.h>
#include <dirent.h>

#include <string>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cerrno>
#include <cstring>

#define ERROR_BODY(message) std::cout << "ERROR:\t" << __FUNCTION__ << ":\t" \
        << message << std::endl
#define pr_errno() ERROR_BODY(std::strerror(errno))
#define pr_errno_msg(message) ERROR_BODY(std::strerror(errno) << std::endl << message)
#define pr_err_msg(message) ERROR_BODY(message)

std::string get_pci_dbdf(uint32_t vendor, uint32_t device, unsigned int func_num)
{
	DIR *dir;
	std::string err = std::string();

	if ((dir = opendir("/sys/bus/pci/devices")) == nullptr) {
		pr_errno();
		return err;
	}

	// iterate over all PCIe devices
	struct dirent *d;
	std::ifstream ifstr;
	std::string bdf_found;
	while ((d = readdir(dir)) != nullptr) {

		// only consider actual device folders, not ./ and ../
		if (strstr(d->d_name, "0000:") != nullptr) {
			bdf_found = std::string(d->d_name);
			// Continue only if the function number matches
			if ((unsigned int)(bdf_found.back() - '0') != func_num)
				continue;
			std::string path("/sys/bus/pci/devices/" + bdf_found);
			// read vendor id
			ifstr.open(path + "/vendor");
			std::string tmp((std::istreambuf_iterator<char>(ifstr)), std::istreambuf_iterator<char>());
			ifstr.close();

			// check if vendor id is correct
			if (std::stoul(tmp, nullptr, 16) == vendor) {
				// read device id
				ifstr.open(path + "/device");
				std::string tmp((std::istreambuf_iterator<char>(ifstr)),
								std::istreambuf_iterator<char>());
				ifstr.close();
				// check if device also fits
				if (std::stoul(tmp, nullptr, 16) == device)
						return bdf_found;
			}
		}
	}
	pr_err_msg("BDF not found for 0x" << std::setbase(16)
			   << vendor << ":0x" << device
			   << " Func" << func_num);

	return err;
}


void mmap_pcie_bar_test(uint32_t vendor, uint32_t device, unsigned int func_num, unsigned int bar_num, uint64_t offset)
{
	auto dbdf = get_pci_dbdf(vendor, device, func_num);
	std::cout << "dbdf " << dbdf << ", Func num: " << func_num << ", Bar num: " <<  bar_num << "\n";
	if (dbdf.empty())
		exit(EXIT_FAILURE);
	std::string sysfs_bar_file = "/sys/bus/pci/devices/" + dbdf + "/resource" + std::to_string(bar_num);// + "_wc";
	std::cout << "Open bar file: " << sysfs_bar_file << "\n";
	int bar_fd = open(sysfs_bar_file.c_str(), O_RDWR | O_SYNC);
	if (bar_fd < 0) {
		pr_errno_msg("Maybe execute via sudo?");
		return;
	}

	struct stat st;
	stat(sysfs_bar_file.c_str(), &st); // get BAR size through sysfs file size. Doesn't work!
	std::cout << "stat return bar size: " << st.st_size << "\n";
	uint64_t bar_size = st.st_size;
	std::cout << "mmap size: " << bar_size / 1048576 << " MB\n";
	void *bar = mmap(0, bar_size, PROT_READ | PROT_WRITE, MAP_SHARED, bar_fd, 0);
	if (bar == MAP_FAILED) {
		pr_errno();
		close(bar_fd);
		return;
	}
	std::cout << "mmap succeeded, read something now\n";
	uint8_t *b = (uint8_t *)bar;
	printf("Offset 0x%llX (%lu M): ", offset * 1048576ull, offset);
	for(uint64_t i = 0;i < 16ull;i++)
	  printf("0x%02X ", b[offset * 1048576 + i]);
	printf("\n");
	munmap(bar, bar_size);
	close(bar_fd);
}


// AMD
#define VENDOR   0x1002
// RX 7900 XTX
#define DEVICE   0x744c
#define FUNCTION_NUM 0
#define BAR_NUM      0

int main(int argc, char *argv[])
{
	if(argc != 2) {
		printf("Format: ./main <offset_in_mbytes>\n");
		std::exit(1);
	}
	uint64_t offset = std::stoull(argv[1]);
	mmap_pcie_bar_test(VENDOR, DEVICE, FUNCTION_NUM, BAR_NUM, offset);
}

