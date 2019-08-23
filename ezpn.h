#include "pubnub_sync.h"

#include "core/pubnub_sync_subscribe_loop.h"
#include "core/pubnub_helper.h"

#include "core/../posix/monotonic_clock_get_time.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/***********************************************************************
 * ERROR
 **********************************************************************/

typedef enum pubnub_res Zs;
typedef enum pubnub_cancel_res Zc;
typedef struct ezpn *Z;

#define	ZTMP_MAX	10

typedef struct Zt
  {
    struct Zt	*next;
    char	*buf;
    size_t	len, fill;
    int		use;
    Z		z;
  } *Zt;

struct ezpn
  {
    pubnub_t	*z;
    struct pubnub_subloop_descriptor	*sub, _sub;

    const char	*msg;
    Zs		err;
    const char	*err_s;

    const char	*pk, *sk;

    int		code;
    int		argc;
    char	**argv;

    const char	*name;
    const char	*chan;

    int64_t	t_start;
    int64_t	t_end;

    struct Zt	tmps[ZTMP_MAX], *tmp;
  };


/***********************************************************************
 * DEBUG
 **********************************************************************/

#define	DP(...)	debugprintf(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

static void
debugprintf(const char *file, int line, const char *fn, const char *s, ...)
{
  va_list       list;

  fprintf(stderr, "[[%s:%d:%s ", file, line, fn);
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, "]]\n");
  fflush(stderr);
}

#define	OOPZ(S, ...)	do { oopz(z, __FILE__, __LINE__, __FUNCTION__, S, ##__VA_ARGS__); } while (0)
#define	FATALZ(X)	do { if (X) OOPZ("%s", #X); } while (0)

static void
oopz(Z z, const char *file, int line, const char *fn, const char *s, ...)
{
  va_list	list;

  fprintf(stderr, "OOPS:%s:%d:%s: ", file, line, fn);
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, "\n");
  fflush(stderr);
  exit(23);
}


/***********************************************************************
 * State
 **********************************************************************/

static Z
z_args(Z z, int argc, char **argv)
{
  z->argc	= argc;
  z->argv	= argv;
  if (!z->name || !*z->name)
    z->name	= argv[0];
  return z;
}

static const char *
zarg(Z z, int n, const char *def)
{
  return n<0 || n>=z->argc ? def : z->argv[n];
}

static int64_t
zclock(Z z)
{
  struct timespec ts;

  if (monotonic_clock_get_time(&ts))
    OOPZ("cannot get time");
  return (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
}

static Z
z_err(Z z, Zs st)
{
  z->err	= st;
  z->err_s	= pubnub_res_2_string(st);
  return z;
}


static Z
z_st(Z z, Zs st, const char *what)
{
  if (st != PNR_OK)
    OOPZ("%s failed: %d('%s')", what, st, pubnub_res_2_string(st));
  return z;
}

static Z
z_started(Z z, Zs st, const char *what)
{
  if (st != PNR_STARTED)
    OOPZ("%s wrong state: %d('%s')", what, st, pubnub_res_2_string(st));
  return z;
}

static Z
z_is0(Z z, int st, const char *what)
{
  if (st)
    OOPZ("%s failed: %d\n", what, st);
  return z;
}

static Z
z_await(Z z, const char *what)
{
  return z_st(z, pubnub_await(z->z), what);
}

static Z
z_log(Z z, const char *what, const char *s, ...)
{
  va_list	list;

  fprintf(stderr, "%s: %s: ", z->name, what);
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, "\n");

  return z;
}

static int
zerr(Z z)
{
  return z->err;
}


/***********************************************************************
 * Setup
 **********************************************************************/

static void *
zalloc(Z z, void *ptr, size_t len)
{
  void *tmp;

  tmp	= realloc(ptr, len);
  if (!tmp) OOPZ("OOM");
  if (!ptr)
    memset(tmp, 0, len);
  return tmp;
}

static Z
z_init(Z z)
{
  Zt	t;

  if (!z)
    z	= zalloc(z, NULL, sizeof *z);

  z->pk	= getenv("PUBNUB_PUB_KEY");
  z->sk	= getenv("PUBNUB_SUB_KEY");

  z->tmp	= 0;
  for (t=z->tmps; t < &z->tmps[sizeof z->tmps/sizeof *z->tmps]; t++)
    {
      t->next	= z->tmp;
      t->z	= z;
      z->tmp	= t;
    }
  return z;
}

static Z
z_open(Z z)
{
  if (!z->pk && !z->sk)
    OOPZ("missing pub/sub key");
  if (z->z)
    OOPZ("already open");

  z->z	= pubnub_alloc();
  if (!z->z)
    OOPZ("alloc failed");

#if 0
  z->name	= name ? name : "";
#endif
  z->t_start	= zclock(z);
  pubnub_init(z->z, z->pk, z->sk);
  pubnub_set_blocking_io(z->z);

  return z;
}

static Z
z_free(Z z)
{
  Zc	c;

  c	= pubnub_cancel(z->z);
  if (c == PN_CANCEL_STARTED)
    z_await(z, "cancel");
  else
    z_st(z, c, "cancel");

  z_is0(z, pubnub_free(z->z), "free");
  z->z	= 0;
  return z;
}

static int
zret(Z z)
{
  return z->code;
}

static int
zok(Z z)
{
  return !z->code && !z->err;
}


/***********************************************************************
 * TMP
 **********************************************************************/

static Zt
zt_new(Z z)
{
  Zt	t;

  if (!z->tmp)
    OOPZ("out of temporaries");
  t		= z->tmp;
  z->tmp	= t->next;
  t->next	= 0;
  t->fill	= 0;
  return t;
}

static Zt
zt_add_c(Zt t, char c)
{
  if (!t->buf || t->fill>=t->len)
    t->buf	= zalloc(t->z, t->buf, t->len+=BUFSIZ);
  t->buf[t->fill++]	= c;
  return t;
}

static Zt
zt_add_c_json(Zt t, char c)
{
  switch (c)
    {
    case '"':
    case '\\':
      zt_add_c(t, '\\');
      break;
    }
  return zt_add_c(t, c);
}

static Zt
zjson_s(Z z, const char *str)
{
  Zt	t;

  t	= zt_new(z);
  zt_add_c(t, '"');
  while (*str)
    zt_add_c_json(t, *str++);
  zt_add_c(t, '"');
  zt_add_c(t, 0);
  return t;
}

static Z
zt_free(Zt t)
{
  Z	z;

  z	= t->z;
  FATALZ(!z || t->next);
  t->z		= 0;
  t->next	= z->tmp;
  z->tmp	= t;
  return z;
}


/***********************************************************************
 * Subloop
 **********************************************************************/

static const char *
zchan(Z z, const char *chan)
{
  if (chan)
    z->chan	= chan;
  FATALZ(!z->chan);
  return z->chan;
}

static Z
z_sub_open(Z z, const char *chan)
{

  z->_sub	= pubnub_subloop_define(z->z, zchan(z, chan));
  z->sub	= &z->_sub;

  z->chan	= z->sub->channel;
  FATALZ(!z->chan);

  return z_log(z, "entering channel", "%s", z->chan);
}

static Z
z_sub_msg(Z z)
{
  z->msg	= 0;
  return z_err(z, pubnub_subloop_fetch(z->sub, &z->msg));
}


/***********************************************************************
 * Pub
 **********************************************************************/

static Z
z_pub_str(Z z, const char *chan, const char *msg)
{
  Zt	tmp;

  tmp	= zjson_s(z, msg);
  z_started(z, pubnub_publish(z->z, zchan(z, chan), tmp->buf), "publish");
  zt_free(tmp);
  return z_log(z, "publishing", "%s: [%s]", chan, tmp->buf);
}


static Z
z_pub_await(Z z)
{
  z_err(z, pubnub_await(z->z));
  if (!zerr(z))
    z->msg	= pubnub_last_publish_result(z->z);
  return z;
}

