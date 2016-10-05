#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <thread>
#include <algorithm>
#include <sys/wait.h>
#include <iostream>
#include <list>
#include <string>

static int waitForKey(const char *message);
static void test_streamsCanBeOpened (const char *inStream, const char *outStream, bool trace);
static void test_streamsCanBeReOpened (const char *inStream, const char *outStream);
static void test_writeAscendingPattern(const char *outStream);
static void test_writeAscendingPatternInChunks(const char *outStream);
static void test_writeToOutDoesNotGenerateStuffAtIn (const char *inStream, const char *outStream);
static void test_smallWriteDoesNotBlock (const char *outStream);
static void test_hugeWriteDoesNotFail (const char *outStream);
static void test_hugeWriteDoesBlock (const char *outStream);
static void test_loopBack (const char *inStream, const char *outStream);
static void test_firstByteAfterReconnectIsFirstByteOfMessage (const char *inStream, const char *outStream);
static void test_writeNLMessageInteractive(const char *outStream);

int main (int numArgs, char *argv[])
{
  if (numArgs < 3)
  {
    printf ("\nusage: lpc-bb-test /dev/name-of-lpc-input-stream /dev/name-of-lpc-output-stream\n");
    return EXIT_FAILURE;
  }

  const char *inStream = argv[1];
  const char *outStream = argv[2];

  test_writeNLMessageInteractive(outStream);

  // test_streamsCanBeOpened (inStream, outStream, true);
  // test_streamsCanBeReOpened (inStream, outStream);
  // test_writeAscendingPattern(outStream);
  test_writeAscendingPatternInChunks(outStream);
  test_writeToOutDoesNotGenerateStuffAtIn (inStream, outStream);
  test_smallWriteDoesNotBlock (outStream);
  test_hugeWriteDoesNotFail (outStream);
  test_hugeWriteDoesBlock (outStream);
  test_smallWriteDoesNotBlock (outStream);
  test_hugeWriteDoesNotFail (outStream);
  test_hugeWriteDoesBlock (outStream);
  test_loopBack (inStream, outStream);
  test_firstByteAfterReconnectIsFirstByteOfMessage (inStream, outStream);

  return EXIT_SUCCESS;
}

static int waitForKey(const char *message)
{
  printf ("\n--> %s", message);
  std::string in;
  std::getline (std::cin, in);
  return std::stoi(in);
}

static char *createAscendingPattern(size_t len)
{
  char *pattern = (char*) malloc (len);

  for (size_t i = 0; i < len; i++)
  {
    pattern[i] = (char) i;
  }

  return pattern;
}

static void test_writeNLMessageInteractive(const char *outStream)
{
  struct Message
  {
    int16_t type;
    int16_t length;
    int32_t paramID;
    int32_t paramValue;
  };

  assert(sizeof (Message) == 12);


  int out = open (outStream, O_APPEND | O_NONBLOCK | O_SYNC | O_WRONLY);

  std::list<Message> messages;

  for (int i = 0; i < 400; i++)
  {
    Message msg;
    msg.type = 0x0400;
    msg.length = 2;
    msg.paramID = i;
    msg.paramValue = i << 2;
    messages.push_back (msg);

    if(messages.size() == 50)
    {
      for (Message &m : messages)
      {
        AGAIN:
        ssize_t result = write (out, &m, 12);

        if(result == -1)
        {
          if(errno == EAGAIN)
          {
            printf ("EAGAIN!\n");
            sleep(1);
            goto AGAIN;
          }
        }
      }

      fsync(out);
      messages.clear();
    }
  }



#if 0
    msg.paramID = waitForKey("Parameter ID (-1 = send, -2 = quit):");


    if(msg.paramID == -1)
    {
      for(Message &m : messages)
      {
        fwrite(&m, 12, 1, out);
      }

      fflush(out);
      messages.clear();
      printf("wrote and flushed!\n");
      continue;
    }
    else if(msg.paramID == -2)
    {
      break;
    }
    else
    {
      msg.paramValue = waitForKey("Parameter value:");
      messages.push_back (msg);
      printf("pushed into queue!\n");
    }
#endif

  close (out);
}

static void test_streamsCanBeOpened (const char *inStream, const char *outStream, bool trace)
{
  if(trace)
    waitForKey (__PRETTY_FUNCTION__);

  FILE *in = fopen (inStream, "r");
  assert (in != NULL);

  FILE *out = fopen (outStream, "a");
  assert (out != NULL);

  fclose (in);
  fclose (out);

  if(trace)
    printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_streamsCanBeReOpened (const char *inStream, const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);

  for (int i = 0; i < 100; i++)
    test_streamsCanBeOpened (inStream, outStream, false);

  printf ("\n<--%s", __PRETTY_FUNCTION__);
}

static void test_writeAscendingPattern(const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);

  FILE *out = fopen (outStream, "a");
  size_t len = 20;
  char *bytes = createAscendingPattern (len);
  
  fwrite (bytes, len, 1, out);

  free(bytes);
  fclose(out);

  printf ("\n<--%s", __PRETTY_FUNCTION__);
}

static void test_writeAscendingPatternInChunks(const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);
  
  FILE *out = fopen (outStream, "a");
  size_t numChunks = 10;
  size_t chunkLen = 20;
  size_t len = chunkLen * numChunks;
  char *bytes = createAscendingPattern (len);
  
  for(size_t i = 0; i < numChunks; i++)
  {
    fwrite (bytes + i * chunkLen, chunkLen, 1, out);
    fflush(out);
    sleep(1);
  }
  
  free(bytes);
  fclose(out);

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void readTillEOF (FILE* in)
{
  waitForKey (__PRETTY_FUNCTION__);

  char buffer[1024];

  size_t numBytesRead = 0;

  do
  {
    numBytesRead = fread (buffer, sizeof(buffer), 1, in);
  }
  while (numBytesRead > 0);

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_writeToOutDoesNotGenerateStuffAtIn (const char *inStream, const char *outStream)
{
  printf ("\n--> %s", __PRETTY_FUNCTION__);

  FILE *in = fopen (inStream, "r");
  FILE *out = fopen (outStream, "a");

  readTillEOF (in);

  char foo[1024];
  fwrite (foo, sizeof(foo), 1, out);

  assert (fread (foo, sizeof(foo), 1, in) == 0);

  fclose (in);
  fclose (out);

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_smallWriteDoesNotBlock (const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);

  int out = open (outStream, O_APPEND | O_NONBLOCK | O_SYNC | O_WRONLY);

  ssize_t sizes[] =
  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

  for (ssize_t size : sizes)
  {
    char foo[size];

    ssize_t ret = write (out, foo, sizeof(foo));

    if (ret != size)
      printf ("Write failed with error %d (%s)\n", errno, strerror (errno));

    assert (ret == size);
  }

  close (out);

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_hugeWriteDoesNotFail (const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);

  FILE *out = fopen (outStream, "a");

  ssize_t oneMB = 1024 * 1024;
  char *oneMBOfFoo = createAscendingPattern (oneMB);

  ssize_t ret = fwrite (oneMBOfFoo, 1, oneMB, out);
  assert (ret == oneMB);

  free (oneMBOfFoo);

  fclose (out);

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_hugeWriteDoesBlock (const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);

  int out = open (outStream, O_APPEND | O_NONBLOCK | O_SYNC | O_WRONLY);

  ssize_t oneMB = 1024 * 1024;
  char *oneMBOfFoo = createAscendingPattern (oneMB);

  ssize_t max = 1024 * oneMB;
  ssize_t ret = 0;
  ssize_t sum = 0;

  do
  {
    ret = write (out, oneMBOfFoo, oneMB);

    if (ret == -1)
    {
      assert (errno == EAGAIN || errno == EWOULDBLOCK);
      break;
    }
    else
    {
      assert (ret == oneMB);
      sum += ret;
    }
  }
  while (ret == oneMB && sum < max);

  if (sum >= max)
    printf ("test_hugeWriteDoesBlock failed: write() did never block\n");

  free (oneMBOfFoo);
  close (out);

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_loopBack (const char *inStream, const char *outStream, const char *pattern, ssize_t len,
                           ssize_t writeChunksize, ssize_t readChunksize)
{
  waitForKey (__PRETTY_FUNCTION__);

  std::thread reader ([&]()
  {
    printf ("\n%s", __PRETTY_FUNCTION__);

    FILE *in = fopen (inStream, "r");

    char *readPattern = (char*)malloc(len);

    ssize_t todo = len;
    ssize_t done = 0;

    while(todo)
    {
      ssize_t todoNow = std::min(readChunksize, todo);
      ssize_t ret = fread (readPattern + done, todoNow, 1, in);
      assert(ret == todoNow);
      todo -= todoNow;
      done += todoNow;
    }

    fclose (in);

    assert(memcmp(pattern, readPattern, len) == 0);

    free(readPattern);
  });

  std::thread writer ([&]()
  {
    printf ("\n%s", __PRETTY_FUNCTION__);

    FILE *out = fopen (outStream, "a");

    ssize_t todo = len;
    ssize_t done = 0;

    while(todo)
    {
      ssize_t todoNow = std::min(writeChunksize, todo);
      ssize_t ret = fwrite (pattern + done, 1, todoNow, out);
      assert(ret == todoNow);
      todo -= todoNow;
      done += todoNow;
    }

    fclose (out);
  });

  reader.join ();
  writer.join ();

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_loopBack (const char *inStream, const char *outStream, size_t len)
{
  waitForKey (__PRETTY_FUNCTION__);

  char *pattern = createAscendingPattern (len);

  size_t chunkSizes[] =
  { 1, 2, 3, 5, 7, 9, 63, 64, 65, 127, 128, 129, 1023, 1024, 1025, 2047, 2048, 2049 };

  for (size_t inSize : chunkSizes)
  {
    if (len >= inSize)
    {
      for (size_t outSize : chunkSizes)
      {
        if (len >= outSize)
        {
          test_loopBack (inStream, outStream, pattern, len, outSize, inSize);
        }
      }
    }
  }

  free (pattern);

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}

static void test_loopBack (const char *inStream, const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);

  size_t patternLengths[] =
  { 1, 2, 12, 13, 63, 64, 65, 127, 128, 129, 1023, 1024, 1025, 2047, 2048, 2049 };

  for (size_t patternLen : patternLengths)
    test_loopBack (inStream, outStream, patternLen);
}

static void test_firstByteAfterReconnectIsFirstByteOfMessage (const char *inStream, const char *outStream)
{
  waitForKey (__PRETTY_FUNCTION__);

  const char *msg = "Just a message";
  ssize_t len = strlen (msg);

  std::thread writer ([&]()
  {
    FILE *out = fopen (outStream, "a");

    for(int i = 0; i < 100; i++)
    {
      ssize_t ret = fwrite (msg, 1, len, out);
      assert(ret == len);

      std::this_thread::sleep_for(std::chrono::milliseconds(11));
    }

    fclose (out);
  });

  std::thread reader ([&]()
  {
    char read[len];

    for(int i = 0; i < 50; i++)
    {
      FILE *in = fopen (inStream, "r");

      if((i % 3) == 0)
      {
        fread(read, len / 2, 1, in);
      }
      else
      {
        ssize_t bytesRead = fread(read, len, 1, in);

        for(ssize_t b = 0; b < bytesRead; b++)
          assert(read[b] == msg[b]);
      }

      fclose (in);

      std::this_thread::sleep_for(std::chrono::milliseconds(13));
    }
  });

  reader.join ();
  writer.join ();

  printf ("\n<-- %s", __PRETTY_FUNCTION__);
}
