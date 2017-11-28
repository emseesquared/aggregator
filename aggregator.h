#ifndef __AGGREGATOR_H__
#define __AGGREGATOR_H__
#include <cstdint>
#include <string>
#include <unordered_map>

class Aggregator {
public:
  Aggregator() = default;
  Aggregator(const Aggregator&) = delete;

  void registerEvent(const rapidjson::Document&);
  void merge(Aggregator&&);
  rapidjson::Document toJSON() const;

private:
  using Timestamp = uint32_t;

  using MapByProps = std::unordered_map<std::string, uint32_t>;
  using MapByFactsProps = std::unordered_map<std::string, MapByProps>;
  using MapByDatesFactsProps = std::unordered_map<Timestamp, MapByFactsProps>;

  MapByDatesFactsProps data_;
};

#endif
