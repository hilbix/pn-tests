#include "pubnub_sync.h"
#include <stdio.h>
#include <stdarg.h>

#define DP(A...)	debugprintf(__FILE__, __LINE__, __FUNCTION__, A)

void
debugprintf(const char *file, int line, const char *fn, const char *s, ...)
{
  va_list	list;

  fprintf(stderr, "[[%s:%d:%s ", file, line, fn);
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, "]]\n");
}

int main()
{
  enum pubnub_res rslt;
  pubnub_t *pbp = pubnub_alloc();
  const char	*pk, *ps;

  if (NULL == pbp)
    {
      puts("Failed to allocate a Pubnub context");
      return -1;
    }

  pk	= getenv("PUBNUB_PUB_KEY");
  ps	= getenv("PUBNUB_SUB_KEY");
  if (!pk && !ps)
    {
      printf("missing environment PUBNUB_PUB_KEY/PUBNUB_SUB_KEY\n");
      pubnub_free(pbp);
      return -1;
    }

  pubnub_init(pbp, pk, ps);
  DP("init %p", pbp);

  rslt = pubnub_publish(pbp, "pubnub_onboarding_channel", "\"Hello from C sync\"");
  DP("pub %d", rslt);
  if (rslt != PNR_STARTED)
    {
      printf("Unexpected result of publishing: %d\n", rslt);
      pubnub_free(pbp);
      return -1;
    }

  rslt = pubnub_await(pbp);
  DP("await %d", rslt);
  if (PNR_OK == rslt)
    {
      printf("Published! Response from Pubnub: %s\n", pubnub_last_publish_result(pbp));
    }
  else
    {
      printf("Publish failed with code: %d\n", rslt);
    }

  pubnub_free(pbp);
  DP("bye");

  return 0;
}

