/** @file fitbit.c
 *  @brief Coding challenge from FitBit
 *
 *  The biggest design decision was whether or not to use an array to track
 *  the values read in from a file.  The nice thing about using an array in this
 *  case is that since we only expect values up to 0xFFF (4095), the size of the
 *  array need only be 4k.  The alternative would be a binary tree, which could
 *	possibly eat up much more memory, depending on the use cases.  Furthermore,
 *  the array implementation is O(1) for insertions/deletions/search/indexing.
 *  The binary tree would cost O(log(n)) for all operations.  Since we need to
 *  balance battery life versus memory usage, and since all the use cases are
 *  unspecified, 4k of RAM is a reasonable amount of RAM to use to achieve O(1).
 *
 *  The other major design decision was what to do about the 2nd list.  One
 *  option would be to use a circular buffer to track the last 32 numbers.  The
 *  other option was basically to just read the file again.  Since storing 64
 *  bytes is a rather tiny amount of data and file I/O is generally expensive,
 *  this is the option that was chosen.
 *
 *  @author Tristan Muntsinger
 *  @bug No known bugs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

/* various helper macros */
#define ASSERT(predicate) typedef char ASSERT_FAILED[2*!!(predicate)-1];
#define IS_EVEN(x)		  ((x) % 2 == 0)
#define IS_ODD(x)  		  ((x) % 2 == 1)
#define NUM_ARGS		  2
#define VALUE_ARRAY_SIZE  (MAX_VALUE + 1)
#define BITS_PER_BYTE	  8
#define BYTES_IN_24BITS   (24 / BITS_PER_BYTE)
#define UPPER_12BITS_MASK(x) \
	((x) & (MAX_VALUE << BITS_PER_VALUE)) >> BITS_PER_VALUE;
#define LOWER_12BITS_MASK(x) \
	((x) & MAX_VALUE);

#ifdef DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) do {} while(0)
#endif

/* bit-size defines */
#define BITS_PER_VALUE	  12			/* 12-bits */
#define MAX_VALUE		  0xFFF			/* 12-bits */

/* number of elements to output */
#define OUTPUT_COUNT	  32

/* error codes */
#define SUCCESS			  0
#define FAILURE			 -1

/* lists that track our data */
uint8_t  valueCount[VALUE_ARRAY_SIZE] = {0};
uint16_t lastValues[OUTPUT_COUNT] = {0};
size_t   lastValuesIdx = 0;
size_t   lastValuesCount = 0;

/* inserts a 12-bit value into our 2 lists */
void insertValue(uint16_t value)
{
	valueCount[value]++;
	lastValues[lastValuesIdx++] = value;
	lastValuesIdx %= OUTPUT_COUNT;
	if(lastValuesCount < OUTPUT_COUNT)
		lastValuesCount++;
}

/* gets 24 bits from inputFile and puts into buf */
int get24bits(FILE* inputFile, uint32_t* buf)
{
	assert(inputFile);
	uint8_t buffer[BYTES_IN_24BITS];
	int retVal = fread(buffer, 1, sizeof(buffer), inputFile);
	if(buf)
		*buf = (buffer[0] << 16) | (buffer[1] << 8) | (buffer[2]);
	return retVal;
}

/* parses input and increments valueCount[] (also stores filesize) */
int parseInput(const char* input)
{
	assert(input);
	
	FILE* inputFile = fopen(input, "rb");
	if(!inputFile)
		return FAILURE;
	
	int max = 0;
	int retVal;
	while(1) {
		uint32_t value;
		retVal = get24bits(inputFile, &value);
		
		/* if we read nothing, we're done */
		if(retVal == 0)
			break;
		
		/* not enough data to generate a value -- that's an error */
		if(retVal == 1) {
			retVal = FAILURE;
			goto cleanup;
		}
		
		if(retVal >= 2) {
			uint16_t upper = UPPER_12BITS_MASK(value);
			assert(upper <= MAX_VALUE && upper >= 0);
			insertValue(upper);
			if(upper > max)
				max = upper;
		}
		
		if(retVal >= 3) {
			uint16_t lower = LOWER_12BITS_MASK(value);
			assert(lower <= MAX_VALUE && lower >= 0);
			insertValue(lower);
			if(lower > max)
				max = lower;
		}
	}
	
	retVal = max;

cleanup:
	fclose(inputFile);
	return retVal;
}

/* gets the total count of values, until count fills up to our max */
int getCount(int max)
{
	assert(max <= MAX_VALUE && max >= 0);
	int count = 0;
	int i;
	for(i=max; i>=0 && count < OUTPUT_COUNT; i--)
		if(valueCount[i] > 0)
			count += valueCount[i];
	return count;
}

/* gets the index where we need to start printing the last N numbers */
int getStart(int max)
{
	assert(max <= MAX_VALUE && max >= 0);
	int count = 0;
	int i;
	
	/* count backwards to get starting point */
	for(i=max; i >= 0 && count < OUTPUT_COUNT; i--)
		if(valueCount[i] > 0)
			count += valueCount[i];
	
	i++;	/* undo last iteration */
	return i;
}

/* outputs to file the list of sorted max values */
int generateSortedOutput(const char* output, int max)
{
	assert(output && max <= MAX_VALUE && max >= 0);
	
	FILE* outputFile = fopen(output, "w");
	if(!outputFile)
		return FAILURE;
	
	fprintf(outputFile, "--Sorted Max %d Values--\r\n", OUTPUT_COUNT);
	
	int count = getCount(max);
	int start = getStart(max);
	
	int retVal = SUCCESS;
	int i, j;
	for(i=start; count > 0; i++, count--) {
		if(valueCount[i] <= 0)
			continue;
		
		int thisCount = valueCount[i];
		if(i == start && count > OUTPUT_COUNT)
			thisCount -= count - OUTPUT_COUNT;	/* don't print too many */
		
		for(j=0; j<thisCount; j++) {
			if(fprintf(outputFile, "%d\r\n", i) < SUCCESS) {
				retVal = FAILURE;
				goto cleanup;
			}
		}
	}
	
cleanup:
	fclose(outputFile);
	return retVal;
}

/* outputs to file the last values in the input file */
int generateLastOutput(const char* output)
{
	assert(output);
	
	FILE* outputFile = fopen(output, "a");
	if(!outputFile)
		return FAILURE;
	
	fprintf(outputFile, "--Last %d Values--\r\n", OUTPUT_COUNT);
	
	int retVal = SUCCESS;
	
	int idx = (lastValuesIdx - lastValuesCount) % OUTPUT_COUNT;
	
	int i;
	for(i=0; i<lastValuesCount; i++) {
		uint16_t value = lastValues[idx++];
		assert(value <= MAX_VALUE && value >= 0);
		if(fprintf(outputFile, "%d\r\n", value) < SUCCESS) {
			retVal = FAILURE;
			goto cleanup;
		}
		idx %= OUTPUT_COUNT;
	}
	
cleanup:
	fclose(outputFile);
	return retVal;
}

/* prints the program usage */
void printUsage(const char* argv0)
{
	assert(argv0);
	printf("usage: %s <in-file> <out-file>\n", argv0);
}

/* translates a binary file into a text file w/ lists of sorted & last values */
int main(int argc, char* argv[])
{
	if(argc != NUM_ARGS + 1) {
		printUsage(*argv);
		return FAILURE;
	}
	
	const char* input  = argv[1];
	const char* output = argv[2];
	
	int max = parseInput(input);
	if(max < 0) {
		printf("ERROR: Input file either doesn't exist or is invalid.\r\n");
		return FAILURE;
	}
	
	if(generateSortedOutput(output, max) < SUCCESS) {
		printf("ERROR: Failed to write to output file.\r\n");
		return FAILURE;
	}
	
	if(generateLastOutput(output) < SUCCESS) {
		printf("ERROR: Failed to write to output file.\r\n");
		return FAILURE;
	}
	
	return SUCCESS;
}