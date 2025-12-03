#include "Evaluation.h"
#include "Shell.h"
#include "InternalCommands.h" // Cần thiết để kiểm tra lệnh nội bộ

#include <fcntl.h>    // For open flags (O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, O_APPEND)
#include <signal.h>   // For signal handling (future use, but good to have)
#include <stdio.h>    // For perror, fprintf
#include <stdlib.h>   // For exit, atoi
#include <string.h>   // For strcmp (used in internal commands check)
#include <sys/wait.h> // For waitpid, WIFEXITED, WEXITSTATUS
#include <unistd.h>   // For fork, execvp, dup2, close, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO
#include <sys/stat.h> // For mode_t (S_IRUSR, S_IWUSR, etc.)

#include <errno.h>  // Cần cho biến errno và ECHILD
#include <signal.h> // Cần cho sigaction, SIGCHLD

void handler_sigchld(int sig)
{
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
  errno = saved_errno;
}

int evaluateExpr(Expression *expr)
{
  static int init_signal = 0;
  if (!init_signal)
  {
    struct sigaction sa;
    sa.sa_handler = &handler_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    init_signal = 1;
  }
  // Nếu biểu thức rỗng, không làm gì cả
  if (expr == NULL || expr->type == ET_EMPTY)
  {
    shellStatus = 0;
    return shellStatus;
  }

  // Đặt trạng thái shell về 0 trước khi thực thi lệnh thành công
  shellStatus = 0;

  switch (expr->type)
  {

  case ET_SIMPLE:
  {
    cmd_func_t internal_cmd_fct = findCommandFct(expr->argv[0]);
    if (internal_cmd_fct != NULL)
    {
      shellStatus = internal_cmd_fct(expr->argc, expr->argv);
    }
    else
    {
      pid_t pid = fork();
      if (pid == -1)
      {
        perror("fork");
        shellStatus = 1;
      }
      else if (pid == 0)
      { // Process con
        // Cần khôi phục xử lý tín hiệu mặc định cho con để nó hoạt động đúng
        // (Optional nhưng tốt: signal(SIGCHLD, SIG_DFL); )
        execvp(expr->argv[0], expr->argv);
        perror(expr->argv[0]);
        exit(127);
      }
      else
      { // Process cha
        int status;
        // SỬA Ở ĐÂY:
        // Chờ process con cụ thể (pid).
        pid_t w = waitpid(pid, &status, 0);

        if (w == -1)
        {
          // Nếu lỗi là ECHILD, nghĩa là Handler đã thu dọn process này rồi.
          // Ta coi như thành công (hoặc giữ status cũ).
          if (errno == ECHILD)
          {
            // Process đã bị "reaper" thu dọn, ta không biết exit code chính xác
            // nhưng process đã kết thúc.
            // Thường gán shellStatus = 0 hoặc giữ nguyên.
            shellStatus = 0;
          }
          else
          {
            perror("waitpid");
            shellStatus = 1;
          }
        }
        else
        {
          if (WIFEXITED(status))
          {
            shellStatus = WEXITSTATUS(status);
          }
          else
          {
            shellStatus = 1;
          }
        }
      }
    }
    break;
  }

  case ET_REDIRECT:
  {
    // Để thực hiện chuyển hướng, chúng ta cần tạo một process con
    // để các thay đổi về file descriptor không ảnh hưởng đến shell cha.
    pid_t pid = fork();
    if (pid == -1)
    {
      perror("fork");
      shellStatus = 1;
    }
    else if (pid == 0)
    {                          // Process con
      int fd_to_redirect = -1; // File descriptor đích (từ file hoặc FD khác)
      int old_fd;              // File descriptor gốc sẽ bị thay thế (0, 1, hoặc 2)

      // Xác định file descriptor gốc (0, 1, 2) sẽ bị chuyển hướng.
      // Nếu expr->redirect.fd là -1 (cho &> hoặc &>>), mặc định xử lý STDOUT_FILENO (1) trước.
      if (expr->redirect.fd == -1)
      { // Ví dụ: `cmd &> file`
        old_fd = STDOUT_FILENO;
      }
      else
      { // Ví dụ: `cmd > file` (old_fd=1), `cmd < file` (old_fd=0), `cmd 2> file` (old_fd=2)
        old_fd = expr->redirect.fd;
      }

      // 1. Xác định đích của chuyển hướng: đến một file hay đến một FD khác
      if (expr->redirect.toOtherFd)
      { // Ví dụ: `2>&1`
        // Chuyển tên file (thực ra là số FD dưới dạng chuỗi) thành số nguyên
        fd_to_redirect = atoi(expr->redirect.fileName);
      }
      else
      { // Chuyển hướng đến một file thực tế (ví dụ: `> output.txt`, `< input.txt`)
        int open_flags = 0;
        // Quyền cho file mới tạo: rw-r--r--
        mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

        if (expr->redirect.type == REDIR_IN)
        { // < (nhập)
          open_flags = O_RDONLY;
        }
        else if (expr->redirect.type == REDIR_OUT)
        {                                            // > (ghi đè)
          open_flags = O_WRONLY | O_CREAT | O_TRUNC; // Ghi đè, tạo nếu không tồn tại
        }
        else if (expr->redirect.type == REDIR_APP)
        {                                             // >> (ghi thêm)
          open_flags = O_WRONLY | O_CREAT | O_APPEND; // Ghi thêm, tạo nếu không tồn tại
        }

        // Mở file và lấy file descriptor
        fd_to_redirect = open(expr->redirect.fileName, open_flags, file_perms);
        if (fd_to_redirect == -1)
        {
          perror(expr->redirect.fileName); // In lỗi nếu không mở được file
          exit(1);                         // Thoát process con với lỗi
        }
      }

      // 2. Thực hiện chuyển hướng chính bằng dup2
      // old_fd (0, 1, hoặc 2) sẽ được đóng và trỏ đến cùng một file/FD mà fd_to_redirect đang trỏ đến.
      if (dup2(fd_to_redirect, old_fd) == -1)
      {
        perror("dup2");
        // Nếu có lỗi dup2, đóng fd_to_redirect nếu nó là một file được mở
        if (!expr->redirect.toOtherFd && fd_to_redirect != -1)
        {
          close(fd_to_redirect);
        }
        exit(1);
      }

      // 3. Xử lý trường hợp đặc biệt cho &> (chuyển hướng STDOUT và STDERR cùng một lúc)
      if (expr->redirect.fd == -1)
      {
        // Nếu old_fd là STDOUT_FILENO, thì STDERR_FILENO cũng cần được chuyển hướng
        if (dup2(fd_to_redirect, STDERR_FILENO) == -1)
        {
          perror("dup2 for stderr");
          // Nếu có lỗi dup2, đóng fd_to_redirect nếu nó là một file được mở
          if (!expr->redirect.toOtherFd && fd_to_redirect != -1)
          {
            close(fd_to_redirect);
          }
          exit(1);
        }
      }

      // 4. Đóng file descriptor gốc (fd_to_redirect) nếu nó là một file mới được mở
      // và không phải là một trong các FD chuẩn (0, 1, 2) mà bây giờ đã bị ghi đè.
      // Điều này là quan trọng để tránh rò rỉ file descriptor.
      // Cách đơn giản nhất là chỉ đóng nếu nó được mở bởi `open()` và không phải là 0, 1, 2
      // sau khi nó đã được `dup2` thành công.
      if (!expr->redirect.toOtherFd && fd_to_redirect >= 3)
      {
        close(fd_to_redirect);
      }

      // 5. Sau khi chuyển hướng được thiết lập, thực thi lệnh con (expr->left)
      evaluateExpr(expr->left); // Gọi đệ quy để xử lý lệnh thực tế
      exit(shellStatus);        // Process con thoát với trạng thái của lệnh đã thực thi
    }
    else
    { // Process cha
      int status;
      // Chờ process con chuyển hướng kết thúc
      waitpid(pid, &status, 0);
      if (WIFEXITED(status))
      {
        shellStatus = WEXITSTATUS(status);
      }
      else
      {
        shellStatus = 1;
      }
    }
    break;
  }

  case ET_SEQUENCE:
  {                            // Lệnh 1 ; Lệnh 2
    evaluateExpr(expr->left);  // Thực thi lệnh trái
    evaluateExpr(expr->right); // Sau đó thực thi lệnh phải
    break;
  }

  case ET_SEQUENCE_AND:
  {                           // Lệnh 1 && Lệnh 2
    evaluateExpr(expr->left); // Thực thi lệnh trái
    if (shellStatus == 0)
    {                            // Nếu lệnh trái thành công (exit status 0)
      evaluateExpr(expr->right); // Thì thực thi lệnh phải
    }
    break;
  }

  case ET_SEQUENCE_OR:
  {                           // Lệnh 1 || Lệnh 2
    evaluateExpr(expr->left); // Thực thi lệnh trái
    if (shellStatus != 0)
    {                            // Nếu lệnh trái thất bại (exit status khác 0)
      evaluateExpr(expr->right); // Thì thực thi lệnh phải
    }
    break;
  }

  case ET_PIPE:
  {                // Lệnh 1 | Lệnh 2
    int pipefd[2]; // pipefd[0] là đầu đọc, pipefd[1] là đầu ghi
    if (pipe(pipefd) == -1)
    {
      perror("pipe");
      shellStatus = 1;
      return shellStatus;
    }

    pid_t pid_left = fork();
    if (pid_left == -1)
    {
      perror("fork left command for pipe");
      shellStatus = 1;
      close(pipefd[0]);
      close(pipefd[1]);
      return shellStatus;
    }
    else if (pid_left == 0)
    {                   // Process con bên trái (Lệnh 1)
      close(pipefd[0]); // Đóng đầu đọc của pipe (không cần thiết cho lệnh này)
      // Chuyển hướng STDOUT của lệnh trái sang đầu ghi của pipe
      if (dup2(pipefd[1], STDOUT_FILENO) == -1)
      {
        perror("dup2 stdout for pipe left");
        exit(1);
      }
      close(pipefd[1]); // Sau khi dup2, có thể đóng đầu ghi gốc

      evaluateExpr(expr->left); // Thực thi lệnh trái
      exit(shellStatus);        // Thoát với trạng thái của lệnh trái
    }

    pid_t pid_right = fork();
    if (pid_right == -1)
    {
      perror("fork right command for pipe");
      shellStatus = 1;
      close(pipefd[0]);
      close(pipefd[1]);
      // Cần chờ lệnh trái nếu nó vẫn đang chạy
      int status_left;
      waitpid(pid_left, &status_left, 0);
      return shellStatus;
    }
    else if (pid_right == 0)
    {                   // Process con bên phải (Lệnh 2)
      close(pipefd[1]); // Đóng đầu ghi của pipe (không cần thiết cho lệnh này)
      // Chuyển hướng STDIN của lệnh phải sang đầu đọc của pipe
      if (dup2(pipefd[0], STDIN_FILENO) == -1)
      {
        perror("dup2 stdin for pipe right");
        exit(1);
      }
      close(pipefd[0]); // Sau khi dup2, có thể đóng đầu đọc gốc

      evaluateExpr(expr->right); // Thực thi lệnh phải
      exit(shellStatus);         // Thoát với trạng thái của lệnh phải
    }

    // Process cha
    close(pipefd[0]); // Đóng cả hai đầu của pipe trong process cha
    close(pipefd[1]); // vì process cha không sử dụng pipe trực tiếp

    int status_left, status_right;
    waitpid(pid_left, &status_left, 0);   // Chờ lệnh trái
    waitpid(pid_right, &status_right, 0); // Chờ lệnh phải

    // Trạng thái của pipe thường là trạng thái của lệnh cuối cùng trong pipe
    if (WIFEXITED(status_right))
    {
      shellStatus = WEXITSTATUS(status_right);
    }
    else
    {
      shellStatus = 1;
    }
    break;
  }

  case ET_BG:
  {
    pid_t pid = fork();
    if (pid == -1)
    {
      perror("fork for background command");
      shellStatus = 1;
    }
    else if (pid == 0)
    {
      // Process con
      evaluateExpr(expr->left);
      exit(shellStatus);
    }
    else
    {
      // Process cha
      // Không wait => Đúng.
      // In thông báo background (tùy chọn)
      // printf("[%d]\n", pid);
      shellStatus = 0;
    }
    break;
  }

  // ET_EMPTY đã được xử lý ở đầu hàm
  default:
    fprintf(stderr, "Error: Expression type not yet implemented: %d\n", expr->type);
    shellStatus = 1;
    break;
  }

  return shellStatus;
}
