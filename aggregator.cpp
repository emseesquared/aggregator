#include <algorithm>
#include <array>
#include <ctime>
#include <functional>
#include <vector>

#include <rapidjson/document.h>

#include "aggregator.h"

namespace json = rapidjson;

namespace {
template <typename Key, typename Value>
std::vector<const Key*> 
getSortedKeyPtrs(const std::unordered_map<Key, Value>& map, 
		 std::function<bool(const Key&, const Key&)> comparator = std::less<Key>()) {
  std::vector<const Key*> result;
  result.reserve(map.size());

  std::transform(map.begin(),
		 map.end(), 
		 std::back_inserter(result),
		 [](const auto& val) { return &(val.first); });

  std::sort(result.begin(), 
	    result.end(), 
	    [&comparator](auto lhs, auto rhs) { return comparator(*lhs, *rhs); });

  return result;
}

std::string getDayStr(const std::time_t timestamp) {
  // Yeah this sucks I know. But still faster than stringstream
  char buf[11];
  std::strftime(buf, 11, "%Y-%m-%d", std::gmtime(&timestamp));

  std::string result;
  result.reserve(10);

  // Strip leading zeros from month and day
  for (short i = 0; buf[i] != '\0'; ++i)
    if (buf[i] != '0' || result.back() != '-')
      result.push_back(buf[i]);

  return result;
}

struct UintStrLess {
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    auto mm = std::mismatch(lhs.begin(), lhs.end(), rhs.begin());
  
    if (mm.first == lhs.end())
      return !(mm.second == rhs.end());
    if (mm.second == rhs.end())
      return false;

    const auto numlenL = std::find(mm.first, lhs.end(), ',') - mm.first;
    const auto numlenR = std::find(mm.second, rhs.end(), ',') - mm.second;
    if (numlenL != numlenR)
      return numlenL < numlenR;

    return *(mm.first) < *(mm.second);
  }
};

const std::array<std::string, 10> propNames = { "prop1", "prop2", "prop3", "prop4", 
						"prop5", "prop6", "prop7", "prop8", 
						"prop9", "prop10" };
const uint32_t secondsInADay =  60 * 60 * 24;

}  // namespace

void Aggregator::registerEvent(const json::Document& doc) {
  Timestamp day = std::atol(doc["ts_fact"].GetString());
  // Unix timestamps have no leap seconds so this works
  day-= day % secondsInADay;

  const auto& props = doc["props"];

  auto it = propNames.cbegin();
  std::string propStr(props[it->c_str()].GetString());
  while (++it != propNames.cend()) {
    propStr.push_back(',');
    propStr+= props[it->c_str()].GetString();
  }

  ++data_[day][doc["fact_name"].GetString()][propStr];
}

void Aggregator::merge(Aggregator&& rhs) {
  if (data_.empty()) {
    std::swap(data_, rhs.data_);
    return;
  }
    
  for (const auto& byDate : rhs.data_)
    for (const auto& byFactName : byDate.second)
      for (const auto& byPropsComb : byFactName.second)
	data_[byDate.first][byFactName.first][byPropsComb.first]+= byPropsComb.second;
}

json::Document Aggregator::toJSON() const {
  json::Document resultDoc;
  auto& allocator = resultDoc.GetAllocator();
  resultDoc.SetObject();

  const auto dates = getSortedKeyPtrs(data_);
  for (const auto date : dates) {
    json::Value dateObj(json::kObjectType);
    
    const auto dateIter = data_.find(*date);
    const auto factNames = getSortedKeyPtrs(dateIter->second);

    for (const auto factName : factNames) {
      json::Value factObj(json::kObjectType);

      const auto factIter = dateIter->second.find(*factName);
      const auto propLines = getSortedKeyPtrs<std::string>(factIter->second, UintStrLess());

      for (const auto propLine : propLines)
	factObj.AddMember(json::Value(propLine->c_str(), allocator).Move(),
			  json::Value(factIter->second.find(*propLine)->second).Move(),
			  allocator);

      dateObj.AddMember(json::Value(factName->c_str(), allocator).Move(),
			factObj.Move(),
			allocator);
    }

    resultDoc.AddMember(json::Value(getDayStr(*date).c_str(), allocator).Move(),
			dateObj.Move(),
			allocator);
  }

  return resultDoc;
}
