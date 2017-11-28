#include <iostream>
#include <fstream>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include "threads_runner.h"

namespace json = rapidjson;

const std::string RESULTS_FILE = "agr.txt";

int main(const int argc, const char** argv) {
  try {
    ThreadsRunner runner(ParamsHolder::parseArgs(argc, argv));
    auto resultDoc = runner.getResult();

    std::cout << "Writing results to " 
	      << RESULTS_FILE << std::endl;

    std::ofstream rStream(RESULTS_FILE);
    json::OStreamWrapper rStreamWrapper(rStream);
    json::PrettyWriter<json::OStreamWrapper> writer(rStreamWrapper);

    resultDoc.Accept(writer);
  }
  catch (std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Usage: aggr [path] [number of files] [number of threads]" << std::endl;

    return 1;
  }

  return 0;
}
