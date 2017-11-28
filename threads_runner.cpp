#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <rapidjson/document.h>

#include "threads_runner.h"

namespace json = rapidjson;

const std::string FILENAME_PREFIX = "file";
const std::string FILENAME_POSTFIX = ".log";

ParamsHolder ParamsHolder::parseArgs(const int argc, const char** argv) {
  if (argc < 3)
    throw std::invalid_argument("Too few arguments");

  if (argc > 4)
    throw std::invalid_argument("Too many arguments");

  ParamsHolder result;

  // First arg -- directory
  result.dir_ = argv[1];    
  if (!result.dir_.empty())
    while (result.dir_.back() == '/')
      result.dir_.pop_back();

  // Second arg -- number of files
  const int secArg = atoi(argv[2]);
  if (secArg <= 0)
    throw std::invalid_argument("Incorrect number of files");
  result.filesNum_ = static_cast<unsigned>(secArg);

  // Third (optional) arg -- numbers of threads
  if (argc > 3) {
    const int thirdArg = atoi(argv[3]);
    if (thirdArg <= 0)
      throw std::invalid_argument("Incorrect number of threads");

    result.threadsNum_ = static_cast<unsigned>(thirdArg);
  }
  else {
    result.threadsNum_ = std::thread::hardware_concurrency();
    // Sadly, zero is a possible result
    if (!result.threadsNum_)
      ++result.threadsNum_;
  }

  // Threads not more than files
  result.threadsNum_ = std::min(result.threadsNum_, 
				result.filesNum_);

  return result;
}

std::string ThreadsRunner::popNextTask() {
  std::lock_guard<std::mutex> l(mut_);

  if (!hasMoreTasks())
    return std::string();

  ++filesRequested_;

  std::ostringstream fileName;
  fileName << params_.getDir() << "/"
	   << FILENAME_PREFIX << filesRequested_ << FILENAME_POSTFIX;

  std::cout << "Processing " << fileName.str() << std::endl;
  return fileName.str();
}

void ThreadsRunner::mergeResults(Aggregator&& rhs) {
  std::lock_guard<std::mutex> l(mut_);
  result_.merge(std::move(rhs));
}

json::Document ThreadsRunner::getResult() {
  if (hasMoreTasks())
    runThreads();

  std::cout << "Parsing finished, building result" << std::endl;

  // NB: sorting can potentially be using multithreaded qsort if too
  // slow...
  return result_.toJSON();
}
  
bool ThreadsRunner::hasMoreTasks() const {
  return filesRequested_ < params_.getFilesNum();
}

void ThreadsRunner::threadRoutine() {
  Aggregator threadResult;
  std::string fileName;

  while (!(fileName = popNextTask()).empty()) {
    std::ifstream stream(fileName);
    if (!stream) {
      std::lock_guard<std::mutex> l(mut_);
      std::cerr << "! Cannot open file " << fileName << std::endl;
      continue;
    }

    std::string line;
    while (std::getline(stream, line)) {
      json::Document document;
      document.Parse<json::kParseNumbersAsStringsFlag>(line.c_str());

      if (document.IsObject())
	threadResult.registerEvent(document);
    }
  }

  mergeResults(std::move(threadResult));
}

void ThreadsRunner::runThreads() {
  std::cout << "Running with " << params_.getThreadsNum() 
	    << " thread" << (params_.getThreadsNum() > 1 ? "s" : "")
	    << "..." << std::endl;

  std::vector<std::thread> threads;
  threads.reserve(params_.getThreadsNum());
    
  for (unsigned i = 0; i < params_.getThreadsNum(); ++i)
    threads.emplace_back(&ThreadsRunner::threadRoutine, this);

  for (auto& thread : threads)
    thread.join();
}
