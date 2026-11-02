#include "adapters/cli_worker.hpp"

#include "core/brief.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace conductor::adapters {

// shortcut: no timeout/kill and no usage parsing yet; Phase 0 adds both per CLI's JSON output.
core::RunOutcome CliWorker::run(const core::Brief& brief, const std::filesystem::path& sandbox) {
  if (argv_.empty()) throw std::invalid_argument("CliWorker " + id_ + ": empty argv");
  std::filesystem::create_directories(sandbox);
  const auto brief_path = sandbox / "brief.md";
  const auto result_path = sandbox / "result.md";
  {
    std::ofstream out(brief_path);
    out << core::render(brief);
    if (!out) throw std::runtime_error("cannot write " + brief_path.string());
  }

  std::filesystem::permissions(brief_path, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
  const auto stderr_path = sandbox / "stderr.log";

  // Everything the child needs is built before fork.
  std::vector<char*> argv;
  for (auto& a : argv_) argv.push_back(a.data());
  argv.push_back(nullptr);

  // Close-on-exec pipe: stays silent if exec succeeds, carries errno if the child fails before it.
  int err_pipe[2];
  if (pipe(err_pipe) < 0) throw std::system_error(errno, std::generic_category(), "pipe");
  fcntl(err_pipe[0], F_SETFD, FD_CLOEXEC);
  fcntl(err_pipe[1], F_SETFD, FD_CLOEXEC);

  pid_t pid = fork();
  if (pid < 0) {
    int e = errno;
    close(err_pipe[0]);
    close(err_pipe[1]);
    throw std::system_error(e, std::generic_category(), "fork");
  }
  if (pid == 0) {
    close(err_pipe[0]);
    int in = open(brief_path.c_str(), O_RDONLY | O_CLOEXEC);
    int out = open(result_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    int err = open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (in >= 0 && out >= 0 && err >= 0 && dup2(in, STDIN_FILENO) >= 0 && dup2(out, STDOUT_FILENO) >= 0 &&
        dup2(err, STDERR_FILENO) >= 0 && chdir(sandbox.c_str()) == 0) {
      // shortcut: setenv after fork is only safe while Conductor is single-threaded; switch to an envp + execve when the scheduler adds threads.
      for (const auto& [k, v] : env_) setenv(k.c_str(), v.c_str(), 1);
      execvp(argv[0], argv.data());
    }
    int e = errno;
    (void)!write(err_pipe[1], &e, sizeof e);
    _exit(127);
  }

  close(err_pipe[1]);
  int child_errno = 0;
  ssize_t n;
  while ((n = read(err_pipe[0], &child_errno, sizeof child_errno)) < 0 && errno == EINTR) {}
  close(err_pipe[0]);

  int status = 0;
  while (waitpid(pid, &status, 0) < 0)
    if (errno != EINTR) throw std::system_error(errno, std::generic_category(), "waitpid");
  if (n > 0) throw std::system_error(child_errno, std::generic_category(), "CliWorker " + id_ + ": cannot start " + argv_[0]);

  int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
  return {code, result_path, std::nullopt};
}

}  // namespace conductor::adapters
