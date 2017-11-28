#ifndef __THREADS_RUNNER_H__
#define __THREADS_RUNNER_H__
#include <mutex>
#include <string>

#include "aggregator.h"

class ParamsHolder {
public:
  ParamsHolder(ParamsHolder&&) = default;

  static ParamsHolder parseArgs(const int, const char**);

  const std::string& getDir() const { 
    return dir_;
  }

  unsigned getFilesNum() const { 
    return filesNum_;
  }

  unsigned getThreadsNum() const { 
    return threadsNum_;
  }

private:
  ParamsHolder() = default;

  std::string dir_;
  unsigned filesNum_;
  unsigned threadsNum_;
};

class ThreadsRunner {
public:
  ThreadsRunner(ParamsHolder&& params): params_(std::move(params)) { }

  rapidjson::Document getResult();

  void threadRoutine();

  std::string popNextTask();
  void mergeResults(Aggregator&&);
  
private:
  bool hasMoreTasks() const;
  void runThreads();

  std::mutex mut_;

  ParamsHolder params_;
  unsigned filesRequested_ = 0;

  Aggregator result_;
};

#endif
