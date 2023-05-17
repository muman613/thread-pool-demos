//#include <iostream>
#include <string>
#include <cstdio>
#include <thread>
//#include <vector>
// #include <LogMessage.hpp>
//#include <fooQueue.h>
#include "fooSimpleThreadPool.h"
#include "curl/curl.h"
#include "curl/easy.h"

using namespace foo::utils;

/**
 *
 * @param get_url
 * @param filename
 * @return
 */
bool do_download_url( const std::string & get_url, const std::string & filename)
{
  auto curl_easy = curl_easy_init();
  if (curl_easy) {
    FILE * fp = fopen(filename.c_str(), "w");
    if (fp == nullptr) {
      throw std::runtime_error("Unable to open file");
    }
    [[maybe_unused]] CURLcode res;
    curl_easy_setopt(curl_easy, CURLOPT_URL, get_url.c_str());
    curl_easy_setopt(curl_easy, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, fp);

    res = curl_easy_perform(curl_easy);
//    , "Res ", res);
    fclose(fp);
  }

  return true;
}

#ifdef ENABLE_LAMBDA_TEST
void queue_lamba_jobs( fooSimpleThreadPool & pool) {
  for (int i = 0 ; i < 100 ; i++) {
    pool.queue_job([]() -> bool {
        , "Called from job thread id ", std::this_thread::get_id());
        std::this_thread::sleep_for(std::chrono::microseconds(100 * (std::rand() % 10000)));
        return false;
    });
  }
}
#endif

int main(int argc, char * argv[])
{
  fooSimpleThreadPool pool(8);

  pool.queue_job(std::bind(do_download_url, std::string("https://www.stackoverflow.com"), std::string("stackoverflow.html")));
  pool.queue_job(std::bind(do_download_url, std::string("https://www.foxnews.com"), std::string("foxnews.html")));
  pool.queue_job(std::bind(do_download_url, std::string("https://www.cnn.com"), std::string("cnn.html")));

  while (pool.jobs_pending()) {
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }

  pool.stop_threads();

  return 0;
}