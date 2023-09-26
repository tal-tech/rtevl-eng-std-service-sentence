#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>
#include<vector>
#include<string>
#include<cstring>
#include<cstdlib>
#include<iterator>
#include "speech_evaluator.h"
// using namespace staticjson;
using namespace std::chrono;


template<class Container>
void StringSplit(const std::string& str, Container& container, char delim = ' ')
{
  std::stringstream ss(str);
  std::string token;
  while(std::getline(ss, token, delim)) {
    if (token != "") {
      container.push_back(token);
    }
  }
}

bool SingleTest(SpeechEvaluator& se, std::string& text_ref, const std::string& wav_fn, std::ofstream& ofs)
{
  std::ifstream wav(wav_fn, std::ios::binary);
  std::vector<char> wav_content(static_cast<std::istreambuf_iterator<char>>(wav), {});

  int data_offset = 0;
  if (data_offset != 0) {
    data_offset += 8;
  }
  char* wav_data = wav_content.data() + data_offset;
  int wav_len = static_cast<int>(wav_content.size()) - data_offset;


  std::string eval_ref_str=R"raw({"assess_ref":{"text":"是什么声音，像一串小铃铛，轻轻地走过村边？是什么光芒，像一匹明洁的丝绸，映照着蓝天？啊，河流醒来了！","core_type":"cn.pred.score"}})raw";
  const char *oov_result;
  unsigned int oov_result_len;
  int oov_ret = se.DetectOOV(eval_ref_str.c_str(), eval_ref_str.size(),
                             &oov_result, &oov_result_len);
  if (oov_ret != 1000) {
    std::cout << "Error: DetectOOV failed! " << oov_ret << std::endl;
    return false;
  }

  int start_ret = se.Start(eval_ref_str.c_str(), eval_ref_str.size());
  if (start_ret != 1000) {
    std::cout << "Error: Start failed! " << start_ret << std::endl;
    return false;
  }

  //std::vector<std::string> fn_splits;
  //StringSplit(wav_fn, fn_splits, '/');
  std::string mid_result;

  //std::string fn = fn_splits[fn_splits.size() - 1];

  constexpr int step = 4000 ;
  int i = 0;
  int size = 0;
  while(i < wav_len)
  {
    char const* mid_result_c;
    unsigned int mid_result_len;
    int r;
    size = std::min(step, static_cast<int>(wav_len - i));
    if (wav_len - i <= step) {
      char tmp[step];
      memcpy(tmp, wav_data + i, wav_len - i);
      r = se.Evaluate(wav_data + i, size, false, &mid_result_c, &mid_result_len);
      r = se.Evaluate(0, 0, true, &mid_result_c, &mid_result_len);
      mid_result = std::string(mid_result_c, mid_result_len);
      std::cout << mid_result << std::endl;
      ofs << wav_fn << " " << oov_result << " " << r << " " << mid_result << std::endl;
    }
    else {
      r = se.Evaluate(wav_data + i, step, false, &mid_result_c, &mid_result_len);
      mid_result = std::string(mid_result_c, mid_result_len);
      if (r > 2000 && r < 3000) {
        // 截停
        std::cout << mid_result << std::endl;
        ofs << wav_fn << " " << oov_result << " " << r << " " << mid_result << std::endl;
        break;
      }
    }
    i += size;
  }

  se.Reset();
  return true;
}

void PerformanceTest(const std::string& cfg_fn, const std::string& param_fn, const std::string& text_fn)
{
  std::ifstream ftext(text_fn);
  std::string text_line;
  std::string wav_line;
  std::ofstream ofs("eval_result.txt");

  int err_code;
  SpeechEvaluator* se = CreateEvalInstance(cfg_fn.c_str(), &err_code);
  if (se == nullptr) {
    std::cerr << "CreateEvalInstance error: " << err_code << std::endl;
    return;
  }

  std::ifstream param_file(param_fn);
  std::string param_str="";
  std::string line;

while (std::getline(param_file, line)) {
    param_str+=line;
}
//  std::string param_str((std::istream_iterator<char>(param_file)),std::istream_iterator<char>());
  if (int setparam_ret = se->SetParams(param_str.c_str(), param_str.size()) !=1000) {
	  std::cout<<param_str.size()<<std::endl;
    std::cerr << "SetParams failed! " << setparam_ret << std::endl;
  }

  while(std::getline(ftext, text_line))
  {
    if (!text_line.empty() && text_line[0] == '#') {
      continue;
    }
    auto pos = text_line.find(" ");
    std::string wav_fn = text_line.substr(0, pos);
    std::string text = text_line.substr(pos+1);
    milliseconds start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    SingleTest(*se, text, wav_fn, ofs);
    milliseconds end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    std::cout << "Time cost: " << std::to_string(end_time.count() - start_time.count()) << std::endl;
  }

  ofs.close();
  FreeEvalInstance(se);
}

int main(int argc, char* argv[])
{
  /*	
  std::string cfg_fn(argv[1]);
  std::string param_fn(argv[2]);
  std::string text_fn(argv[3]);
  */
  std::string cfg_fn="./ai_model/models/online_v1.9.0.en.native/cfg_en.json";
  std::string param_fn="./ai_model/param.json";
  std::string text_fn="./ai_model/test.json";
  using namespace std::chrono;

  milliseconds s = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  PerformanceTest(cfg_fn, param_fn, text_fn);
  milliseconds e = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

  std::cout << "time cost: " << e.count() - s.count() << std::endl;
}
