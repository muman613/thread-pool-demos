#include "curl/curl.h"
#include "curl/easy.h"
#include "fooComplexThreadPool.h"
#include <boost/algorithm/string.hpp>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace foo::utils;

struct downloadData {
  std::string url;
  std::string filename;
  bool success;
};

using downloadJobData = struct downloadData;
using jobDesc = jobDescription<downloadJobData>;
using jobFunc = ComplexThreadPool<downloadJobData>::jobFunc;

/**
 *
 * @param get_url
 * @param filename
 * @return
 */
bool do_download_url(const std::string &get_url, const std::string &filename) {
  //  , "do_download_url(", get_url, ", ", filename, ")" );
  auto curl_easy = curl_easy_init();
  if (curl_easy) {
    FILE *fp = fopen(filename.c_str(), "w");
    if (fp == nullptr) {
      throw std::runtime_error("Unable to open file");
    }
    CURLcode res;
    curl_easy_setopt(curl_easy, CURLOPT_URL, get_url.c_str());
    curl_easy_setopt(curl_easy, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, fp);

    res = curl_easy_perform(curl_easy);
    //    , "Res ", res);
    fclose(fp);
  }

  return true;
}

downloadJobData worker_thread(const jobDescription<downloadJobData> &job_data) {
  auto download_data = job_data.data;

  download_data.success =
      do_download_url(download_data.url, download_data.filename);

  return download_data;
}

std::size_t load_queues(ComplexThreadPool<downloadJobData> &pool,
                        const std::string &filename) {
  std::vector<std::string> fileData;
  std::ifstream ifs{filename.c_str()};
  std::size_t num_jobs{SIZE_MAX};

  if (ifs.is_open()) {
    std::string line;
    while (!ifs.eof()) {
      ifs >> line;
      fileData.push_back(line);
    }

    num_jobs = 0;

    for (const auto &file_line : fileData) {
      auto sep = file_line.find(',');
      if (sep != std::string::npos) {
        std::string url = file_line.substr(0, sep);
        std::string fname = file_line.substr(sep + 1);
        boost::trim(url);
        boost::trim(fname);

        //        , "Found : url ", url, " fn ", fname);

        jobDesc jd{"download job", {url, fname, false}};
        pool.queue_job(std::pair<jobFunc, jobDesc>(worker_thread, jd));
        num_jobs++;
      }
    }
  }

  return num_jobs;
}

int main(int argc, char *argv[]) {
  ComplexThreadPool<downloadJobData> pool{8};

  const auto num_jobs = load_queues(pool, "database.txt");

  while (num_jobs != pool.result_count()) {
    std::this_thread::sleep_for(std::chrono::microseconds(10000));
  }
  
  return 0;
}