#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "adapters/cli_worker.hpp"
#include "store/store.hpp"

#ifdef CONDUCTOR_WITH_ML
#include "ml/onnx_lane_classifier.hpp"
#endif

namespace {

int usage() {
  std::cerr << "usage:\n"
               "  conductor init <db>                                 create/migrate the database\n"
               "  conductor spike <brief.md> <sandbox-dir> -- <cmd...> run one CLI worker headless\n"
               "  conductor assign <task> [worker]                    (not implemented yet)\n"
               "  conductor manual <task>                             (not implemented yet)\n"
               "  conductor submit <task> <result-file>               (not implemented yet)\n"
               "  conductor review <task> accept|reject [notes]       (not implemented yet)\n"
#ifdef CONDUCTOR_WITH_ML
               "  conductor classify <model.onnx> <text>              suggest a lane\n"
#endif
      ;
  return 2;
}

// Phase 0: spawn one CLI worker, feed it a brief, write its result file.
int spike(const std::vector<std::string>& args) {
  if (args.size() < 5 || args[3] != "--") return usage();
  std::ifstream in(args[1]);
  if (!in) {
    std::cerr << "cannot read " << args[1] << "\n";
    return 1;
  }
  std::stringstream text;
  text << in.rdbuf();

  conductor::adapters::CliWorker worker("spike", {conductor::core::Lane::General},
                                        {args.begin() + 4, args.end()});
  conductor::core::Brief brief;
  brief.goal = text.str();
  auto outcome = worker.run(brief, args[2]);
  std::cout << "exit=" << outcome.exit_code << " result=" << outcome.result_file.string() << "\n";
  return outcome.exit_code;
}

}  // namespace

int main(int argc, char** argv) try {
  std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty()) return usage();

  if (args[0] == "init" && args.size() == 2) {
    conductor::store::Store(args[1]).migrate();
    std::cout << "database ready: " << args[1] << "\n";
    return 0;
  }
  if (args[0] == "spike") return spike(args);
  for (const char* planned : {"assign", "manual", "submit", "review"})
    if (args[0] == planned) {
      std::cerr << "conductor " << planned << ": not implemented yet (see docs/setup-plan.md)\n";
      return 3;
    }
#ifdef CONDUCTOR_WITH_ML
  if (args[0] == "classify" && args.size() == 3) {
    auto p = conductor::ml::OnnxLaneClassifier(args[1]).classify(args[2]);
    std::cout << conductor::core::to_string(p.lane) << " " << p.confidence << "\n";
    return 0;
  }
#endif
  return usage();
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
