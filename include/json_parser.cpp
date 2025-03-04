
#include "headers/json_parser.h"

void ns::to_json(json &j, const question &q) {
  j = json{{"id", q.id}, {"question", q.q_name}, {"answer", q.q_answer}};
}

void ns::from_json(const json &j, question &q) {
  j.at("id").get_to(q.id);
  j.at("question").get_to(q.q_name);
  j.at("correct_answer").get_to(q.q_answer);
}

json_instance::json_instance(std::string filename) {
  f.open(filename);
  if (!f.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  this->data = json::parse(f);
}
json_instance::~json_instance() { f.close(); }
void json_instance::populate(json_instance *quiz) {
  for (const auto &item : quiz->data["questions"]) {
    quiz->qs.push_back(item.get<ns::question>());
  }
}
std::string json_instance::get_nth_question(int nth) {
  if (nth < 0 || nth > qs.size())
    throw std::out_of_range("Invalid question number.");
  return qs[nth].q_name;
}
std::string json_instance::get_nth_answer(int nth) {
  if (nth < 0 || nth > qs.size())
    throw std::out_of_range("Invalid question number.");
  return qs[nth].q_answer;
}

int json_instance::quiz_size() { return qs.size(); }
