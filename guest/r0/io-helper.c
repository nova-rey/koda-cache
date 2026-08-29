/* KODA R0 raw-region helper. It never mounts or parses NTFS. */
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void usage(const char *p) { fprintf(stderr, "usage: %s --device PATH --offset N --length N\n", p); }
static int u64(const char *s, uint64_t *out) {
  char *end = NULL; errno = 0; unsigned long long n = strtoull(s, &end, 10);
  if (errno || end == s || *end) return -1;
  *out = (uint64_t)n;
  return 0;
}
static int read_full(int fd, void *buf, size_t n, off_t off) {
  unsigned char *p = buf; size_t done = 0;
  while (done < n) { ssize_t got = pread(fd, p + done, n - done, off + (off_t)done); if (got <= 0) return -1; done += (size_t)got; }
  return 0;
}
static int write_full(int fd, const void *buf, size_t n, off_t off) {
  const unsigned char *p = buf; size_t done = 0;
  while (done < n) { ssize_t got = pwrite(fd, p + done, n - done, off + (off_t)done); if (got <= 0) return -1; done += (size_t)got; }
  return 0;
}
static uint64_t digest64(const unsigned char *p, size_t n) {
  uint64_t h = UINT64_C(1469598103934665603);
  for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= UINT64_C(1099511628211); }
  return h;
}
int main(int argc, char **argv) {
  const char *device = NULL; uint64_t offset = 0, length = 0;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--device") && i + 1 < argc) device = argv[++i];
    else if (!strcmp(argv[i], "--offset") && i + 1 < argc && u64(argv[++i], &offset) == 0) {}
    else if (!strcmp(argv[i], "--length") && i + 1 < argc && u64(argv[++i], &length) == 0) {}
    else { usage(argv[0]); return 2; }
  }
  if (!device || !length || length > 1024 * 1024 || offset > UINT64_MAX - length || offset % 4096 || length % 4096) {
    fprintf(stderr, "invalid device or bounded 4096-byte-aligned region\n"); return 2;
  }
  size_t half = (size_t)length / 2;
  unsigned char *challenge = calloc(1, half), *response = calloc(1, half), *readback = calloc(1, half);
  if (!challenge || !response || !readback) { fprintf(stderr, "allocation failed\n"); return 2; }
  int fd = open(device, O_RDWR | O_CLOEXEC | O_SYNC);
  if (fd < 0) { fprintf(stderr, "open: %s\n", strerror(errno)); return 3; }
  int rc = read_full(fd, challenge, half, (off_t)offset);
  for (size_t i = 0; rc == 0 && i < half; ++i) response[i] = challenge[i] ^ 0xA5U;
  if (!rc) rc = write_full(fd, response, half, (off_t)(offset + half));
  if (!rc && fsync(fd) != 0) rc = -1;
  if (!rc) rc = read_full(fd, readback, half, (off_t)(offset + half));
  if (!rc && memcmp(response, readback, half) != 0) rc = -1;
  int saved_errno = errno; if (close(fd) != 0) rc = -1;
  if (rc) { fprintf(stderr, "raw I/O failed: %s\n", strerror(saved_errno)); return 4; }
  printf("{\"schema\":\"koda.r0.guest-report.v1\",\"io_complete\":true,\"flushed\":true,\"released\":true,\"offset_bytes\":%" PRIu64 ",\"length_bytes\":%" PRIu64 ",\"challenge_digest64\":\"%016" PRIx64 "\",\"response_digest64\":\"%016" PRIx64 "\"}\n", offset, length, digest64(challenge, half), digest64(response, half));
  free(challenge); free(response); free(readback); return 0;
}
