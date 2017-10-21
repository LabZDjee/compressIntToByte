/*
 * compressIntToByte.c
 *
 *  Created on: Oct 8, 2017
 *      Author: djee
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  defines a byte (unsigned char) as a shift of 4 bits and a mantissa of (4 + 1) bits
   (additional 1 bit is a hidden leading bit as defined by IEEE 754 for floats)
  encodes values from 0 to 516,095 with a 3 % accuracy (1/32)
  encoding scheme:
   shift is 0 -> mantissa
   otherwise  -> (0x10 + mantissa) << (shift - 1)
  shift is encoded as bits b7 to b4 in byte and mantissa as bits b3 to b0
  here is a table of the decoded values for mantissa from 0 to 15 (in columns)
  and shift values from 0 to 15 (in lines)
   sh:0       0      1      2      3      4      5      6      7      8      9     10     11     12     13     14     15
   sh:1      16     17     18     19     20     21     22     23     24     25     26     27     28     29     30     31
   sh:2      32     34     36     38     40     42     44     46     48     50     52     54     56     58     60     62
   sh:3      64     68     72     76     80     84     88     92     96    100    104    108    112    116    120    124
   sh:4     128    136    144    152    160    168    176    184    192    200    208    216    224    232    240    248
   sh:5     256    272    288    304    320    336    352    368    384    400    416    432    448    464    480    496
   sh:6     512    544    576    608    640    672    704    736    768    800    832    864    896    928    960    992
   sh:7    1024   1088   1152   1216   1280   1344   1408   1472   1536   1600   1664   1728   1792   1856   1920   1984
   sh:8    2048   2176   2304   2432   2560   2688   2816   2944   3072   3200   3328   3456   3584   3712   3840   3968
   sh:9    4096   4352   4608   4864   5120   5376   5632   5888   6144   6400   6656   6912   7168   7424   7680   7936
   sh:10   8192   8704   9216   9728  10240  10752  11264  11776  12288  12800  13312  13824  14336  14848  15360  15872
   sh:11  16384  17408  18432  19456  20480  21504  22528  23552  24576  25600  26624  27648  28672  29696  30720  31744
   sh:12  32768  34816  36864  38912  40960  43008  45056  47104  49152  51200  53248  55296  57344  59392  61440  63488
   sh:13  65536  69632  73728  77824  81920  86016  90112  94208  98304 102400 106496 110592 114688 118784 122880 126976
   sh:14 131072 139264 147456 155648 163840 172032 180224 188416 196608 204800 212992 221184 229376 237568 245760 253952
   sh:15 262144 278528 294912 311296 327680 344064 360448 376832 393216 409600 425984 442368 458752 475136 491520 507904
  Note: negative values cannot be encoded

 */

/*
 * compress unsigned long ulVal to a byte and returns this byte
 * pResult is not NULL its dereference is not 0 if result is within limits
 *  or 0 if uVal is too big and cannot be encoded correctly
 * note: this compression is done with rounding
 */
unsigned char compressIntToByte(unsigned long ulVal, int* pResult)
{
  const unsigned long cMax = 31ul<<14 | ((1<<13)-1);
  int i;
  unsigned char bVal;
  if(ulVal<=cMax) {
   if(pResult!=NULL)
    *pResult = !0;
   for(i=19; i>=4; i--) {
    if(ulVal & (1ul<<i))
     break;
   }
   if(i<=4)
    return((unsigned char)(ulVal | (i==4 ? 0x10 : 0)));
   bVal = (unsigned char)(ulVal>>(i-5));
   if(bVal&1) {
    bVal++;
   }
   if(bVal&(1<<6)) {
     i++;
   }
   return((unsigned char)(((bVal>>1)&15) | ((i-3)<<4)));
  }
  if(pResult!=NULL)
   *pResult = 0;
  return(255);
}

/*
 * uncompress byte ucVal and return it as a result
 */
unsigned long uncompressByteToInt(unsigned char ucVal)
{
  const int shift = ucVal>>4;
  const int mantissa = ucVal&0x0f;
  if(shift>0) {
   return(((unsigned long)(0x10+mantissa))<<(shift-1));
  }
  return(mantissa);
}

/***************************************************
 * tests
 **************************************************/

/* define element of test vector */
typedef struct _compressIntToByteTestVect{
    unsigned long input; /* value to compress */
    unsigned char compressed; /* compressed value */
    unsigned long uncompressed; /* uncompress value of the compress value */
    int compressResult; /* store result of compression */
} tCompressIntToByteTestVect;

tCompressIntToByteTestVect testVectors[] = {
  {0ul, 0, 0ul, 1},
  {5ul, 0x05, 5, 1},
  {16ul, 0x10, 16ul, 1},
  {30ul, 0x1e, 30, 1},
  {32ul, 0x20, 32ul, 1},
  {33ul, 0x21, 34ul, 1},
  {40ul, 0x24, 40, 1},
  {41ul, 0x25, 42, 1},
  {80ul, 0x34, 80ul, 1},
  {85ul, 0x35, 84ul, 1},
  {86ul, 0x36, 88ul, 1},
  {95ul, 0x38, 96ul, 1},
  {100ul, 0x39, 100, 1},
  {187ul, 0x47, 184, 1},
  {188ul, 0x48, 192, 1},
  {252ul, 0x50, 256, 1},
  {687ul, 0x65, 672ul, 1},
  {688ul, 0x66, 704ul, 1},
  {704ul, 0x66, 704ul, 1},
  {750ul, 0x67, 736ul, 1},
  {1024ul, 0x70, 1024ul, 1},
  {1055ul, 0x70, 1024ul, 1},
  {1059ul, 0x71, 1088ul, 1},
  {1059ul, 0x71, 1088ul, 1},
  {1472ul, 0x77, 1472ul, 1},
  {1504ul, 0x78, 1536ul, 1},
  {3967ul, 0x8f, 3968ul, 1},
  {4031ul, 0x8f, 3968ul, 1},
  {6400ul, 0x99, 6400ul, 1},
  {10200ul, 0xa4, 10240ul, 1},
  {10700ul, 0xa5, 10752ul, 1},
  {24100ul, 0xb8, 24576ul, 1},
  {47120ul, 0xc7, 47104ul, 1},
  {48144ul, 0xc8, 49152ul, 1},
  {64511ul, 0xcf, 63488ul, 1},
  {64512ul, 0xd0, 65536ul, 1},
  {65408ul, 0xd0, 65536, 1},
  {88000ul, 0xd5, 86016ul, 1},
  {88120ul, 0xd6, 90112ul, 1},
  {120000ul, 0xdd, 118784ul, 1},
  {120831ul, 0xdd, 118784ul, 1},
  {120832ul, 0xde, 122880ul, 1},
  {333333ul, 0xf4, 327680ul, 1},
  {335871ul, 0xf4, 327680ul, 1},
  {335872ul, 0xf5, 344064ul, 1},
  {425985ul, 0xfa,425984ul, 1},
  {482345ul, 0xfd,475136ul, 1},
  {507904ul, 0xff, 507904ul, 1},
  {507905ul, 0xff, 507904ul, 1},
  {516095ul, 0xff, 507904ul, 1},
  {516096ul, 0xff, 507904ul, 0},
  {0xfffffffful, 0xff, 507904ul, 0},
};

/*
 * run tests
 *  returns a non 0 value if all tests passed, 0 otherwise
 */
int test_compressIntToByte(void)
{
  const int nbTests =sizeof(testVectors)/sizeof(*testVectors);
  int i, nbFailures=0;
  unsigned char compressed;
  unsigned long uncompressed;
  int result;
  for (i=0; i<nbTests; i++) {
   compressed = compressIntToByte(testVectors[i].input, &result);
   if(compressed != testVectors[i].compressed) {
    nbFailures++;
    printf("failure: compressIntToByte(%lu, ...) gives %i != %i in test\n",
          testVectors[i].input, (int)compressed, (int)testVectors[i].compressed);
   } else {
       if(result != testVectors[i].compressResult) {
           nbFailures++;
           printf("failure: compressIntToByte(%lu, ...) gives result %i != %i in test\n",
                 testVectors[i].input, result, testVectors[i].compressResult);
       } else {
           uncompressed = uncompressByteToInt(compressed);
           if(uncompressed != testVectors[i].uncompressed) {
             nbFailures++;
             printf("failure: uncompressByteToInt(%i) gives %lu != %lu in test\n",
                   (int)compressed, uncompressed, testVectors[i].uncompressed);
           }
       }
   }
  }
  printf("test_compressIntToByteTable: %i steps: %i failure%s\n", nbTests, nbFailures, nbFailures!=1?"s":"");
  return(nbFailures>0);
}

/*
 * builds a table on stdout to present all the 256 decoded values
 */
void test_makeCompressIntToByteTable(void)
{
  int i, j;
  for(j=0; j<16; j++){
   printf("shift:%-2i", j);
   for(i=0; i<16; i++) {
    printf(" %6lu", uncompressByteToInt((unsigned char)((j<<4)+i)));
   }
   printf("\n");
  }
}

int main(void)
{
 unsigned long val;
 unsigned char byt;
 int result;
 int again=1;
 char str[256];
 setvbuf(stdout, NULL, _IONBF, 0);
 test_compressIntToByte();
 do {
     printf("? ");
     scanf("%s", str);
     if(!strcmp(str, "quit"))
      again=0;
     else if(!strcmp(str, "table")){
      test_makeCompressIntToByteTable();
     }
     else {
       val=atol(str);
       byt = compressIntToByte(val, &result);
       printf("{%luul, 0x%02x, %luul, %i},\n", val, (int)byt, uncompressByteToInt(byt), result);
     }
 } while(again);
 printf("bye!\n");
 return(0);
}
