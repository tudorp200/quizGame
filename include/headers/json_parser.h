
#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace ns {
struct question {
  int id;
  std::string q_name;
  std::string q_answer;
};

void to_json(json &j, const question &q);

void from_json(const json &j, question &q);
} // namespace ns

class json_instance {
private:
  json data;
  std::ifstream f;
  std::vector<ns::question> qs;

public:
  json_instance(std::string filename);
  ~json_instance();
  static void populate(json_instance *);
  int quiz_size();
  std::string get_nth_question(int);
  std::string get_nth_answer(int);
};

#endif // !JSON_PARSER_H
