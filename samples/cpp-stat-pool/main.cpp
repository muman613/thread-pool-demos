
#include "fooComplexThreadPool.h"
#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
namespace po = boost::program_options;

using namespace foo::utils;

struct cppJobData {
  fs::path cpp_file_path;
  size_t num_lines{0};
  size_t num_blanks{0};
  size_t num_whitespace{0};
};

using jobFunc = ComplexThreadPool<cppJobData>::jobFunc;
using jobDesc = jobDescription<cppJobData>;

cppJobData worker_thread(const jobDescription<cppJobData> &job_data) {
  fs::path target_file = job_data.data.cpp_file_path;
  auto result = job_data.data;

  if (fs::exists(target_file)) {
    std::ifstream ifs{target_file};
    std::string line;

    if (ifs.is_open()) {
      while (std::getline(ifs, line)) {
        if (line == "") {
          result.num_blanks++;
        }
        for (const char ch : line) {
          if (std::isspace(static_cast<unsigned char>(ch))) {
            result.num_whitespace++;
          }
        }
        result.num_lines++;
      }
    }
  }
  return result;
}

int main(int argc, char *argv[]) {
  po::options_description options{"foobar12"};
  options.add_options()("help", "Display help message")(
      "scan-dir", po::value<std::string>()->default_value("."),
      "Directory to scan");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << options << "\n";
    return 1;
  }

  auto path_to_scan = vm["scan-dir"].as<std::string>();

  ComplexThreadPool<cppJobData> pool{8};

  fs::path path(path_to_scan);

  if (fs::exists(path)) {
    std::cout << "Scan directory exists..." << std::endl;
  } else {
    std::cerr << "Scan directory does not exist. Exiting with error..."
              << std::endl;
    return 10;
  }

  std::size_t num_jobs{0};

  try {
    for (auto &p : fs::recursive_directory_iterator(
             path, fs::directory_options::skip_permission_denied)) {
      auto ext = p.path().extension();
      if ((ext == ".cpp") or (ext == ".h")) {
        jobDesc desc;
        desc.data.cpp_file_path = p;
        pool.queue_job(std::pair<jobFunc, jobDesc>(worker_thread, desc));
        num_jobs++;
      }
    }

  } catch (fs::filesystem_error &e) {
    std::cout << "Caught exception : " << e.what() << std::endl;
  }

  while (num_jobs != pool.result_count()) {
    std::this_thread::sleep_for(std::chrono::microseconds(10000));
  }
  auto results = pool.get_results();
  // sort results

  std::sort(results.begin(), results.end(),
            [](const jobDesc &a, const jobDesc &b) {
              return ((a.data.cpp_file_path < b.data.cpp_file_path));
            });

  for (const auto &job_results : results) {
    auto num_lines = job_results.data.num_lines;
    auto num_blanks = job_results.data.num_blanks;
    auto num_whitespace = job_results.data.num_whitespace;

    auto pct = ((float)num_blanks * (float)100) / (float)num_lines;

    std::cout << "File : " << job_results.data.cpp_file_path.string()
              << " Total Lines : " << num_lines << " Blanks : " << num_blanks
              << " (" << pct << "%) White-space : " << num_whitespace
              << std::endl;
  }

  return 0;
}