#include "test_pbuf.h"

#include "lwip/pbuf.h"
#include "lwip/stats.h"

#if !LWIP_STATS || !MEM_STATS ||!MEMP_STATS
#error "This tests needs MEM- and MEMP-statistics enabled"
#endif
#if !LWIP_TCP || !TCP_QUEUE_OOSEQ || !LWIP_WND_SCALE
#error "This test needs TCP OOSEQ queueing and window scaling enabled"
#endif

/* Setups/teardown functions */

static void
pbuf_setup(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
pbuf_teardown(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}


#define TESTBUFSIZE_1 65535
#define TESTBUFSIZE_2 65530
#define TESTBUFSIZE_3 50050
static u8_t testbuf_1[TESTBUFSIZE_1];
static u8_t testbuf_1a[TESTBUFSIZE_1];
static u8_t testbuf_2[TESTBUFSIZE_2];
static u8_t testbuf_2a[TESTBUFSIZE_2];
static u8_t testbuf_3[TESTBUFSIZE_3];
static u8_t testbuf_3a[TESTBUFSIZE_3];

/* Test functions */
START_TEST(test_pbuf_alloc_zero_pbufs)
{
  struct pbuf *p;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, 0, PBUF_ROM);
  fail_unless(p != NULL);
  if (p != NULL) {
    pbuf_free(p);
  }

  p = pbuf_alloc(PBUF_RAW, 0, PBUF_RAM);
  fail_unless(p != NULL);
  if (p != NULL) {
    pbuf_free(p);
  }

  p = pbuf_alloc(PBUF_RAW, 0, PBUF_REF);
  fail_unless(p != NULL);
  if (p != NULL) {
    pbuf_free(p);
  }

  p = pbuf_alloc(PBUF_RAW, 0, PBUF_POOL);
  fail_unless(p != NULL);
  if (p != NULL) {
    pbuf_free(p);
  }
}
END_TEST

/** Call pbuf_copy on a pbuf with zero length */
START_TEST(test_pbuf_copy_zero_pbuf)
{
  struct pbuf *p1, *p2, *p3;
  err_t err;
  LWIP_UNUSED_ARG(_i);

  p1 = pbuf_alloc(PBUF_RAW, 1024, PBUF_RAM);
  fail_unless(p1 != NULL);
  fail_unless(p1->ref == 1);

  p2 = pbuf_alloc(PBUF_RAW, 2, PBUF_POOL);
  fail_unless(p2 != NULL);
  fail_unless(p2->ref == 1);
  p2->len = p2->tot_len = 0;

  pbuf_cat(p1, p2);
  fail_unless(p1->ref == 1);
  fail_unless(p2->ref == 1);

  p3 = pbuf_alloc(PBUF_RAW, p1->tot_len, PBUF_POOL);
  err = pbuf_copy(p3, p1);
  fail_unless(err == ERR_VAL);

  pbuf_free(p1);
  pbuf_free(p3);
}
END_TEST

/** Call pbuf_copy on pbufs with chains of different sizes */
START_TEST(test_pbuf_copy_unmatched_chains)
{
  uint16_t i, j;
  err_t err;
  struct pbuf *source, *dest, *p;
  LWIP_UNUSED_ARG(_i);

  source = NULL;
  /* Build source pbuf from linked 16 byte parts,
   * with payload bytes containing their offset */
  for (i = 0; i < 8; i++) {
    p = pbuf_alloc(PBUF_RAW, 16, PBUF_RAM);
    fail_unless(p != NULL);
    for (j = 0; j < p->len; j++) {
        ((u8_t*)p->payload)[j] = (u8_t)((i << 4) | j);
    }
    if (source) {
        pbuf_cat(source, p);
    } else {
        source = p;
    }
  }
  for (i = 0; i < source->tot_len; i++) {
    fail_unless(pbuf_get_at(source, i) == i);
  }

  /* Build dest pbuf from other lengths */
  dest = pbuf_alloc(PBUF_RAW, 35, PBUF_RAM);
  fail_unless(dest != NULL);
  p = pbuf_alloc(PBUF_RAW, 81, PBUF_RAM);
  fail_unless(p != NULL);
  pbuf_cat(dest, p);
  p = pbuf_alloc(PBUF_RAW, 27, PBUF_RAM);
  fail_unless(p != NULL);
  pbuf_cat(dest, p);

  /* Copy contents and verify data */
  err = pbuf_copy(dest, source);
  fail_unless(err == ERR_OK);
  for (i = 0; i < source->tot_len; i++) {
    fail_unless(pbuf_get_at(dest, i) == i);
  }

  pbuf_free(source);
  pbuf_free(dest);
}
END_TEST

START_TEST(test_pbuf_copy_partial_pbuf)
{
  struct pbuf *a, *b, *dest;
  char lwip[] = "lwip ";
  char packet[] = "packet";
  err_t err;
  LWIP_UNUSED_ARG(_i);

  a = pbuf_alloc(PBUF_RAW, 5, PBUF_REF);
  fail_unless(a != NULL);
  a->payload = lwip;
  b = pbuf_alloc(PBUF_RAW, 7, PBUF_REF);
  fail_unless(b != NULL);
  b->payload = packet;
  pbuf_cat(a, b);
  dest = pbuf_alloc(PBUF_RAW, 14, PBUF_RAM);
  memset(dest->payload, 0, dest->len);
  fail_unless(dest != NULL);

  /* Don't copy if data will not fit */
  err = pbuf_copy_partial_pbuf(dest, a, a->tot_len, 4);
  fail_unless(err == ERR_ARG);
  /* Don't copy if length is longer than source */
  err = pbuf_copy_partial_pbuf(dest, a, a->tot_len + 1, 0);
  fail_unless(err == ERR_ARG);
  /* Normal copy */
  err = pbuf_copy_partial_pbuf(dest, a, a->tot_len, 0);
  fail_unless(err == ERR_OK);
  fail_unless(strcmp("lwip packet", (char*)dest->payload) == 0);
  /* Copy at offset */
  err = pbuf_copy_partial_pbuf(dest, a, a->tot_len, 1);
  fail_unless(err == ERR_OK);
  fail_unless(strcmp("llwip packet", (char*)dest->payload) == 0);
  /* Copy at offset with shorter length */
  err = pbuf_copy_partial_pbuf(dest, a, 6, 6);
  fail_unless(err == ERR_OK);
  fail_unless(strcmp("llwip lwip p", (char*)dest->payload) == 0);
  /* Copy with shorter length */
  err = pbuf_copy_partial_pbuf(dest, a, 5, 0);
  fail_unless(err == ERR_OK);
  fail_unless(strcmp("lwip  lwip p", (char*)dest->payload) == 0);

  pbuf_free(dest);
  pbuf_free(a);
}
END_TEST

START_TEST(test_pbuf_split_64k_on_small_pbufs)
{
  struct pbuf *p, *rest=NULL;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, 1, PBUF_POOL);
  pbuf_split_64k(p, &rest);
  fail_unless(p->tot_len == 1);
  pbuf_free(p);
}
END_TEST

START_TEST(test_pbuf_queueing_bigger_than_64k)
{
  int i;
  err_t err;
  struct pbuf *p1, *p2, *p3, *rest2=NULL, *rest3=NULL;
  LWIP_UNUSED_ARG(_i);

  for(i = 0; i < TESTBUFSIZE_1; i++) {
    testbuf_1[i] = (u8_t)rand();
  }
  for(i = 0; i < TESTBUFSIZE_2; i++) {
    testbuf_2[i] = (u8_t)rand();
  }
  for(i = 0; i < TESTBUFSIZE_3; i++) {
    testbuf_3[i] = (u8_t)rand();
  }

  p1 = pbuf_alloc(PBUF_RAW, TESTBUFSIZE_1, PBUF_POOL);
  fail_unless(p1 != NULL);
  p2 = pbuf_alloc(PBUF_RAW, TESTBUFSIZE_2, PBUF_POOL);
  fail_unless(p2 != NULL);
  p3 = pbuf_alloc(PBUF_RAW, TESTBUFSIZE_3, PBUF_POOL);
  fail_unless(p3 != NULL);
  err = pbuf_take(p1, testbuf_1, TESTBUFSIZE_1);
  fail_unless(err == ERR_OK);
  err = pbuf_take(p2, testbuf_2, TESTBUFSIZE_2);
  fail_unless(err == ERR_OK);
  err = pbuf_take(p3, testbuf_3, TESTBUFSIZE_3);
  fail_unless(err == ERR_OK);

  pbuf_cat(p1, p2);
  pbuf_cat(p1, p3);

  pbuf_split_64k(p1, &rest2);
  fail_unless(p1->tot_len == TESTBUFSIZE_1);
  fail_unless(rest2->tot_len == (u16_t)((TESTBUFSIZE_2+TESTBUFSIZE_3) & 0xFFFF));
  pbuf_split_64k(rest2, &rest3);
  fail_unless(rest2->tot_len == TESTBUFSIZE_2);
  fail_unless(rest3->tot_len == TESTBUFSIZE_3);

  pbuf_copy_partial(p1, testbuf_1a, TESTBUFSIZE_1, 0);
  pbuf_copy_partial(rest2, testbuf_2a, TESTBUFSIZE_2, 0);
  pbuf_copy_partial(rest3, testbuf_3a, TESTBUFSIZE_3, 0);
  fail_if(memcmp(testbuf_1, testbuf_1a, TESTBUFSIZE_1));
  fail_if(memcmp(testbuf_2, testbuf_2a, TESTBUFSIZE_2));
  fail_if(memcmp(testbuf_3, testbuf_3a, TESTBUFSIZE_3));

  pbuf_free(p1);
  pbuf_free(rest2);
  pbuf_free(rest3);
}
END_TEST

/* Test for bug that writing with pbuf_take_at() did nothing
 * and returned ERR_OK when writing at beginning of a pbuf
 * in the chain.
 */
START_TEST(test_pbuf_take_at_edge)
{
  err_t res;
  u8_t *out;
  int i;
  u8_t testdata[] = { 0x01, 0x08, 0x82, 0x02 };
  struct pbuf *p = pbuf_alloc(PBUF_RAW, 1024, PBUF_POOL);
  struct pbuf *q = p->next;
  LWIP_UNUSED_ARG(_i);
  /* alloc big enough to get a chain of pbufs */
  fail_if(p->tot_len == p->len);
  memset(p->payload, 0, p->len);
  memset(q->payload, 0, q->len);

  /* copy data to the beginning of first pbuf */
  res = pbuf_take_at(p, &testdata, sizeof(testdata), 0);
  fail_unless(res == ERR_OK);

  out = (u8_t*)p->payload;
  for (i = 0; i < (int)sizeof(testdata); i++) {
    fail_unless(out[i] == testdata[i],
      "Bad data at pos %d, was %02X, expected %02X", i, out[i], testdata[i]);
  }

  /* copy data to the just before end of first pbuf */
  res = pbuf_take_at(p, &testdata, sizeof(testdata), p->len - 1);
  fail_unless(res == ERR_OK);

  out = (u8_t*)p->payload;
  fail_unless(out[p->len - 1] == testdata[0],
    "Bad data at pos %d, was %02X, expected %02X", p->len - 1, out[p->len - 1], testdata[0]);
  out = (u8_t*)q->payload;
  for (i = 1; i < (int)sizeof(testdata); i++) {
    fail_unless(out[i-1] == testdata[i],
      "Bad data at pos %d, was %02X, expected %02X", p->len - 1 + i, out[i-1], testdata[i]);
  }

  /* copy data to the beginning of second pbuf */
  res = pbuf_take_at(p, &testdata, sizeof(testdata), p->len);
  fail_unless(res == ERR_OK);

  out = (u8_t*)p->payload;
  for (i = 0; i < (int)sizeof(testdata); i++) {
    fail_unless(out[i] == testdata[i],
      "Bad data at pos %d, was %02X, expected %02X", p->len+i, out[i], testdata[i]);
  }
  pbuf_free(p);
}
END_TEST

/* Verify pbuf_put_at()/pbuf_get_at() when using
 * offsets equal to beginning of new pbuf in chain
 */
START_TEST(test_pbuf_get_put_at_edge)
{
  u8_t *out;
  u8_t testdata = 0x01;
  u8_t getdata;
  struct pbuf *p = pbuf_alloc(PBUF_RAW, 1024, PBUF_POOL);
  struct pbuf *q = p->next;
  LWIP_UNUSED_ARG(_i);
  /* alloc big enough to get a chain of pbufs */
  fail_if(p->tot_len == p->len);
  memset(p->payload, 0, p->len);
  memset(q->payload, 0, q->len);

  /* put byte at the beginning of second pbuf */
  pbuf_put_at(p, p->len, testdata);

  out = (u8_t*)q->payload;
  fail_unless(*out == testdata,
    "Bad data at pos %d, was %02X, expected %02X", p->len, *out, testdata);

  getdata = pbuf_get_at(p, p->len);
  fail_unless(*out == getdata,
    "pbuf_get_at() returned bad data at pos %d, was %02X, expected %02X", p->len, getdata, *out);
  pbuf_free(p);
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
pbuf_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_pbuf_alloc_zero_pbufs),
    TESTFUNC(test_pbuf_copy_zero_pbuf),
    TESTFUNC(test_pbuf_copy_unmatched_chains),
    TESTFUNC(test_pbuf_copy_partial_pbuf),
    TESTFUNC(test_pbuf_split_64k_on_small_pbufs),
    TESTFUNC(test_pbuf_queueing_bigger_than_64k),
    TESTFUNC(test_pbuf_take_at_edge),
    TESTFUNC(test_pbuf_get_put_at_edge)
  };
  return create_suite("PBUF", tests, sizeof(tests)/sizeof(testfunc), pbuf_setup, pbuf_teardown);
}
