#ifndef TESTS_LIB_H
#define TESTS_LIB_H

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <syscall.h>

extern const char *test_name;
extern bool quiet;

void msg (const char *, ...) PRINTF_FORMAT (1, 2);
void fail (const char *, ...) PRINTF_FORMAT (1, 2) NO_RETURN;

/* Takes an expression to test for SUCCESS and a message, which
   may include printf-style arguments.  Logs the message, then
   tests the expression.  If it is zero, indicating failure,
   emits the message as a failure.

   Somewhat tricky to use:

     - SUCCESS must not have side effects that affect the
       message, because that will cause the original message and
       the failure message to differ.

     - The message must not have side effects of its own, because
       it will be printed twice on failure, or zero times on
       success if quiet is set. */

/*
SUCCESS를 테스트하는 표현식과 printf 스타일의 인자를 포함할 수 있는 메시지를 가져옵니다.
메시지를 기록한 다음 표현식을 테스트합니다.
이 값이 실패를 나타내는 0이면 메시지를 실패로 출력합니다.

사용하기 다소 까다로운 점:
SUCCESS는 메시지에 영향을 미치는 부작용이 있어서는 안 됩니다.
그렇게 되면 원래 메시지와 실패 메시지가 다르게 나타날 수 있습니다.
메시지 자체가 부작용을 가져서는 안 됩니다.
왜냐하면 실패 시 메시지가 두 번 출력되거나,
quiet가 설정된 경우 성공 시 메시지가 전혀 출력되지 않을 수 있기 때문입니다.
*/
#define CHECK(SUCCESS, ...)                     \
        do                                      \
          {                                     \
            msg (__VA_ARGS__);                  \
            if (!(SUCCESS))                     \
              fail (__VA_ARGS__);               \
          }                                     \
        while (0)

void shuffle (void *, size_t cnt, size_t size);

void exec_children (const char *child_name, pid_t pids[], size_t child_cnt);
void wait_children (pid_t pids[], size_t child_cnt);

void check_file_handle (int fd, const char *file_name,
                        const void *buf_, size_t filesize);
void check_file (const char *file_name, const void *buf, size_t filesize);

void compare_bytes (const void *read_data, const void *expected_data,
                    size_t size, size_t ofs, const char *file_name);

#endif /* test/lib.h */
